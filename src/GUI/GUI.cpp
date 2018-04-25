#include <wx/wxprec.h>
#ifndef WX_PRECOMP
    #include <wx/wx.h>
#endif

#include "MainFrame.hpp"
#include "GUI.hpp"

namespace Slic3r { namespace GUI {

enum
{
    ID_Hello = 1
};
bool App::OnInit()
{
    MainFrame *frame = new MainFrame( "Slic3r", wxDefaultPosition, wxDefaultSize, this->gui_config);
  
    frame->Show( true );

    this->SetAppName("Slic3r");

    return true;
}

}} // namespace Slic3r::GUI
