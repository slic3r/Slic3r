#ifndef PREVIEW3D_HPP
#define PREVIEW3D_HPP
#include <wx/wxprec.h>
#ifndef WX_PRECOMP
    #include <wx/wx.h>
#endif

#include "PlaterObject.hpp"
#include "Scene3D.hpp"
#include "Model.hpp"
#include "Config.hpp"
#include "Print.hpp"

namespace Slic3r { namespace GUI {

class PreviewScene3D : public Scene3D {
public:
    PreviewScene3D(wxWindow* parent, const wxSize& size) : Scene3D(parent,size){}
    // TODO: load_print_toolpaths(Print);
    // TODO: load_print_object_toolpaths(PrintObject);
    void resetObjects(){volumes.clear();}
};

class Preview3D : public wxPanel {
public:
    void reload_print();
    Preview3D(wxWindow* parent, const wxSize& size, std::shared_ptr<Slic3r::Print> _print, std::vector<PlaterObject>& _objects, std::shared_ptr<Model> _model, std::shared_ptr<Config> _config);
    void enabled(bool enable = true) {}
private:
    void load_print();
    void set_z(float z);
    bool loaded = false, _enabled = false;
    std::vector<float> layers_z;
    std::shared_ptr<Slic3r::Print> print;
    PreviewScene3D canvas;
    wxSlider* slider;
    wxStaticText* z_label;
    std::vector<PlaterObject>& objects; //< reference to parent vector
    std::shared_ptr<Slic3r::Model> model;
    std::shared_ptr<Slic3r::Config> config;
};

} } // Namespace Slic3r::GUI
#endif
