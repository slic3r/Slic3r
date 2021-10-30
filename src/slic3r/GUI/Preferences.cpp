#include "Preferences.hpp"
#include "OptionsGroup.hpp"
#include "GUI_App.hpp"
#include "Plater.hpp"
#include "I18N.hpp"
#include "libslic3r/AppConfig.hpp"
#include <wx/notebook.h>

namespace Slic3r {
namespace GUI {

PreferencesDialog::PreferencesDialog(wxWindow* parent) : 
    DPIDialog(parent, wxID_ANY, _L("Preferences"), wxDefaultPosition, 
              wxDefaultSize, wxDEFAULT_DIALOG_STYLE)
{
#ifdef __WXOSX__
    isOSX = true;
#endif
	build();
}

static std::shared_ptr<ConfigOptionsGroup>create_options_tab(const wxString& title, wxNotebook* tabs)
{
	wxPanel* tab = new wxPanel(tabs, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxBK_LEFT | wxTAB_TRAVERSAL);
	tabs->AddPage(tab, title);
	tab->SetFont(wxGetApp().normal_font());

	wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
	sizer->SetSizeHints(tab);
	tab->SetSizer(sizer);

	std::shared_ptr<ConfigOptionsGroup> optgroup = std::make_shared<ConfigOptionsGroup>(tab);
	optgroup->title_width = 40;
	optgroup->label_width = 40;
	return optgroup;
}

std::shared_ptr<ConfigOptionsGroup> PreferencesDialog::create_general_options_group(const wxString& title, wxNotebook* tabs)
{

	std::shared_ptr<ConfigOptionsGroup> optgroup = std::make_shared<ConfigOptionsGroup>((wxPanel*)tabs->GetPage(0), title);
	optgroup->title_width = 40;
	optgroup->label_width = 40;
	optgroup->m_on_change = [this](t_config_option_key opt_key, boost::any value) {
		if (opt_key == "default_action_on_close_application" || opt_key == "default_action_on_select_preset")
			m_values[opt_key] = boost::any_cast<bool>(value) ? "none" : "discard";
		else if (std::unordered_set<std::string>{ "splash_screen_editor", "splash_screen_gcodeviewer", "auto_switch_preview" }.count(opt_key) > 0)
			m_values[opt_key] = boost::any_cast<std::string>(value);
		else
			m_values[opt_key] = boost::any_cast<bool>(value) ? "1" : "0";
	};
	return optgroup;
}
std::shared_ptr<ConfigOptionsGroup> PreferencesDialog::create_gui_options_group(const wxString& title, wxNotebook* tabs)
{

	std::shared_ptr<ConfigOptionsGroup> optgroup = std::make_shared<ConfigOptionsGroup>((wxPanel*)tabs->GetPage(3), title);
	optgroup->title_width = 40;
	optgroup->label_width = 40;
	optgroup->m_on_change = [this, tabs](t_config_option_key opt_key, boost::any value) {
		if (opt_key == "suppress_hyperlinks")
			m_values[opt_key] = boost::any_cast<bool>(value) ? "1" : "";
		else if (opt_key.find("color") != std::string::npos)
			m_values[opt_key] = boost::any_cast<std::string>(value);
		else if (opt_key.find("tab_icon_size") != std::string::npos)
			m_values[opt_key] = std::to_string(boost::any_cast<int>(value));
		else
			m_values[opt_key] = boost::any_cast<bool>(value) ? "1" : "0";

		if (opt_key == "use_custom_toolbar_size") {
			m_icon_size_sizer->ShowItems(boost::any_cast<bool>(value));
			m_optgroups_gui.front()->parent()->Layout();
			tabs->Layout();
			this->layout();
		}
	};
	return optgroup;
}

static void activate_options_tab(std::shared_ptr<ConfigOptionsGroup> optgroup, int padding = 20)
{
	optgroup->activate();
	optgroup->update_visibility(comSimple);
	wxBoxSizer* sizer = static_cast<wxBoxSizer*>(static_cast<wxPanel*>(optgroup->parent())->GetSizer());
	sizer->Add(optgroup->sizer, 0, wxEXPAND | wxALL, padding);
}

void PreferencesDialog::build()
{
	SetBackgroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOW));
	const wxFont& font = wxGetApp().normal_font();
	SetFont(font);

	auto app_config = get_app_config();

	wxNotebook* tabs = new wxNotebook(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxNB_TOP | wxTAB_TRAVERSAL | wxNB_NOPAGETHEME);

	// Add "General" tab
	m_optgroups_general.clear();
	m_optgroups_general.emplace_back(create_options_tab(_L("General"), tabs));


	bool is_editor = wxGetApp().is_editor();

	ConfigOptionDef def;
	Option option(def, "");

