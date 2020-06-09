#include "FreeCADDialog.hpp"
#include "I18N.hpp"
#include "libslic3r/Utils.hpp"
#include "GUI.hpp"
#include "GUI_ObjectList.hpp"
#include "slic3r/Utils/Http.hpp"
#include "AppConfig.hpp"
#include "Tab.hpp"
#include <wx/scrolwin.h>
#include <wx/display.h>
#include <wx/file.h>
#include <wx/gbsizer.h>
#include "wxExtensions.hpp"
#include <iostream>

//C++11

#include <ctime>
#include <stdio.h>
#include <stdlib.h>
#include <regex>
#include <boost/locale.hpp>
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

    //TODO: auto tab

    // Downloads a file (http get operation). Cancels if the Updater is being destroyed.
    bool get_file_from_web(const std::string &url, const boost::filesystem::path &target_path)
    {
        bool res = false;
        boost::filesystem::path tmp_path = target_path;
        tmp_path += (boost::format(".%1%%2%") % get_current_pid() % ".download").str();

        BOOST_LOG_TRIVIAL(info) << boost::format("Get: `%1%`\n\t-> `%2%`\n\tvia tmp path `%3%`")
            % url
            % target_path.string()
            % tmp_path.string();

        Slic3r::Http::get(url)
            .on_progress([](Http::Progress, bool &cancel) {
        })
            .on_error([&](std::string body, std::string error, unsigned http_status) {
            (void)body;
            BOOST_LOG_TRIVIAL(error) << boost::format("Error getting: `%1%`: HTTP %2%, %3%")
                % url
                % http_status
                % error;
        })
            .on_complete([&](std::string body, unsigned /* http_status */) {
            boost::filesystem::fstream file(tmp_path, std::ios::out | std::ios::binary | std::ios::trunc);
            file.write(body.c_str(), body.size());
            file.close();
            boost::filesystem::rename(tmp_path, target_path);
            res = true;
        })
            .perform_sync();

        return res;
    }

    // Downloads a file (http get operation). Cancels if the Updater is being destroyed.
    void get_string_from_web_async(const std::string &url, FreeCADDialog* free, std::function<void(FreeCADDialog*, std::string)> listener)
    {

        Slic3r::Http::get(url)
            .on_progress([](Http::Progress, bool &cancel) {
        })
            .on_error([&url](std::string body, std::string error, unsigned http_status) {
            (void)body;
            BOOST_LOG_TRIVIAL(error) << boost::format("Error getting: `%1%`: HTTP %2%, %3%")
                % url
                % http_status
                % error;
        })
            .on_complete([free, listener](std::string body, unsigned http_status ) {
            listener(free, body);
        })
            .perform();
    }

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
    : DPIDialog(NULL, wxID_ANY, wxString(SLIC3R_APP_NAME) + " - " + _(L("FreePySCAD : script engine for FreeCAD")),
#if ENABLE_SCROLLABLE
        wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)
#else
    wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE)
