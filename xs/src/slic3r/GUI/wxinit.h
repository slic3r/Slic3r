#ifndef slic3r_wxinit_hpp_
#define slic3r_wxinit_hpp_

#include <wx/wx.h>
#include <wx/html/htmlwin.h>

// Perl redefines a _ macro, so we undef this one
#undef _

// legacy macros
// https://wiki.wxwidgets.org/EventTypes_and_Event-Table_Macros
#ifndef wxEVT_BUTTON
#define wxEVT_BUTTON wxEVT_COMMAND_BUTTON_CLICKED
#endif

#ifndef wxEVT_HTML_LINK_CLICKED
#define wxEVT_HTML_LINK_CLICKED wxEVT_COMMAND_HTML_LINK_CLICKED
#endif

#endif