	if (is_editor) {

        //activate_options_tab(m_optgroups_general.back(), 3);
        m_optgroups_general.emplace_back(create_general_options_group(_L("Automation"), tabs));

        def.label = L("Auto-center parts");
        def.type = coBool;
        def.tooltip = L("If this is enabled, Slic3r will auto-center objects "
            "around the print bed center.");
        def.set_default_value(new ConfigOptionBool{ app_config->get("autocenter") == "1" });
        option = Option(def, "autocenter");
        m_optgroups_general.back()->append_single_option_line(option);

        def.label = L("Background processing");
        def.type = coBool;
        def.tooltip = L("If this is enabled, Slic3r will pre-process objects as soon "
            "as they\'re loaded in order to save time when exporting G-code.");
        def.set_default_value(new ConfigOptionBool{ app_config->get("background_processing") == "1" });
        option = Option(def, "background_processing");
        m_optgroups_general.back()->append_single_option_line(option);

		def_combobox_auto_switch_preview.label = L("Switch to Preview when sliced");
		def_combobox_auto_switch_preview.type = coStrings;
		def_combobox_auto_switch_preview.tooltip = L("When an object is sliced, it will switch your view from the curent view to the "
			"preview (and then gcode-preview) automatically, depending on the option choosen.");
		def_combobox_auto_switch_preview.gui_type = "f_enum_open";
		def_combobox_auto_switch_preview.gui_flags = "show_value";
		def_combobox_auto_switch_preview.enum_values.push_back(_u8L("Don't switch"));
		def_combobox_auto_switch_preview.enum_values.push_back(_u8L("Switch when possible"));
		def_combobox_auto_switch_preview.enum_values.push_back(_u8L("Only if on plater"));
		def_combobox_auto_switch_preview.enum_values.push_back(_u8L("Only when GCode is ready"));
		if (app_config->get("auto_switch_preview") == "0")
			def_combobox_auto_switch_preview.set_default_value(new ConfigOptionStrings{ def_combobox_auto_switch_preview.enum_values[0] });
		else if (app_config->get("auto_switch_preview") == "1")
			def_combobox_auto_switch_preview.set_default_value(new ConfigOptionStrings{ def_combobox_auto_switch_preview.enum_values[1] });
		else if (app_config->get("auto_switch_preview") == "2")
			def_combobox_auto_switch_preview.set_default_value(new ConfigOptionStrings{ def_combobox_auto_switch_preview.enum_values[2] });
		else if (app_config->get("auto_switch_preview") == "3")
			def_combobox_auto_switch_preview.set_default_value(new ConfigOptionStrings{ def_combobox_auto_switch_preview.enum_values[3] });
		else
			def_combobox_auto_switch_preview.set_default_value(new ConfigOptionStrings{ def_combobox_auto_switch_preview.enum_values[2] });
		option = Option(def_combobox_auto_switch_preview, "auto_switch_preview");
		m_optgroups_general.back()->append_single_option_line(option);


        activate_options_tab(m_optgroups_general.back(), 3);
        m_optgroups_general.emplace_back(create_general_options_group(_L("Presets and updates"), tabs));

        // Please keep in sync with ConfigWizard
        def.label = L("Check for application updates");
        def.type = coBool;
        def.tooltip = L("If enabled, Slic3r will check for the new versions of itself online. When a new version becomes available a notification is displayed at the next application startup (never during program usage). This is only a notification mechanisms, no automatic installation is done.");
        def.set_default_value(new ConfigOptionBool(app_config->get("version_check") == "1"));
            option = Option(def, "version_check");
        m_optgroups_general.back()->append_single_option_line(option);

        // Please keep in sync with ConfigWizard
        def.label = L("Update built-in Presets automatically");
        def.type = coBool;
        def.tooltip = L("If enabled, Slic3r downloads updates of built-in system presets in the background. These updates are downloaded into a separate temporary location. When a new preset version becomes available it is offered at application startup.");
        def.set_default_value(new ConfigOptionBool(app_config->get("preset_update") == "1"));
        option = Option(def, "preset_update");
        m_optgroups_general.back()->append_single_option_line(option);

        def.label = L("Suppress \" - default - \" presets");
        def.type = coBool;
        def.tooltip = L("Suppress \" - default - \" presets in the Print / Filament / Printer "
            "selections once there are any other valid presets available.");
        def.set_default_value(new ConfigOptionBool{ app_config->get("no_defaults") == "1" });
        option = Option(def, "no_defaults");
        m_optgroups_general.back()->append_single_option_line(option);
		m_values_need_restart.push_back("no_defaults");

        def.label = L("Show incompatible print and filament presets");
        def.type = coBool;
        def.tooltip = L("When checked, the print and filament presets are shown in the preset editor "
            "even if they are marked as incompatible with the active printer");
        def.set_default_value(new ConfigOptionBool{ app_config->get("show_incompatible_presets") == "1" });
        option = Option(def, "show_incompatible_presets");
        m_optgroups_general.back()->append_single_option_line(option);

        def.label = L("Main GUI always in expert mode");
        def.type = coBool;
        def.tooltip = L("If enabled, the gui will be in expert mode even if the simple or advanced mode is selected (but not the setting tabs).");
        def.set_default_value(new ConfigOptionBool{ app_config->get("objects_always_expert") == "1" });
        option = Option(def, "objects_always_expert");
        m_optgroups_general.back()->append_single_option_line(option);

        activate_options_tab(m_optgroups_general.back(), 3);
        m_optgroups_general.emplace_back(create_general_options_group(_L("Files"), tabs));

        // Please keep in sync with ConfigWizard
        def.label = L("Export sources full pathnames to 3mf and amf");
        def.type = coBool;
        def.tooltip = L("If enabled, allows the Reload from disk command to automatically find and load the files when invoked.");
        def.set_default_value(new ConfigOptionBool(app_config->get("export_sources_full_pathnames") == "1"));
        option = Option(def, "export_sources_full_pathnames");
        m_optgroups_general.back()->append_single_option_line(option);

#if ENABLE_CUSTOMIZABLE_FILES_ASSOCIATION_ON_WIN
#ifdef _WIN32
		// Please keep in sync with ConfigWizard
		def.label = (boost::format(_u8L("Associate .3mf files to %1%")) % SLIC3R_APP_NAME).str();
		def.type = coBool;
		def.tooltip = L("If enabled, sets Slic3r as default application to open .3mf files.");
		def.set_default_value(new ConfigOptionBool(app_config->get("associate_3mf") == "1"));
		option = Option(def, "associate_3mf");
		m_optgroups_general.back()->append_single_option_line(option);

		def.label = (boost::format(_u8L("Associate .stl files to %1%")) % SLIC3R_APP_NAME).str();
		def.type = coBool;
		def.tooltip = L("If enabled, sets Slic3r as default application to open .stl files.");
		def.set_default_value(new ConfigOptionBool(app_config->get("associate_stl") == "1"));
		option = Option(def, "associate_stl");
		m_optgroups_general.back()->append_single_option_line(option);
#endif // _WIN32
#endif // ENABLE_CUSTOMIZABLE_FILES_ASSOCIATION_ON_WIN

        def.label = L("Remember output directory");
        def.type = coBool;
        def.tooltip = L("If this is enabled, Slic3r will prompt the last output directory "
            "instead of the one containing the input files.");
        def.set_default_value(new ConfigOptionBool{ app_config->has("remember_output_path") ? app_config->get("remember_output_path") == "1" : true });
        option = Option(def, "remember_output_path");
        m_optgroups_general.back()->append_single_option_line(option);

        activate_options_tab(m_optgroups_general.back(), 3);
        m_optgroups_general.emplace_back(create_general_options_group(_L("Dialogs"), tabs));

		def.label = L("Show drop project dialog");
		def.type = coBool;
		def.tooltip = L("When checked, whenever dragging and dropping a project file on the application, shows a dialog asking to select the action to take on the file to load.");
		def.set_default_value(new ConfigOptionBool{ app_config->get("show_drop_project_dialog") == "1" });
		option = Option(def, "show_drop_project_dialog");
		m_optgroups_general.back()->append_single_option_line(option);

		def.label = L("Show overwrite dialog.");
		def.type = coBool;
		def.tooltip = L("If this is enabled, Slic3r will prompt for when overwriting files from save dialogs.");
		def.set_default_value(new ConfigOptionBool{ app_config->has("show_overwrite_dialog") ? app_config->get("show_overwrite_dialog") == "1" : true });
		option = Option(def, "show_overwrite_dialog");
		m_optgroups_general.back()->append_single_option_line(option);

		
#if __APPLE__
		def.label = (boost::format(_u8L("Allow just a single %1% instance")) % SLIC3R_APP_NAME).str();
		def.type = coBool;
	def.tooltip = L("On OSX there is always only one instance of app running by default. However it is allowed to run multiple instances of same app from the command line. In such case this settings will allow only one instance.");
#else
		def.label = (boost::format(_u8L("Allow just a single %1% instance")) % SLIC3R_APP_NAME).str();
		def.type = coBool;
		def.tooltip = L("If this is enabled, when starting Slic3r and another instance of the same Slic3r is already running, that instance will be reactivated instead.");
#endif
		def.set_default_value(new ConfigOptionBool{ app_config->has("single_instance") ? app_config->get("single_instance") == "1" : false });
		option = Option(def, "single_instance");
		m_optgroups_general.back()->append_single_option_line(option);

		def.label = L("Ask for unsaved changes when closing application");
		def.type = coBool;
		def.tooltip = L("When closing the application, always ask for unsaved changes");
		def.set_default_value(new ConfigOptionBool{ app_config->get("default_action_on_close_application") == "none" });
		option = Option(def, "default_action_on_close_application");
		m_optgroups_general.back()->append_single_option_line(option);

		def.label = L("Ask for unsaved changes when selecting new preset");
		def.type = coBool;
		def.tooltip = L("Always ask for unsaved changes when selecting new preset");
		def.set_default_value(new ConfigOptionBool{ app_config->get("default_action_on_select_preset") == "none" });
		option = Option(def, "default_action_on_select_preset");
		m_optgroups_general.back()->append_single_option_line(option);

		def.label = L("Always keep current preset changes on a new project");
		def.type = coBool;
		def.tooltip = L("When you create a new project, it will keep the current preset state, and won't open the preset change dialog.");
		def.set_default_value(new ConfigOptionBool{ app_config->get("default_action_preset_on_new_project") == "1" });
		option = Option(def, "default_action_preset_on_new_project");
		m_optgroups_general.back()->append_single_option_line(option);

		def.label = L("Ask for unsaved project changes");
		def.type = coBool;
		def.tooltip = L("Always ask if you want to save your project change if you are going to loose some changes. Or it will discard them by deafult.");
		def.set_default_value(new ConfigOptionBool{ app_config->get("default_action_on_new_project") == "1" });
		option = Option(def, "default_action_on_new_project");
		m_optgroups_general.back()->append_single_option_line(option);
	}
#if ENABLE_CUSTOMIZABLE_FILES_ASSOCIATION_ON_WIN
#ifdef _WIN32
	else {
		def.label = (boost::format(_u8L("Associate .gcode files to %1%")) % GCODEVIEWER_APP_NAME).str();
		def.type = coBool;
		def.tooltip = (boost::format(_u8L("If enabled, sets %1% as default application to open .gcode files.")) % GCODEVIEWER_APP_NAME).str();
		def.set_default_value(new ConfigOptionBool(app_config->get("associate_gcode") == "1"));
		option = Option(def, "associate_gcode");
		m_optgroups_general.back()->append_single_option_line(option);
	}
#endif // _WIN32
#endif // ENABLE_CUSTOMIZABLE_FILES_ASSOCIATION_ON_WIN

#if __APPLE__
	def.label = L("Use Retina resolution for the 3D scene");
	def.type = coBool;
	def.tooltip = L("If enabled, the 3D scene will be rendered in Retina resolution. "
	                "If you are experiencing 3D performance problems, disabling this option may help.");
	def.set_default_value(new ConfigOptionBool{ app_config->get("use_retina_opengl") == "1" });
	option = Option (def, "use_retina_opengl");
	m_optgroups_general.back()->append_single_option_line(option);
#endif

