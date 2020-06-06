#include "FreeCADDialog.hpp"
#include "I18N.hpp"
#include "libslic3r/Utils.hpp"
#include "GUI.hpp"
#include "GUI_ObjectList.hpp"
#include "AppConfig.hpp"
#include "Tab.hpp"
#include <wx/scrolwin.h>
#include <wx/display.h>
#include <wx/file.h>
#include <wx/gbsizer.h>
#include "wxExtensions.hpp"
#include <iostream>

//C++11

#include <stdio.h>
#include <stdlib.h>
#include <boost/process.hpp>
#include <boost/iostreams/device/file_descriptor.hpp>
#include <boost/iostreams/stream.hpp>
#include <boost/iostreams/tee.hpp>
#include <boost/iostreams/filtering_stream.hpp>

#if ENABLE_SCROLLABLE
static wxSize get_screen_size(wxWindow* window)
{
    const auto idx = wxDisplay::GetFromWindow(window);
    wxDisplay display(idx != wxNOT_FOUND ? idx : 0u);
    return display.GetClientArea().GetSize();
}
#endif // ENABLE_SCROLLABLE

namespace Slic3r {
namespace GUI {

std::string create_help_text() {
    std::stringstream ss;

    ss << " == common 3D primitives ==\n";
    ss << "cube(x,y,z)\n";
    ss << "cylinder(r|d,h)\n";
    ss << "poly_int(a,nb)\n";
    ss << "poly_ext(r,nb)\n";
    ss << "cone(r1|d2,r2|d2,h)\n";
    ss << "iso_thread(d,p,h,internal,offset)\n";
    ss << "solid_slices(array_points, centers)\n";
    ss << "importStl(filepath)\n"; 
    ss << " == common 3D operation ==\n";
    ss << "cut()(...3D)\n";
    ss << "union()(...3D)\n";
    ss << "intersection()(...3D)\n";
    ss << " == common object modifier ==\n";
    ss << ".x/y/z() | .center()\n";
    ss << ".move(x,y,z)\n";
    ss << ".rotate(x,y,z)\n";
    ss << " == common 1D primitives ==\n";
    ss << "line([x1,y1,z1],[x2,y2,z2])\n";
    ss << "arc([x1,y1,z1],[x2,y2,z2],[x3,y3,z3])\n";
    ss << " == common 1D or 2D primitives ==\n";
    ss << "circle(r)\n";
    ss << "polygon([points],closed)\n";
    ss << "bezier([points],closed)\n";
    ss << "create_wire(closed)(...1D)\n";
    ss << " == common 2D primitives ==\n";
    ss << "square(width,height)\n";
    ss << "text(text,size)\n";
    ss << "gear(nb, mod, angle, isext, hprec)\n";
    ss << " === 2D to 3D (single object) ===\n";
    ss << "linear_extrude(height,twist,taper)(obj_2D)\n";
    ss << "extrude(x,y,z,taper)(obj_2D)\n";
    ss << "rotate_extrude(angle)(obj_2D)\n";
    ss << "path_extrude(frenet,transition)(path_1D, patron_2D)\n";
    return ss.str();
}

FreeCADDialog::FreeCADDialog(GUI_App* app, MainFrame* mainframe)
    : DPIDialog(NULL, wxID_ANY, wxString(SLIC3R_APP_NAME) + " - " + _(L("FreeCAD script engine")),
#if ENABLE_SCROLLABLE
        wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)
#else
    wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE)
#endif // ENABLE_SCROLLABLE
{

    this->gui_app = app;
    this->main_frame = mainframe;
    SetBackgroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOW));

    // fonts
    const wxFont& font = wxGetApp().normal_font();
    const wxFont& bold_font = wxGetApp().bold_font();
    SetFont(font);


    auto main_sizer = new wxGridBagSizer(5,5); //(int rows, int cols, int vgap, int hgap)

    //main view
    createSTC();

    m_errors = new wxTextCtrl(this, wxID_ANY, "",
        wxDefaultPosition, wxSize(600, 100), wxHW_SCROLLBAR_AUTO | wxTE_MULTILINE);
    m_errors->SetEditable(false);

    m_help = new wxTextCtrl(this, wxID_ANY, create_help_text(),
        wxDefaultPosition, wxSize(200, 500), wxTE_MULTILINE);
    m_help->SetEditable(false);

    //wxSizerItem* Add(wxSizer * sizer, const wxGBPosition & pos, const wxGBSpan & span = wxDefaultSpan, int flag = 0, int border = 0, wxObject * userData = NULL)
    //wxSizerItem* Add(wxSizer * sizer,                                              int proportion = 0, int flag = 0, int border = 0, wxObject * userData = NULL)
    main_sizer->Add(m_text, wxGBPosition(1,1), wxGBSpan(1,1), wxEXPAND | wxALL, 5);
    main_sizer->Add(m_help, wxGBPosition(1, 2), wxGBSpan(2, 1), wxEXPAND | wxVERTICAL, 5);
    main_sizer->Add(m_errors, wxGBPosition(2, 1), wxGBSpan(1, 1), 0, 5);
    /*main_sizer->Add(m_text, 1, wxEXPAND | wxALL, 5);
    main_sizer->Add(m_errors, 1, wxALL, 5);
    main_sizer->Add(m_help, 1, wxALL, 5);*/

    wxStdDialogButtonSizer* buttons = new wxStdDialogButtonSizer();

    wxButton* bt = new wxButton(this, wxID_FILE1, _(L("Generate")));
    bt->Bind(wxEVT_BUTTON, &FreeCADDialog::create_geometry, this);
    buttons->Add(bt);

    wxButton* close = new wxButton(this, wxID_CLOSE, _(L("Close")));
    close->Bind(wxEVT_BUTTON, &FreeCADDialog::closeMe, this);
    buttons->AddButton(close);
    close->SetDefault();
    close->SetFocus();
    SetAffirmativeId(wxID_CLOSE);
    buttons->Realize();
    main_sizer->Add(buttons, wxGBPosition(3, 1), wxGBSpan(1, 2), wxEXPAND | wxALL, 5);

    SetSizer(main_sizer);
    main_sizer->SetSizeHints(this);

}

