#include "Plater/Plate3D.hpp"
#include "misc_ui.hpp"

namespace Slic3r { namespace GUI {

Plate3D::Plate3D(wxWindow* parent, const wxSize& size, std::vector<PlaterObject>& _objects, std::shared_ptr<Model> _model, std::shared_ptr<Config> _config) :
    Scene3D(parent, size), objects(_objects), model(_model), config(_config)
{ 

    //this->glContext = new wxGLContext(this);
    //this->Bind(wxEVT_PAINT, [this](wxPaintEvent& e) { this->repaint(e); });
        //delete this->glContext;
        //this->glContext = new wxGLContext(this); // Update glContext (look for better way) 
    //});

    // Bind the varying mouse events
    this->Bind(wxEVT_LEFT_DOWN, [this](wxMouseEvent &e) { this->mouse_down(e); });
    this->Bind(wxEVT_RIGHT_DOWN, [this](wxMouseEvent &e) { this->mouse_down(e); });
}

void Plate3D::mouse_down(wxMouseEvent &e){
    if(!hover){
        //select logic
    }
}


void Plate3D::mouse_move(wxMouseEvent &e){
    if(!e.Dragging()){
        pos = Point(e.GetX(),e.GetY());
        mouse = true;
        Refresh();
    } else {
        Scene3D::mouse_move(e);
    }
}


void Plate3D::update(){
    volumes.clear();
    for(const PlaterObject &object: objects){
        const auto &modelobj = model->objects.at(object.identifier);
        for(ModelInstance *instance: modelobj->instances){
            for(ModelVolume* volume: modelobj->volumes){
                volumes.push_back(load_object(*volume,*instance));
            } 
        }
    }
    color_volumes();
}

void Plate3D::color_volumes(){
    uint i = 0;
    for(const PlaterObject &object: objects){
        const auto &modelobj = model->objects.at(object.identifier);
        for(ModelInstance *instance: modelobj->instances){
            for(ModelVolume* volume: modelobj->volumes){
                auto& rendervolume = volumes.at(i);
                if(object.selected){
                    rendervolume.color = ui_settings->color->SELECTED_COLOR();
                }else{
                    rendervolume.color = ui_settings->color->COLOR_PARTS();
                }
                i++;
            } 
        }
    }
    if(hover){
        volumes.at(hover_volume).color = ui_settings->color->HOVER_COLOR();
    }
}

void Plate3D::before_render(){
    if (!mouse)return;

    //glDisable(GL_MULTISAMPLE) if ($self->{can_multisample});
    glDisable(GL_LIGHTING);
    uint i = 1;
    for(Volume &volume : volumes){
        volume.color = wxColor((i>>16)&0xFF,(i>>8)&0xFF,i&0xFF);
        i++;
    }
    draw_volumes();
    glFlush();
    glFinish();
    GLubyte color[4] = {0,0,0,0};
    glReadPixels(pos.x, GetSize().GetHeight()-  pos.y, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, color);

    uint index = (color[0]<<16) + (color[1]<<8) + color[2];
    hover = false;
    ///*$self->_hover_volume_idx(undef);
    //$_->hover(0) for @{$self->volumes};
    if (index != 0 && index <= volumes.size()) {
        hover = true;
        hover_volume = index - 1;
        /*
        $self->volumes->[$volume_idx]->hover(1);
        my $group_id = $self->volumes->[$volume_idx]->select_group_id;
        if ($group_id != -1) {
            $_->hover(1) for grep { $_->select_group_id == $group_id } @{$self->volumes};
        }*/
        
        //$self->on_hover->($volume_idx) if $self->on_hover;
    }
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glFlush();
    glFinish();
    glEnable(GL_LIGHTING);
    color_volumes();
    mouse = false;
}

} } // Namespace Slic3r::GUI
