# Configuration of mesh modifiers and their parameters.
# This panel is inserted into ObjectSettingsDialog.

package Slic3r::GUI::Plater::ObjectPartsPanel;
use strict;
use warnings;
use utf8;

use File::Basename qw(basename);
use Wx qw(:misc :sizer :treectrl :button wxTAB_TRAVERSAL wxSUNKEN_BORDER wxBITMAP_TYPE_PNG wxID_CANCEL
    wxTheApp);
use List::Util qw(max);
use Wx::Event qw(EVT_BUTTON EVT_TREE_ITEM_COLLAPSING EVT_TREE_SEL_CHANGED EVT_TREE_ITEM_RIGHT_CLICK);
use Slic3r::Geometry qw(X Y Z MIN MAX scale unscale deg2rad rad2deg);
use base 'Wx::Panel';

use constant ICON_OBJECT        => 0;
use constant ICON_SOLIDMESH     => 1;
use constant ICON_MODIFIERMESH  => 2;

sub new {
    my $class = shift;
    my ($parent, %params) = @_;
    my $self = $class->SUPER::new($parent, -1, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL);
    
    my $object = $self->{model_object} = $params{model_object};
    
    # create TreeCtrl
    my $tree = $self->{tree} = Wx::TreeCtrl->new($self, -1, wxDefaultPosition, [300, 100], 
        wxTR_NO_BUTTONS | wxSUNKEN_BORDER | wxTR_HAS_VARIABLE_ROW_HEIGHT
        | wxTR_SINGLE | wxTR_NO_BUTTONS);
    {
        $self->{tree_icons} = Wx::ImageList->new(16, 16, 1);
        $tree->AssignImageList($self->{tree_icons});
        $self->{tree_icons}->Add(Wx::Bitmap->new($Slic3r::var->("brick.png"), wxBITMAP_TYPE_PNG));     # ICON_OBJECT
        $self->{tree_icons}->Add(Wx::Bitmap->new($Slic3r::var->("package.png"), wxBITMAP_TYPE_PNG));   # ICON_SOLIDMESH
        $self->{tree_icons}->Add(Wx::Bitmap->new($Slic3r::var->("plugin.png"), wxBITMAP_TYPE_PNG));    # ICON_MODIFIERMESH
        
        my $rootId = $tree->AddRoot("Object", ICON_OBJECT);
        $tree->SetPlData($rootId, { type => 'object' });
    }
    
    # buttons
    $self->{btn_load_part} = Wx::Button->new($self, -1, "Load part…", wxDefaultPosition, wxDefaultSize, wxBU_LEFT);
    $self->{btn_load_modifier} = Wx::Button->new($self, -1, "Load modifier…", wxDefaultPosition, wxDefaultSize, wxBU_LEFT);
    $self->{btn_load_lambda_modifier} = Wx::Button->new($self, -1, "Create modifier…", wxDefaultPosition, wxDefaultSize, wxBU_LEFT);
    $self->{btn_delete} = Wx::Button->new($self, -1, "Delete part", wxDefaultPosition, wxDefaultSize, wxBU_LEFT);
    if ($Slic3r::GUI::have_button_icons) {
        $self->{btn_load_part}->SetBitmap(Wx::Bitmap->new($Slic3r::var->("brick_add.png"), wxBITMAP_TYPE_PNG));
        $self->{btn_load_modifier}->SetBitmap(Wx::Bitmap->new($Slic3r::var->("brick_add.png"), wxBITMAP_TYPE_PNG));
        $self->{btn_load_lambda_modifier}->SetBitmap(Wx::Bitmap->new($Slic3r::var->("brick_add.png"), wxBITMAP_TYPE_PNG));
        $self->{btn_delete}->SetBitmap(Wx::Bitmap->new($Slic3r::var->("brick_delete.png"), wxBITMAP_TYPE_PNG));
    }
    
    # buttons sizer
    my $buttons_sizer = Wx::BoxSizer->new(wxHORIZONTAL);
    $buttons_sizer->Add($self->{btn_load_part}, 0);
    $buttons_sizer->Add($self->{btn_load_modifier}, 0);
    $buttons_sizer->Add($self->{btn_load_lambda_modifier}, 0);
    $buttons_sizer->Add($self->{btn_delete}, 0);
    $self->{btn_load_part}->SetFont($Slic3r::GUI::small_font);
    $self->{btn_load_modifier}->SetFont($Slic3r::GUI::small_font);
    $self->{btn_load_lambda_modifier}->SetFont($Slic3r::GUI::small_font);
    $self->{btn_delete}->SetFont($Slic3r::GUI::small_font);
    
    # part settings panel
    $self->{settings_panel} = Slic3r::GUI::Plater::OverrideSettingsPanel->new($self, on_change => sub { $self->{part_settings_changed} = 1; });
    my $settings_sizer = Wx::StaticBoxSizer->new($self->{staticbox} = Wx::StaticBox->new($self, -1, "Part Settings"), wxVERTICAL);
    $settings_sizer->Add($self->{settings_panel}, 1, wxEXPAND | wxALL, 0);

    my $optgroup_movers;
	# initialize the movement target before it's used.
	# on Windows this causes a segfault due to calling distance_to() 
	# on the object.
	$self->{move_target} =  Slic3r::Pointf3->new;
    $optgroup_movers = $self->{optgroup_movers} = Slic3r::GUI::OptionsGroup->new(
        parent      => $self,
        title       => 'Move',
        on_change   => sub {
            my ($opt_id) = @_;
            # There seems to be an issue with wxWidgets 3.0.2/3.0.3, where the slider
            # genates tens of events for a single value change.
            # Only trigger the recalculation if the value changes
            # or a live preview was activated and the mesh cut is not valid yet.
            my $new = Slic3r::Pointf3->new(map $optgroup_movers->get_value($_), qw(x y z));
            if ($self->{move_target}->distance_to($new) > 0) {
                $self->{move_target} = $new;
                wxTheApp->CallAfter(sub {
                    $self->_update;
                });
            }
        },
        label_width  => 20,
    );
    $optgroup_movers->append_single_option_line(Slic3r::GUI::OptionsGroup::Option->new(
        opt_id      => 'x',
        type        => 'slider',
        label       => 'X',
        default     => 0,
        full_width  => 1,
    ));
    $optgroup_movers->append_single_option_line(Slic3r::GUI::OptionsGroup::Option->new(
        opt_id      => 'y',
        type        => 'slider',
        label       => 'Y',
        default     => 0,
        full_width  => 1,
    ));
    $optgroup_movers->append_single_option_line(Slic3r::GUI::OptionsGroup::Option->new(
        opt_id      => 'z',
        type        => 'slider',
        label       => 'Z',
        default     => 0,
        full_width  => 1,
    ));
 
    # left pane with tree
    my $left_sizer = $self->{left_sizer} = Wx::BoxSizer->new(wxVERTICAL);
    $left_sizer->Add($tree, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 10);
    $left_sizer->Add($buttons_sizer, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 10);
    $left_sizer->Add($settings_sizer, 1, wxEXPAND | wxALL, 0);
    $left_sizer->Add($optgroup_movers->sizer, 0, wxEXPAND | wxBOTTOM | wxLEFT | wxRIGHT, 10);
    
    # right pane with preview canvas
    my $canvas;
    if ($Slic3r::GUI::have_OpenGL) {
        $canvas = $self->{canvas} = Slic3r::GUI::3DScene->new($self);
        $canvas->enable_picking(1);
        $canvas->select_by('volume');
        
        $canvas->on_select(sub {
            my ($volume_idx) = @_;
            
            # convert scene volume to model object volume
            $self->reload_tree($canvas->volume_idx($volume_idx));
        });
        
        $canvas->load_object($self->{model_object}, undef, [0]);
        $canvas->set_auto_bed_shape;
        $canvas->SetSize([500,700]);
        $canvas->zoom_to_volumes;
    }
    
    $self->{sizer} = Wx::BoxSizer->new(wxHORIZONTAL);
    $self->{sizer}->Add($left_sizer, 0, wxEXPAND | wxALL, 0);
    $self->{sizer}->Add($canvas, 1, wxEXPAND | wxALL, 0) if $canvas;
    
    $self->SetSizer($self->{sizer});
    $self->{sizer}->SetSizeHints($self);
    
    # attach events
    EVT_TREE_ITEM_COLLAPSING($self, $tree, sub {
        my ($self, $event) = @_;
        $event->Veto;
    });
    EVT_TREE_SEL_CHANGED($self, $tree, sub {
        my ($self, $event) = @_;
        return if $self->{disable_tree_sel_changed_event};
        $self->selection_changed;
    });
    EVT_TREE_ITEM_RIGHT_CLICK($self, $tree, sub { 
        my ($self, $event) = @_;
        my $item = $event->GetItem;
        my $frame = $self->GetFrame;
        my $menu = Wx::Menu->new;
        
        {
            my $scaleMenu = Wx::Menu->new;
            wxTheApp->append_menu_item($scaleMenu, "Uniformly… ", 'Scale the selected object along the XYZ axes',
                sub { $self->changescale(undef, 0) });
            wxTheApp->append_menu_item($scaleMenu, "Along X axis…", 'Scale the selected object along the X axis',
                sub { $self->changescale(X, 0) }, undef, 'bullet_red.png');
            wxTheApp->append_menu_item($scaleMenu, "Along Y axis…", 'Scale the selected object along the Y axis',
                sub { $self->changescale(Y, 0) }, undef, 'bullet_green.png');
            wxTheApp->append_menu_item($scaleMenu, "Along Z axis…", 'Scale the selected object along the Z axis',
                sub { $self->changescale(Z, 0) }, undef, 'bullet_blue.png');
            wxTheApp->append_submenu($menu, "Scale", 'Scale the selected object by a given factor',
                $scaleMenu, undef, 'arrow_out.png');
        }
        {
            my $scaleToSizeMenu = Wx::Menu->new;
            wxTheApp->append_menu_item($scaleToSizeMenu, "Uniformly… ", 'Scale the selected object along the XYZ axes',
                sub { $self->changescale(undef, 1) });
            wxTheApp->append_menu_item($scaleToSizeMenu, "Along X axis…", 'Scale the selected object along the X axis',
                sub { $self->changescale(X, 1) }, undef, 'bullet_red.png');
            wxTheApp->append_menu_item($scaleToSizeMenu, "Along Y axis…", 'Scale the selected object along the Y axis',
                sub { $self->changescale(Y, 1) }, undef, 'bullet_green.png');
            wxTheApp->append_menu_item($scaleToSizeMenu, "Along Z axis…", 'Scale the selected object along the Z axis',
                sub { $self->changescale(Z, 1) }, undef, 'bullet_blue.png');
            wxTheApp->append_submenu($menu, "Scale to size", 'Scale the selected object to match a given size',
                $scaleToSizeMenu, undef, 'arrow_out.png');
        }
        {
            my $rotateMenu = Wx::Menu->new;
            wxTheApp->append_menu_item($rotateMenu, "Around X axis…", 'Rotate the selected object by an arbitrary angle around X axis',
                sub { $self->rotate(undef, X) }, undef, 'bullet_red.png');
            wxTheApp->append_menu_item($rotateMenu, "Around Y axis…", 'Rotate the selected object by an arbitrary angle around Y axis',
                sub { $self->rotate(undef, Y) }, undef, 'bullet_green.png');
            wxTheApp->append_menu_item($rotateMenu, "Around Z axis…", 'Rotate the selected object by an arbitrary angle around Z axis',
                sub { $self->rotate(undef, Z) }, undef, 'bullet_blue.png');
            wxTheApp->append_submenu($menu, "Rotate", 'Rotate the selected object by an arbitrary angle',
                $rotateMenu, undef, 'arrow_rotate_anticlockwise.png');
        }
        $frame->PopupMenu($menu, $event->GetPoint);
    });
    EVT_BUTTON($self, $self->{btn_load_part}, sub { $self->on_btn_load(0) });
    EVT_BUTTON($self, $self->{btn_load_modifier}, sub { $self->on_btn_load(1) });
    EVT_BUTTON($self, $self->{btn_load_lambda_modifier}, sub { $self->on_btn_lambda(1) });
    EVT_BUTTON($self, $self->{btn_delete}, \&on_btn_delete);

    $self->reload_tree;
    
    return $self;
}

