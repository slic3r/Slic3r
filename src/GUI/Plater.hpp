#ifndef PLATER_HPP
#define PLATER_HPP
#include <wx/wxprec.h>
#ifndef WX_PRECOMP
    #include <wx/wx.h>
#endif

#include <wx/notebook.h>

#include <stack>

#include "libslic3r.h"
#include "Model.hpp"
#include "Print.hpp"
#include "Config.hpp"

#include "Plater/PlaterObject.hpp"
#include "Plater/Plate2D.hpp"
#include "Settings.hpp"

namespace Slic3r { namespace GUI {

using UndoOperation = int;
using obj_index = unsigned int;

class PlaterObject;
class Plate2D;

class Plater : public wxPanel 
{
public:
    Plater(wxWindow* parent, const wxString& title, std::shared_ptr<Settings> _settings);
    void add();

private:
    std::shared_ptr<Slic3r::Print> print {std::make_shared<Print>(Slic3r::Print())};
    std::shared_ptr<Slic3r::Model> model {std::make_shared<Model>(Slic3r::Model())};
    std::shared_ptr<Settings> settings {};

    std::shared_ptr<Slic3r::Config> config { Slic3r::Config::new_from_defaults(
        {"bed_shape", "complete_objects", "extruder_clearance_radius", "skirts", "skirt_distance", 
        "brim_width", "serial_port", "serial_speed", "host_type", "print_host", "octoprint_apikey",
        "shortcuts", "filament_colour"})};

    bool processed {false};

    std::vector<PlaterObject> objects {}; //< Main object vector.

    size_t object_identifier {0U}; //< Counter for adding objects to Slic3r

    std::stack<UndoOperation> undo {}; 
    std::stack<UndoOperation> redo {}; 

    wxNotebook* preview_notebook {new wxNotebook(this, -1, wxDefaultPosition, wxSize(335,335), wxNB_BOTTOM)};

    Plate2D* canvas2D {}; //< 2D plater canvas

    /// Handles the actual load of the file from the dialog handoff.
    std::vector<int> load_file(const wxString& file, const int obj_idx_to_load = -1);

    const std::string LogChannel {"GUI_Plater"}; //< Which log these messages should go to.

    std::vector<int> load_model_objects(ModelObject* model_object);
    std::vector<int> load_model_objects(ModelObjectPtrs model_objects);

    bool scaled_down {false};
    bool outside_bounds {false};

};



} } // Namespace Slic3r::GUI

#endif  // PLATER_HPP
