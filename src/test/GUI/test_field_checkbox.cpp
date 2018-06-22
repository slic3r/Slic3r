#include <catch.hpp>

#ifndef WX_PRECOMP
    #include "wx/app.h"
    #include "wx/checkbox.h"
#endif // WX_PRECOMP

#include "testableframe.h"

SCENARIO( "checkboxes return true when checked.", "[checkbox]" ) {
    GIVEN( "A checkbox item") {
        wxCheckBox* m_check = new wxCheckBox(wxTheApp->GetTopWindow(), wxID_ANY, "Check box");
        WHEN ( "the box is checked") {
            m_check->SetValue(true);
            THEN ( "the checkbox reads true") {
                REQUIRE(m_check->IsChecked() == true);
            }
        }
        WHEN ( "the box is not checked") {
            m_check->SetValue(false);
            THEN ( "the checkbox reads false") {
                REQUIRE(m_check->IsChecked() == false);
            }
        }
    }
}