sub reload_tree {
    my ($self, $selected_volume_idx) = @_;
    
    $selected_volume_idx //= -1;
    my $object  = $self->{model_object};
    my $tree    = $self->{tree};
    my $rootId  = $tree->GetRootItem;
    
    # despite wxWidgets states that DeleteChildren "will not generate any events unlike Delete() method",
    # the MSW implementation of DeleteChildren actually calls Delete() for each item, so
    # EVT_TREE_SEL_CHANGED is being called, with bad effects (the event handler is called; this 
    # subroutine is never continued; an invisible EndModal is called on the dialog causing Plater
    # to continue its logic and rescheduling the background process etc. GH #2774)
    $self->{disable_tree_sel_changed_event} = 1;
    $tree->DeleteChildren($rootId);
    $self->{disable_tree_sel_changed_event} = 0;
    
    my $selectedId = $rootId;
    foreach my $volume_id (0..$#{$object->volumes}) {
        my $volume = $object->volumes->[$volume_id];
        
        my $icon = $volume->modifier ? ICON_MODIFIERMESH : ICON_SOLIDMESH;
        my $itemId = $tree->AppendItem($rootId, $volume->name || $volume_id, $icon);
        if ($volume_id == $selected_volume_idx) {
            $selectedId = $itemId;
        }
        $tree->SetPlData($itemId, {
            type        => 'volume',
            volume_id   => $volume_id,
        });
    }
    $tree->ExpandAll;
    
    Slic3r::GUI->CallAfter(sub {
        $self->{tree}->SelectItem($selectedId);
        
        # SelectItem() should trigger EVT_TREE_SEL_CHANGED as per wxWidgets docs,
        # but in fact it doesn't if the given item is already selected (this happens
        # on first load)
        $self->selection_changed;
    });
}

