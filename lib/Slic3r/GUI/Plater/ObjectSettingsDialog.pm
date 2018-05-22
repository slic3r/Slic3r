# This dialog opens up when double clicked on an object line in the list at the right side of the platter.
# One may load additional STLs and additional modifier STLs,
# one may change the properties of the print per each modifier mesh or a Z-span.

package Slic3r::GUI::Plater::ObjectSettingsDialog;
use strict;
use warnings;
use utf8;

use Wx qw(:dialog :id :misc :sizer :systemsettings :notebook wxTAB_TRAVERSAL wxTheApp);
use Wx::Event qw(EVT_BUTTON EVT_MENU);
use base 'Wx::Dialog';

sub new {
    my $class = shift;
    my ($parent, %params) = @_;
    my $self = $class->SUPER::new($parent, -1, "Settings for " . $params{object}->name, wxDefaultPosition, [1000,700], wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER);
    $self->{$_} = $params{$_} for keys %params;
    
    $self->{tabpanel} = Wx::Notebook->new($self, -1, wxDefaultPosition, wxDefaultSize, wxNB_TOP | wxTAB_TRAVERSAL);
    $self->{tabpanel}->AddPage($self->{parts} = Slic3r::GUI::Plater::ObjectPartsPanel->new($self->{tabpanel}, model_object => $params{model_object}), "Parts");
    $self->{tabpanel}->AddPage($self->{adaptive_layers} = Slic3r::GUI::Plater::ObjectDialog::AdaptiveLayersTab->new( $self->{tabpanel}, 
        plater         => $parent,
        model_object   => $params{model_object},
        obj_idx        => $params{obj_idx}
        ), "Adaptive Layers");
    $self->{tabpanel}->AddPage($self->{layers} = Slic3r::GUI::Plater::ObjectDialog::LayersTab->new($self->{tabpanel}), "Layer height table");
    
    my $buttons = $self->CreateStdDialogButtonSizer(wxOK);
    EVT_BUTTON($self, wxID_OK, sub {
        # validate user input
        return if !$self->{parts}->CanClose;
        return if !$self->{layers}->CanClose;
        
        # notify tabs
        $self->{layers}->Closing;
        
        # save window size
        wxTheApp->save_window_pos($self, "object_settings");
        
        $self->EndModal(wxID_OK);
        $self->Destroy;
    });
    
    my $sizer = Wx::BoxSizer->new(wxVERTICAL);
    $sizer->Add($self->{tabpanel}, 1, wxEXPAND | wxTOP | wxLEFT | wxRIGHT, 10);
    $sizer->Add($buttons, 0, wxEXPAND | wxBOTTOM | wxLEFT | wxRIGHT, 10);
    
    $self->SetSizer($sizer);
    $self->SetMinSize($self->GetSize);
    
    wxTheApp->restore_window_pos($self, "object_settings");
    
    return $self;
}

sub PartsChanged {
    my ($self) = @_;
    return $self->{parts}->PartsChanged;
}

sub PartSettingsChanged {
    my ($self) = @_;
    return $self->{parts}->PartSettingsChanged || $self->{layers}->LayersChanged;
}


package Slic3r::GUI::Plater::ObjectDialog::BaseTab;
use base 'Wx::Panel';

sub model_object {
    my ($self) = @_;
    return $self->GetParent->GetParent->{model_object};
}

package Slic3r::GUI::Plater::ObjectDialog::AdaptiveLayersTab;
use Slic3r::Geometry qw(X Y Z scale unscale);
use Slic3r::Print::State ':steps';
use List::Util qw(min max sum first);
use Wx qw(wxTheApp :dialog :id :misc :sizer :systemsettings :statictext wxTAB_TRAVERSAL);
use base 'Slic3r::GUI::Plater::ObjectDialog::BaseTab';

