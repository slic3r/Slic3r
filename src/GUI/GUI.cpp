#include <wx/wxprec.h>
#ifndef WX_PRECOMP
    #include <wx/wx.h>
#endif

#include "MainFrame.hpp"
#include "GUI.hpp"
//using namespace Slic3r;


enum
{
    ID_Hello = 1
};
bool Slic3rGUI::OnInit()
{
    MainFrame *frame = new MainFrame( "Slic3r", wxDefaultPosition, wxDefaultSize);
  
    frame->Show( true );
    return true;
}