    if (is_editor) {
        activate_options_tab(m_optgroups_general.back(), 3);
        m_optgroups_general.emplace_back(create_general_options_group(_L("Splash screen"), tabs));
    }

    // Show/Hide splash screen
	def.label = L("Show splash screen");
	def.type = coBool;
	def.tooltip = L("Show splash screen");
	def.set_default_value(new ConfigOptionBool{ app_config->get("show_splash_screen") == "1" });
	option = Option(def, "show_splash_screen");
	m_optgroups_general.back()->append_single_option_line(option);

	def.label = L("Random splash screen");
	def.type = coBool;
	def.tooltip = L("Show a random splash screen image from the list at each startup");
	def.set_default_value(new ConfigOptionBool{ app_config->get("show_splash_screen_random") == "1" });
	option = Option(def, "show_splash_screen_random");
	m_optgroups_general.back()->append_single_option_line(option);
	
	// splashscreen image
	{
		ConfigOptionDef def_combobox;
		def_combobox.label = L("Splash screen image");
		def_combobox.type = coStrings;
		def_combobox.tooltip = L("Choose the image to use as splashscreen");
		def_combobox.gui_type = "f_enum_open";
		def_combobox.gui_flags = "show_value";
		def_combobox.enum_values.push_back(std::string(SLIC3R_APP_NAME)  + L(" icon"));
		//get all images in the spashscreen dir
		for (const boost::filesystem::directory_entry& dir_entry : boost::filesystem::directory_iterator(boost::filesystem::path(Slic3r::resources_dir()) / "splashscreen"))
			if (dir_entry.path().has_extension()&& std::set<std::string>{ ".jpg", ".JPG", ".jpeg" }.count(dir_entry.path().extension().string()) > 0 )
				def_combobox.enum_values.push_back(dir_entry.path().filename().string());
		std::string current_file_name = app_config->get(is_editor ? "splash_screen_editor" : "splash_screen_gcodeviewer");
		if (std::find(def_combobox.enum_values.begin(), def_combobox.enum_values.end(), current_file_name) == def_combobox.enum_values.end())
			current_file_name = def_combobox.enum_values[0];
		def_combobox.set_default_value(new ConfigOptionStrings{ current_file_name });
		option = Option(def_combobox, is_editor ? "splash_screen_editor" : "splash_screen_gcodeviewer");
		m_optgroups_general.back()->append_single_option_line(option);
	}

#if ENABLE_CTRL_M_ON_WINDOWS
#if defined(_WIN32) || defined(__APPLE__)
    if (is_editor) {
        activate_options_tab(m_optgroups_general.back(), 3);
        m_optgroups_general.emplace_back(create_general_options_group(_L("Others"), tabs));
    }
	def.label = L("Enable support for legacy 3DConnexion devices");
	def.type = coBool;
	def.tooltip = L("If enabled, the legacy 3DConnexion devices settings dialog is available by pressing CTRL+M");
	def.set_default_value(new ConfigOptionBool{ app_config->get("use_legacy_3DConnexion") == "1" });
	option = Option(def, "use_legacy_3DConnexion");
	m_optgroups_general.back()->append_single_option_line(option);
#endif // _WIN32 || __APPLE__
#endif // ENABLE_CTRL_M_ON_WINDOWS

