#ifndef PREVIEW2D_HPP
#define PREVIEW2D_HPP
#include <wx/wxprec.h>
#ifndef WX_PRECOMP
    #include <wx/wx.h>
#endif

#include "Model.hpp"
#include "Config.hpp"

namespace Slic3r { namespace GUI {

class Preview2D : public wxPanel {
public:
    void reload_print() {};
    Preview2D(wxWindow* parent, const wxSize& size, std::vector<PlaterObject>& _objects, std::shared_ptr<Model> _model, std::shared_ptr<Config> _config) :
    wxPanel(parent, wxID_ANY, wxDefaultPosition, size, wxTAB_TRAVERSAL), objects(_objects), model(_model), config(_config)
    {}

    void enabled(bool enable = true) {}
private:
    std::vector<PlaterObject>& objects; //< reference to parent vector
    std::shared_ptr<Slic3r::Model> model;
    std::shared_ptr<Slic3r::Config> config;
};

} } // Namespace Slic3r::GUI
#endif
