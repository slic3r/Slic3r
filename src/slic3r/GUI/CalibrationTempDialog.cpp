#include "CalibrationTempDialog.hpp"
#include "I18N.hpp"
#include "libslic3r/Utils.hpp"
#include "GUI.hpp"
#include "GUI_ObjectList.hpp"
#include "Tab.hpp"
#include <wx/scrolwin.h>
#include <wx/display.h>
#include <wx/file.h>
#include "wxExtensions.hpp"

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

    CalibrationTempDialog::CalibrationTempDialog(GUI_App* app, MainFrame* mainframe)
        : DPIDialog(NULL, wxID_ANY, wxString(SLIC3R_APP_NAME) + " - " + _(L("Temp calibration - test objects generation")),
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

    auto main_sizer = new wxBoxSizer(wxVERTICAL);

    //html
    html_viewer = new wxHtmlWindow(this, wxID_ANY,
        wxDefaultPosition, wxSize(800, 500), wxHW_SCROLLBAR_AUTO);
    html_viewer->LoadPage("./resources/calibration/filament_temp/filament_temp.html");
    main_sizer->Add(html_viewer, 1, wxEXPAND | wxALL, 5);

    wxStdDialogButtonSizer* buttons = new wxStdDialogButtonSizer();
    wxButton* bt = new wxButton(this, wxID_FILE1, _(L("Generate +- 10")));
    bt->Bind(wxEVT_BUTTON, &CalibrationTempDialog::create_geometry_2, this);
    buttons->Add(bt);
    bt = new wxButton(this, wxID_FILE1, _(L("Generate +- 20")));
    bt->Bind(wxEVT_BUTTON, &CalibrationTempDialog::create_geometry_4, this);
    buttons->Add(bt);
    wxButton* close = new wxButton(this, wxID_CLOSE, _(L("Close")));
    close->Bind(wxEVT_BUTTON, &CalibrationTempDialog::closeMe, this);
    buttons->AddButton(close);
    close->SetDefault();
    close->SetFocus();
    SetAffirmativeId(wxID_CLOSE);
    buttons->Realize();
    main_sizer->Add(buttons, 0, wxEXPAND | wxALL, 5);

    SetSizer(main_sizer);
    main_sizer->SetSizeHints(this);
}

void CalibrationTempDialog::closeMe(wxCommandEvent& event_args) {
    this->Destroy();
}

ModelObject* CalibrationTempDialog::add_part(ModelObject* model_object, std::string input_file, Vec3d move, Vec3d scale) {
    Model model;
    try {
        model = Model::read_from_file(input_file);
    }
    catch (std::exception & e) {
        auto msg = _(L("Error!")) + " " + input_file + " : " + e.what() + ".";
        show_error(this, msg);
        exit(1);
    }

    for (ModelObject* object : model.objects) {
        Vec3d delta = Vec3d::Zero();
        if (model_object->origin_translation != Vec3d::Zero())
        {
            object->center_around_origin();
            delta = model_object->origin_translation - object->origin_translation;
        }
        for (ModelVolume* volume : object->volumes) {
            volume->translate(delta + move);
            if (scale != Vec3d{ 1,1,1 }) {
                volume->scale(scale);
            }
            ModelVolume* new_volume = model_object->add_volume(*volume);
            new_volume->set_type(ModelVolumeType::MODEL_PART);
            new_volume->name = boost::filesystem::path(input_file).filename().string();

            //volumes_info.push_back(std::make_pair(from_u8(new_volume->name), new_volume->get_mesh_errors_count() > 0));

            // set a default extruder value, since user can't add it manually
            new_volume->config.set_key_value("extruder", new ConfigOptionInt(0));

            //move to bed
            /* const TriangleMesh& hull = new_volume->get_convex_hull();
            float min_z = std::numeric_limits<float>::max();
            for (const stl_facet& facet : hull.stl.facet_start) {
                for (int i = 0; i < 3; ++i)
                    min_z = std::min(min_z, Vec3f::UnitZ().dot(facet.vertex[i]));
            }
            volume->translate(Vec3d(0,0,-min_z));*/
        }
    }
    assert(model.objects.size() == 1);
    return model.objects.empty()?nullptr: model.objects[0];
}

