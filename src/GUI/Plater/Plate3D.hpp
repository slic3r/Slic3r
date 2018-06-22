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
    void update();
    Plate3D(wxWindow* parent, const wxSize& size, std::vector<PlaterObject>& _objects, std::shared_ptr<Model> _model, std::shared_ptr<Config> _config);
protected:
    void before_render();
    void mouse_move(wxMouseEvent &e);
    void mouse_down(wxMouseEvent &e);
private:
    void color_volumes();
    Point pos;
    bool hover = false, mouse = false;
    uint hover_volume;
    std::vector<PlaterObject>& objects; //< reference to parent vector
    std::shared_ptr<Slic3r::Model> model;
    std::shared_ptr<Slic3r::Config> config;
};

} } // Namespace Slic3r::GUI
#endif