    activate_options_tab(m_optgroups_general.back(), m_optgroups_general.back()->parent()->GetSizer()->GetItemCount() > 1 ? 3 : 20);

	
	// Add "Paths" tab
	m_optgroup_paths = create_options_tab(_L("Paths"), tabs);
	m_optgroup_paths->title_width = 10;
	m_optgroup_paths->m_on_change = [this](t_config_option_key opt_key, boost::any value) {
		m_values[opt_key] = boost::any_cast<std::string>(value);
	};
	def.label = L("FreeCAD path");
	def.type = coString;
	def.tooltip = L("If it point to a valid freecad instance (the bin directory or the python executable), you can use the built-in python script to quickly generate geometry.");
	def.set_default_value(new ConfigOptionString{ app_config->get("freecad_path") });
	option = Option(def, "freecad_path");
	//option.opt.full_width = true;
	option.opt.width = 50;
	m_optgroup_paths->append_single_option_line(option);

	activate_options_tab(m_optgroup_paths);

	// Add "Camera" tab
	m_optgroup_camera = create_options_tab(_L("Camera"), tabs);
	m_optgroup_camera->m_on_change = [this](t_config_option_key opt_key, boost::any value) {
		m_values[opt_key] = boost::any_cast<bool>(value) ? "1" : "0";
	};