#endif // ENABLE_SCROLLABLE
{

    this->gui_app = app;
    this->main_frame = mainframe;
    SetBackgroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOW));

    //def
    PyCommand py1{ "cube", PyCommandType::pctOBJECT, {"x","y","z"}, "cube(x,y,z)\ncube(l)" };
    PyCommand py2 = { "cube", PyCommandType::pctOBJECT, {"x","y","z"}, "cube(x,y,z)\ncube(l)" };
    commands.emplace_back(PyCommand{ "cube", PyCommandType::pctOBJECT, {"x","y","z"}, "cube(x,y,z)\ncube(l)" });
    commands.emplace_back(PyCommand{"cylinder", PyCommandType::pctOBJECT, { "r","h","fn=","angle=","d=","r1=","r2=","d1=","d2=" },
        "cylinder(r,h)\ncylinder(d=,h=,[fn=,angle=])\ncylinder(r1=,r2=,h=)\ncylinder(d1=,d2=,h=)"});
    commands.emplace_back(PyCommand{"move", PyCommandType::pctMODIFIER, { "x","y","z" }, "move(x,y,z)"});
    commands.emplace_back(PyCommand{"rotate", PyCommandType::pctMODIFIER, { "x","y","z" }, "rotate(x,y,z)"});
    commands.emplace_back(PyCommand{"cut", PyCommandType::pctOPERATION | PyCommandType::pctNO_PARAMETER,"cut()(...obj)"});
    commands.emplace_back(PyCommand{"union", PyCommandType::pctOPERATION | PyCommandType::pctNO_PARAMETER, "union()(...obj)"});
    commands.emplace_back(PyCommand{"intersection", PyCommandType::pctOPERATION | PyCommandType::pctNO_PARAMETER, "intersection()(...obj)"});
    commands.emplace_back(PyCommand{"linear_extrude", PyCommandType::pctOPERATION,"linear_extrude(height,[twist=,taper=,slices=,convexity=])(2D_obj)"});
    commands.emplace_back(PyCommand{"rotate_extrude", PyCommandType::pctOPERATION, "rotate_extrude(angle,[convexity])(2D_obj)"});
    commands.emplace_back(PyCommand{"path_extrude", PyCommandType::pctOPERATION, "path_extrude(frenet,transition)(2D_obj)"});
    commands.emplace_back(PyCommand{"mirror", PyCommandType::pctOPERATION, { "x","y","z" }, "mirror(x,y,z)(obj)"});
    commands.emplace_back(PyCommand{"offset", PyCommandType::pctOPERATION, { "length","fillet" }, "offset(length,fillet)(...obj)"});
    commands.emplace_back(PyCommand{"chamfer", PyCommandType::pctOPERATION, { "l" }, "chamfer(l)(...obj)"});
    commands.emplace_back(PyCommand{"fillet", PyCommandType::pctOPERATION, { "l" }, "fillet(l)(...obj)"});
    commands.emplace_back(PyCommand{"poly_ext", PyCommandType::pctOBJECT, {"r", "nb", "h", "d="}, "poly_ext(r,nb,h)\npoly_ext(d=,nb=,h=)"});
    commands.emplace_back(PyCommand{"poly_int", PyCommandType::pctOBJECT, { "a", "nb", "h", "d=" }, "poly_int(a,nb,h)\npoly_int(d=,nb=,h=)"});
    commands.emplace_back(PyCommand{"triangle", PyCommandType::pctOBJECT, { "x","y","z" }, "triangle(x,y,z)"});
    commands.emplace_back(PyCommand{ "iso_thread", PyCommandType::pctOBJECT, {"d","p","h","internal","offset","fn="},
        "iso_thread(d,p,h,internal, offset,[fn=])\nm3 screw: iso_thread(3,0.5,10,False,0)\nm3 nut: cut()(...,iso_thread(3,0.5,3,True,0.15))" });
    commands.emplace_back(PyCommand{"text", PyCommandType::pctOBJECT});
    commands.emplace_back(PyCommand{"gear", PyCommandType::pctOBJECT});
    commands.emplace_back(PyCommand{"importStl", PyCommandType::pctOBJECT, "importStl(filename,ids)"});
    commands.emplace_back(PyCommand{"solid_slices", PyCommandType::pctOBJECT});
    commands.emplace_back(PyCommand{"create_wire", PyCommandType::pctOPERATION, { "closed" }, "create_wire(closed)(...1D_obj)"});
    commands.emplace_back(PyCommand{"line", PyCommandType::pctOBJECT});
    commands.emplace_back(PyCommand{"arc", PyCommandType::pctOBJECT});
    commands.emplace_back(PyCommand{"circle", PyCommandType::pctOBJECT});
    commands.emplace_back(PyCommand{"polygon", PyCommandType::pctOBJECT});
    commands.emplace_back(PyCommand{"bezier", PyCommandType::pctOBJECT});
    commands.emplace_back(PyCommand{"square", PyCommandType::pctOBJECT});
    commands.emplace_back(PyCommand{"importSvg", PyCommandType::pctOBJECT, "importSvg(filename,ids)"});
    commands.emplace_back(PyCommand{"xy", PyCommandType::pctMODIFIER | PyCommandType::pctNO_PARAMETER});
    commands.emplace_back(PyCommand{"z", PyCommandType::pctMODIFIER | PyCommandType::pctNO_PARAMETER});
    commands.emplace_back(PyCommand{"center", PyCommandType::pctMODIFIER | PyCommandType::pctNO_PARAMETER});
    commands.emplace_back(PyCommand{"x", PyCommandType::pctMODIFIER | PyCommandType::pctNO_PARAMETER});
    commands.emplace_back(PyCommand{"y", PyCommandType::pctMODIFIER | PyCommandType::pctNO_PARAMETER});
    commands.emplace_back(PyCommand{"xz", PyCommandType::pctMODIFIER | PyCommandType::pctNO_PARAMETER});
    commands.emplace_back(PyCommand{"yz", PyCommandType::pctMODIFIER | PyCommandType::pctNO_PARAMETER});
    commands.emplace_back(PyCommand{"xyz", PyCommandType::pctMODIFIER | PyCommandType::pctNO_PARAMETER});
    // alias
    commands.emplace_back(PyCommand{"difference", PyCommandType::pctOPERATION | PyCommandType::pctNO_PARAMETER, "difference()(...obj)"});
    commands.emplace_back(PyCommand{"translate", PyCommandType::pctMODIFIER, "translate(x,y,z)"});
    commands.emplace_back(PyCommand{"extrude", PyCommandType::pctOPERATION, "extrude(x,y,z,taper,[convexity=])"});
    //redraw
    commands.emplace_back(PyCommand{"redraw", PyCommandType::pctOPERATION | PyCommandType::pctNO_PARAMETER,
        "redraw(...obj3D)\nEvery object inside this command\nwill be added into SuperSlicer.\n"});
    // beta / buggy
    commands.emplace_back(PyCommand{"scale", PyCommandType::pctMODIFIER | PyCommandType::pctDO_NOT_SHOW});

    word_regex = std::regex("[a-z]+");

    // fonts
    const wxFont& font = wxGetApp().normal_font();
    const wxFont& bold_font = wxGetApp().bold_font();
    SetFont(font);

    wxIcon *freecad_icon = new wxIcon();
    freecad_icon->LoadFile( (boost::filesystem::path(Slic3r::resources_dir()) / "icons" / "Freecad.svg").generic_string());
    //sadly, this do nothing in dialog
    //this->SetIcon(freecad_icon);

    auto main_sizer = new wxGridBagSizer(1,1); //(int vgap, int hgap)
    // |       |_icon_|
    // |editor_| help |
    // |_err___|______|
    // |__bts_________|

    //main view
    createSTC();

    m_errors = new wxTextCtrl(this, wxID_ANY, "",
        wxDefaultPosition, wxSize(600, 100), wxHW_SCROLLBAR_AUTO | wxTE_MULTILINE);
    m_errors->SetEditable(false);

    m_help = new wxTextCtrl(this, wxID_ANY, create_help_text(),
        wxDefaultPosition, wxSize(200, 500), wxTE_MULTILINE);
    m_help->SetEditable(false);

    //wxBoxSizer *m_icons = new wxBoxSizer(wxHORIZONTAL);
    //m_icons->Add(16,16,0,0,0,freecad_icon);

    //wxSizerItem* Add(wxSizer * sizer, const wxGBPosition & pos, const wxGBSpan & span = wxDefaultSpan, int flag = 0, int border = 0, wxObject * userData = NULL)
    //wxSizerItem* Add(wxSizer * sizer,                                              int proportion = 0, int flag = 0, int border = 0, wxObject * userData = NULL)
    main_sizer->Add(m_text, wxGBPosition(1,1), wxGBSpan(2,1), wxEXPAND | wxALL, 2);
    //main_sizer->Add(m_icons, wxGBPosition(1, 2), wxGBSpan(1, 1), 0, 2);
    //main_sizer->Add(m_help, wxGBPosition(2, 2), wxGBSpan(2, 1), wxEXPAND | wxVERTICAL, 2);
    main_sizer->Add(m_help, wxGBPosition(1, 2), wxGBSpan(3, 1), wxEXPAND | wxVERTICAL, 2);
    main_sizer->Add(m_errors, wxGBPosition(3, 1), wxGBSpan(1, 1), 0, 2);
    
    wxStdDialogButtonSizer* buttons = new wxStdDialogButtonSizer();

    wxButton* bt_create_geometry = new wxButton(this, wxID_FILE1, _(L("Generate")));
    bt_create_geometry->Bind(wxEVT_BUTTON, &FreeCADDialog::create_geometry, this);
    buttons->Add(bt_create_geometry);
    wxButton* bt_new = new wxButton(this, wxID_FILE3, _(L("New")));
    bt_new->Bind(wxEVT_BUTTON, &FreeCADDialog::new_script, this);
    buttons->Add(bt_new);
    wxButton* bt_load = new wxButton(this, wxID_FILE2, _(L("Load")));
    bt_load->Bind(wxEVT_BUTTON, &FreeCADDialog::load_script, this);
    buttons->Add(bt_load);
    wxButton* bt_save = new wxButton(this, wxID_FILE3, _(L("Save")));
    bt_save->Bind(wxEVT_BUTTON, &FreeCADDialog::save_script, this);
    buttons->Add(bt_save);
    wxButton* bt_quick_save = new wxButton(this, wxID_FILE4, _(L("Quick Save")));
    bt_quick_save->Bind(wxEVT_BUTTON, &FreeCADDialog::quick_save, this);
    bt_quick_save->Hide();
    buttons->Add(bt_quick_save);

    wxButton* close = new wxButton(this, wxID_CLOSE, _(L("Close")));
    close->Bind(wxEVT_BUTTON, &FreeCADDialog::closeMe, this);
    buttons->AddButton(close);
    close->SetDefault();
    close->SetFocus();
    SetAffirmativeId(wxID_CLOSE);
    buttons->Realize();
    main_sizer->Add(buttons, wxGBPosition(4, 1), wxGBSpan(1, 2), wxEXPAND | wxALL, 5);

    SetSizer(main_sizer);
    main_sizer->SetSizeHints(this);

    //set keyboard shortcut
    wxAcceleratorEntry entries[2];
    //entries[0].Set(wxACCEL_CTRL, (int) 'X', bt_create_geometry->GetId());
    //entries[2].Set(wxACCEL_SHIFT, (int) 'W', wxID_FILE1);
    entries[0].Set(wxACCEL_NORMAL, WXK_ESCAPE, wxID_CLOSE);
    entries[1].Set(wxACCEL_NORMAL, WXK_F5, bt_create_geometry->GetId()); // wxID_FILE1);
    entries[1].Set(wxACCEL_CTRL, (int) 'N', bt_new->GetId());
    entries[1].Set(wxACCEL_CTRL | wxACCEL_SHIFT, (int) 'S', bt_save->GetId());
    entries[1].Set(wxACCEL_CTRL, (int) 'S', bt_quick_save->GetId());
    this->SetAcceleratorTable(wxAcceleratorTable(2, entries));

}

