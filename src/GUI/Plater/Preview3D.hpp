#ifndef PREVIEW3D_HPP
#define PREVIEW3D_HPP
#include <wx/wxprec.h>
#ifndef WX_PRECOMP
    #include <wx/wx.h>
#endif

namespace Slic3r { namespace GUI {

class Preview3D : public wxPanel {
public:
    void reload_print() {};
};

} } // Namespace Slic3r::GUI
#endif