sub get_selection {
    my ($self) = @_;
    
    my $nodeId = $self->{tree}->GetSelection;
    if ($nodeId->IsOk) {
        return $self->{tree}->GetPlData($nodeId);
    }
    return undef;
}

sub selection_changed {
    my ($self) = @_;
    
    # deselect all meshes
    if ($self->{canvas}) {
        $_->selected(0) for @{$self->{canvas}->volumes};
    }
    
    # disable things as if nothing is selected
    $self->{btn_delete}->Disable;
    $self->{settings_panel}->disable;
    $self->{settings_panel}->set_config(undef);

    # reset move sliders
    $self->{optgroup_movers}->set_value("x", 0);
    $self->{optgroup_movers}->set_value("y", 0);
    $self->{optgroup_movers}->set_value("z", 0);
    $self->{move_target} = Slic3r::Pointf3->new;
    
    if (my $itemData = $self->get_selection) {
        my ($config, @opt_keys);
        if ($itemData->{type} eq 'volume') {
            # select volume in 3D preview
            if ($self->{canvas}) {
                $self->{canvas}->volumes->[ $itemData->{volume_id} ]{selected} = 1;
            }
            $self->{btn_delete}->Enable;
            
            # attach volume config to settings panel
            my $volume = $self->{model_object}->volumes->[ $itemData->{volume_id} ];
   
            my $movers = $self->{optgroup_movers};
            
            my $obj_bb = $self->{model_object}->raw_bounding_box;
            my $vol_bb = $volume->mesh->bounding_box;
            my $vol_size = $vol_bb->size;
            $movers->get_field('x')->set_range($obj_bb->x_min - $vol_size->x, $obj_bb->x_max);
            $movers->get_field('y')->set_range($obj_bb->y_min - $vol_size->y, $obj_bb->y_max);  #,,
            $movers->get_field('z')->set_range($obj_bb->z_min - $vol_size->z, $obj_bb->z_max);
            $movers->get_field('x')->set_value($vol_bb->x_min);
            $movers->get_field('y')->set_value($vol_bb->y_min);
            $movers->get_field('z')->set_value($vol_bb->z_min);
            
            $self->{left_sizer}->Show($movers->sizer);

            $config = $volume->config;
            $self->{staticbox}->SetLabel('Part Settings');
            
            # get default values
            @opt_keys = @{Slic3r::Config::PrintRegion->new->get_keys};
        } elsif ($itemData->{type} eq 'object') {
            # select nothing in 3D preview
            
            # attach object config to settings panel
            $self->{left_sizer}->Hide($self->{optgroup_movers}->sizer);
            $self->{staticbox}->SetLabel('Object Settings');
            @opt_keys = (map @{$_->get_keys}, Slic3r::Config::PrintObject->new, Slic3r::Config::PrintRegion->new);
            $config = $self->{model_object}->config;
        }
        # get default values
        my $default_config = Slic3r::Config->new_from_defaults(@opt_keys);
        
        # append default extruder
        push @opt_keys, 'extruder';
        $default_config->set('extruder', 0);
        $config->set_ifndef('extruder', 0);
        $self->{settings_panel}->set_default_config($default_config);
        $self->{settings_panel}->set_config($config);
        $self->{settings_panel}->set_opt_keys(\@opt_keys);
        $self->{settings_panel}->set_fixed_options([qw(extruder)]);
        $self->{settings_panel}->enable;
    }
    
    $self->{canvas}->Render if $self->{canvas};
}