void FreeCADDialog::closeMe(wxCommandEvent& event_args) {
    bool ok = this->write_text_in_file(m_text->GetText(), boost::filesystem::path(Slic3r::data_dir()) / "temp" / "current_pyscad.py");
    this->gui_app->change_calibration_dialog(this, nullptr);
    this->Destroy();
}

void FreeCADDialog::load_script(wxCommandEvent& event_args) {
    wxFileDialog dialog(this,
        _(L("Choose one file (py):")),
        gui_app->app_config->get_last_dir(), 
        "",
        "FreePySCAD files (*.py)|*.py",
        wxFD_OPEN | wxFD_FILE_MUST_EXIST);

    if (dialog.ShowModal() == wxID_OK) {
        opened_file = boost::filesystem::path(dialog.GetPath().wx_str());
        load_text_from_file(opened_file);
    }
}

bool FreeCADDialog::load_text_from_file(const boost::filesystem::path &path) {
    if (boost::filesystem::exists(path)) {
        try {
            std::locale loc = boost::locale::generator()("en_US.UTF-8");
            // Open the stream to 'lock' the file.
            boost::filesystem::ifstream in;
            in.imbue(loc);
            in.open(path);
            // Obtain the size of the file.
            const uintmax_t sz = boost::filesystem::file_size(path);
            // Create a buffer.
            std::string result(sz, '\0');
            // Read the whole file into the buffer.
            in.read(result.data(), sz);
            m_text->SetTextRaw(result.c_str());
            in.close();
        }
        catch (std::exception ex) {
            //TODO: emit error meessage on std / boost
            return false;
        }
        return true;
    }
    return false;
}