void FreeCADDialog::closeMe(wxCommandEvent& event_args) {
    this->gui_app->change_calibration_dialog(this, nullptr);
    this->Destroy();
}


void FreeCADDialog::createSTC()
{
    m_text = new wxStyledTextCtrl(this, wxID_ANY,
        wxDefaultPosition, wxSize(600,400), wxHW_SCROLLBAR_AUTO);

    //m_text->SetMarginWidth(MARGIN_LINE_NUMBERS, 50);
    m_text->StyleSetForeground(wxSTC_STYLE_LINENUMBER, wxColour(75, 75, 75));
    m_text->StyleSetBackground(wxSTC_STYLE_LINENUMBER, wxColour(220, 220, 220));
    //m_text->SetMarginType(MARGIN_LINE_NUMBERS, wxSTC_MARGIN_NUMBER);

    m_text->SetWrapMode(wxSTC_WRAP_WORD);

    m_text->StyleClearAll();
    m_text->SetLexer(wxSTC_LEX_PYTHON);
    /*
    
    
#define wxSTC_P_DEFAULT 0
#define wxSTC_P_COMMENTLINE 1
#define wxSTC_P_NUMBER 2
#define wxSTC_P_STRING 3
#define wxSTC_P_CHARACTER 4
#define wxSTC_P_WORD 5
#define wxSTC_P_TRIPLE 6
#define wxSTC_P_TRIPLEDOUBLE 7
#define wxSTC_P_CLASSNAME 8
#define wxSTC_P_DEFNAME 9
#define wxSTC_P_OPERATOR 10
#define wxSTC_P_IDENTIFIER 11
#define wxSTC_P_COMMENTBLOCK 12
#define wxSTC_P_STRINGEOL 13
#define wxSTC_P_WORD2 14
#define wxSTC_P_DECORATOR 15
    */
    m_text->StyleSetForeground(wxSTC_P_DEFAULT, wxColour(255, 0, 0));
    m_text->StyleSetForeground(wxSTC_P_COMMENTLINE, wxColour(255, 0, 0));
    m_text->StyleSetForeground(wxSTC_P_NUMBER, wxColour(255, 0, 0));
    m_text->StyleSetForeground(wxSTC_P_STRING, wxColour(0, 150, 0));
    m_text->StyleSetForeground(wxSTC_H_TAGUNKNOWN, wxColour(0, 150, 0));
    m_text->StyleSetForeground(wxSTC_H_ATTRIBUTE, wxColour(0, 0, 150));
    m_text->StyleSetForeground(wxSTC_H_ATTRIBUTEUNKNOWN, wxColour(0, 0, 150));
    m_text->StyleSetForeground(wxSTC_H_COMMENT, wxColour(150, 150, 150));
    m_text->StyleSetForeground(wxSTC_P_DEFAULT, wxColour(128, 128, 128));
    m_text->StyleSetForeground(wxSTC_P_COMMENTLINE, wxColour(0, 128, 0));
    m_text->StyleSetForeground(wxSTC_P_NUMBER, wxColour(0, 128, 128)); 
    m_text->StyleSetForeground(wxSTC_P_STRING, wxColour(128, 0, 128));
    m_text->StyleSetItalic(wxSTC_P_STRING,true),
    m_text->StyleSetForeground(wxSTC_P_CHARACTER, wxColour(128, 0, 128));
    m_text->StyleSetItalic(wxSTC_P_CHARACTER, true),
    m_text->StyleSetForeground(wxSTC_P_WORD, wxColour(0, 0, 128));
    m_text->StyleSetBold(wxSTC_P_WORD, true),
    m_text->StyleSetForeground(wxSTC_P_TRIPLE, wxColour(128, 0, 0));
    m_text->StyleSetForeground(wxSTC_P_TRIPLEDOUBLE, wxColour(128, 0, 0));
    m_text->StyleSetForeground(wxSTC_P_CLASSNAME, wxColour(0, 0, 255));
    m_text->StyleSetBold(wxSTC_P_CLASSNAME, true),
    m_text->StyleSetForeground(wxSTC_P_DEFNAME, wxColour(0, 128, 128));
    m_text->StyleSetBold(wxSTC_P_DEFNAME, true),
    m_text->StyleSetForeground(wxSTC_P_OPERATOR, wxColour(0, 0, 0));
    m_text->StyleSetBold(wxSTC_P_OPERATOR, true),
    m_text->StyleSetForeground(wxSTC_P_IDENTIFIER, wxColour(128, 128, 128)); 
    m_text->StyleSetForeground(wxSTC_P_COMMENTBLOCK, wxColour(128, 128, 128));
    m_text->StyleSetForeground(wxSTC_P_STRINGEOL, wxColour(0, 0, 0));
    m_text->StyleSetBackground(wxSTC_P_STRINGEOL, wxColour(224, 192, 224));

}