sub new {
    my $class = shift;
    my ($parent, %params) = @_;
    my $self = $class->SUPER::new($parent, -1, wxDefaultPosition, wxDefaultSize);
    my $model_object = $self->{model_object} = $params{model_object};
    my $obj_idx = $self->{obj_idx} = $params{obj_idx};
    my $plater = $self->{plater} = $params{plater};
    my $object = $self->{object} = $self->{plater}->{print}->get_object($self->{obj_idx});

    # store last raft height to correctly draw z-indicator plane during a running background job where the printObject is not valid
    $self->{last_raft_height} = 0;

    # Initialize 3D toolpaths preview
    if ($Slic3r::GUI::have_OpenGL) {
        $self->{preview3D} = Slic3r::GUI::Plater::3DPreview->new($self, $plater->{print});
        $self->{preview3D}->canvas->set_auto_bed_shape;
        $self->{preview3D}->canvas->SetSize([500,500]);
        $self->{preview3D}->canvas->SetMinSize($self->{preview3D}->canvas->GetSize);
        # object already processed?
        wxTheApp->CallAfter(sub {
            if (!$plater->{processed}) {
                $self->_trigger_slicing(0); # trigger processing without invalidating STEP_SLICE to keep current height distribution
            }else{
                $self->{preview3D}->reload_print($obj_idx);
                $self->{preview3D}->canvas->zoom_to_volumes;
                $self->{preview_zoomed} = 1;
            }
        });
    }

    $self->{splineControl} = Slic3r::GUI::Plater::SplineControl->new($self, Wx::Size->new(150, 200), $object);

    my $optgroup;
    $optgroup = $self->{optgroup} = Slic3r::GUI::OptionsGroup->new(
        parent      => $self,
        title       => 'Adaptive quality %',
        on_change   => sub {
            my ($opt_id) = @_;
            # There seems to be an issue with wxWidgets 3.0.2/3.0.3, where the slider
            # genates tens of events for a single value change.
            # Only trigger the recalculation if the value changes
            # or a live preview was activated and the mesh cut is not valid yet.
            if ($self->{adaptive_quality} != $optgroup->get_value($opt_id)) {
                $self->{adaptive_quality} = $optgroup->get_value($opt_id);
                $self->{model_object}->config->set('adaptive_slicing_quality', $optgroup->get_value($opt_id));
                $self->{object}->config->set('adaptive_slicing_quality', $optgroup->get_value($opt_id));
                $self->{object}->invalidate_step(STEP_LAYERS);
                # trigger re-slicing
                $self->_trigger_slicing;
            }
        },
        label_width  => 0,
    );

    $optgroup->append_single_option_line(Slic3r::GUI::OptionsGroup::Option->new(
        opt_id      => 'adaptive_slicing_quality',
        type        => 'slider',
        label       => '',
        default     => $object->config->get('adaptive_slicing_quality'),
        min         => 0,
        max         => 100,
        full_width  => 1,
    ));
    $optgroup->get_field('adaptive_slicing_quality')->set_scale(1);
    $self->{adaptive_quality} = $object->config->get('adaptive_slicing_quality');
    # init quality slider
    if(!$object->config->get('adaptive_slicing')) {
        # disable slider
        $optgroup->get_field('adaptive_slicing_quality')->disable;
    }

    my $right_sizer = Wx::BoxSizer->new(wxVERTICAL);
    $right_sizer->Add($self->{splineControl}, 1, wxEXPAND | wxALL, 0);
    $right_sizer->Add($optgroup->sizer, 0, wxEXPAND | wxALL, 0);


    $self->{sizer} = Wx::BoxSizer->new(wxHORIZONTAL);
    $self->{sizer}->Add($self->{preview3D}, 3, wxEXPAND | wxTOP | wxBOTTOM, 0) if $self->{preview3D};
    $self->{sizer}->Add($right_sizer, 1, wxEXPAND | wxTOP | wxBOTTOM, 10);

    $self->SetSizerAndFit($self->{sizer});

    # init spline control values
    # determine min and max layer height from perimeter extruder capabilities.
    my %extruders;
    for my $region_id (0 .. ($object->region_count - 1)) {
        foreach (qw(perimeter_extruder infill_extruder solid_infill_extruder)) {
            my $extruder_id = $self->{plater}->{print}->get_region($region_id)->config->get($_)-1;
            $extruders{$extruder_id} = $extruder_id;
        }
    }
    my $min_height = max(map {$self->{plater}->{print}->config->get_at('min_layer_height', $_)} (values %extruders));
    my $max_height = min(map {$self->{plater}->{print}->config->get_at('max_layer_height', $_)} (values %extruders));

    $self->{splineControl}->set_size_parameters($min_height, $max_height, unscale($object->size->z));

    $self->{splineControl}->on_layer_update(sub {
        # trigger re-slicing
        $self->_trigger_slicing;
    });

    $self->{splineControl}->on_z_indicator(sub {
        my ($z) = @_;

        if($z) { # compensate raft height
	        $z += $self->{last_raft_height};
        }
        $self->{preview3D}->canvas->SetCuttingPlane(Z, $z, []);
        $self->{preview3D}->canvas->Render;
    });

    return $self;
}

# This is called by the plater after processing to update the preview and spline
sub reload_preview {
    my ($self) = @_;
    $self->{splineControl}->update;
    $self->{preview3D}->reload_print($self->{obj_idx});
    my $object = $self->{plater}->{print}->get_object($self->{obj_idx});
    if($object->layer_count-1 > 0) {
        my $first_layer = $self->{object}->get_layer(0);
        $self->{last_raft_height} = max(0, $first_layer->print_z - $first_layer->height);
        $self->{preview3D}->set_z(unscale($self->{object}->size->z));
        if(!$self->{preview_zoomed}) {
            $self->{preview3D}->canvas->set_auto_bed_shape;
            $self->{preview3D}->canvas->zoom_to_volumes;
            $self->{preview_zoomed} = 1;
        }
    }
}