void FreeCADDialog::save_script(wxCommandEvent& event_args) {
    wxFileDialog dialog(this,
        _(L("Choose one file (py):")),
        gui_app->app_config->get_last_dir(),
        "",
        "FreePySCAD files (*.py)|*.py",
        wxFD_SAVE);

    if (dialog.ShowModal() == wxID_OK) {
        opened_file = boost::filesystem::path(dialog.GetPath().wx_str());
        write_text_in_file(m_text->GetText(), opened_file);
    }
}
void FreeCADDialog::quick_save(wxCommandEvent& event_args) {

    if (boost::filesystem::exists(opened_file) ){
        write_text_in_file(m_text->GetText(), opened_file);
    }
}

bool FreeCADDialog::write_text_in_file(const wxString &towrite, const boost::filesystem::path &file) {
    try {
        //add text if the saved file exist
        boost::filesystem::create_directories(file.parent_path());
        std::locale loc = boost::locale::generator()("en_US.UTF-8");
        // Open the stream to 'lock' the file.
        boost::filesystem::ofstream out;
        out.imbue(loc);
        out.open(file);
        out << towrite;
        out.close();
    }
    catch (std::exception ex) {
        return false;
    }
    return true;
}

const PyCommand* FreeCADDialog::get_command(const wxString &str) const {
    const PyCommand *command = nullptr;
    for (const PyCommand &cmd : commands) {
        if (cmd.name == str) {
            command = &cmd;
            break;
        }
    }
    return command;
}

void FreeCADDialog::on_autocomp_complete(wxStyledTextEvent& event) {
    //ad extra characters after autocomplete
    const PyCommand *command = get_command(event.GetString());
    if (command == nullptr)
        return;
    wxStyledTextCtrl* stc = (wxStyledTextCtrl*)event.GetEventObject();
    int currentPos = stc->GetCurrentPos();
    bool has_already_parenthese = stc->GetCharAt(currentPos) == '(';
    if ( ((command->type & PyCommandType::pctOPERATION) != 0) && !has_already_parenthese) {
        stc->InsertText(currentPos, "()(),");
        if(((command->type & PyCommandType::pctNO_PARAMETER) != 0))
            stc->GotoPos(currentPos + 3);
        else
            stc->GotoPos(currentPos + 1);
    } else if (((command->type & PyCommandType::pctOBJECT) != 0) && !has_already_parenthese) {
        stc->InsertText(currentPos, "(),");
        if (((command->type & PyCommandType::pctNO_PARAMETER) != 0))
            stc->GotoPos(currentPos + 2);
        else
            stc->GotoPos(currentPos + 1);
    } else if (((command->type & PyCommandType::pctMODIFIER) != 0) && !has_already_parenthese) {
        stc->InsertText(currentPos, "()");
        if (((command->type & PyCommandType::pctNO_PARAMETER) != 0))
            stc->GotoPos(currentPos + 2);
        else
            stc->GotoPos(currentPos + 1);
        //TODO: check if it's a object.modif(), a modif()(object) a modif().object ?
    }
    if (!command->tooltip.empty())
        stc->CallTipShow(currentPos, command->tooltip);
}