    def.label = L("Use perspective camera");
    def.type = coBool;
    def.tooltip = L("If enabled, use perspective camera. If not enabled, use orthographic camera.");
    def.set_default_value(new ConfigOptionBool{ app_config->get("use_perspective_camera") == "1" });
    option = Option(def, "use_perspective_camera");
    m_optgroup_camera->append_single_option_line(option);

	def.label = L("Use free camera");
	def.type = coBool;
	def.tooltip = L("If enabled, use free camera. If not enabled, use constrained camera.");
	def.set_default_value(new ConfigOptionBool(app_config->get("use_free_camera") == "1"));
	option = Option(def, "use_free_camera");
	m_optgroup_camera->append_single_option_line(option);

	def.label = L("Reverse direction of zoom with mouse wheel");
	def.type = coBool;
	def.tooltip = L("If enabled, reverses the direction of zoom with mouse wheel");
	def.set_default_value(new ConfigOptionBool(app_config->get("reverse_mouse_wheel_zoom") == "1"));
	option = Option(def, "reverse_mouse_wheel_zoom");
	m_optgroup_camera->append_single_option_line(option);

	activate_options_tab(m_optgroup_camera);

	// Add "GUI" tab
	m_optgroups_gui.clear();
	m_optgroups_gui.emplace_back(create_options_tab(_L("GUI"), tabs));

		//activate_options_tab(m_optgroups_general.back(), 3);
	m_optgroups_gui.emplace_back(create_gui_options_group(_L("Controls"), tabs));


	def.label = L("Sequential slider applied only to top layer");
	def.type = coBool;
	def.tooltip = L("If enabled, changes made using the sequential slider, in preview, apply only to gcode top layer. "
					"If disabled, changes made using the sequential slider, in preview, apply to the whole gcode.");
	def.set_default_value(new ConfigOptionBool{ app_config->get("seq_top_layer_only") == "1" });
	option = Option(def, "seq_top_layer_only");
	m_optgroups_gui.back()->append_single_option_line(option);

	if (is_editor) {
		def.label = L("Show sidebar collapse/expand button");
		def.type = coBool;
		def.tooltip = L("If enabled, the button for the collapse sidebar will be appeared in top right corner of the 3D Scene");
		def.set_default_value(new ConfigOptionBool{ app_config->get("show_collapse_button") == "1" });
		option = Option(def, "show_collapse_button");
		m_optgroups_gui.back()->append_single_option_line(option);

		def.label = L("Suppress to open hyperlink in browser");
		def.type = coBool;
		def.tooltip = L("If enabled, the descriptions of configuration parameters in settings tabs wouldn't work as hyperlinks. "
			"If disabled, the descriptions of configuration parameters in settings tabs will work as hyperlinks.");
		def.set_default_value(new ConfigOptionBool{ app_config->get("suppress_hyperlinks") == "1" });
		option = Option(def, "suppress_hyperlinks");
		m_optgroups_gui.back()->append_single_option_line(option);

		activate_options_tab(m_optgroups_gui.back(), 3);
		m_optgroups_gui.emplace_back(create_gui_options_group(_L("Appearance"), tabs));

		def.label = L("Use custom size for toolbar icons");
		def.type = coBool;
		def.tooltip = L("If enabled, you can change size of toolbar icons manually.");
		def.set_default_value(new ConfigOptionBool{ app_config->get("use_custom_toolbar_size") == "1" });
		option = Option(def, "use_custom_toolbar_size");
		m_optgroups_gui.back()->append_single_option_line(option);

		create_icon_size_slider(m_optgroups_gui.back().get());
		m_icon_size_sizer->ShowItems(app_config->get("use_custom_toolbar_size") == "1");

		def.label = L("Tab icon size");
		def.type = coInt;
		def.tooltip = L("Size of the tab icons, in pixels. Set to 0 to remove icons.");
		def.set_default_value(new ConfigOptionInt{ atoi(app_config->get("tab_icon_size").c_str()) });
		option = Option(def, "tab_icon_size");
		option.opt.width = 6;
		m_optgroups_gui.back()->append_single_option_line(option);
		m_values_need_restart.push_back("tab_icon_size");

		def.label = L("Display setting icons");
		def.type = coBool;
		def.tooltip = L("The settings have a lock and dot to show how they are modified. You can hide them by uncheking this option.");
		def.set_default_value(new ConfigOptionBool{ app_config->get("setting_icon") == "1" });
		option = Option(def, "setting_icon");
		option.opt.width = 6;
		m_optgroups_gui.back()->append_single_option_line(option);
		m_values_need_restart.push_back("setting_icon");

		def.label = L("Use custom tooltip");
		def.type = coBool;
		def.tooltip = L("On some OS like MacOS or some Linux, tooltips can't stay on for a long time. This setting replaces native tooltips with custom dialogs to improve readability (only for settings)."
			"\nNote that for the number controls, you need to hover the arrows to get the custom tooltip. Also, it keeps the focus but will give it back when it closes. It won't show up if you are editing the field.");
		def.set_default_value(new ConfigOptionBool{ app_config->has("use_rich_tooltip") ? app_config->get("use_rich_tooltip") == "1" :
#if __APPLE__
			true
#else
			false
#endif
			});
		option = Option(def, "use_rich_tooltip");
		m_optgroups_gui.back()->append_single_option_line(option);
		m_values_need_restart.push_back("use_rich_tooltip");
	}



