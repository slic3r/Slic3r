#include "Plater/Plate3D.hpp"
#include "misc_ui.hpp"

namespace Slic3r { namespace GUI {

Plate3D::Plate3D(wxWindow* parent, const wxSize& size, std::vector<PlaterObject>& _objects, std::shared_ptr<Model> _model, std::shared_ptr<Config> _config) :
    Scene3D(parent, size), objects(_objects), model(_model), config(_config)
{ 
    // Bind the extra mouse events
    this->Bind(wxEVT_LEFT_DOWN, [this](wxMouseEvent &e) { this->mouse_down(e); });
    this->Bind(wxEVT_RIGHT_DOWN, [this](wxMouseEvent &e) { this->mouse_down(e); });
}

void Plate3D::mouse_down(wxMouseEvent &e){
    if(hover){
        on_select_object(hover_object);
        moving = true;
        moving_volume = hover_volume;
        move_start = Point(e.GetX(),e.GetY());
    }else{
        on_select_object(-1);
    }
    hover = false;
}

void Plate3D::mouse_up(wxMouseEvent &e){
    if(moving){
        //translate object
        moving = false;
        uint i = 0;
        for(const PlaterObject &object: objects){
            const auto &modelobj = model->objects.at(object.identifier);
            for(ModelInstance *instance: modelobj->instances){
                uint size = modelobj->volumes.size();
                if(i <= moving_volume && moving_volume < i+size){
                    instance->offset.translate(volumes.at(i).origin);
                    modelobj->update_bounding_box();
                    on_instances_moved();
                    Refresh();
                    return;
                }else{
                    i+=size;
                }
            }
        }
    }
    Scene3D::mouse_up(e);
}

void Plate3D::mouse_move(wxMouseEvent &e){
    if(!e.Dragging()){
        pos = Point(e.GetX(),e.GetY());
        mouse = true;
        Refresh();
    } else if(moving){
        const auto p = Point(e.GetX(),e.GetY());
        const auto current = mouse_ray(p).intersect_plane(0);
        const auto old = mouse_ray(move_start).intersect_plane(0);
        move_start = p;
        uint i = 0;
        for(const PlaterObject &object: objects){
            const auto &modelobj = model->objects.at(object.identifier);
            for(ModelInstance *instance: modelobj->instances){
                uint size = modelobj->volumes.size();
                if(i <= moving_volume && moving_volume < i+size){
                    for(ModelVolume* volume: modelobj->volumes){
                        volumes.at(i).origin.translate(old.vector_to(current));
                        i++;
                    }
                    Refresh();
                    return;
                }else{
                    i+=size;
                }
            }
        }
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
                TriangleMesh copy = volume->mesh;
                instance->transform_mesh(&copy);
                GLVertexArray model;
                model.load_mesh(copy);
                volumes.push_back(Volume{ wxColor(200,200,200), Pointf3(0,0,0), model, copy.bounding_box()});
            }
        }
    }
    color_volumes();
    Refresh();
}

void Plate3D::color_volumes(){
    uint i = 0;
    for(const PlaterObject &object: objects){
        const auto &modelobj = model->objects.at(object.identifier);
        bool hover_object = hover && i <= hover_volume && hover_volume < i+modelobj->instances.size()*modelobj->volumes.size();
        for(ModelInstance *instance: modelobj->instances){
            for(ModelVolume* volume: modelobj->volumes){
                auto& rendervolume = volumes.at(i);
                if(object.selected){
                    rendervolume.color = ui_settings->color->SELECTED_COLOR();
                }else if(hover_object){
                    rendervolume.color = ui_settings->color->HOVER_COLOR();
                } else {
                    rendervolume.color = ui_settings->color->COLOR_PARTS();
                }
                i++;
            }
        }
    }
}

void Plate3D::before_render(){
    if (!mouse){
        color_volumes();
        return;
    }

    // Color each volume a different color, render and test which color is beneath the mouse.
    
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

    // Handle the hovered volume
    uint index = (color[0]<<16) + (color[1]<<8) + color[2];
    hover = false;
    ///*$self->_hover_volume_idx(undef);
    //$_->hover(0) for @{$self->volumes};
    if (index != 0 && index <= volumes.size()) {
        hover = true;
        hover_volume = index - 1;
        uint k = 0;
        for(const PlaterObject &object: objects){
            const auto &modelobj = model->objects.at(object.identifier);
            if(k <= hover_volume && hover_volume < k+modelobj->instances.size()*modelobj->volumes.size()){
                hover_object = k;
                break;
            }
            k++;
        }
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
