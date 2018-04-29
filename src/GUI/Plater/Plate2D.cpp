#include "Plater/Plate2D.hpp"

#include <wx/colour.h>

namespace Slic3r { namespace GUI {

Plate2D::Plate2D(wxWindow* parent, const wxSize& size, std::vector<Plater2DObject>& _objects, std::shared_ptr<Model> _model, std::shared_ptr<Config> _config, std::shared_ptr<Settings> _settings) :
    wxPanel(parent, wxID_ANY, wxDefaultPosition, size, wxTAB_TRAVERSAL), objects(_objects), model(_model), config(_config), settings(_settings)
{ 
}

} } // Namespace Slic3r::GUI