sub on_btn_load {
    my ($self, $is_modifier) = @_;
    
    my @input_files = wxTheApp->open_model($self);
    foreach my $input_file (@input_files) {
        my $model = eval { Slic3r::Model->read_from_file($input_file) };
        if ($@) {
            Slic3r::GUI::show_error($self, $@);
            next;
        }
        
        for my $obj_idx (0..($model->objects_count-1)) {
            my $object = $model->objects->[$obj_idx];
            for my $vol_idx (0..($object->volumes_count-1)) {
                my $new_volume = $self->{model_object}->add_volume($object->get_volume($vol_idx));
                $new_volume->set_modifier($is_modifier);
                $new_volume->set_name(basename($input_file));
                 
                # input_file needed to reload / update modifiers' volumes
                $new_volume->set_input_file($input_file);
                $new_volume->set_input_file_obj_idx($obj_idx);
                $new_volume->set_input_file_vol_idx($vol_idx);
                
                # apply the same translation we applied to the object
                $new_volume->mesh->translate(@{$self->{model_object}->origin_translation});
                
                # set a default extruder value, since user can't add it manually
                $new_volume->config->set_ifndef('extruder', 0);
                
                $self->{parts_changed} = 1;
            }
        }
    }
    
    $self->_parts_changed;
}