	activate_options_tab(m_optgroups_gui.back(), 3);
	m_optgroups_gui.emplace_back(create_gui_options_group(_L("Colors"), tabs));
	// color prusa -> susie eb7221
	//ICON 237, 107, 33 -> ed6b21 ; 2172eb
	//DARK 237, 107, 33 -> ed6b21 ; 32, 113, 234 2071ea
	//MAIN 253, 126, 66 -> fd7e42 ; 66, 141, 253 428dfd
	//LIGHT 254, 177, 139 -> feac8b; 139, 185, 254 8bb9fe
	//TEXT 1.0f, 0.49f, 0.22f, 1.0f ff7d38 ; 0.26f, 0.55f, 1.0f, 1.0f 428cff

	def.label = L("Very dark gui color");
	def.type = coString;
	def.tooltip = _u8L("Very dark color, in the RGB hex format.")
		+ " "  + _u8L("Mainly used as background or dark text color.")
		+ "\n" + _u8L("Slic3r(yellow): ada230, PrusaSlicer(orange): c46737, SuperSlicer(blue): 0047c7");
	def.set_default_value(new ConfigOptionString{ app_config->get("color_very_dark") });
	option = Option(def, "color_very_dark");
	option.opt.width = 6;
	m_optgroups_gui.back()->append_single_option_line(option);
	m_values_need_restart.push_back("color_very_dark");

	def.label = L("Dark gui color");
	def.type = coString;
	def.tooltip = _u8L("Dark color, in the RGB hex format.")
		+ " " + _u8L("Mainly used as icon color.")
		+ "\n" + _u8L("Slic3r(yellow): cabe39, PrusaSlicer(orange): ed6b21, SuperSlicer(blue): 2172eb");
	def.set_default_value(new ConfigOptionString{ app_config->get("color_dark") });
	option = Option(def, "color_dark");
	option.opt.width = 6;
	m_optgroups_gui.back()->append_single_option_line(option);
	m_values_need_restart.push_back("color_dark");

	def.label = L("Gui color");
	def.type = coString;
	def.tooltip = _u8L("Main color, in the RGB hex format.")
		+ " " + _u8L("Slic3r(yellow): eddc21, PrusaSlicer(orange): fd7e42, SuperSlicer(blue): 428dfd");
	def.set_default_value(new ConfigOptionString{ app_config->get("color") });
	option = Option(def, "color");
	option.opt.width = 6;
	m_optgroups_gui.back()->append_single_option_line(option);
	m_values_need_restart.push_back("color");

	def.label = L("Light gui color");
	def.type = coString;
	def.tooltip = _u8L("Light color, in the RGB hex format.")
		+ " " + _u8L("Slic3r(yellow): ffee38, PrusaSlicer(orange): feac8b, SuperSlicer(blue): 8bb9fe");
	def.set_default_value(new ConfigOptionString{ app_config->get("color_light") });
	option = Option(def, "color_light");
	option.opt.width = 6;
	m_optgroups_gui.back()->append_single_option_line(option);
	m_values_need_restart.push_back("color_light");

	def.label = L("Very light gui color");
	def.type = coString;
	def.tooltip = _u8L("Very light color, in the RGB hex format.")
		+ " " + _u8L("Mainly used as light text color.")
		+ "\n" + _u8L("Slic3r(yellow): fef48b, PrusaSlicer(orange): ff7d38, SuperSlicer(blue): 428cff");
	def.set_default_value(new ConfigOptionString{ app_config->get("color_very_light") });
	option = Option(def, "color_very_light");
	option.opt.width = 6;
	m_optgroups_gui.back()->append_single_option_line(option);
	m_values_need_restart.push_back("color_very_light");

	activate_options_tab(m_optgroups_gui.back(), 3);

