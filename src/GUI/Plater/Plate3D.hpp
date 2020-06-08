#ifndef PLATE3D_HPP
#define PLATE3D_HPP
#include <wx/wxprec.h>
#ifndef WX_PRECOMP
    #include <wx/wx.h>
#endif
#include "Plater/PlaterObject.hpp"
#include "Scene3D.hpp"
#include "Settings.hpp"
#include "Model.hpp"
#include "Config.hpp"

namespace Slic3r { namespace GUI {

class Plate3D : public Scene3D {
public:
    Plate3D(wxWindow* parent, const wxSize& size, std::vector<PlaterObject>& _objects, std::shared_ptr<Model> _model, std::shared_ptr<Config> _config);
    
    /// Called to regenerate rendered volumes from the model 
    void update();
    
    /// Registered function to fire when objects are selected.
    std::function<void (const unsigned int obj_idx)> on_select_object {};
    
    /// Registered function to fire when an instance is moved.
    std::function<void ()> on_instances_moved {};
    
    void selection_changed(){Refresh();}
 protected:
    // Render each volume as a different color and check what color is beneath
    // the mouse to determine the hovered volume
    void before_render();

    // Mouse events are needed to handle selecting and moving objects
    void mouse_up(wxMouseEvent &e);
    void mouse_move(wxMouseEvent &e);
    void mouse_down(wxMouseEvent &e);

private:
    void color_volumes();
    Point pos, move_start;
    bool hover = false, mouse = false, moving = false;
    uint hover_volume, hover_object, moving_volume;
    
    std::vector<PlaterObject>& objects; //< reference to parent vector
    std::shared_ptr<Slic3r::Model> model;
    std::shared_ptr<Slic3r::Config> config;
};

} } // Namespace Slic3r::GUI
#endif