sub on_btn_lambda {
    my ($self, $is_modifier) = @_;
    
    my $dlg = Slic3r::GUI::Plater::LambdaObjectDialog->new($self);
    if ($dlg->ShowModal() == wxID_CANCEL) {
        return;
    }
    my $params = $dlg->ObjectParameter;
    my $type = "".$params->{"type"};
    my $name = "lambda-".$params->{"type"};
    my $mesh;

    if ($type eq "box") {
        $mesh = Slic3r::TriangleMesh::make_cube(@{$params->{"dim"}});
    } elsif ($type eq "cylinder") {
        $mesh = Slic3r::TriangleMesh::make_cylinder($params->{"cyl_r"}, $params->{"cyl_h"});
    } elsif ($type eq "sphere") {
        $mesh = Slic3r::TriangleMesh::make_sphere($params->{"sph_rho"});
    } elsif ($type eq "slab") {
        my $size = $self->{model_object}->bounding_box->size;
        $mesh = Slic3r::TriangleMesh::make_cube(
            $size->x*1.5,
            $size->y*1.5, #**
            $params->{"slab_h"},
        );
        # box sets the base coordinate at 0,0, move to center of plate
        $mesh->translate(
            -$size->x*1.5/2.0,
            -$size->y*1.5/2.0,  #**
            0,
        );
    } else {
        return;
    }

    my $center = $self->{model_object}->bounding_box->center; 
    if (!$Slic3r::GUI::Settings->{_}{autocenter}) {
        #TODO what we really want to do here is just align the
        # center of the modifier to the center of the part.
        $mesh->translate($center->x, $center->y, 0);
    }

    $mesh->repair;
    my $new_volume = $self->{model_object}->add_volume(mesh => $mesh);
    $new_volume->set_modifier($is_modifier);
    $new_volume->set_name($name);

    # set a default extruder value, since user can't add it manually
    $new_volume->config->set_ifndef('extruder', 0);

    $self->_parts_changed($self->{model_object}->volumes_count-1);
}