void FreeCADDialog::on_word_change_for_autocomplete(wxStyledTextEvent& event) {
    wxStyledTextCtrl* stc = (wxStyledTextCtrl*)event.GetEventObject();
    // Find the word start
    int current_pos = stc->GetCurrentPos();
    int word_start_pos = stc->WordStartPosition(current_pos, true);
    int len_entered = current_pos - word_start_pos;
    const wxString str = stc->GetTextRange(word_start_pos, current_pos + 1);
    const wxString event_string = event.GetString();
    if ((event.GetModificationType() & (wxSTC_MOD_INSERTTEXT | wxSTC_PERFORMED_USER)) != (wxSTC_MOD_INSERTTEXT | wxSTC_PERFORMED_USER)) {
        return; // not our event
    }
    //std::cout << "word_change "<<event.GetModificationType()<<" typed: " << (int)stc->GetCharAt(current_pos) << " with len_entered " << len_entered
    //<< ", event_string='"<< event_string << "' (last='"<< (event_string.empty()?-1:event_string.Last().GetValue()) <<"') ; Str is '" << str << "' with length " << str.length() 
    //    << "' chars are (c-1)='" << stc->GetCharAt(current_pos - 1) << "' : (c)='" << stc->GetCharAt(current_pos)
    //<< "', text length=" << stc->GetTextLength() << ", currentPos=" << current_pos << " , " << int('\n') << "\n";
    //std::cout << "test: " << (!(stc->GetCharAt(current_pos - 1) <= '\n')) << ", 2=" << (len_entered >= 0) << ", 3=" << (!str.empty()) 
    //    << ", 4=" << (std::regex_match(str.ToStdString(), word_regex)) 
    //    <<", Mod5="<<((event.GetModificationType() & wxSTC_STARTACTION) != 0)
    //    <<", 6="<< (current_pos <= 1 || str != ".")<<", 6b="<< (str == ".")
    //    << "\n";

    if ((event.GetModificationType() & wxSTC_STARTACTION) != 0 && (str.empty() || str.Last() != '.'))
        return;
    //if (!event_string.empty() && !str.empty() && int(str[str.length() - 1]) != event_string.Last().GetValue()) std::cout << "removecall?\n";
    if (len_entered >= 0 && !str.empty() && std::regex_match(str.ToStdString(), word_regex)) {
        //check for possible words
        //todo: check for '.' to filter for modifiers
        int nb_words = 0;
        wxString possible;
        for (const PyCommand &cmd : commands) {
            if (cmd.name == str) return;
            if (cmd.name.StartsWith(str) && ((cmd.type & PyCommandType::pctDO_NOT_SHOW) == 0) ) {
                nb_words++; possible += possible.empty() ? cmd.name : (" " + cmd.name);
            }
        }
        // Display the autocompletion list
        if (nb_words >= 1)
            stc->AutoCompShow(len_entered, possible);
    } else if (!str.empty() && str.Last() == '.') {
        //wxString possible;
        //for(const wxString &str : modif_words)
        //    possible += possible.empty() ? str : (" " + str);
        wxString possible;
        for (const PyCommand &cmd : commands) {
            if (((cmd.type & PyCommandType::pctMODIFIER) != 0) && ((cmd.type & PyCommandType::pctDO_NOT_SHOW) == 0)) {
                possible += possible.empty() ? cmd.name : (" " + cmd.name);
            }
        }
        //std::cout << "autocomplete: modifier: '"<< possible.ToStdString() <<"'\n";
        if (possible.length() >= 1)
            stc->AutoCompShow(0, possible);
    }
}

void FreeCADDialog::on_char_add(wxStyledTextEvent& event) {
    //if (event.GetUpdated() != wxSTC_UPDATE_CONTENT) return;
    wxStyledTextCtrl* stc = (wxStyledTextCtrl*)event.GetEventObject();
    // Find the word start
    int current_pos = stc->GetCurrentPos();
    int word_start_pos = stc->WordStartPosition(current_pos, true);
    int len_entered = current_pos - word_start_pos;
    const wxString str = stc->GetTextRange(word_start_pos, current_pos + 1);
    //if(current_pos>1)
    //    std::cout << "char typed: " << (char)stc->GetCharAt(current_pos)<<" with length "<< len_entered 
    //    << ", str is "<< str << "chars are '"<< stc->GetCharAt(current_pos - 1) << "' : '" << stc->GetCharAt(current_pos)
    //    <<"', text length="<< stc->GetTextLength()<<", currentPos="<< current_pos<<" , "<<int('\n')<<"\n";
    if(current_pos > 2 && stc->GetCharAt(current_pos-1) == '\n'){
        //TODO: check that we are really in a freepyscad section.
        int lastpos = current_pos - 2;
        if (stc->GetCharAt(lastpos) == '\r')
            lastpos--;
        // do we need to move the ',' ?
        if (stc->GetCharAt(current_pos) == ',') {
            stc->SetTargetStart(current_pos);
            stc->SetTargetEnd(current_pos + 1);
            stc->ReplaceTarget("");
        }
        //do we need to add the ','?
        if (stc->GetCharAt(lastpos) == ')')
            stc->InsertText(lastpos + 1, ",");
    } else if (stc->GetTextLength() > current_pos && event.GetKey() == int(',') && stc->GetCharAt(current_pos - 1) == ',' && stc->GetCharAt(current_pos) == ',') {
        stc->SetTargetStart(current_pos);
        stc->SetTargetEnd(current_pos + 1);
        stc->ReplaceTarget("");
    } else if (stc->GetTextLength() > current_pos && event.GetKey() == int(')') && stc->GetCharAt(current_pos - 1) == ')' && stc->GetCharAt(current_pos) == ')') {
        stc->SetTargetStart(current_pos);
        stc->SetTargetEnd(current_pos + 1);
        stc->ReplaceTarget("");
    } else if (stc->GetTextLength() > current_pos && event.GetKey() == int('"') && stc->GetCharAt(current_pos - 1) == '"' && stc->GetCharAt(current_pos) == '"') {
        stc->SetTargetStart(current_pos);
        stc->SetTargetEnd(current_pos + 1);
        stc->ReplaceTarget("");
    } else if (stc->GetTextLength() > current_pos && event.GetKey() == int('"') && stc->GetCharAt(current_pos - 1) == '"' 
        && (stc->GetCharAt(current_pos) == ')' || stc->GetCharAt(current_pos) == ',') ) {
        stc->InsertText(current_pos, "\"");
    }
}

