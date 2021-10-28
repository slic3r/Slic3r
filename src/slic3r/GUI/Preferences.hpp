#ifndef slic3r_Preferences_hpp_
#define slic3r_Preferences_hpp_

#include "GUI.hpp"
#include "GUI_Utils.hpp"

#include <wx/dialog.h>
#include <map>
#include <vector>

class wxRadioBox;

namespace Slic3r {
namespace GUI {

class ConfigOptionsGroup;

class PreferencesDialog : public DPIDialog
{
	std::map<std::string, std::string>	m_values;
	std::vector<std::string>			m_values_need_restart;
	std::vector<std::shared_ptr<ConfigOptionsGroup>> m_optgroups_general;
	std::shared_ptr<ConfigOptionsGroup>	m_optgroup_paths;
	std::shared_ptr<ConfigOptionsGroup>	m_optgroup_camera;
	std::vector<std::shared_ptr<ConfigOptionsGroup>> m_optgroups_gui;
#if ENABLE_ENVIRONMENT_MAP
	std::shared_ptr<ConfigOptionsGroup>	m_optgroup_render;
#endif // ENABLE_ENVIRONMENT_MAP

    ConfigOptionDef def_combobox_auto_switch_preview;

	wxSizer*                            m_icon_size_sizer;
	wxRadioBox*							m_layout_mode_box;
    bool                                isOSX {false};
	bool								m_settings_layout_changed {false};
	bool								m_seq_top_layer_only_changed{ false };
public:
	PreferencesDialog(wxWindow* parent);
	~PreferencesDialog() {}

	bool settings_layout_changed() const { return m_settings_layout_changed; }
	bool seq_top_layer_only_changed() const { return m_seq_top_layer_only_changed; }

	void	build();
	void	accept();

protected:
    void on_dpi_changed(const wxRect &suggested_rect) override;
    void layout();
    void create_icon_size_slider(ConfigOptionsGroup* parent);
    void create_settings_mode_widget(wxNotebook* tabs);
	std::shared_ptr<ConfigOptionsGroup> create_general_options_group(const wxString& title, wxNotebook* tabs);
	std::shared_ptr<ConfigOptionsGroup> create_gui_options_group(const wxString& title, wxNotebook* tabs);
};

} // GUI
} // Slic3r


#endif /* slic3r_Preferences_hpp_ */