sub on_btn_delete {
    my ($self) = @_;

    my $itemData = $self->get_selection;
    if ($itemData && $itemData->{type} eq 'volume') {
        my $volume = $self->{model_object}->volumes->[$itemData->{volume_id}];

        # if user is deleting the last solid part, throw error
        if (!$volume->modifier && scalar(grep !$_->modifier, @{$self->{model_object}->volumes}) == 1) {
            Slic3r::GUI::show_error($self, "You can't delete the last solid part from this object.");
            return;
        }

        $self->{model_object}->delete_volume($itemData->{volume_id});
        $self->{parts_changed} = 1;
    }
    
    $self->_parts_changed;
}

sub _parts_changed {
    my ($self, $selected_volume_idx) = @_;
    
    $self->{parts_changed} = 1;
    $self->reload_tree($selected_volume_idx);
    if ($self->{canvas}) {
        $self->{canvas}->reset_objects;
        $self->{canvas}->load_object($self->{model_object});
        $self->{canvas}->zoom_to_volumes;
        $self->{canvas}->Render;
    }
}

sub CanClose {
    my $self = shift;
    
    return 1;  # skip validation for now
    
    # validate options before allowing user to dismiss the dialog
    # the validate method only works on full configs so we have
    # to merge our settings with the default ones
    my $config = Slic3r::Config->merge($self->GetParent->GetParent->GetParent->GetParent->GetParent->config, $self->model_object->config);
    eval {
        $config->validate;
    };
    return 0 if Slic3r::GUI::catch_error($self);    
    return 1;
}

sub PartsChanged {
    my ($self) = @_;
    return $self->{parts_changed};
}

sub PartSettingsChanged {
    my ($self) = @_;
    return $self->{part_settings_changed};
}

sub _update {
    my ($self) = @_;

    my $itemData = $self->get_selection;
    if ($itemData && $itemData->{type} eq 'volume') {
        my $volume = $self->{model_object}->volumes->[$itemData->{volume_id}];
        $volume->mesh->translate(@{ $volume->mesh->bounding_box->min_point->vector_to($self->{move_target}) });
    }

    $self->{parts_changed} = 1;
    my @objects = ();
    push @objects, $self->{model_object};
    $self->{canvas}->reset_objects;
    $self->{canvas}->load_object($_, undef, [0]) for @objects;
    $self->{canvas}->Render;
}

