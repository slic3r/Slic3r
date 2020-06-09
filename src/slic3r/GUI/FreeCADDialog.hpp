#ifndef slic3r_GUI_FreeCADDialog_hpp_
#define slic3r_GUI_FreeCADDialog_hpp_

#include <map>
#include <vector>

#include "GUI_App.hpp"
#include "GUI_Utils.hpp"
#include "MainFrame.hpp"
#include "wxExtensions.hpp"
#include <wx/wx.h>
#include <wx/stc/stc.h>
#include <wx/html/htmlwin.h>
#include <wx/textctrl.h>
#include <boost/process.hpp>

namespace Slic3r { 
namespace GUI {


enum PyCommandType : uint16_t {
    pctNONE = 0x0,
    pctOPERATION = 0x1 << 0,
    pctOBJECT = 0x1 << 1,
    pctMODIFIER = 0x1 << 2,
    pctNO_PARAMETER = 0x1 << 3,
    pctDO_NOT_SHOW = 0x1 << 4
};

class PyCommand {
public:
    wxString name;
    PyCommandType type;
    wxString tooltip;
    std::vector<std::string> args;
    PyCommand(wxString lbl, PyCommandType modifier) : name(lbl), type(modifier) { auto lol = { "Z","z" }; }
    PyCommand(wxString lbl, uint16_t modifier) : name(lbl), type(PyCommandType(modifier)) {}
    PyCommand(wxString lbl, PyCommandType modifier, std::string tooltip) : name(lbl), type(modifier), tooltip(tooltip) {}
    PyCommand(wxString lbl, uint16_t modifier, std::string tooltip) : name(lbl), type(PyCommandType(modifier)), tooltip(tooltip) {}
    PyCommand(wxString lbl, PyCommandType modifier, std::initializer_list<const char*> args, std::string tooltip) : name(lbl), type(modifier), tooltip(tooltip) { for(const char* arg: args) this->args.emplace_back(arg); }
    PyCommand(wxString lbl, uint16_t modifier, std::initializer_list<const char*> args, std::string tooltip) : name(lbl), type(PyCommandType(modifier)), tooltip(tooltip) { for (const char* arg : args) this->args.emplace_back(arg); }
};

class FreeCADDialog : public DPIDialog
{

public:
    FreeCADDialog(GUI_App* app, MainFrame* mainframe);
    virtual ~FreeCADDialog() { }
    
protected:
    void closeMe(wxCommandEvent& event_args);
    void createSTC();

    bool init_start_python();
    void create_geometry(wxCommandEvent& event_args);
    bool end_python();

    void new_script(wxCommandEvent& event_args) { m_text->ClearAll(); }
    void load_script(wxCommandEvent& event_args);
    void save_script(wxCommandEvent& event_args);
    void quick_save(wxCommandEvent& event_args);
    void on_dpi_changed(const wxRect& suggested_rect) override;

    void on_word_change_for_autocomplete(wxStyledTextEvent& event);
    void on_char_add(wxStyledTextEvent& event);
    void on_key_type(wxKeyEvent& event);
    void on_autocomp_complete(wxStyledTextEvent& event);
    bool write_text_in_file(const wxString &towrite, const boost::filesystem::path &file);
    bool load_text_from_file(const boost::filesystem::path &file);
    void test_update_script_file(std::string &json);

    wxStyledTextCtrl* m_text;
    wxTextCtrl* m_errors;
    wxTextCtrl* m_help;
    MainFrame* main_frame;
    GUI_App* gui_app;

    std::vector<PyCommand> commands;

    std::regex word_regex;
    bool update_done = false;

    boost::filesystem::path opened_file;
    struct ExecVar {
        boost::process::opstream pyin;
        boost::asio::io_service ios;
        std::future<std::string> data_out;
        std::future<std::string> data_err;
        std::unique_ptr<boost::process::child> process;
    };
    std::unique_ptr<ExecVar> exec_var;

    bool ready = false;

protected:
    const PyCommand* get_command(const wxString &name) const;
};

} // namespace GUI
} // namespace Slic3r

#endif