// note: this works on KEY, not on CHAR, so be sure the key is the right one for all keyboard layout.
// space, back, del are ok but no ascii char
void FreeCADDialog::on_key_type(wxKeyEvent& event)
{
    if (event.GetKeyCode() == WXK_SPACE && event.GetModifiers() == wxMOD_CONTROL)
    {
        //get word, if any
        int current_pos = m_text->GetCurrentPos();
        int word_start_pos = m_text->WordStartPosition(current_pos, true);
        const wxString str = m_text->GetTextRange(word_start_pos, current_pos);
        //std::cout << "ctrl-space! " << event.GetEventType() << " '" << str.ToStdString() << "' " << int(m_text->GetCharAt(current_pos - 1)) << "\n";
        if (current_pos > 0 && m_text->GetCharAt(current_pos - 1) == '.') {
            //only modifiers
            wxString possible;
            for (const PyCommand &cmd : commands) {
                if (((cmd.type & PyCommandType::pctMODIFIER) != 0) && ((cmd.type & PyCommandType::pctDO_NOT_SHOW) == 0)) {
                    possible += possible.empty() ? cmd.name : (" " + cmd.name);
                }
            }
            m_text->AutoCompShow(0, possible);
            return;
        }   
        //propose words
        int nb_words = 0;
        wxString possible;
        for (const PyCommand &cmd : commands) {
            if (str.IsEmpty() || cmd.name.StartsWith(str) && ((cmd.type & PyCommandType::pctDO_NOT_SHOW) == 0)) {
                nb_words++; possible += possible.empty() ? cmd.name : (" " + cmd.name);
            }
        }
        //std::cout << "space autocomplete: find " << nb_words << " forstring '" << str << "'\n";
        // Display the autocompletion list
        if (nb_words >= 1)
            m_text->AutoCompShow(str.length(), possible);
    }else if (event.GetKeyCode() == WXK_BACK && event.GetModifiers() == wxMOD_NONE)
    {
        
        //check if we can delete the whole word in one go
        //see if previous word is a command
        int current_pos = m_text->GetCurrentPos();
        if (m_text->GetCharAt(current_pos - 1) == '(' && m_text->GetCharAt(current_pos) == ')')
            current_pos--;
        while (m_text->GetCharAt(current_pos - 2) == '(' && m_text->GetCharAt(current_pos -1) == ')')
            current_pos -= 2;
        int word_start_pos = m_text->WordStartPosition(current_pos, true);
        wxString str = m_text->GetTextRange(word_start_pos, current_pos);
        if (str.length() > 2) {
            int del_more = 0;
            if (m_text->GetCharAt(current_pos) == '(' && m_text->GetCharAt(current_pos + 1) == ')') {
                del_more += 2;
            }
            if (m_text->GetCharAt(current_pos+2) == '(' && m_text->GetCharAt(current_pos + 3) == ')') {
                del_more += 2;
            }
            const PyCommand *cmd = get_command(str);
            if (cmd != nullptr) {
                //delete the whole word
                m_text->SetTargetStart(current_pos - str.length());
                m_text->SetTargetEnd(current_pos + del_more);
                m_text->ReplaceTarget("");
                return; // don't use the backdel event, it's already del 
            }
        }
        event.Skip(true);
    } else {
        event.Skip(true);
    }
    //event.SetWillBeProcessedAgain();
}
void FreeCADDialog::createSTC()
{
    m_text = new wxStyledTextCtrl(this, wxID_ANY,
        wxDefaultPosition, wxSize(600,400), wxHW_SCROLLBAR_AUTO);

    //m_text->SetMarginWidth(MARGIN_LINE_NUMBERS, 50);
    m_text->StyleSetForeground(wxSTC_STYLE_LINENUMBER, wxColour(75, 75, 75));
    m_text->StyleSetBackground(wxSTC_STYLE_LINENUMBER, wxColour(220, 220, 220));
    //m_text->SetMarginType(MARGIN_LINE_NUMBERS, wxSTC_MARGIN_NUMBER);

    m_text->SetTabWidth(4);
    m_text->SetIndent(4);
    m_text->SetUseTabs(true);
    m_text->SetIndentationGuides(wxSTC_IV_LOOKFORWARD);
    m_text->SetBackSpaceUnIndents(true);
    m_text->SetTabIndents(true);

    m_text->SetWrapMode(wxSTC_WRAP_WORD);

    m_text->StyleClearAll();
    m_text->SetLexer(wxSTC_LEX_PYTHON);
    //m_text->Bind(wxEVT_STC_CHANGE, &FreeCADDialog::on_word_change_for_autocomplete, this);
    m_text->Bind(wxEVT_STC_MODIFIED, &FreeCADDialog::on_word_change_for_autocomplete, this);
    m_text->Bind(wxEVT_STC_CHARADDED, &FreeCADDialog::on_char_add, this);
    m_text->Bind(wxEVT_KEY_DOWN, &FreeCADDialog::on_key_type, this);
    m_text->Bind(wxEVT_STC_AUTOCOMP_COMPLETED, &FreeCADDialog::on_autocomp_complete, this);
    m_text->Connect(wxID_ANY,
            wxEVT_KEY_DOWN,
            wxKeyEventHandler(FreeCADDialog::on_key_type),
            (wxObject*)NULL,
            this);

    m_text->StyleSetForeground(wxSTC_P_DEFAULT, wxColour(0, 0, 0));
    m_text->StyleSetForeground(wxSTC_P_COMMENTLINE, wxColour(128, 255, 128)); // comment, grennsish
    m_text->StyleSetForeground(wxSTC_P_COMMENTBLOCK, wxColour(128, 255, 128)); // comment, grennsish
    m_text->StyleSetForeground(wxSTC_P_NUMBER, wxColour(255, 128, 0)); // number red-orange
    m_text->StyleSetForeground(wxSTC_P_STRING, wxColour(128, 256, 0)); // string, light green
    m_text->StyleSetBackground(wxSTC_P_STRINGEOL, wxColour(255, 0, 0)); // End of line where string is not closed
    m_text->StyleSetForeground(wxSTC_P_CHARACTER, wxColour(128, 256, 0));
    m_text->StyleSetForeground(wxSTC_P_WORD, wxColour(0, 0, 128));
    m_text->StyleSetBold(wxSTC_P_WORD, true),
        m_text->StyleSetForeground(wxSTC_P_WORD2, wxColour(0, 0, 128));
    m_text->StyleSetForeground(wxSTC_P_TRIPLE, wxColour(128, 0, 0)); // triple quote
    m_text->StyleSetForeground(wxSTC_P_TRIPLEDOUBLE, wxColour(128, 0, 0)); //triple double quote
    m_text->StyleSetForeground(wxSTC_P_DEFNAME, wxColour(0, 128, 128)); // Function or method name definition
    m_text->StyleSetBold(wxSTC_P_DEFNAME, true),
    m_text->StyleSetForeground(wxSTC_P_OPERATOR, wxColour(255, 0, 0));
    m_text->StyleSetBold(wxSTC_P_OPERATOR, true),
    
    m_text->StyleSetForeground(wxSTC_P_IDENTIFIER, wxColour(255, 64, 255)); // function call and almost all defined words in the language, violet

    //add text if the saved file exist
    boost::filesystem::path temp_file(Slic3r::data_dir());
    temp_file = temp_file / "temp" / "current_pyscad.py";
    load_text_from_file(temp_file);

}

