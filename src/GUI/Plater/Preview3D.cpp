#include "Preview3D.hpp"
#include <wx/event.h>

namespace Slic3r { namespace GUI {

Preview3D::Preview3D(wxWindow* parent, const wxSize& size, std::shared_ptr<Slic3r::Print> _print, std::vector<PlaterObject>& _objects, std::shared_ptr<Model> _model, std::shared_ptr<Config> _config) :
    wxPanel(parent, wxID_ANY, wxDefaultPosition, size, wxTAB_TRAVERSAL), print(_print), objects(_objects), model(_model), config(_config), canvas(this,size)
{

    // init GUI elements
    slider = new wxSlider(
        this, -1,
        0,                              // default
        0,                              // min
        // we set max to a bogus non-zero value because the MSW implementation of wxSlider
        // will skip drawing the slider if max <= min:
        1,                              // max
        wxDefaultPosition,
        wxDefaultSize,
        wxVERTICAL | wxSL_INVERSE
    );
    
    this->z_label = new wxStaticText(this, -1, "", wxDefaultPosition,
        wxSize(40,-1), wxALIGN_CENTRE_HORIZONTAL);
    //z_label->SetFont(Slic3r::GUI::small_font);
    
    auto* vsizer = new wxBoxSizer(wxVERTICAL);
    vsizer->Add(slider, 1, wxALL | wxEXPAND, 3);
    vsizer->Add(z_label, 0, wxALL | wxEXPAND, 3);
    
    auto* sizer = new wxBoxSizer(wxHORIZONTAL);
    sizer->Add(&canvas, 1, wxALL | wxEXPAND, 0);
    sizer->Add(vsizer, 0, wxTOP | wxBOTTOM | wxEXPAND, 5);

    this->Bind(wxEVT_SLIDER, [this](wxCommandEvent &e){ 
        //$self->set_z($self->{layers_z}[$slider->GetValue])
        //  if $self->enabled;
    }); 
    this->Bind(wxEVT_CHAR, [this](wxKeyEvent &e) {
        /*my ($s, $event) = @_;
        
        my $key = $event->GetKeyCode;
        if ($key == 85 || $key == 315) {
            $slider->SetValue($slider->GetValue + 1);
            $self->set_z($self->{layers_z}[$slider->GetValue]);
        } elsif ($key == 68 || $key == 317) {
            $slider->SetValue($slider->GetValue - 1);
            $self->set_z($self->{layers_z}[$slider->GetValue]);
        } else {
            $event->Skip;
        }*/
    });
    
    SetSizer(sizer);
    SetMinSize(GetSize());
    sizer->SetSizeHints(this);
    
    // init canvas
    reload_print();
    
}
void Preview3D::reload_print(){
   
    canvas.resetObjects();
    loaded = false;
    load_print();
}

void Preview3D::load_print() {
    if(loaded) return;
    
    // we require that there's at least one object and the posSlice step
    // is performed on all of them (this ensures that _shifted_copies was
    // populated and we know the number of layers)
    if(!print->step_done(posSlice)) {
        _enabled = false;
        slider->Hide();
        canvas.Refresh();  // clears canvas
        return;
    }
    
    size_t z_idx = 0;
    {
        layers_z.clear();
        // Load all objects on the plater + support material
        for(auto* object : print->objects) {
            for(auto layer : object->layers){
                layers_z.push_back(layer->print_z);
            }
            for(auto layer : object->support_layers) {
                layers_z.push_back(layer->print_z);
            }
        }
        
        _enabled = true;
        std::sort(layers_z.begin(),layers_z.end());
        slider->SetRange(0, layers_z.size()-1);
        z_idx = slider->GetValue();
        // If invalid z_idx,  move the slider to the top
        if (z_idx >= layers_z.size() || slider->GetValue() == 0) {
            slider->SetValue(layers_z.size()-1);
            //$z_idx = @{$self->{layer_z}} ? -1 : undef;
            z_idx = slider->GetValue(); // not sure why the perl version makes z_idx invalid
        }
        slider->Show();
        Layout();
    }
    if (IsShown()) {
        // set colors
        /*canvas.color_toolpaths_by($Slic3r::GUI::Settings->{_}{color_toolpaths_by});
        if ($self->canvas->color_toolpaths_by eq 'extruder') {
            my @filament_colors = map { s/^#//; [ map $_/255, (unpack 'C*', pack 'H*', $_), 255 ] }
                @{$self->print->config->filament_colour};
            $self->canvas->colors->[$_] = $filament_colors[$_] for 0..$#filament_colors;
        } else {
            $self->canvas->colors([ $self->canvas->default_colors ]);
        }*/
        
        // load skirt and brim
        //$self->canvas->load_print_toolpaths($self->print);
        
        /*foreach my $object (@{$self->print->objects}) {
            canvas.load_print_object_toolpaths($object);
        }*/
        loaded = true;
    }
    
    set_z(layers_z.at(z_idx));
}
void Preview3D::set_z(float z) {
    if(!_enabled) return;
    z_label->SetLabel(std::to_string(z));
    //canvas.set_toolpaths_range(0, $z);
    if(IsShown())canvas.Refresh();
}

/*
void set_bed_shape() {
    my ($self, $bed_shape) = @_;
    $self->canvas->set_bed_shape($bed_shape);
}
*/

} } // Namespace Slic3r::GUI

