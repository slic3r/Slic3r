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
#include "Plater/Plate3D.hpp"
#include "Plater/Preview3D.hpp"
#include "Settings.hpp"

#include "MainFrame.hpp"

namespace Slic3r { namespace GUI {

using UndoOperation = int;
using obj_index = unsigned int;

class PlaterObject;
class Plate2D;
class MainFrame;

/// Extension of wxPanel class to handle the main plater.
/// 2D, 3D, preview, etc tabs.
class Plater : public wxPanel 
{
public:
    Plater(wxWindow* parent, const wxString& title, std::shared_ptr<Settings> _settings);
    void add();
    
    /// Arrange models
    void arrange();

    /// Ask if there are any unsaved changes.
    bool prompt_unsaved_changes() { return true; }
private:
    std::shared_ptr<Slic3r::Print> print {std::make_shared<Print>(Slic3r::Print())};
    std::shared_ptr<Slic3r::Model> model {std::make_shared<Model>(Slic3r::Model())};
    std::shared_ptr<Settings> settings {};

    std::shared_ptr<Slic3r::Config> config { Slic3r::Config::new_from_defaults(
        {"bed_shape", "complete_objects", "extruder_clearance_radius", "skirts", "skirt_distance", 
        "brim_width", "serial_port", "serial_speed", "host_type", "print_host", "octoprint_apikey",
        "shortcuts", "filament_colour", "duplicate_distance"})};

    bool processed {false};

    std::vector<PlaterObject> objects {}; //< Main object vector.

    size_t object_identifier {0U}; //< Counter for adding objects to Slic3r

    std::stack<UndoOperation> undo {}; 
    std::stack<UndoOperation> redo {}; 

    wxNotebook* preview_notebook {new wxNotebook(this, -1, wxDefaultPosition, wxSize(335,335), wxNB_BOTTOM)};
    wxBoxSizer* right_sizer {new wxBoxSizer(wxVERTICAL)};

    wxToolBar* htoolbar {nullptr}; //< toolbar for non-MSW platforms.
    wxBoxSizer* btoolbar {nullptr}; //< button-based toolbar for Windows

    Plate2D* canvas2D {nullptr}; //< 2D plater canvas
    Plate3D* canvas3D {nullptr}; //< 3D plater canvas

    Preview3D* preview3D {nullptr}; //< 3D Preview 

    /// Handles the actual load of the file from the dialog handoff.
    std::vector<int> load_file(const std::string file, const int obj_idx_to_load = -1);

    const std::string LogChannel {"GUI_Plater"}; //< Which log these messages should go to.

    std::vector<int> load_model_objects(ModelObject* model_object);
    std::vector<int> load_model_objects(ModelObjectPtrs model_objects);

    bool scaled_down {false};
    bool outside_bounds {false};
    MainFrame* GetFrame();

    void select_object(size_t& obj_idx) { };
    int get_object_index(size_t object_id);

    Slic3r::Pointf bed_centerf() {
        auto bed_shape { Slic3r::Polygon::new_scale(this->config->get<ConfigOptionPoints>("bed_shape").values) };
        return Slic3r::Pointf();
    }

    /// Build thumbnails for the models
    void make_thumbnail(size_t idx);

    /// Complete thumbnail transformation and refresh canvases  
    void on_thumbnail_made(size_t idx); 
    void refresh_canvases();


    /// Run everything that needs to happen when models change.
    /// Includes updating canvases, reloading menus, etc.
    void on_model_change(bool force_autocenter = false);
};



} } // Namespace Slic3r::GUI

#endif  // PLATER_HPP
