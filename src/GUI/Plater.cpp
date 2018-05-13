#include <memory>
#include <wx/progdlg.h>
#include <wx/window.h> 


#include "Plater.hpp"
#include "ProgressStatusBar.hpp"
#include "Log.hpp"
#include "MainFrame.hpp"
#include "BoundingBox.hpp"

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

Plater::Plater(wxWindow* parent, const wxString& title, std::shared_ptr<Settings> _settings) : 
    wxPanel(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL, title), settings(_settings)
{

    // Set callback for status event for worker threads
    /*
    this->print->set_status_cb([=](std::string percent percent, std::wstring message) {
        wxPostEvent(this, new wxPlThreadEvent(-1, PROGRESS_BAR_EVENT, 
    });
    */

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
    canvas2D = new Plate2D(preview_notebook, wxDefaultSize, objects, model, config, settings);
    preview_notebook->AddPage(canvas2D, _("2D"));

    canvas2D->on_select_object = std::function<void (ObjIdx obj_idx)>(on_select_object);
    canvas2D->on_double_click = std::function<void ()>(on_double_click);
    canvas2D->on_right_click = std::function<void (const wxPoint& pos)>([=](const wxPoint& pos){ on_right_click(canvas2D, pos); });
    canvas2D->on_instances_moved = std::function<void ()>(on_instances_moved);


    canvas3D = new Plate3D(preview_notebook, wxDefaultSize, objects, model, config, settings);
    preview_notebook->AddPage(canvas3D, _("3D"));

    preview3D = new Preview3D(preview_notebook, wxDefaultSize, objects, model, config, settings);
    preview_notebook->AddPage(preview3D, _("Preview"));

    preview2D = new Preview2D(preview_notebook, wxDefaultSize, objects, model, config, settings);
    preview_notebook->AddPage(preview2D, _("Toolpaths"));

    previewDLP = new PreviewDLP(preview_notebook, wxDefaultSize, objects, model, config, settings);
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


    this->canvas2D->update_bed_size();

    // Toolbar
    this->build_toolbar();

    // Finally assemble the sizers into the display.
    
    // export/print/send/export buttons

    // right panel sizer
    auto right_sizer = this->right_sizer;
//    $right_sizer->Add($presets, 0, wxEXPAND | wxTOP, 10) if defined $presets;
//    $right_sizer->Add($buttons_sizer, 0, wxEXPAND | wxBOTTOM, 5);
//    $right_sizer->Add($self->{settings_override_panel}, 1, wxEXPAND, 5);
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

}
void Plater::add() {
    Log::info(LogChannel, L"Called Add function");

    auto& start_object_id = this->object_identifier;
    const auto& input_files{open_model(this, *(this->settings), wxTheApp->GetTopWindow())};
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
    settings->skein_directory = input_file.GetPath();
    settings->save_settings();
    
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
    this->redo = std::stack<UndoOperation>();
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
            if (settings->autocenter) {
                need_arrange = true;
                o->center_around_origin();

                o->add_instance();
                o->instances.back()->offset = this->bed_centerf();
            } else {
                need_arrange = false;
                if (settings->autoalignz) {
                    o->align_to_ground();
                }
                o->add_instance();
            }
        } else {
            if (settings->autoalignz) {
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
    bool success {this->model->arrange_objects(this->config->min_object_distance(), &bb)};

    if (success) {
        GetFrame()->statusbar->SetStatusText(_("Objects were arranged."));
    } else {
        GetFrame()->statusbar->SetStatusText(_("Arrange failed."));
    }
    this->on_model_change(true);
}

void Plater::on_model_change(bool force_autocenter) {
    if (force_autocenter || settings->autocenter) {
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

wxMenu* Plater::object_menu() {
    return new wxMenu();
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

    auto obj = this->selected_object();
    bool have_sel {obj != this->objects.end()};
    /*
        if (my $menu = $self->GetFrame->{plater_select_menu}) {
        $_->Check(0) for $menu->GetMenuItems;
        if ($have_sel) {
            $menu->FindItemByPosition($obj_idx)->Check(1);
        }
    }
    
    my $method = $have_sel ? 'Enable' : 'Disable';
    $self->{"btn_$_"}->$method
        for grep $self->{"btn_$_"}, qw(remove increase decrease rotate45cw rotate45ccw changescale split cut layers settings);
    
    if ($self->{htoolbar}) {
        $self->{htoolbar}->EnableTool($_, $have_sel)
            for (TB_REMOVE, TB_MORE, TB_FEWER, TB_45CW, TB_45CCW, TB_SCALE, TB_SPLIT, TB_CUT, TB_LAYERS, TB_SETTINGS);
    }
    
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
    } catch (InvalidObjectException& ex) {
        Slic3r::Log::error(LogChannel, LOG_WSTRING("Failed to delete object " << obj_ref->identifier << " from Model."));
    }
    try {
        this->print->delete_object(obj_ref->identifier);
    } catch (InvalidObjectException& ex) {
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

    if(settings->autocenter) {
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

void Plater::rotate(double angle) {
    //TODO
} 

void Plater::split_object() {
    //TODO
} 

void Plater::changescale() {
    //TODO
}

void Plater::object_cut_dialog() {
    //TODO
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

}} // Namespace Slic3r::GUI