void FreeCADDialog::on_dpi_changed(const wxRect& suggested_rect)
{
    msw_buttons_rescale(this, em_unit(), { wxID_OK });

    Layout();
    Fit();
    Refresh();
}

time_t parse_iso_time(std::string &str) {
    int y, M, d, h, m;
    float s;
    sscanf(str.c_str(), "%d-%d-%dT%d:%d:%fZ", &y, &M, &d, &h, &m, &s); 
    tm time;
    time.tm_year = y - 1900; // Year since 1900
    time.tm_mon = M - 1;     // 0-11
    time.tm_mday = d;        // 1-31
    time.tm_hour = h;        // 0-23
    time.tm_min = m;         // 0-59
    time.tm_sec = (int)s;    // 0-61 (0-60 in C++11)
    return mktime(&time);
}

void FreeCADDialog::test_update_script_file(std::string &json) {
    //check if it's more recent
    std::stringstream ss;
    ss << json;
    boost::property_tree::ptree root;
    boost::property_tree::read_json(ss, root);
    const boost::filesystem::path pyscad_path(boost::filesystem::path(Slic3r::data_dir()) / "scripts" / "FreePySCAD");
    try {
        std::string str_date;
        for (const auto &entry : root.get_child("commit.committer")) {
            if (entry.first == "date") {
                str_date = entry.second.data();
                break;
            }
        }
        using namespace boost::locale;
        boost::locale::generator gen;
        std::locale loc = gen.generate(""); // or "C", "en_US.UTF-8" etc.
        std::locale::global(loc);
        std::cout.imbue(loc);

        std::cout << "root.commit.committer.date=" << str_date << "\n";
        std::time_t commit_time = parse_iso_time(str_date);
        std::cout << "github time_t = "<<commit_time<<"\n";
        std::time_t last_modif = boost::filesystem::last_write_time(pyscad_path / "freepyscad.py");
        std::cout << "pyscad_path time_t = " << commit_time << "\n";
        if (commit_time > last_modif) {
            std::cout << "have to update!!\n";
            get_file_from_web("https://raw.githubusercontent.com/supermerill/FreePySCAD/master/__init__.py", pyscad_path / "__init__.py");
            get_file_from_web("https://raw.githubusercontent.com/supermerill/FreePySCAD/master/Init.py", pyscad_path / "Init.py");
            get_file_from_web("https://raw.githubusercontent.com/supermerill/FreePySCAD/master/freepyscad.py", pyscad_path / "freepyscad.py");
        }
    }
    catch (std::exception ex) {
        std::cerr << "Error, cannot parse https://api.github.com/repos/supermerill/FreePySCAD/commits/master: " << ex.what() << "\n";
    }
}