# Trigger background slicing at the plater
sub _trigger_slicing {
    my ($self, $invalidate) = @_;
    $invalidate //= 1;
    my $object = $self->{plater}->{print}->get_object($self->{obj_idx});
    $self->{model_object}->set_layer_height_spline($self->{object}->layer_height_spline); # push modified spline object to model_object
    #$self->{plater}->pause_background_process;
    $self->{plater}->stop_background_process;
    if (!$Slic3r::GUI::Settings->{_}{background_processing}) {
        $self->{plater}->statusbar->SetCancelCallback(sub {
            $self->{plater}->stop_background_process;
            $self->{plater}->statusbar->SetStatusText("Slicing cancelled");
            $self->{plater}->preview_notebook->SetSelection(0);
        });
        $object->invalidate_step(STEP_SLICE) if($invalidate);
        $self->{plater}->start_background_process;
    }else{
        $object->invalidate_step(STEP_SLICE) if($invalidate);
        $self->{plater}->schedule_background_process;
    }
}


package Slic3r::GUI::Plater::ObjectDialog::LayersTab;
use Wx qw(:dialog :id :misc :sizer :systemsettings);
use Wx::Grid;
use Wx::Event qw(EVT_GRID_CELL_CHANGED);
use base 'Slic3r::GUI::Plater::ObjectDialog::BaseTab';

sub new {
    my $class = shift;
    my ($parent, %params) = @_;
    my $self = $class->SUPER::new($parent, -1, wxDefaultPosition, wxDefaultSize);
    
    my $sizer = Wx::BoxSizer->new(wxVERTICAL);
    
    {
        my $label = Wx::StaticText->new($self, -1, "You can use this section to override the layer height for parts of this object. The values from this table will override the default layer height and adaptive layer heights, but not the interactively modified height curve.",
            wxDefaultPosition, wxDefaultSize);
        $label->SetFont(Wx::SystemSettings::GetFont(wxSYS_DEFAULT_GUI_FONT));
        $label->Wrap(800);
        $sizer->Add($label, 0, wxEXPAND | wxALL, 10);
    }
    
    my $grid = $self->{grid} = Wx::Grid->new($self, -1, wxDefaultPosition, wxDefaultSize);
    $sizer->Add($grid, 1, wxEXPAND | wxALL, 10);
    $grid->CreateGrid(0, 3);
    $grid->DisableDragRowSize;
    $grid->HideRowLabels if &Wx::wxVERSION_STRING !~ / 2\.8\./;
    $grid->SetColLabelValue(0, "Min Z (mm)");
    $grid->SetColLabelValue(1, "Max Z (mm)");
    $grid->SetColLabelValue(2, "Layer height (mm)");
    $grid->SetColSize($_, -1) for 0..2;
    $grid->SetDefaultCellAlignment(wxALIGN_CENTRE, wxALIGN_CENTRE);
    
    # load data
    foreach my $range (@{ $self->model_object->layer_height_ranges }) {
        $grid->AppendRows(1);
        my $i = $grid->GetNumberRows-1;
        $grid->SetCellValue($i, $_, $range->[$_]) for 0..2;
    }
    $grid->AppendRows(1); # append one empty row
    
    EVT_GRID_CELL_CHANGED($grid, sub {
        my ($grid, $event) = @_;
        
        # remove any non-numeric character
        my $value = $grid->GetCellValue($event->GetRow, $event->GetCol);
        $value =~ s/,/./g;
        $value =~ s/[^0-9.]//g;
        $grid->SetCellValue($event->GetRow, $event->GetCol, $value);
        
        # if there's no empty row, let's append one
        for my $i (0 .. $grid->GetNumberRows) {
            if ($i == $grid->GetNumberRows) {
                # if we're here then we found no empty row
                $grid->AppendRows(1);
                last;
            }
            if (!grep $grid->GetCellValue($i, $_), 0..2) {
                # exit loop if this row is empty
                last;
            }
        }
        
        $self->{layers_changed} = 1;
    });
    
    $self->SetSizer($sizer);
    $sizer->SetSizeHints($self);
    
    return $self;
}

sub CanClose {
    my $self = shift;
    
    # validate ranges before allowing user to dismiss the dialog
    
    foreach my $range ($self->_get_ranges) {
        my ($min, $max, $height) = @$range;
        if ($max <= $min) {
            Slic3r::GUI::show_error($self, "Invalid Z range $min-$max.");
            return 0;
        }
        if ($min < 0 || $max < 0) {
            Slic3r::GUI::show_error($self, "Invalid Z range $min-$max.");
            return 0;
        }
        if ($height < 0) {
            Slic3r::GUI::show_error($self, "Invalid layer height $height.");
            return 0;
        }
        # TODO: check for overlapping ranges
    }
    
    return 1;
}

sub Closing {
    my $self = shift;
    
    # save ranges into the plater object
    $self->model_object->set_layer_height_ranges([ $self->_get_ranges ]);
}

sub _get_ranges {
    my $self = shift;
    
    my @ranges = ();
    for my $i (0 .. $self->{grid}->GetNumberRows-1) {
        my ($min, $max, $height) = map $self->{grid}->GetCellValue($i, $_), 0..2;
        next if $min eq '' || $max eq '' || $height eq '';
        push @ranges, [ $min, $max, $height ];
    }
    return sort { $a->[0] <=> $b->[0] } @ranges;
}

sub LayersChanged {
    my ($self) = @_;
    return $self->{layers_changed};
}

1;
