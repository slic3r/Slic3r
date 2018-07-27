#ifndef OBJECTCUTDIALOG_HPP
#define OBJECTCUTDIALOG_HPP


#include <wx/dialog.h>

#include "Scene3D.hpp"
#include "Model.hpp"

namespace Slic3r { namespace GUI {

class ObjectCutCanvas : public Scene3D {
    // Not sure yet

protected:
    // Draws the cutting plane
    void after_render();
};

struct CutOptions {
    float z;
    enum {X,Y,Z} axis;
    bool keep_upper, keep_lower, rotate_lower;
    bool preview;
};

class ObjectCutDialog : public wxDialog {
public:
    ObjectCutDialog(wxWindow* parent, ModelObject* _model);
private:
    // Mark whether the mesh cut is valid.
    // If not, it needs to be recalculated by _update() on wxTheApp->CallAfter() or on exit of the dialog.
    bool mesh_cut_valid = false;
    // Note whether the window was already closed, so a pending update is not executed.
    bool already_closed = false;
    
//    ObjectCutCanvas canvas;
    ModelObject* model_object;
    
    //std::shared_ptr<Slic3r::Model> model;

    void _mesh_slice_z_pos();
    void _life_preview_active();
    void _perform_cut();
    void _update();
    void NewModelObjects();

};

}} // namespace Slic3r::GUI
#endif // OBJECTCUTDIALOG_HPP