sub changescale {
    my ($self, $axis, $tosize) = @_;
    my $itemData = $self->get_selection;
    if ($itemData && $itemData->{type} eq 'volume') {
        my $volume = $self->{model_object}->volumes->[$itemData->{volume_id}];
        my $object_size = $volume->bounding_box->size;
        if (defined $axis) {
            my $axis_name = $axis == X ? 'X' : $axis == Y ? 'Y' : 'Z';
            my $scale;
            if (defined $tosize) {
                my $cursize = $object_size->[$axis];
                # Wx::GetNumberFromUser() does not support decimal numbers
                my $newsize = Wx::GetTextFromUser(
                        sprintf("Enter the new size for the selected mesh:"),
                        "Scale along $axis_name",
                        $cursize, $self);
                return if !$newsize || $newsize !~ /^\d*(?:\.\d*)?$/ || $newsize < 0;
                $scale = $newsize / $cursize * 100;
            } else {
                # Wx::GetNumberFromUser() does not support decimal numbers
                $scale = Wx::GetTextFromUser("Enter the scale % for the selected object:",
                        "Scale along $axis_name", 100, $self);
                $scale =~ s/%$//;
                return if !$scale || $scale !~ /^\d*(?:\.\d*)?$/ || $scale < 0;
            }
            my $versor = [1,1,1];
            $versor->[$axis] = $scale/100;
            $volume->mesh->scale_xyz(Slic3r::Pointf3->new(@$versor));
        } else {
            my $scale;
            if ($tosize) {
                my $cursize = max(@$object_size);
                # Wx::GetNumberFromUser() does not support decimal numbers
                my $newsize = Wx::GetTextFromUser("Enter the new max size for the selected object:",
                        "Scale", $cursize, $self);
                return if !$newsize || $newsize !~ /^\d*(?:\.\d*)?$/ || $newsize < 0;
                $scale = $newsize / $cursize;
            } else {
                # max scale factor should be above 2540 to allow importing files exported in inches
                # Wx::GetNumberFromUser() does not support decimal numbers
                $scale = Wx::GetTextFromUser("Enter the scale % for the selected object:", 'Scale',
                        100, $self);
                return if !$scale || $scale !~ /^\d*(?:\.\d*)?$/ || $scale < 0;
            }
            return if !$scale || $scale < 0;
            $volume->mesh->scale($scale);
        }
        $self->_parts_changed;
    }
}

sub rotate {
    my $self = shift;
    my ($angle, $axis) = @_;
    # angle is in degrees
    my $itemData = $self->get_selection;
    if ($itemData && $itemData->{type} eq 'volume') {
        my $volume = $self->{model_object}->volumes->[$itemData->{volume_id}];
        if (!defined $angle) {
            my $axis_name = $axis == X ? 'X' : $axis == Y ? 'Y' : 'Z';
            my $default = $axis == Z ? 0 : 0;
            # Wx::GetNumberFromUser() does not support decimal numbers
            $angle = Wx::GetTextFromUser("Enter the rotation angle:", "Rotate around $axis_name axis",
                    $default, $self);
            return if !$angle || $angle !~ /^-?\d*(?:\.\d*)?$/ || $angle == -1;
        }
        if ($axis == X) { $volume->mesh->rotate_x(deg2rad($angle)); }

        if ($axis == Y) { $volume->mesh->rotate_y(deg2rad($angle)); } 
        if ($axis == Z) { $volume->mesh->rotate_z(deg2rad($angle)); }
            
        $self->_parts_changed;
    }
}
sub GetFrame {
    my ($self) = @_;
    return &Wx::GetTopLevelParent($self);
}
1;