	//create layout options
	create_settings_mode_widget(tabs);

#if ENABLE_ENVIRONMENT_MAP
	if (is_editor) {
		// Add "Render" tab
		m_optgroup_render = create_options_tab(_L("Render"), tabs);
	m_optgroup_render->m_on_change = [this](t_config_option_key opt_key, boost::any value) {
		m_values[opt_key] = boost::any_cast<bool>(value) ? "1" : "0";
	};

	def.label = L("Use environment map");
	def.type = coBool;
	def.tooltip = L("If enabled, renders object using the environment map.");
	def.set_default_value(new ConfigOptionBool{ app_config->get("use_environment_map") == "1" });
	option = Option(def, "use_environment_map");
	m_optgroup_render->append_single_option_line(option);

		activate_options_tab(m_optgroup_render);
	}
#endif // ENABLE_ENVIRONMENT_MAP

	auto sizer = new wxBoxSizer(wxVERTICAL);
	sizer->Add(tabs, 1, wxEXPAND | wxTOP | wxLEFT | wxRIGHT, 5);

	auto buttons = CreateStdDialogButtonSizer(wxOK | wxCANCEL);
	wxButton* btn = static_cast<wxButton*>(FindWindowById(wxID_OK, this));
	btn->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { accept(); });
	sizer->Add(buttons, 0, wxALIGN_CENTER_HORIZONTAL | wxBOTTOM | wxTOP, 10);

	SetSizer(sizer);
	sizer->SetSizeHints(this);
	this->CenterOnParent();
}

void PreferencesDialog::accept()
{
	bool need_restart = false;
	for (auto key : m_values_need_restart)
		if (m_values.find(key) != m_values.end())
			need_restart = true;
	if (need_restart) {
		warning_catcher(this, wxString::Format(_L("You need to restart %s to make the changes effective."), SLIC3R_APP_NAME));
	}

	auto app_config = get_app_config();

	m_seq_top_layer_only_changed = false;
	if (auto it = m_values.find("seq_top_layer_only"); it != m_values.end())
		m_seq_top_layer_only_changed = app_config->get("seq_top_layer_only") != it->second;

	m_settings_layout_changed = false;
	for (const std::string& key : { "old_settings_layout_mode",
								    "new_settings_layout_mode",
								    "dlg_settings_layout_mode" })
	{
	    auto it = m_values.find(key);
	    if (it != m_values.end() && app_config->get(key) != it->second) {
			m_settings_layout_changed = true;
			break;
		}
	}

	for (const std::string& key : {"default_action_on_close_application", "default_action_on_select_preset"}) {
	    auto it = m_values.find(key);
		if (it != m_values.end() && it->second != "none" && app_config->get(key) != "none")
			m_values.erase(it); // we shouldn't change value, if some of those parameters was selected, and then deselected
	}

	auto it_auto_switch_preview = m_values.find("auto_switch_preview");
	if (it_auto_switch_preview != m_values.end()) {
		std::vector<std::string> values = def_combobox_auto_switch_preview.enum_values;
		for(size_t i=0; i< values.size(); i++)
		if (values[i] == it_auto_switch_preview->second)
			it_auto_switch_preview->second = std::to_string(i);
	}

	auto it_background_processing = m_values.find("background_processing");
	if (it_background_processing != m_values.end() && it_background_processing->second == "1") {
		bool warning = app_config->get("auto_switch_preview") != "0";
		if (it_auto_switch_preview != m_values.end())
			warning = it_auto_switch_preview->second == "1";
		if(warning) {
			wxMessageDialog dialog(nullptr, "Using background processing with automatic tab switching may be combersome"
				", are-you sure to keep the automatic tab switching?", _L("Are you sure?"), wxOK | wxCANCEL | wxICON_QUESTION);
			if (dialog.ShowModal() == wxID_CANCEL)
				m_values["auto_switch_preview"] = "0";
		}
	}

	for (std::map<std::string, std::string>::iterator it = m_values.begin(); it != m_values.end(); ++it)
		app_config->set(it->first, it->second);

	app_config->save();
	EndModal(wxID_OK);

	if (m_settings_layout_changed)
		;// application will be recreated after Preference dialog will be destroyed
	else
	// Nothify the UI to update itself from the ini file.
    wxGetApp().update_ui_from_settings();
}

void PreferencesDialog::on_dpi_changed(const wxRect &suggested_rect)
{
	for (int i = 0; i < (int)m_optgroups_general.size(); i++) {
		if (m_optgroups_general[i]) {
			m_optgroups_general[i]->msw_rescale();
		} else {
			m_optgroups_general.erase(m_optgroups_general.begin() + i);
			i--;
		}
	}
	m_optgroup_paths->msw_rescale();
	m_optgroup_camera->msw_rescale();
	for (int i = 0; i < (int)m_optgroups_gui.size(); i++) {
		if (m_optgroups_gui[i]) {
			m_optgroups_gui[i]->msw_rescale();
		} else {
			m_optgroups_gui.erase(m_optgroups_gui.begin() + i);
			i--;
		}
	}

    msw_buttons_rescale(this, em_unit(), { wxID_OK, wxID_CANCEL });

    layout();
}

void PreferencesDialog::layout()
{
    const int em = em_unit();

    SetMinSize(wxSize(47 * em, 28 * em));
    Fit();

    Refresh();
}