void CalibrationTempDialog::create_geometry(uint8_t nb_delta) {
    Plater* plat = this->main_frame->plater();
    Model& model = plat->model();
    plat->reset();
    std::vector<size_t> objs_idx = plat->load_files(std::vector<std::string>{
            "./resources/calibration/filament_temp/Smart_compact_temperature_calibration_item.amf"}, true, false);

    assert(objs_idx.size() == 1);
    const DynamicPrintConfig* printConfig = this->gui_app->get_tab(Preset::TYPE_PRINT)->get_config();
    const DynamicPrintConfig* filamentConfig = this->gui_app->get_tab(Preset::TYPE_FILAMENT)->get_config();
    const DynamicPrintConfig* printerConfig = this->gui_app->get_tab(Preset::TYPE_PRINTER)->get_config();

    // -- get temps
    const ConfigOptionInts* temperature_config = filamentConfig->option<ConfigOptionInts>("temperature");
    assert(temperature_config->values.size() >= 1);
    int16_t temperature = 5* (temperature_config->values[0]/5);
    size_t nb_items = 1 + 2 * nb_delta;
    
    /// --- scale ---
    //model is created for a 0.4 nozzle, scale xy with nozzle size.
    const ConfigOptionFloats* nozzle_diameter_config = printerConfig->option<ConfigOptionFloats>("nozzle_diameter");
    assert(nozzle_diameter_config->values.size() > 0);
    float nozzle_diameter = nozzle_diameter_config->values[0];
    float xyzScale = nozzle_diameter / 0.4;
    //do scaling
    if (xyzScale < 0.9 || 1.1 < xyzScale) {
        for (size_t i = 0; i < 5; i++)
            model.objects[objs_idx[0]]->scale(xyzScale, xyzScale * 0.5, xyzScale);
    } else {
        xyzScale = 1;
        model.objects[objs_idx[0]]->scale(xyzScale, xyzScale * 0.5, xyzScale);
    }

    //add 8 others
    std::vector<ModelObject*>tower;
    tower.push_back(model.objects[objs_idx[0]]);
    float zshift = (1 - xyzScale) / 2;
    if (temperature - (int8_t)nb_delta * 5 > 175 && temperature - (int8_t)nb_delta * 5 < 290) {
        tower.push_back(add_part(model.objects[objs_idx[0]], "./resources/calibration/filament_temp/t"+std::to_string(temperature - (int8_t)nb_delta * 5 )+".amf",
            Vec3d{ xyzScale * 5, - xyzScale * 2.5, zshift - xyzScale * 2.5}, Vec3d{ xyzScale, xyzScale, xyzScale * 0.43 }));
    }
    for (int16_t i = 1; i < nb_items; i++) {
        tower.push_back(add_part(model.objects[objs_idx[0]], "./resources/calibration/filament_temp/Smart_compact_temperature_calibration_item.amf", 
            Vec3d{ 0,0, zshift + i * 10 }, Vec3d{ xyzScale, xyzScale * 0.5, xyzScale }));
        if (temperature - (int8_t)nb_delta * 5 + i * 5 > 175 && temperature - (int8_t)nb_delta * 5 + i * 5 < 290) {
            tower.push_back(add_part(model.objects[objs_idx[0]], "./resources/calibration/filament_temp/t" + std::to_string(temperature - (int8_t)nb_delta * 5 + i * 5) + ".amf",
                Vec3d{ xyzScale * 5, -xyzScale * 2.5, zshift + xyzScale * (i * 10 - 2.5) }, Vec3d{ xyzScale, xyzScale, xyzScale * 0.43 }));
        }
    }


    /// --- translate ---
    const ConfigOptionPoints* bed_shape = printerConfig->option<ConfigOptionPoints>("bed_shape");
    Vec2d bed_size = BoundingBoxf(bed_shape->values).size();
    Vec2d bed_min = BoundingBoxf(bed_shape->values).min;
    model.objects[objs_idx[0]]->translate({ bed_min.x() + bed_size.x() / 2, bed_min.y() + bed_size.y() / 2, 0 });

    /// --- main config, please modify object config when possible ---
    DynamicPrintConfig new_print_config = *printConfig; //make a copy
    new_print_config.set_key_value("complete_objects", new ConfigOptionBool(false));
    
    /// -- generate the heat change gcode
    std::string str_layer_gcode = "{if layer_num > 0 and layer_z  <= " + std::to_string(2 * xyzScale) + "}\nM104 S" + std::to_string(temperature - (int8_t)nb_delta * 5);
    for (int16_t i = 1; i < nb_items; i++) {
        str_layer_gcode += "\n{ elsif layer_z >= " + std::to_string(i * 10 * xyzScale) + " and layer_z <= " + std::to_string((1 + i * 10) * xyzScale) + " }\nM104 S" + std::to_string(temperature - (int8_t)nb_delta * 5 + i * 5);
    }
    str_layer_gcode += "\n{endif}\n";
    DynamicPrintConfig new_printer_config = *printerConfig; //make a copy
    new_printer_config.set_key_value("layer_gcode", new ConfigOptionString(str_layer_gcode));

    /// --- custom config ---
    float brim_width = printConfig->option<ConfigOptionFloat>("brim_width")->value;
    if (brim_width < nozzle_diameter * 8) {
        model.objects[objs_idx[0]]->config.set_key_value("brim_width", new ConfigOptionFloat(nozzle_diameter * 8));
    }
    model.objects[objs_idx[0]]->config.set_key_value("brim_ears", new ConfigOptionBool(false));
    model.objects[objs_idx[0]]->config.set_key_value("perimeters", new ConfigOptionInt(1));
    model.objects[objs_idx[0]]->config.set_key_value("extra_perimeters_overhangs", new ConfigOptionBool(true));
    model.objects[objs_idx[0]]->config.set_key_value("bottom_solid_layers", new ConfigOptionInt(2));
    model.objects[objs_idx[0]]->config.set_key_value("top_solid_layers", new ConfigOptionInt(3)); 
    model.objects[objs_idx[0]]->config.set_key_value("gap_fill", new ConfigOptionBool(false)); 
    model.objects[objs_idx[0]]->config.set_key_value("thin_perimeters", new ConfigOptionBool(true));
    model.objects[objs_idx[0]]->config.set_key_value("layer_height", new ConfigOptionFloat(nozzle_diameter / 2));
    model.objects[objs_idx[0]]->config.set_key_value("first_layer_height", new ConfigOptionFloatOrPercent(nozzle_diameter / 2, false));
    model.objects[objs_idx[0]]->config.set_key_value("fill_density", new ConfigOptionPercent(7));
    model.objects[objs_idx[0]]->config.set_key_value("solid_fill_pattern", new ConfigOptionEnum<InfillPattern>(ipRectilinearWGapFill));
    model.objects[objs_idx[0]]->config.set_key_value("top_fill_pattern", new ConfigOptionEnum<InfillPattern>(ipRectilinearWGapFill));

    //update plater
    this->gui_app->get_tab(Preset::TYPE_PRINT)->load_config(new_print_config);
    plat->on_config_change(new_print_config);
    this->gui_app->get_tab(Preset::TYPE_PRINTER)->load_config(new_printer_config);
    plat->on_config_change(new_printer_config);
    plat->changed_objects(objs_idx);
    this->gui_app->get_tab(Preset::TYPE_PRINT)->update_dirty();
    this->gui_app->get_tab(Preset::TYPE_PRINTER)->update_dirty();
    //update everything, easier to code.
    ObjectList* obj = this->gui_app->obj_list();
    obj->update_after_undo_redo();


    plat->reslice();
    plat->select_view_3D("Preview");
}

void CalibrationTempDialog::on_dpi_changed(const wxRect& suggested_rect)
{
    msw_buttons_rescale(this, em_unit(), { wxID_OK });

    Layout();
    Fit();
    Refresh();
}

wxPanel* CalibrationTempDialog::create_header(wxWindow* parent, const wxFont& bold_font)
{
    wxPanel* panel = new wxPanel(parent);
    wxBoxSizer* sizer = new wxBoxSizer(wxHORIZONTAL);

    wxFont header_font = bold_font;
#ifdef __WXOSX__
    header_font.SetPointSize(14);
#else
    header_font.SetPointSize(bold_font.GetPointSize() + 2);
#endif // __WXOSX__

    sizer->AddStretchSpacer();

    // text
    wxStaticText* text = new wxStaticText(panel, wxID_ANY, _(L("Keyboard shortcuts")));
    text->SetFont(header_font);
    sizer->Add(text, 0, wxALIGN_CENTER_VERTICAL);

    sizer->AddStretchSpacer();

    panel->SetSizer(sizer);
    return panel;
}

} // namespace GUI
} // namespace Slic3r
