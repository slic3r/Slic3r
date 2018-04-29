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

namespace Slic3r { namespace GUI {

using UndoOperation = int;

class Plater2DObject;

class Plater : public wxPanel 
{
public:
    Plater(wxWindow* parent, const wxString& title);
    void add();

private:
    Print print;
    Model model;

    bool processed {false};

    std::vector<Plater2DObject> objects;

    std::stack<UndoOperation> undo {}; 
    std::stack<UndoOperation> redo {}; 

    wxNotebook* preview_notebook {new wxNotebook(this, -1, wxDefaultPosition, wxSize(335,335), wxNB_BOTTOM)};

};


// 2D Preview of an object
class Plater2DObject {
public:
    std::string name {""};
    std::string identifier {""};
    bool selected {false};
};

} } // Namespace Slic3r::GUI

#endif  // PLATER_HPP