void FreeCADDialog::on_dpi_changed(const wxRect& suggested_rect)
{
    msw_buttons_rescale(this, em_unit(), { wxID_OK });

    Layout();
    Fit();
    Refresh();
}
void FreeCADDialog::create_geometry(wxCommandEvent& event_args) {
    //Create the script

    std::string program = "C:/Program Files/FreeCAD 0.18/bin/python.exe";

    // cleaning
    boost::filesystem::path object_path(Slic3r::resources_dir());
    object_path = object_path / "generation" / "freecad" / "temp.amf";
    m_errors->Clear();
    if (exists(object_path)) {
        remove(object_path);
    }

    //create synchronous pipe streams
    boost::process::opstream pyin;
    //boost::process::ipstream pyout;
    //boost::process::ipstream pyerr;

    boost::asio::io_service ios;
    std::future<std::string> dataOut;
    std::future<std::string> dataErr;

    boost::filesystem::path pythonpath(gui_app->app_config->get("freecad_path"));  
    pythonpath = pythonpath / "python.exe";
    if (!exists(pythonpath)) {
        m_errors->AppendText("Error, cannot find the freecad python at '"+ pythonpath.string()+"', please update your freecad bin directory in the preferences.");
        return;
    }
    boost::filesystem::path pyscad_path(Slic3r::resources_dir());
    pyscad_path = pyscad_path / "generation" / "freecad";

    boost::process::child c(pythonpath.string() +" -u -i", boost::process::std_in < pyin, boost::process::std_out > dataOut, boost::process::std_err > dataErr, ios);
    pyin << "import FreeCAD" << std::endl;
    pyin << "import Part" << std::endl;
    pyin << "import Draft" << std::endl;
    pyin << "import sys" << std::endl;
    pyin << "sys.path.append('"<< pyscad_path.generic_string() <<"')"<<std::endl;
    pyin << "from Pyscad.pyscad import *" << std::endl;
    pyin << "App.newDocument(\"document\")" << std::endl;
    pyin << "scene().redraw("<< m_text->GetText() <<")" << std::endl;
    pyin << "Mesh.export(App.ActiveDocument.RootObjects, u\"" << object_path.generic_string() << "\")" << std::endl;
    pyin << "print('exported!')" << std::endl;
    pyin << "quit()" << std::endl;
    c.wait();
    ios.run(); 
    std::cout << "exit with code " << c.exit_code() << "\n";

    std::string pyout_str_hello;
    std::cout << "==cout==\n";
    std::cout << dataOut.get();
    std::cout << "==cerr==\n";
    std::string errStr = dataErr.get();
    std::cout << errStr << "\n";
    std::string cleaned = boost::replace_all_copy(boost::replace_all_copy(errStr, ">>> ", ""),"\r","");
    m_errors->AppendText(cleaned);
    std::cout << std::count(cleaned.begin(), cleaned.end(), '\n') << "\n";
    std::cout << std::count(cleaned.begin(), cleaned.end(), '\r') << "\n";
    
    if (!exists(object_path)) {
        m_errors->AppendText("\nError, no object generated.");
        return;
    }

    Plater* plat = this->main_frame->plater();
    Model& model = plat->model();
    plat->reset();
    std::vector<size_t> objs_idx = plat->load_files(std::vector<std::string>{ object_path.generic_string() }, true, false);
    if (objs_idx.empty()) return;
    /// --- translate ---
    const DynamicPrintConfig* printerConfig = this->gui_app->get_tab(Preset::TYPE_PRINTER)->get_config();
    const ConfigOptionPoints* bed_shape = printerConfig->option<ConfigOptionPoints>("bed_shape");
    Vec2d bed_size = BoundingBoxf(bed_shape->values).size();
    Vec2d bed_min = BoundingBoxf(bed_shape->values).min;
    model.objects[objs_idx[0]]->translate({ bed_min.x() + bed_size.x() / 2, bed_min.y() + bed_size.y() / 2, 0 });

    //update plater
    plat->changed_objects(objs_idx);
    //update everything, easier to code.
    ObjectList* obj = this->gui_app->obj_list();
    obj->update_after_undo_redo();


    //plat->reslice();
    plat->select_view_3D("3D");
}

} // namespace GUI
} // namespace Slic3r