void PreferencesDialog::create_icon_size_slider(ConfigOptionsGroup* container)
{
    const auto app_config = get_app_config();

    const int em = em_unit();

    m_icon_size_sizer = new wxBoxSizer(wxHORIZONTAL);

	wxWindow* parent = container->parent();

    if (isOSX)
        // For correct rendering of the slider and value label under OSX
        // we should use system default background
        parent->SetBackgroundStyle(wxBG_STYLE_ERASE);

    auto label = new wxStaticText(parent, wxID_ANY, _L("Icon size in a respect to the default size") + " (%) :");

    m_icon_size_sizer->Add(label, 0, wxALIGN_CENTER_VERTICAL| wxRIGHT | (isOSX ? 0 : wxLEFT), em);

    const int def_val = atoi(app_config->get("custom_toolbar_size").c_str());

    long style = wxSL_HORIZONTAL;
    if (!isOSX)
        style |= wxSL_LABELS | wxSL_AUTOTICKS;

    auto slider = new wxSlider(parent, wxID_ANY, def_val, 30, 100, 
                               wxDefaultPosition, wxDefaultSize, style);

    slider->SetTickFreq(10);
    slider->SetPageSize(10);
    slider->SetToolTip(_L("Select toolbar icon size in respect to the default one."));

    m_icon_size_sizer->Add(slider, 1, wxEXPAND);

    wxStaticText* val_label{ nullptr };
    if (isOSX) {
        val_label = new wxStaticText(parent, wxID_ANY, wxString::Format("%d", def_val));
        m_icon_size_sizer->Add(val_label, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, em);
    }

    slider->Bind(wxEVT_SLIDER, ([this, slider, val_label](wxCommandEvent e) {
        auto val = slider->GetValue();
        m_values["custom_toolbar_size"] = (boost::format("%d") % val).str();

        if (val_label)
            val_label->SetLabelText(wxString::Format("%d", val));
    }), slider->GetId());

    for (wxWindow* win : std::vector<wxWindow*>{ slider, label, val_label }) {
        if (!win) continue;         
        win->SetFont(wxGetApp().normal_font());

        if (isOSX) continue; // under OSX we use wxBG_STYLE_ERASE
        win->SetBackgroundStyle(wxBG_STYLE_PAINT);
    }

	container->parent()->GetSizer()->Add(m_icon_size_sizer, 0, wxEXPAND | wxALL, em);
}

void PreferencesDialog::create_settings_mode_widget(wxNotebook* tabs)
{
	wxString choices[] = { _L("Layout with the tab bar"),
						   _L("Legacy layout"),
						   _L("Access via settings button in the top menu"),
						   _L("Settings in non-modal window") };

    auto app_config = get_app_config();
    int selection = app_config->get("tab_settings_layout_mode") == "1" ? 0 :
                    app_config->get("old_settings_layout_mode") == "1" ? 1 :
                    app_config->get("new_settings_layout_mode") == "1" ? 2 :
                    app_config->get("dlg_settings_layout_mode") == "1" ? 3 :
#ifndef WIN32
        1;
#else
        0;
#endif

	wxWindow* parent = m_optgroups_gui.back()->parent();

	m_layout_mode_box = new wxRadioBox(parent, wxID_ANY, _L("Layout Options"), wxDefaultPosition, wxDefaultSize,
		WXSIZEOF(choices), choices, 4, wxRA_SPECIFY_ROWS);
	m_layout_mode_box->SetFont(wxGetApp().normal_font());
	m_layout_mode_box->SetSelection(selection);

	m_layout_mode_box->Bind(wxEVT_RADIOBOX, [this](wxCommandEvent& e) {
		int selection = e.GetSelection();
		m_values["tab_settings_layout_mode"] = boost::any_cast<bool>(selection == 0) ? "1" : "0";
		m_values["old_settings_layout_mode"] = boost::any_cast<bool>(selection == 1) ? "1" : "0";
		m_values["new_settings_layout_mode"] = boost::any_cast<bool>(selection == 2) ? "1" : "0";
		m_values["dlg_settings_layout_mode"] = boost::any_cast<bool>(selection == 3) ? "1" : "0";
	});
	wxString unstable_warning = _L("!! Can be unstable in some os distribution !!");
	m_layout_mode_box->SetToolTip(_L("Choose how the windows are selectable and displayed:")
		+ "\n* " + _L(" Tab layout: all windows are in the application, all are selectable via a tab.")
#ifndef WIN32
		+ " " + unstable_warning
#endif
		+ "\n* " + _L("Old layout: all windows are in the application, settings are on the top tab bar and the plater choice in on the bottom of the plater view.")
		+ "\n* " + _L("Settings button: all windows are in the application, no tabs: you have to clic on settings gears to switch to settings tabs.")
		+ "\n* " + _L("Settings window: settings are displayed in their own window. You have to clic on settings gears to show the settings window.")
	);

	auto sizer = new wxBoxSizer(wxHORIZONTAL);
	sizer->Add(m_layout_mode_box, 1, wxALIGN_CENTER_VERTICAL);
	wxBoxSizer* parent_sizer = static_cast<wxBoxSizer*>(static_cast<wxPanel*>(tabs->GetPage(3))->GetSizer());
	parent_sizer->Add(sizer, 0, wxEXPAND);
}


} // GUI
} // Slic3r
