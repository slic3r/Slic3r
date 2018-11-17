#include <memory>
#include <climits>

#include <wx/progdlg.h>
#include <wx/window.h> 
#include <wx/numdlg.h> 


#include "Plater.hpp"
#include "ProgressStatusBar.hpp"
#include "Log.hpp"
#include "MainFrame.hpp"
#include "BoundingBox.hpp"
#include "Geometry.hpp"
#include "Dialogs/AnglePicker.hpp"
#include "Dialogs/ObjectCutDialog.hpp"


namespace Slic3r { namespace GUI {

const auto TB_ADD           {wxNewId()};
const auto TB_REMOVE        {wxNewId()};
const auto TB_RESET         {wxNewId()};
const auto TB_ARRANGE       {wxNewId()};
const auto TB_EXPORT_GCODE  {wxNewId()};
const auto TB_EXPORT_STL    {wxNewId()};
const auto TB_MORE          {wxNewId()};
const auto TB_FEWER         {wxNewId()};
const auto TB_45CW          {wxNewId()};
const auto TB_45CCW         {wxNewId()};
const auto TB_SCALE         {wxNewId()};
const auto TB_SPLIT         {wxNewId()};
const auto TB_CUT           {wxNewId()};
const auto TB_LAYERS        {wxNewId()};
const auto TB_SETTINGS      {wxNewId()};

const auto PROGRESS_BAR_EVENT = wxNewEventType();

Plater::Plater(wxWindow* parent, const wxString& title) : 
    wxPanel(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL, title),
    _presets(new PresetChooser(dynamic_cast<wxWindow*>(this), this->print))
{

    // Set callback for status event for worker threads
    /*
    this->print->set_status_cb([=](std::string percent percent, std::wstring message) {
        wxPostEvent(this, new wxPlThreadEvent(-1, PROGRESS_BAR_EVENT, 
    });
    */
    _presets->load();

    // Initialize handlers for canvases
    auto on_select_object {[this](ObjIdx obj_idx) { this->select_object(obj_idx); }};
    auto on_double_click {[this]() { if (this->selected_object() != this->objects.end()) this->object_settings_dialog(); }};
    auto on_right_click {[this](wxPanel* canvas, const wxPoint& pos) 
        {
            auto obj = this->selected_object();
            if (obj == this->objects.end()) return;

            auto menu = this->object_menu();
            canvas->PopupMenu(menu, pos);
            delete menu;
        }};
    auto on_instances_moved {[this]() { this->on_model_change(); }};

    /* 
    # Initialize 3D plater
    if ($Slic3r::GUI::have_OpenGL) {
        $self->{canvas3D} = Slic3r::GUI::Plater::3D->new($self->{preview_notebook}, $self->{objects}, $self->{model}, $self->{config});
        $self->{preview_notebook}->AddPage($self->{canvas3D}, '3D');
        $self->{canvas3D}->set_on_select_object($on_select_object);
        $self->{canvas3D}->set_on_double_click($on_double_click);
        $self->{canvas3D}->set_on_right_click(sub { $on_right_click->($self->{canvas3D}, @_); });
        $self->{canvas3D}->set_on_instances_moved($on_instances_moved);
        $self->{canvas3D}->on_viewport_changed(sub {
            $self->{preview3D}->canvas->set_viewport_from_scene($self->{canvas3D});
        });
    }
    */

    // initialize 2D Preview Canvas
    canvas2D = new Plate2D(preview_notebook, wxDefaultSize, objects, model, config);
    preview_notebook->AddPage(canvas2D, _("2D"));

    canvas2D->on_select_object = std::function<void (ObjIdx obj_idx)>(on_select_object);
    canvas2D->on_double_click = std::function<void ()>(on_double_click);
    canvas2D->on_right_click = std::function<void (const wxPoint& pos)>([=](const wxPoint& pos){ on_right_click(canvas2D, pos); });
    canvas2D->on_instances_moved = std::function<void ()>(on_instances_moved);


    canvas3D = new Plate3D(preview_notebook, wxDefaultSize, objects, model, config);
    preview_notebook->AddPage(canvas3D, _("3D"));

    canvas3D->on_select_object = std::function<void (ObjIdx obj_idx)>(on_select_object);
    canvas3D->on_instances_moved = std::function<void ()>(on_instances_moved);
    
    preview3D = new Preview3D(preview_notebook, wxDefaultSize, print, objects, model, config);
    preview_notebook->AddPage(preview3D, _("Preview"));

    preview2D = new Preview2D(preview_notebook, wxDefaultSize, objects, model, config);
    preview_notebook->AddPage(preview2D, _("Toolpaths"));

    previewDLP = new PreviewDLP(preview_notebook, wxDefaultSize, objects, model, config);
    preview_notebook->AddPage(previewDLP, _("DLP/SLA"));

    /*
    # Initialize 2D preview canvas
    $self->{canvas} = Slic3r::GUI::Plater::2D->new($self->{preview_notebook}, wxDefaultSize, $self->{objects}, $self->{model}, $self->{config});
    $self->{preview_notebook}->AddPage($self->{canvas}, '2D');
    $self->{canvas}->on_select_object($on_select_object);
    $self->{canvas}->on_double_click($on_double_click);
    $self->{canvas}->on_right_click(sub { $on_right_click->($self->{canvas}, @_); });
    $self->{canvas}->on_instances_moved($on_instances_moved);
    # Initialize 3D toolpaths preview
    $self->{preview3D_page_idx} = -1;
    if ($Slic3r::GUI::have_OpenGL) {
        $self->{preview3D} = Slic3r::GUI::Plater::3DPreview->new($self->{preview_notebook}, $self->{print});
        $self->{preview3D}->canvas->on_viewport_changed(sub {
            $self->{canvas3D}->set_viewport_from_scene($self->{preview3D}->canvas);
        });
        $self->{preview_notebook}->AddPage($self->{preview3D}, 'Preview');
        $self->{preview3D_page_idx} = $self->{preview_notebook}->GetPageCount-1;
    }
    
    # Initialize toolpaths preview
    $self->{toolpaths2D_page_idx} = -1;
    if ($Slic3r::GUI::have_OpenGL) {
        $self->{toolpaths2D} = Slic3r::GUI::Plater::2DToolpaths->new($self->{preview_notebook}, $self->{print});
        $self->{preview_notebook}->AddPage($self->{toolpaths2D}, 'Layers');
        $self->{toolpaths2D_page_idx} = $self->{preview_notebook}->GetPageCount-1;
    }
    
    EVT_NOTEBOOK_PAGE_CHANGED($self, $self->{preview_notebook}, sub {
        wxTheApp->CallAfter(sub {
            my $sel = $self->{preview_notebook}->GetSelection;
            if ($sel == $self->{preview3D_page_idx} || $sel == $self->{toolpaths2D_page_idx}) {
                if (!$Slic3r::GUI::Settings->{_}{background_processing} && !$self->{processed}) {
                    $self->statusbar->SetCancelCallback(sub {
                        $self->stop_background_process;
                        $self->statusbar->SetStatusText("Slicing cancelled");
                        $self->{preview_notebook}->SetSelection(0);

                    });
                    $self->start_background_process;
                } else {
                    $self->{preview3D}->load_print
                        if $sel == $self->{preview3D_page_idx};
                }
            }
        });
    });
    */
    wxStaticBoxSizer* object_info_sizer {nullptr};
    {
        auto* box {new wxStaticBox(this, wxID_ANY, _("Info"))};
        object_info_sizer = new wxStaticBoxSizer(box, wxVERTICAL);
        object_info_sizer->SetMinSize(wxSize(350, -1));
        {
            auto* sizer {new wxBoxSizer(wxHORIZONTAL)};
            object_info_sizer->Add(sizer, 0, wxEXPAND | wxBOTTOM, 5);
            auto* text  {new wxStaticText(this, wxID_ANY, _("Object:"), wxDefaultPosition, wxDefaultSize, wxALIGN_LEFT)};
            text->SetFont(ui_settings->small_font());
            sizer->Add(text, 0, wxALIGN_CENTER_VERTICAL);

            /* We supply a bogus width to wxChoice (sizer will override it and stretch 
             * the control anyway), because if we leave the default (-1) it will stretch
             * too much according to the contents, and this is bad with long file names.
             */
            this->object_info.choice = new wxChoice(this, wxID_ANY, wxDefaultPosition, wxSize(100, -1));
            this->object_info.choice->SetFont(ui_settings->small_font());
            sizer->Add(this->object_info.choice, 1, wxALIGN_CENTER_VERTICAL);

            // Select object on change.
            this->Bind(wxEVT_CHOICE, [this](wxCommandEvent& e) { this->select_object(this->object_info.choice->GetSelection()); this->refresh_canvases();});
                
        }
        
        auto* grid_sizer { new wxFlexGridSizer(3, 4, 5, 5)};
        grid_sizer->SetFlexibleDirection(wxHORIZONTAL);
        grid_sizer->AddGrowableCol(1, 1);
        grid_sizer->AddGrowableCol(3, 1);

        add_info_field(this, this->object_info.copies, _("Copies"), grid_sizer);
        add_info_field(this, this->object_info.size, _("Size"), grid_sizer);
        add_info_field(this, this->object_info.volume, _("Volume"), grid_sizer);
        add_info_field(this, this->object_info.facets, _("Facets"), grid_sizer);
        add_info_field(this, this->object_info.materials, _("Materials"), grid_sizer);
        {
            wxString name {"Manifold:"};
            auto* text {new wxStaticText(parent, wxID_ANY, name, wxDefaultPosition, wxDefaultSize, wxALIGN_LEFT)};
            text->SetFont(ui_settings->small_font());
            grid_sizer->Add(text, 0);

            this->object_info.manifold = new wxStaticText(parent, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, wxALIGN_LEFT);
            this->object_info.manifold->SetFont(ui_settings->small_font());

            this->object_info.manifold_warning_icon = new wxStaticBitmap(this, wxID_ANY, wxBitmap(var("error.png"), wxBITMAP_TYPE_PNG));
            this->object_info.manifold_warning_icon->Hide();

            auto* h_sizer {new wxBoxSizer(wxHORIZONTAL)};
            h_sizer->Add(this->object_info.manifold_warning_icon, 0);
            h_sizer->Add(this->object_info.manifold, 0);

            grid_sizer->Add(h_sizer, 0, wxEXPAND);
        }

        object_info_sizer->Add(grid_sizer, 0, wxEXPAND);
    }
    this->selection_changed();
    this->canvas2D->update_bed_size();

    // Toolbar
    this->build_toolbar();

    // Finally assemble the sizers into the display.
    
    // export/print/send/export buttons

    // right panel sizer
    auto* right_sizer {this->right_sizer};
    right_sizer->Add(this->_presets, 0, wxEXPAND | wxTOP, 10);
//    $right_sizer->Add($buttons_sizer, 0, wxEXPAND | wxBOTTOM, 5);
//    $right_sizer->Add($self->{settings_override_panel}, 1, wxEXPAND, 5);
    right_sizer->Add(object_info_sizer, 0, wxEXPAND, 0);
//    $right_sizer->Add($object_info_sizer, 0, wxEXPAND, 0);
//    $right_sizer->Add($print_info_sizer, 0, wxEXPAND, 0);
//    $right_sizer->Hide($print_info_sizer);

    auto hsizer {new wxBoxSizer(wxHORIZONTAL)};
    hsizer->Add(this->preview_notebook, 1, wxEXPAND | wxTOP, 1);
    hsizer->Add(right_sizer, 0, wxEXPAND | wxLEFT | wxRIGHT, 3);

    auto sizer {new wxBoxSizer(wxVERTICAL)};
    if (this->htoolbar != nullptr) sizer->Add(this->htoolbar, 0, wxEXPAND, 0);
    if (this->btoolbar != nullptr) sizer->Add(this->btoolbar, 0, wxEXPAND, 0);
    sizer->Add(hsizer, 1, wxEXPAND,0);

    sizer->SetSizeHints(this);
    this->SetSizer(sizer);

    // Initialize the toolbar
    this->selection_changed();

}
void Plater::add() {
    Log::info(LogChannel, L"Called Add function");

    auto& start_object_id = this->object_identifier;
    const auto& input_files{open_model(this, wxTheApp->GetTopWindow())};
    for (const auto& f : input_files) {
        Log::info(LogChannel, (wxString(L"Calling Load File for ") + f).ToStdWstring());
        this->load_file(f.ToStdString());
    }

    // abort if no objects actually added.
    if (start_object_id == this->object_identifier) return;

    // save the added objects
    auto new_model {this->model};

    // get newly added objects count
    auto new_objects_count = this->object_identifier - start_object_id;
    
    Slic3r::Log::info(LogChannel, (wxString("Obj id:") << object_identifier).ToStdWstring());
    for (auto i = start_object_id; i < new_objects_count + start_object_id; i++) {
        const auto& obj_idx {this->get_object_index(i)};
        new_model->add_object(*(this->model->objects.at(obj_idx)));
    }
    Slic3r::Log::info(LogChannel, (wxString("Obj id:") << object_identifier).ToStdWstring());

    // Prepare for undo
    //this->add_undo_operation("ADD", nullptr, new_model, start_object_id);
   

}

std::vector<int> Plater::load_file(const std::string file, const int obj_idx_to_load) {
    
    auto input_file {wxFileName(file)};
    ui_settings->skein_directory = input_file.GetPath();
    ui_settings->save_settings();
    
    Slic3r::Model model;
    bool valid_load {true};

    auto obj_idx {std::vector<int>()};
    auto progress_dialog {new wxProgressDialog(_(L"Loading…"), _(L"Processing input file…"), 100, this, 0)};
    progress_dialog->Pulse();
    //TODO: Add a std::wstring so we can handle non-roman characters as file names.
    try { 
        model = Slic3r::Model::read_from_file(file);
    } catch (std::runtime_error& e) {
        show_error(this, e.what());
        Slic3r::Log::error(LogChannel, LOG_WSTRING(file << " failed to load: " << e.what()));
        valid_load = false;
    }
    Slic3r::Log::info(LogChannel, LOG_WSTRING("load_valid is " << valid_load));

    if (valid_load) {
        if (model.looks_like_multipart_object()) {
            auto dialog {new wxMessageDialog(this, 
            _("This file contains several objects positioned at multiple heights. Instead of considering them as multiple objects, should I consider\n them this file as a single object having multiple parts?\n"), _("Multi-part object detected"), wxICON_WARNING | wxYES | wxNO)};
            if (dialog->ShowModal() == wxID_YES) {
                model.convert_multipart_object();
            }
        } 
        
        for (auto i = 0U; i < model.objects.size(); i++) {
            auto object {model.objects[i]};
            object->input_file = file;
            for (auto j = 0U; j < object->volumes.size(); j++) {
                auto volume {object->volumes.at(j)};
                volume->input_file = file;
                volume->input_file_obj_idx = i;
                volume->input_file_vol_idx = j;
            }
        }
        auto i {0U};
        if (obj_idx_to_load > 0) {
            Slic3r::Log::info(LogChannel, L"Loading model objects, obj_idx_to_load > 0");
            const size_t idx_load = obj_idx_to_load;
            if (idx_load >= model.objects.size()) return std::vector<int>();
            obj_idx = this->load_model_objects(model.objects.at(idx_load));
            i = idx_load;
        } else {
            Slic3r::Log::info(LogChannel, L"Loading model objects, obj_idx_to_load = 0");
            obj_idx = this->load_model_objects(model.objects);
            Slic3r::Log::info(LogChannel, LOG_WSTRING("obj_idx size: " << obj_idx.size()));
        }

        for (const auto &j : obj_idx) {
            this->objects.at(j).input_file = file;
            this->objects.at(j).input_file_obj_idx = i++;
        }
        GetFrame()->statusbar->SetStatusText(_("Loaded ") + input_file.GetName());

        if (this->scaled_down) {
            GetFrame()->statusbar->SetStatusText(_("Your object appears to be too large, so it was automatically scaled down to fit your print bed."));
        }
        if (this->outside_bounds) {
            GetFrame()->statusbar->SetStatusText(_("Some of your object(s) appear to be outside the print bed. Use the arrange button to correct this."));
        }
    }

    progress_dialog->Destroy();
    this->_redo = std::stack<UndoOperation>();
    return obj_idx;
}



std::vector<int> Plater::load_model_objects(ModelObject* model_object) { 
    ModelObjectPtrs tmp {model_object}; //  wrap in a std::vector
    return load_model_objects(tmp);
}
std::vector<int> Plater::load_model_objects(ModelObjectPtrs model_objects) {
    auto bed_center {this->bed_centerf()};

    auto bed_shape {Slic3r::Polygon::new_scale(this->config->get<ConfigOptionPoints>("bed_shape").values)};
    auto bed_size {bed_shape.bounding_box().size()};

    bool need_arrange {false};

    auto obj_idx {std::vector<int>()};
    Slic3r::Log::info(LogChannel, LOG_WSTRING("Objects: " << model_objects.size()));

    for (auto& obj : model_objects) {
        auto o {this->model->add_object(*obj)};
        o->repair();
        
        auto tmpobj {PlaterObject()};
        const auto objfile {wxFileName::FileName( obj->input_file )};
        tmpobj.name = (std::string() == obj->name ? wxString(obj->name) : objfile.GetName());
        tmpobj.identifier = (this->object_identifier)++;

        this->objects.push_back(tmpobj);
        obj_idx.push_back(this->objects.size() - 1);
        Slic3r::Log::info(LogChannel, LOG_WSTRING("Object array new size: " << this->objects.size()));
        Slic3r::Log::info(LogChannel, LOG_WSTRING("Instances: " << obj->instances.size()));

        if (obj->instances.size() == 0) {
            if (ui_settings->autocenter) {
                need_arrange = true;
                o->center_around_origin();

                o->add_instance();
                o->instances.back()->offset = this->bed_centerf();
            } else {
                need_arrange = false;
                if (ui_settings->autoalignz) {
                    o->align_to_ground();
                }
                o->add_instance();
            }
        } else {
            if (ui_settings->autoalignz) {
                o->align_to_ground();
            }
        }
        {
            // If the object is too large (more than 5x the bed) scale it down.
            auto size {o->bounding_box().size()};
            double ratio {0.0f};
            if (ratio > 5) {
                for (auto& instance : o->instances) {
                    instance->scaling_factor = (1.0f/ratio);
                    this->scaled_down = true;
                }
            }
        }

        { 
            // Provide a warning if downscaling by 5x still puts it over the bed size.

        }
        this->print->auto_assign_extruders(o);
        this->print->add_model_object(o);
    }
    for (const auto& i : obj_idx) { this->make_thumbnail(i); } 
    if (need_arrange) this->arrange();
    return obj_idx;
}

MainFrame* Plater::GetFrame() { return dynamic_cast<MainFrame*>(wxGetTopLevelParent(this)); }

int Plater::get_object_index(ObjIdx object_id) {
    for (size_t i = 0U; i < this->objects.size(); i++) {
        if (this->objects.at(i).identifier == object_id) return static_cast<int>(i);
    }
    return -1;
}

void Plater::make_thumbnail(size_t idx) {
    auto& plater_object {this->objects.at(idx)};
    if (threaded) {
        // spin off a thread to create the thumbnail and post an event when it is done.
    } else {
        plater_object.make_thumbnail(this->model, idx);
        this->on_thumbnail_made(idx);
    }
/*
    my $plater_object = $self->{objects}[$obj_idx];
    $plater_object->thumbnail(Slic3r::ExPolygon::Collection->new);
    my $cb = sub {
        $plater_object->make_thumbnail($self->{model}, $obj_idx);
        
        if ($Slic3r::have_threads) {
            Wx::PostEvent($self, Wx::PlThreadEvent->new(-1, $THUMBNAIL_DONE_EVENT, shared_clone([ $obj_idx ])));
            Slic3r::thread_cleanup();
            threads->exit;
        } else {
            $self->on_thumbnail_made($obj_idx);
        }
    };
    
    @_ = ();
    $Slic3r::have_threads
        ? threads->create(sub { $cb->(); Slic3r::thread_cleanup(); })->detach
        : $cb->();
}
*/
}

void Plater::on_thumbnail_made(size_t idx) {
    this->objects.at(idx).transform_thumbnail(this->model, idx);
    this->refresh_canvases();
}

void Plater::refresh_canvases() {
    if (this->canvas2D != nullptr)
        this->canvas2D->Refresh();
    if (this->canvas3D != nullptr)
        this->canvas3D->update();
    if (this->preview3D != nullptr)
        this->preview3D->reload_print();
    if (this->preview2D != nullptr)
        this->preview2D->reload_print();

    if (this->previewDLP != nullptr)
        this->previewDLP->reload_print();

}

void Plater::arrange() {
    // TODO pause background process
    const Slic3r::BoundingBoxf bb {Slic3r::BoundingBoxf(this->config->get<ConfigOptionPoints>("bed_shape").values)};
    if (this->objects.size() == 0U) { // abort
        GetFrame()->statusbar->SetStatusText(_("Nothing to arrange."));
        return; 
    }
    bool success {this->model->arrange_objects(this->config->config().min_object_distance(), &bb)};

    if (success) {
        GetFrame()->statusbar->SetStatusText(_("Objects were arranged."));
    } else {
        GetFrame()->statusbar->SetStatusText(_("Arrange failed."));
    }
    this->on_model_change(true);
}

void Plater::on_model_change(bool force_autocenter) {
    Log::info(LogChannel, L"Called on_modal_change");

    // reload the select submenu (if already initialized)
    {
        auto* menu = this->GetFrame()->plater_select_menu;

        if (menu != nullptr) {
            auto list = menu->GetMenuItems();
            for (auto it = list.begin();it!=list.end(); it++) { menu->Delete(*it); }
            for (const auto& obj : this->objects) {
                const auto idx {obj.identifier};
                auto name {wxString(obj.name)};
                auto inst_count = this->model->objects.at(idx)->instances.size();
                if (inst_count > 1) {
                    name << " (" << inst_count << "x)";
                }
                wxMenuItem* item {append_menu_item(menu, name, _("Select object."), 
                        [this,idx](wxCommandEvent& e) { this->select_object(idx); this->refresh_canvases(); }, 
                        wxID_ANY, "", "", wxITEM_CHECK)};
                if (obj.selected) item->Check(true);
            }
        }
    }

    if (force_autocenter || ui_settings->autocenter) {
        this->model->center_instances_around_point(this->bed_centerf());
    }
    this->refresh_canvases();
}

ObjRef Plater::selected_object() {
    Slic3r::Log::info(LogChannel, L"Calling selected_object()");
    auto it {this->objects.begin()};
    for (; it != this->objects.end(); it++)
        if (it->selected) return it;
    Slic3r::Log::info(LogChannel, L"No object selected.");
    return this->objects.end();
}

void Plater::object_settings_dialog() { object_settings_dialog(this->selected_object()); }
void Plater::object_settings_dialog(ObjIdx obj_idx) { object_settings_dialog(this->objects.begin() + obj_idx); }
void Plater::object_settings_dialog(ObjRef obj) {

}

void Plater::select_object(ObjRef obj) {
    for (auto& o : this->objects) {
        o.selected = false;
        o.selected_instance = -1;
    }
    // range check the iterator
    if (obj < this->objects.end() && obj >= this->objects.begin()) {
        obj->selected = true;
        obj->selected_instance = 0;
    }
    this->selection_changed(); // selection_changed(1) in perl
}

void Plater::select_object(ObjIdx obj_idx) {
    this->select_object(this->objects.begin() + obj_idx);
}

void Plater::select_object() {
    this->select_object(this->objects.end());
}

void Plater::selection_changed() {
    // Remove selection in 2D plater
    this->canvas2D->set_selected(-1, -1);
    this->canvas3D->selection_changed();

    auto obj = this->selected_object();
    bool have_sel {obj != this->objects.end()};
    auto* menu {this->GetFrame()->plater_select_menu};
    if (menu != nullptr) {
        for (auto item = menu->GetMenuItems().begin(); item != menu->GetMenuItems().end(); item++) {
            (*item)->Check(false);
        }
        if (have_sel) 
            menu->FindItemByPosition(obj->identifier)->Check(true);
    }

    if (this->htoolbar != nullptr) {
        for (auto tb : {TB_REMOVE, TB_MORE, TB_FEWER, TB_45CW, TB_45CCW, TB_SCALE, TB_SPLIT, TB_CUT, TB_LAYERS, TB_SETTINGS}) {
            this->htoolbar->EnableTool(tb, have_sel);
        }
    }
    /*
    
    my $method = $have_sel ? 'Enable' : 'Disable';
    $self->{"btn_$_"}->$method
        for grep $self->{"btn_$_"}, qw(remove increase decrease rotate45cw rotate45ccw changescale split cut layers settings);
    
    if ($self->{object_info_size}) { # have we already loaded the info pane?
        
        if ($have_sel) {
            my $model_object = $self->{model}->objects->[$obj_idx];
            $self->{object_info_choice}->SetSelection($obj_idx);
            $self->{object_info_copies}->SetLabel($model_object->instances_count);
            my $model_instance = $model_object->instances->[0];
            {
                my $size_string = sprintf "%.2f x %.2f x %.2f", @{$model_object->instance_bounding_box(0)->size};
                if ($model_instance->scaling_factor != 1) {
                    $size_string .= sprintf " (%s%%)", $model_instance->scaling_factor * 100;
                }
                $self->{object_info_size}->SetLabel($size_string);
            }
            $self->{object_info_materials}->SetLabel($model_object->materials_count);
            
            my $raw_mesh = $model_object->raw_mesh;
            $raw_mesh->repair;  # this calculates number_of_parts
            if (my $stats = $raw_mesh->stats) {
                $self->{object_info_volume}->SetLabel(sprintf('%.2f', $raw_mesh->volume * ($model_instance->scaling_factor**3)));
                $self->{object_info_facets}->SetLabel(sprintf('%d (%d shells)', $model_object->facets_count, $stats->{number_of_parts}));
                if (my $errors = sum(@$stats{qw(degenerate_facets edges_fixed facets_removed facets_added facets_reversed backwards_edges)})) {
                    $self->{object_info_manifold}->SetLabel(sprintf("Auto-repaired (%d errors)", $errors));
                    $self->{object_info_manifold_warning_icon}->Show;
                    
                    # we don't show normals_fixed because we never provide normals
	                # to admesh, so it generates normals for all facets
                    my $message = sprintf '%d degenerate facets, %d edges fixed, %d facets removed, %d facets added, %d facets reversed, %d backwards edges',
                        @$stats{qw(degenerate_facets edges_fixed facets_removed facets_added facets_reversed backwards_edges)};
                    $self->{object_info_manifold}->SetToolTipString($message);
                    $self->{object_info_manifold_warning_icon}->SetToolTipString($message);
                } else {
                    $self->{object_info_manifold}->SetLabel("Yes");
                }
            } else {
                $self->{object_info_facets}->SetLabel($object->facets);
            }
        } else {
            $self->{object_info_choice}->SetSelection(-1);
            $self->{"object_info_$_"}->SetLabel("") for qw(copies size volume facets materials manifold);
            $self->{object_info_manifold_warning_icon}->Hide;
            $self->{object_info_manifold}->SetToolTipString("");
        }
        $self->Layout;
    }
    # prepagate the event to the frame (a custom Wx event would be cleaner)
    $self->GetFrame->on_plater_selection_changed($have_sel);
*/
}

void Plater::build_toolbar() {
    wxToolTip::Enable(true);
    auto* toolbar = this->htoolbar = new wxToolBar(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTB_HORIZONTAL | wxTB_TEXT | wxBORDER_SIMPLE | wxTAB_TRAVERSAL);
    toolbar->AddTool(TB_ADD, _(L"Add…"), wxBitmap(var("brick_add.png"), wxBITMAP_TYPE_PNG));
    toolbar->AddTool(TB_REMOVE, _("Delete"), wxBitmap(var("brick_delete.png"), wxBITMAP_TYPE_PNG));
    toolbar->AddTool(TB_RESET, _("Delete All"), wxBitmap(var("cross.png"), wxBITMAP_TYPE_PNG));
    toolbar->AddTool(TB_ARRANGE, _("Arrange"), wxBitmap(var("bricks.png"), wxBITMAP_TYPE_PNG));
    toolbar->AddSeparator();
    toolbar->AddTool(TB_MORE, _("More"), wxBitmap(var("add.png"), wxBITMAP_TYPE_PNG));
    toolbar->AddTool(TB_FEWER, _("Fewer"), wxBitmap(var("delete.png"), wxBITMAP_TYPE_PNG));
    toolbar->AddSeparator();
    toolbar->AddTool(TB_45CCW, _(L"45° ccw"), wxBitmap(var("arrow_rotate_anticlockwise.png"), wxBITMAP_TYPE_PNG));
    toolbar->AddTool(TB_45CW, _(L"45° cw"), wxBitmap(var("arrow_rotate_clockwise.png"), wxBITMAP_TYPE_PNG));
    toolbar->AddTool(TB_SCALE, _(L"Scale…"), wxBitmap(var("arrow_out.png"), wxBITMAP_TYPE_PNG));
    toolbar->AddTool(TB_SPLIT, _("Split"), wxBitmap(var("shape_ungroup.png"), wxBITMAP_TYPE_PNG));
    toolbar->AddTool(TB_CUT, _(L"Cut…"), wxBitmap(var("package.png"), wxBITMAP_TYPE_PNG));
    toolbar->AddSeparator();
    toolbar->AddTool(TB_SETTINGS, _(L"Settings…"), wxBitmap(var("cog.png"), wxBITMAP_TYPE_PNG));
    toolbar->AddTool(TB_LAYERS, _(L"Layer heights…"), wxBitmap(var("variable_layer_height.png"), wxBITMAP_TYPE_PNG));

    toolbar->Realize();


    toolbar->Bind(wxEVT_TOOL, [this](wxCommandEvent &e) { this->add(); }, TB_ADD);
    toolbar->Bind(wxEVT_TOOL, [this](wxCommandEvent &e) { this->remove(); }, TB_REMOVE);
    toolbar->Bind(wxEVT_TOOL, [this](wxCommandEvent &e) { this->reset(); }, TB_RESET);
    toolbar->Bind(wxEVT_TOOL, [this](wxCommandEvent &e) { this->arrange(); }, TB_ARRANGE);
    toolbar->Bind(wxEVT_TOOL, [this](wxCommandEvent &e) { this->increase(); }, TB_MORE);
    toolbar->Bind(wxEVT_TOOL, [this](wxCommandEvent &e) { this->decrease(); }, TB_FEWER);
    toolbar->Bind(wxEVT_TOOL, [this](wxCommandEvent &e) { this->rotate(-45); }, TB_45CW);
    toolbar->Bind(wxEVT_TOOL, [this](wxCommandEvent &e) { this->rotate(45); }, TB_45CCW);
    toolbar->Bind(wxEVT_TOOL, [this](wxCommandEvent &e) { this->changescale(); }, TB_SCALE);
    toolbar->Bind(wxEVT_TOOL, [this](wxCommandEvent &e) { this->split_object(); }, TB_SPLIT);
    toolbar->Bind(wxEVT_TOOL, [this](wxCommandEvent &e) { this->object_cut_dialog(); }, TB_CUT);
    toolbar->Bind(wxEVT_TOOL, [this](wxCommandEvent &e) { this->object_layers_dialog(); }, TB_LAYERS);
    toolbar->Bind(wxEVT_TOOL, [this](wxCommandEvent &e) { this->object_settings_dialog(); }, TB_SETTINGS);
}

void Plater::remove() {
    this->remove(-1, false);
}

void Plater::remove(int obj_idx, bool dont_push) {
    
    // TODO: $self->stop_background_process;
    
    // Prevent toolpaths preview from rendering while we modify the Print object
    if (this->preview2D != nullptr) 
        this->preview2D->enabled(false);

    if (this->preview3D != nullptr) 
        this->preview3D->enabled(false);

    if (this->previewDLP != nullptr) 
        this->previewDLP->enabled(false);
    
    ObjRef obj_ref;
    // if no object index is supplied or an invalid one is supplied, remove the selected one
    if (obj_idx < 0 || obj_idx >= this->objects.size()) {
        obj_ref = this->selected_object();
    } else { // otherwise 
        obj_ref = this->objects.begin() + obj_idx;
    }
    std::vector<PlaterObject>::const_iterator const_ref = obj_ref;

    if (obj_ref >= this->objects.end()) return; // do nothing, nothing was selected.

    Slic3r::Log::info(LogChannel, "Assigned obj_ref");
    // Save the object identifier and copy the object for undo/redo operations.
    auto object_id { obj_ref->identifier };
    auto new_model { Slic3r::Model() };
    new_model.add_object(*(this->model->objects.at(obj_ref->identifier)));
   
    Slic3r::Log::info(LogChannel, "Assigned obj_ref");
    try {
        this->model->delete_object(obj_ref->identifier);
    } catch (out_of_range &ex) {
        Slic3r::Log::error(LogChannel, LOG_WSTRING("Failed to delete object " << obj_ref->identifier << " from Model."));
    }
    try {
        this->print->delete_object(obj_ref->identifier);
    } catch (out_of_range &ex) {
        Slic3r::Log::error(LogChannel, LOG_WSTRING("Failed to delete object " << obj_ref->identifier << " from Print."));
    }

    this->objects.erase(const_ref);
    int i = 0;
    for (auto o : this->objects) { o.identifier = i++; } // fix identifiers
    this->object_identifier = this->objects.size();

    this->object_list_changed();
    
    this->select_object();

    this->on_model_change();

    if (!dont_push) {
        Slic3r::Log::info(LogChannel, "Push to undo stack.");
        this->add_undo_operation(UndoCmd::Remove, object_id, new_model);
        Slic3r::Log::info(LogChannel, "Pushed to undo stack.");
    }
}

void Plater::reset(bool dont_push) {
    // TODO: $self->stop_background_process;
    
    // Prevent toolpaths preview from rendering while we modify the Print object
    if (this->preview2D != nullptr) 
        this->preview2D->enabled(false);

    if (this->preview3D != nullptr) 
        this->preview3D->enabled(false);

    if (this->previewDLP != nullptr) 
        this->previewDLP->enabled(false);

    if (!dont_push) {
        Slic3r::Model current_model {*(this->model)};
        std::vector<int> tmp_ids;
        for (const auto& obj : this->objects) {
            tmp_ids.push_back(obj.identifier);
        }
        this->add_undo_operation(UndoCmd::Reset, tmp_ids, current_model);
    }
   
    this->objects.clear();
    this->object_identifier = this->objects.size();

    this->model->clear_objects();
    this->print->clear_objects();

    this->object_list_changed();
    this->select_object();

    this->on_model_change();
}

void Plater::increase(size_t copies, bool dont_push) {
    auto obj {this->selected_object()};
    if (obj == this->objects.end()) return; // do nothing; nothing is selected.
    
    this->stop_background_process();

    auto* model_object {this->model->objects.at(obj->identifier)};
    ModelInstance* instance {model_object->instances.back()};

    for (size_t i = 1; i <= copies; i++) {
        instance = model_object->add_instance(*instance);
        instance->offset.x += 10;
        instance->offset.y += 10;
        this->print->objects.at(obj->identifier)->add_copy(instance->offset);
    }

    if (!dont_push) {
        this->add_undo_operation(UndoCmd::Increase, obj->identifier, copies);
    }

    if(ui_settings->autocenter) {
        this->arrange();
    } else {
        this->on_model_change();
    }
} 

void Plater::decrease(size_t copies, bool dont_push) {
    auto obj {this->selected_object()};
    if (obj == this->objects.end()) return; // do nothing; nothing is selected.
    
    this->stop_background_process();
    auto* model_object {this->model->objects.at(obj->identifier)};
    if (model_object->instances.size() > copies) {
        for (size_t i = 1; i <= copies; i++) {
            model_object->delete_last_instance();
            this->print->objects.at(obj->identifier)->delete_last_copy();
        }
        if (!dont_push) {
            this->add_undo_operation(UndoCmd::Decrease, obj->identifier, copies);
        }
    } else {
        this->remove();
    }
    this->on_model_change();
} 

void Plater::rotate(Axis axis, bool dont_push) {
    ObjRef obj {this->selected_object()};
    if (obj == this->objects.end()) return;
    double angle {0.0};

    auto* model_object {this->model->objects.at(obj->identifier)};
    auto model_instance {model_object->instances.begin()};

    // pop a menu to get the angle
    auto* pick = new AnglePicker<1000>(this, "Set Angle", angle);
    if (pick->ShowModal() == wxID_OK) {
        angle = pick->angle();
        pick->Destroy(); // cleanup afterwards.
        this->rotate(angle, axis, dont_push);
    } else {
        pick->Destroy(); // cleanup afterwards.
    }
}

void Plater::rotate(double angle, Axis axis, bool dont_push) {
    ObjRef obj {this->selected_object()};
    if (obj == this->objects.end()) return;

    auto* model_object {this->model->objects.at(obj->identifier)};
    auto* model_instance {model_object->instances.front()};

    if(obj->thumbnail.expolygons.size() == 0) { return; }

    if (axis == Z) {
        for (auto* instance : model_object->instances)
            instance->rotation += Geometry::deg2rad(angle);
        obj->transform_thumbnail(this->model, obj->identifier);
    } else {
        model_object->transform_by_instance(*model_instance, true);
        model_object->rotate(Geometry::deg2rad(angle), axis);

        // realign object to Z=0
        model_object->center_around_origin();
        this->make_thumbnail(obj->identifier);
    }

    model_object->update_bounding_box();

    this->print->add_model_object(model_object, obj->identifier);

    if (!dont_push) {
        add_undo_operation(UndoCmd::Rotate, obj->identifier, angle, axis);
    }

    this->selection_changed();
    this->on_model_change();

} 

void Plater::split_object() {
    //TODO
} 

void Plater::changescale() {
    //TODO
}

void Plater::object_cut_dialog() {
    //TODO
    ObjRef obj {this->selected_object()};
    if (obj == this->objects.end()) return;

    auto* model_object {this->model->objects.at(obj->identifier)};
    auto cut_dialog = new ObjectCutDialog(nullptr, model_object);
    cut_dialog->ShowModal();
    cut_dialog->Destroy();
}

void Plater::object_layers_dialog() {
    //TODO
}

void Plater::add_undo_operation(UndoCmd cmd, std::vector<int>& obj_ids, Slic3r::Model& model) {
    //TODO
}

void Plater::add_undo_operation(UndoCmd cmd, int obj_id, Slic3r::Model& model) {
    std::vector<int> tmp {obj_id};
    add_undo_operation(cmd, tmp, model);
}

void Plater::add_undo_operation(UndoCmd cmd, int obj_id, size_t copies) {
}

void Plater::add_undo_operation(UndoCmd cmd, int obj_id, double angle, Axis axis) {
}

void Plater::object_list_changed() {
    //TODO
}

void Plater::stop_background_process() {
    //TODO
}

void Plater::start_background_process() {
    //TODO
}

void Plater::pause_background_process() {
    //TODO
}
void Plater::resume_background_process() {
    //TODO
}

wxMenu* Plater::object_menu() {
    auto* frame {this->GetFrame()};
    auto* menu {new wxMenu()};

    append_menu_item(menu, _("Delete"), _("Remove the selected object."), [=](wxCommandEvent& e) { this->remove();}, wxID_ANY, "brick_delete.png", "Ctrl+Del");
    append_menu_item(menu, _("Increase copies"), _("Place one more copy of the selected object."), [=](wxCommandEvent& e) { this->increase();}, wxID_ANY, "add.png", "Ctrl++");
    append_menu_item(menu, _("Decrease copies"), _("Remove one copy of the selected object."), [=](wxCommandEvent& e) { this->decrease();}, wxID_ANY, "delete.png", "Ctrl+-");
    append_menu_item(menu, _(L"Set number of copies…"), _("Change the number of copies of the selected object."), [=](wxCommandEvent& e) { this->set_number_of_copies();}, wxID_ANY, "textfield.png");
    menu->AppendSeparator();
    append_menu_item(menu, _(L"Move to bed center"), _(L"Center object around bed center."), [=](wxCommandEvent& e) { this->center_selected_object_on_bed();}, wxID_ANY, "arrow_in.png");
    append_menu_item(menu, _(L"Rotate 45° clockwise"), _(L"Rotate the selected object by 45° clockwise."), [=](wxCommandEvent& e) { this->rotate(45);}, wxID_ANY, "arrow_rotate_clockwise.png");
    append_menu_item(menu, _(L"Rotate 45° counter-clockwise"), _(L"Rotate the selected object by 45° counter-clockwise."), [=](wxCommandEvent& e) { this->rotate(-45);}, wxID_ANY, "arrow_rotate_anticlockwise.png");
    
    {
        auto* rotateMenu {new wxMenu};

        append_menu_item(rotateMenu, _(L"Around X axis…"), _("Rotate the selected object by an arbitrary angle around X axis."), [this](wxCommandEvent& e) { this->rotate(X); }, wxID_ANY, "bullet_red.png");
        append_menu_item(rotateMenu, _(L"Around Y axis…"), _("Rotate the selected object by an arbitrary angle around Y axis."), [this](wxCommandEvent& e) { this->rotate(Y); }, wxID_ANY, "bullet_green.png");
        append_menu_item(rotateMenu, _(L"Around Z axis…"), _("Rotate the selected object by an arbitrary angle around Z axis."), [this](wxCommandEvent& e) { this->rotate(Z); }, wxID_ANY, "bullet_blue.png");

        append_submenu(menu, _("Rotate"), _("Rotate the selected object by an arbitrary angle"), rotateMenu, wxID_ANY, "textfield.png");
    }
    /*
    
    {
        my $mirrorMenu = Wx::Menu->new;
        wxTheApp->append_menu_item($mirrorMenu, "Along X axis…", 'Mirror the selected object along the X axis', sub {
            $self->mirror(X);
        }, undef, 'bullet_red.png');
        wxTheApp->append_menu_item($mirrorMenu, "Along Y axis…", 'Mirror the selected object along the Y axis', sub {
            $self->mirror(Y);
        }, undef, 'bullet_green.png');
        wxTheApp->append_menu_item($mirrorMenu, "Along Z axis…", 'Mirror the selected object along the Z axis', sub {
            $self->mirror(Z);
        }, undef, 'bullet_blue.png');
        wxTheApp->append_submenu($menu, "Mirror", 'Mirror the selected object', $mirrorMenu, undef, 'shape_flip_horizontal.png');
    }
    
    {
        my $scaleMenu = Wx::Menu->new;
        wxTheApp->append_menu_item($scaleMenu, "Uniformly…", 'Scale the selected object along the XYZ axes', sub {
            $self->changescale(undef);
        });
        wxTheApp->append_menu_item($scaleMenu, "Along X axis…", 'Scale the selected object along the X axis', sub {
            $self->changescale(X);
        }, undef, 'bullet_red.png');
        wxTheApp->append_menu_item($scaleMenu, "Along Y axis…", 'Scale the selected object along the Y axis', sub {
            $self->changescale(Y);
        }, undef, 'bullet_green.png');
        wxTheApp->append_menu_item($scaleMenu, "Along Z axis…", 'Scale the selected object along the Z axis', sub {
            $self->changescale(Z);
        }, undef, 'bullet_blue.png');
        wxTheApp->append_submenu($menu, "Scale", 'Scale the selected object by a given factor', $scaleMenu, undef, 'arrow_out.png');
    }
    
    {
        my $scaleToSizeMenu = Wx::Menu->new;
        wxTheApp->append_menu_item($scaleToSizeMenu, "Uniformly…", 'Scale the selected object along the XYZ axes', sub {
            $self->changescale(undef, 1);
        });
        wxTheApp->append_menu_item($scaleToSizeMenu, "Along X axis…", 'Scale the selected object along the X axis', sub {
            $self->changescale(X, 1);
        }, undef, 'bullet_red.png');
        wxTheApp->append_menu_item($scaleToSizeMenu, "Along Y axis…", 'Scale the selected object along the Y axis', sub {
            $self->changescale(Y, 1);
        }, undef, 'bullet_green.png');
        wxTheApp->append_menu_item($scaleToSizeMenu, "Along Z axis…", 'Scale the selected object along the Z axis', sub {
            $self->changescale(Z, 1);
        }, undef, 'bullet_blue.png');
        wxTheApp->append_submenu($menu, "Scale to size", 'Scale the selected object to match a given size', $scaleToSizeMenu, undef, 'arrow_out.png');
    }
    
    wxTheApp->append_menu_item($menu, "Split", 'Split the selected object into individual parts', sub {
        $self->split_object;
    }, undef, 'shape_ungroup.png');
    wxTheApp->append_menu_item($menu, "Cut…", 'Open the 3D cutting tool', sub {
        $self->object_cut_dialog;
    }, undef, 'package.png');
    wxTheApp->append_menu_item($menu, "Layer heights…", 'Open the dynamic layer height control', sub {
        $self->object_layers_dialog;
    }, undef, 'variable_layer_height.png');
    $menu->AppendSeparator();
    wxTheApp->append_menu_item($menu, "Settings…", 'Open the object editor dialog', sub {
        $self->object_settings_dialog;
    }, undef, 'cog.png');
    $menu->AppendSeparator();
    wxTheApp->append_menu_item($menu, "Reload from Disk", 'Reload the selected file from Disk', sub {
        $self->reload_from_disk;
    }, undef, 'arrow_refresh.png');
    wxTheApp->append_menu_item($menu, "Export object as STL…", 'Export this single object as STL file', sub {
        $self->export_object_stl;
    }, undef, 'brick_go.png');
    wxTheApp->append_menu_item($menu, "Export object and modifiers as AMF…", 'Export this single object and all associated modifiers as AMF file', sub {
        $self->export_object_amf;
    }, undef, 'brick_go.png');
    wxTheApp->append_menu_item($menu, "Export object and modifiers as 3MF…", 'Export this single object and all associated modifiers as 3MF file', sub {
            $self->export_object_tmf;
    }, undef, 'brick_go.png');
    
    return $menu;
}
*/
    return menu;
}

void Plater::set_number_of_copies() {
    this->pause_background_process();

    ObjRef obj {this->selected_object()};
    if (obj == this->objects.end()) return;
    auto* model_object { this->model->objects.at(obj->identifier) };

    long copies = -1;
    copies = wxGetNumberFromUser("", _("Enter the number of copies of the selected object:"), _("Copies"), model_object->instances.size(), 0, 1000, this);
    if (copies < 0) return;
    long instance_count = 0;
    if (model_object->instances.size() <= LONG_MAX) {
        instance_count = static_cast<long>(model_object->instances.size());
    } else {
        instance_count = LONG_MAX;
    }
    long diff {copies - instance_count };

    if      (diff == 0) { this->resume_background_process(); }
    else if (diff > 0)  { this->increase(diff); }
    else if (diff < 0)  { this->decrease(-diff); }
}
void Plater::center_selected_object_on_bed() {
    ObjRef obj {this->selected_object()};
    
    if (obj == this->objects.end()) return;
    auto* model_object { this->model->objects.at(obj->identifier) };
    auto bb {model_object->bounding_box()};
    auto size {bb.size()};

    auto vector { Slic3r::Pointf(
            this->bed_centerf().x - bb.min.x - size.x/2.0,
            this->bed_centerf().y - bb.min.y - size.y/2.0)};
    for (auto* inst : model_object->instances) {
        inst->offset.translate(vector);
    }

    this->refresh_canvases();

}

void Plater::show_preset_editor(preset_t group, unsigned int idx) {
    
}


void Plater::load_presets() {
    this->_presets->load();
}

}} // Namespace Slic3r::GUI

