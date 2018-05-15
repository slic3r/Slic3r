#include "3DScene.hpp"

namespace Slic3r {

// caller is responsible for supplying NO lines with zero length
void
_3DScene::_extrusionentity_to_verts_do(const Lines &lines, const std::vector<double> &widths,
        const std::vector<double> &heights, bool closed, double top_z, const Point &copy,
        GLVertexArray* qverts, GLVertexArray* tverts)
{
    if (lines.empty()) return;
    
    /* It looks like it's faster without reserving capacity...
    // each segment has 4 quads, thus 16 vertices; + 2 caps
    qverts->reserve_more(3 * 4 * (4 * lines.size() + 2));
    
    // two triangles for each corner
    tverts->reserve_more(3 * 3 * 2 * (lines.size() + 1));
    */
    
    Line prev_line;
    Pointf prev_b1, prev_b2;
    Vectorf3 prev_xy_left_normal, prev_xy_right_normal;
    
    // loop once more in case of closed loops
    bool first_done = false;
    for (size_t i = 0; i <= lines.size(); ++i) {
        if (i == lines.size()) i = 0;
        
        const Line &line = lines.at(i);
        if (i == 0 && first_done && !closed) break;
        
        double len = line.length();
        double unscaled_len = unscale(len);
        double dist = widths.at(i)/2;  // scaled
        
        double bottom_z_a, bottom_z_b, middle_z_a, middle_z_b, top_z_a, top_z_b;
        if (line.a.z == -1) {
            top_z_a = top_z;
            bottom_z_a = top_z - heights.at(i);
            middle_z_a = (top_z + bottom_z_a) / 2;
        }
        else {
            top_z_a = line.a.z;
            bottom_z_a = line.a.z - heights.at(i);
            middle_z_a = (line.a.z + bottom_z_a) / 2;
        }
        
        if (line.b.z == -1) {
            top_z_b = top_z;
            bottom_z_b = top_z - heights.at(i);
            middle_z_b = (top_z + bottom_z_b) / 2;
        }
        else {
            top_z_b = line.b.z;
            bottom_z_b = line.b.z - heights.at(i);
            middle_z_b = (line.b.z + bottom_z_b) / 2;
        }
        
        Vectorf v = Vectorf::new_unscale(line.vector());
        v.scale(1/unscaled_len);
        
        Pointf a = Pointf::new_unscale(line.a);
        Pointf b = Pointf::new_unscale(line.b);
        Pointf a1 = a;
        Pointf a2 = a;
        a1.translate(+dist*v.y, -dist*v.x);
        a2.translate(-dist*v.y, +dist*v.x);
        Pointf b1 = b;
        Pointf b2 = b;
        b1.translate(+dist*v.y, -dist*v.x);
        b2.translate(-dist*v.y, +dist*v.x);
        
        // calculate new XY normals
        Vector n = line.normal();
        Vectorf3 xy_right_normal = Vectorf3::new_unscale(n.x, n.y, 0);
        xy_right_normal.scale(1/unscaled_len);
        Vectorf3 xy_left_normal = xy_right_normal;
        xy_left_normal.scale(-1);
        
        if (first_done) {
            // if we're making a ccw turn, draw the triangles on the right side, otherwise draw them on the left side
            double ccw = line.b.ccw(prev_line);
            if (ccw > EPSILON) {
                // top-right vertex triangle between previous line and this one
                {
                    // use the normal going to the right calculated for the previous line
                    tverts->push_norm(prev_xy_right_normal);
                    tverts->push_vert(prev_b1.x, prev_b1.y, middle_z_a);
            
                    // use the normal going to the right calculated for this line
                    tverts->push_norm(xy_right_normal);
                    tverts->push_vert(a1.x, a1.y, middle_z_a);
            
                    // normal going upwards
                    tverts->push_norm(0,0,1);
                    tverts->push_vert(a.x, a.y, top_z_a);
                }
                // bottom-right vertex triangle between previous line and this one
                {
                    // use the normal going to the right calculated for the previous line
                    tverts->push_norm(prev_xy_right_normal);
                    tverts->push_vert(prev_b1.x, prev_b1.y, middle_z_b);
            
                    // normal going downwards
                    tverts->push_norm(0,0,-1);
                    tverts->push_vert(a.x, a.y, bottom_z_a);
            
                    // use the normal going to the right calculated for this line
                    tverts->push_norm(xy_right_normal);
                    tverts->push_vert(a1.x, a1.y, middle_z_a);
                }
            } else if (ccw < -EPSILON) {
                // top-left vertex triangle between previous line and this one
                {
                    // use the normal going to the left calculated for the previous line
                    tverts->push_norm(prev_xy_left_normal);
                    tverts->push_vert(prev_b2.x, prev_b2.y, middle_z_b);
            
                    // normal going upwards
                    tverts->push_norm(0,0,1);
                    tverts->push_vert(a.x, a.y, top_z_a);
            
                    // use the normal going to the right calculated for this line
                    tverts->push_norm(xy_left_normal);
                    tverts->push_vert(a2.x, a2.y, middle_z_a);
                }
                // bottom-left vertex triangle between previous line and this one
                {
                    // use the normal going to the left calculated for the previous line
                    tverts->push_norm(prev_xy_left_normal);
                    tverts->push_vert(prev_b2.x, prev_b2.y, middle_z_b);
            
                    // use the normal going to the right calculated for this line
                    tverts->push_norm(xy_left_normal);
                    tverts->push_vert(a2.x, a2.y, middle_z_a);
            
                    // normal going downwards
                    tverts->push_norm(0,0,-1);
                    tverts->push_vert(a.x, a.y, bottom_z_a);
                }
            }
        }
        
        // if this was the extra iteration we were only interested in the triangles
        if (first_done && i == 0) break;
        
        prev_line = line;
        prev_b1 = b1;
        prev_b2 = b2;
        prev_xy_right_normal = xy_right_normal;
        prev_xy_left_normal  = xy_left_normal;
        
        if (!closed) {
            // terminate open paths with caps
            if (i == 0) {
                // normal pointing downwards
                qverts->push_norm(0,0,-1);
                qverts->push_vert(a.x, a.y, bottom_z_a);
            
                // normal pointing to the right
                qverts->push_norm(xy_right_normal);
                qverts->push_vert(a1.x, a1.y, middle_z_a);
            
                // normal pointing upwards
                qverts->push_norm(0,0,1);
                qverts->push_vert(a.x, a.y, top_z_a);
            
                // normal pointing to the left
                qverts->push_norm(xy_left_normal);
                qverts->push_vert(a2.x, a2.y, middle_z_a);
            }
            // we don't use 'else' because both cases are true if we have only one line
            if (i == lines.size()-1) {
                // normal pointing downwards
                qverts->push_norm(0,0,-1);
                qverts->push_vert(b.x, b.y, bottom_z_b);
            
                // normal pointing to the left
                qverts->push_norm(xy_left_normal);
                qverts->push_vert(b2.x, b2.y, middle_z_b);
            
                // normal pointing upwards
                qverts->push_norm(0,0,1);
                qverts->push_vert(b.x, b.y, top_z_b);
            
                // normal pointing to the right
                qverts->push_norm(xy_right_normal);
                qverts->push_vert(b1.x, b1.y, middle_z_b);
            }
        }
        
        // bottom-right face
        {
            // normal going downwards
            qverts->push_norm(0,0,-1);
            qverts->push_norm(0,0,-1);
            qverts->push_vert(a.x, a.y, bottom_z_a);
            qverts->push_vert(b.x, b.y, bottom_z_b);
            
            qverts->push_norm(xy_right_normal);
            qverts->push_norm(xy_right_normal);
            qverts->push_vert(b1.x, b1.y, middle_z_b);
            qverts->push_vert(a1.x, a1.y, middle_z_a);
        }
        
        // top-right face
        {
            qverts->push_norm(xy_right_normal);
            qverts->push_norm(xy_right_normal);
            qverts->push_vert(a1.x, a1.y, middle_z_a);
            qverts->push_vert(b1.x, b1.y, middle_z_b);
            
            // normal going upwards
            qverts->push_norm(0,0,1);
            qverts->push_norm(0,0,1);
            qverts->push_vert(b.x, b.y, top_z_b);
            qverts->push_vert(a.x, a.y, top_z_a);
        }
         
        // top-left face
        {
            qverts->push_norm(0,0,1);
            qverts->push_norm(0,0,1);
            qverts->push_vert(a.x, a.y, top_z_a);
            qverts->push_vert(b.x, b.y, top_z_b);
            
            qverts->push_norm(xy_left_normal);
            qverts->push_norm(xy_left_normal);
            qverts->push_vert(b2.x, b2.y, middle_z_b);
            qverts->push_vert(a2.x, a2.y, middle_z_a);
        }
        
        // bottom-left face
        {
            qverts->push_norm(xy_left_normal);
            qverts->push_norm(xy_left_normal);
            qverts->push_vert(a2.x, a2.y, middle_z_a);
            qverts->push_vert(b2.x, b2.y, middle_z_b);
            
            // normal going downwards
            qverts->push_norm(0,0,-1);
            qverts->push_norm(0,0,-1);
            qverts->push_vert(b.x, b.y, bottom_z_b);
            qverts->push_vert(a.x, a.y, bottom_z_a);
        }
        
        first_done = true;
    }
}

void
GLVertexArray::load_mesh(const TriangleMesh &mesh)
{
    this->reserve_more(3 * 3 * mesh.facets_count());
    
    for (int i = 0; i < mesh.stl.stats.number_of_facets; ++i) {
        stl_facet &facet = mesh.stl.facet_start[i];
        for (int j = 0; j <= 2; ++j) {
            this->push_norm(facet.normal.x, facet.normal.y, facet.normal.z);
            this->push_vert(facet.vertex[j].x, facet.vertex[j].y, facet.vertex[j].z);
        }
    }
}

}
