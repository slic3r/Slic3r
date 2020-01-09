# How to customize the setting UI

## How it works
The software will search for each tab the needed ui files
* for fff printers:
  * "print.ui" for the Print Settings tab
  * "filament.ui" for the Filament Settings tab
  * "printer.ui" for the Printer Settings tab
* for slaprinters:
  * "sla_print.ui" for the Print Settings tab
  * "sla_material.ui" for the Filament Settings tab
  * "sla_printer.ui" for the Printer Settings tab
If a ui file isn't here, it will build the tab with the default (harcoded) layout.
## syntax
The tree is composed of page, group, lines and settings
A group has to be inside a page.
A line has to be inside a group.
A setting has to be inside a group or a line.
Each object has parameters
### syntax of each object
STR represent a label that can conatins any character but ':', leading and trailing space and tabs are removed.
INT represent an integer
parameters that are inside [] are optionals
each parameter is separated by ':'
* Page: 
	page:STR:STR
	* first STR is for the label and the second for the icon, with or without the .svg / .png
* Group: 
	group[:nolabel][:label_width$INT][:EVENT]:STR
	* EVENT can be extruders_count_event if the group contains extruders_count and is a printer tab ; silent_mode_event if the group contains silent_mode and is a printer tab ; material_density_event if the group contains material_density.
	* label_width$INT is used to set the size of the left column, where labels are draw.
	* nolabel is used to remove the left column, where labels are draw.
* Line:
	line:STR
* setting:
	setting[label$STR][:full_label][:full_width][:simple|advanced|expert][:width$INT][:width$INT][:id$INT]:STR
	* STR, the last parameter: the id name of the setting.
	* label$STR : to override the label by this new one
	* full_label: to override the label by the "full one"
	* full_width: to tell to create a field that span the full width
	* simple|advanced|expert: add one of these to modify the mode in which this setting appear. If it's inside a lien, the first setting of the line decide for all the line.
	* width$INT: change the width of the field. Don't works (yet) with every type of setting.
	* height$INT: change the height of the field. Don't works (yet) with every type of setting.
	* id $INT : for setting only a single value of a setting array.
* recommended_thin_wall_thickness_description: create a text widget to explain recommended thin wall thickness (only in a fff print tab)
* parent_preset_description: create a text widget to explain parent preset
* cooling_description: create a text widget to explain cooling (only in a filament tab)
* volumetric_speed_description: create a text widget to explain volumetric speed (only in a filament tab)
* filament_ramming_parameters: create a  widget for filament ramming
* filament_overrides_page: create a page for overrides (only in a filament tab)
* unregular_pages: create needed special pages for a fff printer tab
* printhost: create printhost settings for the group (only in a printer tab)
* bed_shape: create bed shape widget (only in a printer tab)
* extruders_count: create extruders_count setting (only in a fff printer tab)
* logs: activated logs
### ui file syntax
trailing & leading tabs & spaces are removed, so you can indent as you want.
If the first character is '#', then this line is ignored
You can end page, group and line section by end_page, end_group but it's not mandatory as they do nothing. You have to use end_line because it indicates when the line end and you have to stop adding settings inside. Note that it's added automatically when line, group or page is called.

exemple:

    page:my page:my icon name
    	group:my group name
    		setting:label$Choose your base layer height, if you dare:layer_height
    		line:perimeters
    			settings:label$count:perimeters
    			settings:label$only one is spiral:spiral_vase
    		end_line
    	end_group
    end_page

## notes
A print.ui.legacy is here with the prusaslicer tabs, just rename print.ui and remove the .legacy to switch from the slic3r++ print layout to prusaslicer print layout.
