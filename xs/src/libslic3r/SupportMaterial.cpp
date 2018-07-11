#include "SupportMaterial.hpp"

namespace Slic3r
{

Polygons
SupportMaterial::p(SurfacesPtr &surfaces)
{
    Polygons ret;
    for (auto surface : surfaces) {
        ret.push_back(surface->expolygon.contour);
        for (const auto &hole_polygon : surface->expolygon.holes) {
            ret.push_back(hole_polygon);
        }
    }
    return ret;
}

coordf_t
SupportMaterial::contact_distance(coordf_t layer_height, coordf_t nozzle_diameter)
{
    coordf_t extra = static_cast<float>(object_config->support_material_contact_distance.value);
    if (extra == 0) {
        return layer_height;
    }
    else {
        return nozzle_diameter + extra;
    }
}

vector<coordf_t>
SupportMaterial::support_layers_z(vector<float> contact_z,
                                  vector<float> top_z,
                                  coordf_t max_object_layer_height)
{
    // Quick table to check whether a given Z is a top surface.
    map<float, bool> is_top;
    for (auto z : top_z) is_top[z] = true;

    // determine layer height for any non-contact layer
    // we use max() to prevent many ultra-thin layers to be inserted in case
    // layer_height > nozzle_diameter * 0.75.
    auto nozzle_diameter = config->nozzle_diameter.get_at(object_config->support_material_extruder - 1);
    auto support_material_height = (max_object_layer_height, (nozzle_diameter * 0.75));
    coordf_t _contact_distance = this->contact_distance(support_material_height, nozzle_diameter);

    // Initialize known, fixed, support layers.
    vector<coordf_t> z;
    for (auto c_z : contact_z) z.push_back(c_z);
    for (auto t_z : top_z) {
        z.push_back(t_z);
        z.push_back(t_z + _contact_distance);
    }
    sort(z.begin(), z.end());

    // Enforce first layer height.
    coordf_t first_layer_height = object_config->first_layer_height;
    while (!z.empty() && z.front() <= first_layer_height) z.erase(z.begin());
    z.insert(z.begin(), first_layer_height);

    // Add raft layers by dividing the space between first layer and
    // first contact layer evenly.
    if (object_config->raft_layers > 1 && z.size() >= 2) {
        // z[1] is last raft layer (contact layer for the first layer object) TODO @Samir55 How so?
        coordf_t height = (z[1] - z[0]) / (object_config->raft_layers - 1);

        // since we already have two raft layers ($z[0] and $z[1]) we need to insert
        // raft_layers-2 more
        int idx = 1;
        for (int j = 0; j < object_config->raft_layers - 2; j++) {
            float z_new =
                roundf(static_cast<float>((z[0] + height * idx) * 100)) / 100; // round it to 2 decimal places.
            z.insert(z.begin() + idx, z_new);
            idx++;
        }
    }

    // Create other layers (skip raft layers as they're already done and use thicker layers).
    for (size_t i = z.size(); i >= object_config->raft_layers; i--) {
        coordf_t target_height = support_material_height;
        if (i > 0 && is_top[z[i - 1]]) {
            target_height = nozzle_diameter;
        }

        // Enforce first layer height.
        if ((i == 0 && z[i] > target_height + first_layer_height)
            || (z[i] - z[i - 1] > target_height + EPSILON)) {
            z.insert(z.begin() + i, (z[i] - target_height));
            i++;
        }
    }

    // Remove duplicates and make sure all 0.x values have the leading 0.
    {
        set<coordf_t> s;
        for (auto el : z)
            s.insert(roundf(static_cast<float>((el * 100)) / 100)); // round it to 2 decimal places.
        z = vector<coordf_t>();
        for (auto el : s)
            z.push_back(el);
    }

    return z;
}

vector<int>
SupportMaterial::overlapping_layers(int layer_idx, vector<float> support_z)
{
    vector<int> ret;

    float z_max = support_z[layer_idx];
    float z_min = layer_idx == 0 ? 0 : support_z[layer_idx - 1];

    for (int i = 0; i < support_z.size(); i++) {
        if (i == layer_idx) continue;

        float z_max2 = support_z[i];
        float z_min2 = i == 0 ? 0 : support_z[i - 1];

        if (z_max > z_min2 && z_min < z_max2)
            ret.push_back(i);
    }

    return ret;
}

void
SupportMaterial::clip_with_shape(map<int, Polygons> &support, map<int, Polygons> &shape)
{
    for (auto layer : support) {
        // Don't clip bottom layer with shape so that we
        // can generate a continuous base flange
        // also don't clip raft layers
        if (layer.first == 0) continue;
        else if (layer.first < object_config->raft_layers) continue;

        layer.second = intersection(layer.second, shape[layer.first]);
    }
}

void
SupportMaterial::clip_with_object(map<int, Polygons> &support, vector<coordf_t> support_z, PrintObject &object)
{
    int i = 0;
    for (auto support_layer: support) {
        if (support_layer.second.empty()) {
            i++;
            continue;
        }
        coordf_t z_max = support_z[i];
        coordf_t z_min = (i == 0) ? 0 : support_z[i - 1];

        LayerPtrs layers;
        for (auto layer : object.layers) {
            if (layer->print_z > z_min && (layer->print_z - layer->height) < z_max) {
                layers.push_back(layer);
            }
        }

        // $layer->slices contains the full shape of layer, thus including
        // perimeter's width. $support contains the full shape of support
        // material, thus including the width of its foremost extrusion.
        // We leave a gap equal to a full extrusion width. TODO ask about this line @samir
        Polygons slices;
        for (Layer *l : layers) {
            for (auto s : l->slices.contours()) {
                slices.push_back(s);
            }
        }
        support_layer.second = diff(support_layer.second, offset(slices, flow->scaled_width()));
    }
    /*
        $support->{$i} = diff(
            $support->{$i},
            offset([ map @$_, map @{$_->slices}, @layers ], +$self->flow->scaled_width),
        );
     */
}

map<coordf_t, Polygons>
SupportMaterial::object_top(PrintObject *object, map<coordf_t, Polygons> *contact)
{
    // find object top surfaces
    // we'll use them to clip our support and detect where does it stick.
    map<coordf_t, Polygons> top;
    if (object_config->support_material_buildplate_only.value)
        return top;

    Polygons projection;
    for (auto i = static_cast<int>(object->layers.size() - 1); i >= 0; i--) {

        Layer *layer = object->layers[i];
        SurfacesPtr m_top;

        for (auto r : layer->regions)
            for (auto s : r->slices.filter_by_type(stTop))
                m_top.push_back(s);

        if (!m_top.empty()) {
            // compute projection of the contact areas above this top layer
            // first add all the 'new' contact areas to the current projection
            // ('new' means all the areas that are lower than the last top layer
            // we considered).
            // TODO Ask about this line
            /*
             my $min_top = min(keys %top) // max(keys %$contact);
             */
            double min_top = top.begin()->first;

            // Use <= instead of just < because otherwise we'd ignore any contact regions
            // having the same Z of top layers.
            for (auto el : *contact)
                if (el.first > layer->print_z && el.first <= min_top)
                    for (const auto &p : el.second)
                        projection.push_back(p);

            // Now find whether any projection falls onto this top surface.
            Polygons touching = intersection(projection, p(m_top));
            if (!touching.empty()) {
                // Grow top surfaces so that interface and support generation are generated
                // with some spacing from object - it looks we don't need the actual
                // top shapes so this can be done here.
                top[layer->print_z] = offset(touching, flow->scaled_width());
            }

            // Remove the areas that touched from the projection that will continue on
            // next, lower, top surfaces.
            projection = diff(projection, touching);
        }
    }
    return top;
}

}