bool FreeCADDialog::init_start_python() {
    //reinit
    exec_var.reset(new ExecVar());

    // Get the freecad path (python path)
    boost::filesystem::path pythonpath(gui_app->app_config->get("freecad_path"));
    pythonpath = pythonpath / "python.exe";
    if (!exists(pythonpath)) {
        m_errors->AppendText("Error, cannot find the freecad (version 0.19 or higher) python at '" + pythonpath.string() + "', please update your freecad bin directory in the preferences.");
        return false;
    }

    const boost::filesystem::path scripts_path(boost::filesystem::path(Slic3r::data_dir()) / "scripts");
    boost::filesystem::create_directories(scripts_path / "FreePySCAD");

    if (!boost::filesystem::exists(scripts_path / "FreePySCAD" / "freepyscad.py")) {
        get_file_from_web("https://raw.githubusercontent.com/supermerill/FreePySCAD/master/__init__.py", scripts_path / "FreePySCAD" / "__init__.py");
        get_file_from_web("https://raw.githubusercontent.com/supermerill/FreePySCAD/master/Init.py", scripts_path / "FreePySCAD" / "Init.py");
        get_file_from_web("https://raw.githubusercontent.com/supermerill/FreePySCAD/master/freepyscad.py", scripts_path / "FreePySCAD" / "freepyscad.py");
    }else if (!update_done){
        update_done = true;
        //try to check last version on website
        //it's async so maybe you won't update it in time, but it's not the end of the world. 
        const boost::filesystem::path pyscad_path = scripts_path / "FreePySCAD";
        std::function<void(FreeCADDialog*, std::string&)> truc = &FreeCADDialog::test_update_script_file;
        get_string_from_web_async("https://api.github.com/repos/supermerill/FreePySCAD/commits/master", this, &FreeCADDialog::test_update_script_file);
    }

    exec_var->process.reset(new boost::process::child(pythonpath.string() + " -u -i", boost::process::std_in < exec_var->pyin, 
        boost::process::std_out > exec_var->data_out, boost::process::std_err > exec_var->data_err, exec_var->ios));
    exec_var->pyin << "import FreeCAD" << std::endl;
    exec_var->pyin << "import Part" << std::endl;
    exec_var->pyin << "import Draft" << std::endl;
    exec_var->pyin << "import sys" << std::endl;
    exec_var->pyin << "sys.path.append('" << scripts_path.generic_string() << "')" << std::endl;
    //std::cout << "sys.path.append('" << pyscad_path.generic_string() << "')" << std::endl;
    exec_var->pyin << "from FreePySCAD.freepyscad import *" << std::endl;
    //std::cout << "from FreePySCAD.freepyscad import *" << std::endl;
    exec_var->pyin << "App.newDocument(\"document\")" << std::endl;
#ifdef __WINDOWS__
    exec_var->pyin << "set_font_dir(\"C:/Windows/Fonts/\")" << std::endl;
#endif
#ifdef __APPLE__
    exec_var->pyin << "set_font_dir([\"/System/Library/Fonts/\", \"~/Library/Fonts/\"])" << std::endl;
#endif
#ifdef __linux__
    exec_var->pyin << "set_font_dir([\"/usr/share/fonts/\",\"~/.fonts/\"])" << std::endl;
    // also add 
#endif

    return true;
}

bool FreeCADDialog::end_python() {
    exec_var->pyin << "quit()" << std::endl;
    exec_var->process->wait();
    exec_var->ios.run();
    return true;
}

void FreeCADDialog::create_geometry(wxCommandEvent& event_args) {
    //Create the script

    // cleaning
    boost::filesystem::path object_path(Slic3r::data_dir());
    object_path = object_path / "temp" / "temp.amf";
    m_errors->Clear();
    if (exists(object_path)) {
        remove(object_path);
    }

    //create synchronous pipe streams
    //boost::process::ipstream pyout;
    //boost::process::ipstream pyerr;

    if (!init_start_python()) return;

    //create file
    //search for scene().redraw(...), add it if not present (without defs)
    boost::filesystem::path temp_file(Slic3r::data_dir());
    temp_file = temp_file / "temp";
    boost::filesystem::create_directories(temp_file);
    temp_file = temp_file / "exec_temp.py";
    wxString text = m_text->GetText();
    if (text.find("scene().redraw(") == std::string::npos) {
        int redraw_pos = text.find("redraw()");
        if (redraw_pos == std::string::npos) {
            text = "scene().redraw(\n" + text + "\n)";
        } else {
            text = text.substr(0, redraw_pos) + "scene()." + text.substr(redraw_pos);
        }
    }
    if (!this->write_text_in_file(text, temp_file)) {
        m_errors->AppendText("Error, cannot write into "+ temp_file.string() + "!");
        return;
    }


    //std::cout<< "scene().redraw(" << boost::replace_all_copy(boost::replace_all_copy(m_text->GetText(), "\r", ""), "\n", "") << ")" << std::endl;
    //exec_var->pyin << "scene().redraw("<< boost::replace_all_copy(boost::replace_all_copy(m_text->GetText(), "\r", ""), "\n", "") <<")" << std::endl;
    exec_var->pyin << ("exec(open('" + temp_file.generic_string() + "').read())\n");
    exec_var->pyin << "Mesh.export(App.ActiveDocument.RootObjects, u\"" << object_path.generic_string() << "\")" << std::endl;
    exec_var->pyin << "print('exported!')" << std::endl;
    exec_var->pyin << "App.ActiveDocument.RootObjects" << std::endl;

    end_python();

    std::string pyout_str_hello;
    std::cout << "==cout==\n";
    std::cout << exec_var->data_out.get();
    std::cout << "==cerr==\n";
    std::string errStr = exec_var->data_err.get();
    std::cout << errStr << "\n";
    std::string cleaned = boost::replace_all_copy(boost::replace_all_copy(errStr, ">>> ", ""),"\r","");
    boost::replace_all(cleaned, "QWaitCondition: Destroyed while threads are still waiting\n", "");
    boost::replace_all(cleaned, "Type \"help\", \"copyright\", \"credits\" or \"license\" for more information.\n", "");
    boost::replace_all(cleaned, "\n\n", "\n");
    m_errors->AppendText(cleaned);
    
    if (!exists(object_path)) {
        m_errors->AppendText("\nError, no object generated.");
        return;
    }

    Plater* plat = this->main_frame->plater();
    Model& model = plat->model();
    plat->reset();
    std::vector<size_t> objs_idx = plat->load_files(std::vector<std::string>{ object_path.generic_string() }, true, false, false);
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
