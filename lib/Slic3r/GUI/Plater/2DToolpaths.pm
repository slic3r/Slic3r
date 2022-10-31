# 2D preview of the tool paths of a single layer, using a thin line.
# OpenGL is used to render the paths.
# Vojtech also added a 2D simulation of under/over extrusion in a single layer.

package Slic3r::GUI::Plater::2DToolpaths;
use strict;
use warnings;
use utf8;

use Slic3r::Print::State ':steps';
use Wx qw(:misc :sizer :slider :statictext wxWHITE);
use Wx::Event qw(EVT_SLIDER EVT_KEY_DOWN);
use base qw(Wx::Panel Class::Accessor);

# Color Scheme
use Slic3r::GUI::ColorScheme;

__PACKAGE__->mk_accessors(qw(print enabled));

sub new {
    my $class = shift;
    my ($parent, $print) = @_;

    my $self = $class->SUPER::new($parent, -1, wxDefaultPosition);
    if ( ( defined $Slic3r::GUI::Settings->{_}{colorscheme} ) && ( my $getScheme =  Slic3r::GUI::ColorScheme->can($Slic3r::GUI::Settings->{_}{colorscheme}) ) ) {
        $getScheme->();
        $self->SetBackgroundColour(Wx::Colour->new(@BACKGROUND255));
    } else {
        Slic3r::GUI::ColorScheme->getDefault();
        $self->SetBackgroundColour(Wx::wxWHITE);
    }

    # init GUI elements
    my $canvas = $self->{canvas} = Slic3r::GUI::Plater::2DToolpaths::Canvas->new($self, $print);
    my $slider = $self->{slider} = Wx::Slider->new(
        $self, -1,
        0,                              # default
        0,                              # min
        # we set max to a bogus non-zero value because the MSW implementation of wxSlider
        # will skip drawing the slider if max <= min:
        1,                              # max
        wxDefaultPosition,
        wxDefaultSize,
        wxVERTICAL | wxSL_INVERSE,
    );
    my $z_label = $self->{z_label} = Wx::StaticText->new($self, -1, "", wxDefaultPosition,
        [40,-1], wxALIGN_CENTRE_HORIZONTAL);
    $z_label->SetFont($Slic3r::GUI::small_font);
    
    my $vsizer = Wx::BoxSizer->new(wxVERTICAL);
    $vsizer->Add($slider, 1, wxALL | wxEXPAND | wxALIGN_CENTER, 3);
    $vsizer->Add($z_label, 0, wxALL | wxEXPAND | wxALIGN_CENTER, 3);
    
    my $sizer = Wx::BoxSizer->new(wxHORIZONTAL);
    $sizer->Add($canvas, 1, wxALL | wxEXPAND, 0);
    $sizer->Add($vsizer, 0, wxTOP | wxBOTTOM | wxEXPAND, 5);
    
    EVT_SLIDER($self, $slider, sub {
        $self->set_z($self->{layers_z}[$slider->GetValue])
            if $self->enabled;
    });
    EVT_KEY_DOWN($canvas, sub {
        my ($s, $event) = @_;
        
        my $key = $event->GetKeyCode;
        if ($key == 85 || $key == 315) {
            $slider->SetValue($slider->GetValue + 1);
            $self->set_z($self->{layers_z}[$slider->GetValue]);
        } elsif ($key == 68 || $key == 317) {
            $slider->SetValue($slider->GetValue - 1);
            $self->set_z($self->{layers_z}[$slider->GetValue]);
        } else {
            $event->Skip;
        }
    });
    
    $self->SetSizer($sizer);
    $self->SetMinSize($self->GetSize);
    $sizer->SetSizeHints($self);
    
    # init print
    $self->{print} = $print;
    $self->reload_print;
    
    return $self;
}

sub reload_print {
    my ($self) = @_;
    
    # we require that there's at least one object and the posSlice step
    # is performed on all of them (this ensures that _shifted_copies was
    # populated and we know the number of layers)
    if (!$self->print->object_step_done(STEP_SLICE)) {
        $self->enabled(0);
        $self->{slider}->Hide;
        $self->{canvas}->Refresh;  # clears canvas
        return;
    }
    
    $self->{canvas}->bb($self->print->total_bounding_box);
    $self->{canvas}->_dirty(1);
    
    my %z = ();  # z => 1
    foreach my $object (@{$self->{print}->objects}) {
        foreach my $layer (@{$object->layers}, @{$object->support_layers}) {
            $z{$layer->print_z} = 1;
        }
    }
    $self->enabled(1);
    $self->{layers_z} = [ sort { $a <=> $b } keys %z ];
    $self->{slider}->SetRange(0, scalar(@{$self->{layers_z}})-1);
    if ((my $z_idx = $self->{slider}->GetValue) <= $#{$self->{layers_z}}) {
        $self->set_z($self->{layers_z}[$z_idx]);
    } else {
        $self->{slider}->SetValue(0);
        $self->set_z($self->{layers_z}[0]) if @{$self->{layers_z}};
    }
    $self->{slider}->Show;
    $self->Layout;
}

sub set_z {
    my ($self, $z) = @_;
    
    return if !$self->enabled;
    $self->{z_label}->SetLabel(sprintf '%.2f', $z);
    $self->{canvas}->set_z($z);
}


package Slic3r::GUI::Plater::2DToolpaths::Canvas;

use Wx::Event qw(EVT_PAINT EVT_SIZE EVT_IDLE EVT_MOUSEWHEEL EVT_MOUSE_EVENTS);
use OpenGL qw(:glconstants :glfunctions :glufunctions :gluconstants);
use base qw(Wx::GLCanvas Class::Accessor);
use Wx::GLCanvas qw(:all);
use List::Util qw(min max first);
use Slic3r::Geometry qw(scale unscale epsilon X Y);
use Slic3r::Print::State ':steps';

# Color Scheme
use Slic3r::GUI::ColorScheme;

__PACKAGE__->mk_accessors(qw(
    print z layers color init
    bb
    _camera_bb
    _dirty
    _zoom
    _camera_target
    _drag_start_xy
));

# make OpenGL::Array thread-safe
{
    no warnings 'redefine';
    *OpenGL::Array::CLONE_SKIP = sub { 1 };
}

sub new {
    my ($class, $parent, $print) = @_;

    if ( ( defined $Slic3r::GUI::Settings->{_}{colorscheme} ) && ( Slic3r::GUI::ColorScheme->can($Slic3r::GUI::Settings->{_}{colorscheme}) ) ) {
        my $myGetSchemeName = \&{"Slic3r::GUI::ColorScheme::$Slic3r::GUI::Settings->{_}{colorscheme}"};
        $myGetSchemeName->();
    } else {
        Slic3r::GUI::ColorScheme->getDefault();
    }
    my $self = (Wx::wxVERSION >= 3.000003) ?
        # The wxWidgets 3.0.3-beta have a bug, they crash with NULL attribute list.
        $class->SUPER::new($parent, -1, Wx::wxDefaultPosition, Wx::wxDefaultSize, 0, "",
            [WX_GL_RGBA, WX_GL_DOUBLEBUFFER, WX_GL_DEPTH_SIZE, 24, 0]) :
        $class->SUPER::new($parent);
    # Immediately force creation of the OpenGL context to consume the static variable s_wglContextAttribs.
    $self->GetContext();
    $self->print($print);
    $self->_zoom(1);

    # 2D point in model space
    $self->_camera_target(Slic3r::Pointf->new(0,0));
    
    EVT_PAINT($self, sub {
        my $dc = Wx::PaintDC->new($self);
        $self->Render($dc);
    });
    EVT_SIZE($self, sub { $self->_dirty(1) });
    EVT_IDLE($self, sub {
        return unless $self->_dirty;
        return if !$self->IsShownOnScreen;
        $self->Resize;
        $self->Refresh;
    });
    EVT_MOUSEWHEEL($self, sub {
        my ($self, $e) = @_;
        
        return if !$self->GetParent->enabled;
        
        my $old_zoom = $self->_zoom;
        
        # Calculate the zoom delta and apply it to the current zoom factor
        my $zoom = -$e->GetWheelRotation() / $e->GetWheelDelta();
        if ($Slic3r::GUI::Settings->{_}{invert_zoom}) {
            $zoom *= -1;
        }
        $zoom = max(min($zoom, 4), -4);
        $zoom /= 10;
        $self->_zoom($self->_zoom / (1-$zoom));
        $self->_zoom(1) if $self->_zoom > 1;  # prevent from zooming out too much

        {
            # In order to zoom around the mouse point we need to translate
            # the camera target. This math is almost there but not perfect yet...
            my $camera_bb_size = $self->_camera_bb->size;
            my $size = Slic3r::Pointf->new($self->GetSizeWH);
            my $pos = Slic3r::Pointf->new($e->GetPositionXY);

            # calculate the zooming center in pixel coordinates relative to the viewport center
            my $vec = Slic3r::Pointf->new($pos->x - $size->x/2, $pos->y - $size->y/2);  #-

            # calculate where this point will end up after applying the new zoom
            my $vec2 = $vec->clone;
            $vec2->scale($old_zoom / $self->_zoom);

            # move the camera target by the difference of the two positions
            $self->_camera_target->translate(
                -($vec->x - $vec2->x) * $camera_bb_size->x / $size->x,
                 ($vec->y - $vec2->y) * $camera_bb_size->y / $size->y,  #//
            );
        }
        
        $self->_dirty(1);
        $self->Refresh;
    });
    EVT_MOUSE_EVENTS($self, \&mouse_event);
    
    return $self;
}

sub zoom{
    my($self, $direction) = @_;
    if( $direction eq 'in'){
        $self->_zoom($self->_zoom / (1+0.3));
    }
    elsif($direction eq 'out'){
        $self->_zoom($self->_zoom / (1-0.3));
        $self->_zoom(1) if $self->_zoom > 1;  # prevent from zooming out too much
    }
    #apply changes
    $self->_dirty(1);
    $self->Refresh;
}

sub mouse_event {
    my ($self, $e) = @_;
    
    return if !$self->GetParent->enabled;
    
    my $pos = Slic3r::Pointf->new($e->GetPositionXY);
    if ($e->Entering && &Wx::wxMSW) {
        # wxMSW needs focus in order to catch mouse wheel events
        $self->SetFocus;
    } elsif ($e->Dragging) {
        if ($e->LeftIsDown || $e->MiddleIsDown || $e->RightIsDown) {
            # if dragging, translate view
            
            if (defined $self->_drag_start_xy) {
                my $move = $self->_drag_start_xy->vector_to($pos);  # in pixels
                
                # get viewport and camera size in order to convert pixel to model units
                my ($x, $y) = $self->GetSizeWH;
                my $camera_bb_size = $self->_camera_bb->size;
                
                # compute translation in model units
                $self->_camera_target->translate(
                    -$move->x * $camera_bb_size->x / $x,
                     $move->y * $camera_bb_size->y / $y,   # /**
                );
                
                $self->_dirty(1);
                $self->Refresh;
            }
            $self->_drag_start_xy($pos);
        }
    } elsif ($e->LeftUp || $e->MiddleUp || $e->RightUp) {
        $self->_drag_start_xy(undef);
    } else {
        $e->Skip();
    }
}

sub set_z {
    my ($self, $z) = @_;
    
    my $print = $self->print;
    
    # can we have interlaced layers?
    my $interlaced = (defined first { $_->config->support_material } @{$print->objects})
        || (defined first { $_->config->infill_every_layers > 1 } @{$print->regions});
    
    my $max_layer_height = $print->max_allowed_layer_height;
    
    my @layers = ();
    foreach my $object (@{$print->objects}) {
        foreach my $layer (@{$object->layers}, @{$object->support_layers}) {
            if ($interlaced) {
                push @layers, $layer
                    if $z > ($layer->print_z - $max_layer_height - epsilon)
                        && $z <= $layer->print_z + epsilon;
            } else {
                push @layers, $layer if abs($layer->print_z - $z) < epsilon;
            }
        }
    }
    
    # reverse layers so that we draw the lowermost (i.e. current) on top
    $self->z($z);
    $self->layers([ reverse @layers ]);
    $self->Refresh;
}

sub Render {
    my ($self, $dc) = @_;
    
    # prevent calling SetCurrent() when window is not shown yet
    return unless $self->IsShownOnScreen;
    return unless my $context = $self->GetContext;
    $self->SetCurrent($context);
    $self->InitGL;
    
    if ($DEFAULT_COLORSCHEME==1){
		glClearColor(1, 1, 1, 0);
	} else{
		glClearColor(@BACKGROUND_COLOR, 0);
	}
    
    glClear(GL_COLOR_BUFFER_BIT);
    
    if (!$self->GetParent->enabled || !$self->layers) {
        glFlush();
        $self->SwapBuffers;
        return;
    }
    
    glDisable(GL_DEPTH_TEST);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    
    # anti-alias
    if (0) {
        glEnable(GL_LINE_SMOOTH);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glHint(GL_LINE_SMOOTH_HINT, GL_DONT_CARE);
        glHint(GL_POLYGON_SMOOTH_HINT, GL_DONT_CARE);
    }
    
    my $tess;
    if (!(&Wx::wxMSW && $OpenGL::VERSION < 0.6704)) {
        # We can't use the GLU tessellator on MSW with older OpenGL versions
        # because of an upstream bug:
        # http://sourceforge.net/p/pogl/bugs/16/
        $tess = gluNewTess();
        gluTessCallback($tess, GLU_TESS_BEGIN,     'DEFAULT');
        gluTessCallback($tess, GLU_TESS_END,       'DEFAULT');
        gluTessCallback($tess, GLU_TESS_VERTEX,    'DEFAULT');
        gluTessCallback($tess, GLU_TESS_COMBINE,   'DEFAULT');
        gluTessCallback($tess, GLU_TESS_ERROR,     'DEFAULT');
        gluTessCallback($tess, GLU_TESS_EDGE_FLAG, 'DEFAULT');
    }
    
    foreach my $layer (@{$self->layers}) {
        my $object = $layer->object;
        
        # only draw the slice for the current layer
        next unless abs($layer->print_z - $self->z) < epsilon;
        
        # draw slice contour
        glLineWidth(1);
        foreach my $copy (@{ $object->_shifted_copies }) {
            glPushMatrix();
            glTranslatef(@$copy, 0);
            
            foreach my $slice (@{$layer->slices}) {
                glColor3f(@TOOL_SHADE); # Inside part shade
                
                if ($tess) {
                    gluTessBeginPolygon($tess);
                    foreach my $polygon (@$slice) {
                        gluTessBeginContour($tess);
                        gluTessVertex_p($tess, @$_, 0) for @$polygon;
                        gluTessEndContour($tess);
                    }
                    gluTessEndPolygon($tess);
                }
                
                glColor3f(@TOOL_COLOR); # Perimeter
                foreach my $polygon (@$slice) {
                    foreach my $line (@{$polygon->lines}) {
                        glBegin(GL_LINES);
                        glVertex2f(@{$line->a});
                        glVertex2f(@{$line->b});
                        glEnd();
                    }
                }
            }
            glPopMatrix();
        }
    }
    
    my $skirt_drawn = 0;
    my $brim_drawn = 0;
    foreach my $layer (@{$self->layers}) {
        my $object = $layer->object;
        my $print_z = $layer->print_z;
        
        # draw brim
        if ($self->print->step_done(STEP_BRIM) && $layer->id < $self->print->brim_count && !$brim_drawn) {
            $self->color(@TOOL_DARK);
            $self->_draw(undef, $print_z, $_) for @{$self->print->brim_layer($layer->id)};
            $brim_drawn = 1;
        }
        if ($self->print->step_done(STEP_SKIRT)
            && ($self->print->has_infinite_skirt() || $self->print->config->skirt_height > $layer->id)
            && !$skirt_drawn) {
            $self->color(@TOOL_DARK);
            $self->_draw(undef, $print_z, $_) for @{$self->print->skirt};
            $skirt_drawn = 1;
        }
        
        foreach my $layerm (@{$layer->regions}) {
            if ($object->step_done(STEP_PERIMETERS)) {
                $self->color(@TOOL_STEPPERIM);
                $self->_draw($object, $print_z, $_) for map @$_, @{$layerm->perimeters};
            }
            
            if ($object->step_done(STEP_INFILL)) {
                $self->color(@TOOL_INFILL);
                $self->_draw($object, $print_z, $_) for map @$_, @{$layerm->fills};
            }
        }
        
        if ($object->step_done(STEP_SUPPORTMATERIAL)) {
            if ($layer->isa('Slic3r::Layer::Support')) {
                $self->color(@TOOL_SUPPORT);
                $self->_draw($object, $print_z, $_) for @{$layer->support_fills};
                $self->_draw($object, $print_z, $_) for @{$layer->support_interface_fills};
            }
        }
    }
    
    gluDeleteTess($tess) if $tess;
    glFlush();
    $self->SwapBuffers;
}

sub _draw {
    my ($self, $object, $print_z, $path) = @_;
    
    my @paths = $path->isa('Slic3r::ExtrusionLoop')
        ? @$path
        : ($path);
    
    $self->_draw_path($object, $print_z, $_) for @paths;
}

sub _draw_path {
    my ($self, $object, $print_z, $path) = @_;
    
    return if $print_z - $path->height > $self->z - epsilon;
    
    if (abs($print_z - $self->z) < epsilon) {
        glColor3f($self->color->[0], $self->color->[1], $self->color->[2]);
    } else {
        glColor3f(0.8, 0.8, 0.8);
    }
    
    glLineWidth(1);
    
    if (defined $object) {
        foreach my $copy (@{ $object->_shifted_copies }) {
            glPushMatrix();
            glTranslatef(@$copy, 0);
            foreach my $line (@{$path->polyline->lines}) {
                glBegin(GL_LINES);
                glVertex2f(@{$line->a});
                glVertex2f(@{$line->b});
                glEnd();
            }
            glPopMatrix();
        }
    } else {
        foreach my $line (@{$path->polyline->lines}) {
            glBegin(GL_LINES);
            glVertex2f(@{$line->a});
            glVertex2f(@{$line->b});
            glEnd();
        }
    }
}

sub InitGL {
    my $self = shift;
 
    return if $self->init;
    return unless $self->GetContext;
    $self->init(1);
}

sub GetContext {
    my ($self) = @_;
    
    if (Wx::wxVERSION >= 2.009) {
        return $self->{context} ||= Wx::GLContext->new($self);
    } else {
        return $self->SUPER::GetContext;
    }
}
 
sub SetCurrent {
    my ($self, $context) = @_;
    
    if (Wx::wxVERSION >= 2.009) {
        return $self->SUPER::SetCurrent($context);
    } else {
        return $self->SUPER::SetCurrent;
    }
}

sub Resize {
    my ($self) = @_;
 
    return unless $self->GetContext;
    return unless $self->bb;
    $self->_dirty(0);
    
    $self->SetCurrent($self->GetContext);
    my ($x, $y) = $self->GetSizeWH;
    glViewport(0, 0, $x, $y);
    
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    
    my $bb = $self->bb->clone;
    
    # center bounding box around origin before scaling it
    my $bb_center = $bb->center;
    $bb->translate(@{$bb_center->negative});
    
    # scale bounding box according to zoom factor
    $bb->scale($self->_zoom);
    
    # reposition bounding box around original center
    $bb->translate(@{$bb_center});
    
    # translate camera
    $bb->translate(@{$self->_camera_target});
    
    # keep camera_bb within total bb
    # (i.e. prevent user from panning outside the bounding box)
    {
        my @translate = (0,0);
        if ($bb->x_min < $self->bb->x_min) {
            $translate[X] += $self->bb->x_min - $bb->x_min;
        }
        if ($bb->y_min < $self->bb->y_min) {
            $translate[Y] += $self->bb->y_min - $bb->y_min;
        }
        if ($bb->x_max > $self->bb->x_max) {
            $translate[X] -= $bb->x_max - $self->bb->x_max;
        }
        if ($bb->y_max > $self->bb->y_max) {
            $translate[Y] -= $bb->y_max - $self->bb->y_max;
        }
        $self->_camera_target->translate(@translate);
        $bb->translate(@translate);
    }
    
    # save camera
    $self->_camera_bb($bb);
    
    my ($x1, $y1, $x2, $y2) = ($bb->x_min, $bb->y_min, $bb->x_max, $bb->y_max);
    if (($x2 - $x1)/($y2 - $y1) > $x/$y) {
        # adjust Y
        my $new_y = $y * ($x2 - $x1) / $x;
        $y1 = ($y2 + $y1)/2 - $new_y/2;
        $y2 = $y1 + $new_y;
    } else {
        my $new_x = $x * ($y2 - $y1) / $y;
        $x1 = ($x2 + $x1)/2 - $new_x/2;
        $x2 = $x1 + $new_x;
    }
    glOrtho($x1, $x2, $y1, $y2, 0, 1);
    
    glMatrixMode(GL_MODELVIEW);
}

sub line {
    my (
        $x1, $y1, $x2, $y2,     # coordinates of the line
        $w,                     # width/thickness of the line in pixel
        $Cr, $Cg, $Cb,          # RGB color components
        $Br, $Bg, $Bb,          # color of background when alphablend=false
                                # Br=alpha of color when alphablend=true
        $alphablend,            # use alpha blend or not
    ) = @_;
    
    my $t;
    my $R;
    my $f = $w - int($w);
    my $A;
    
    if ($alphablend) {
        $A = $Br;
    } else {
        $A = 1;
    }
    
    # determine parameters t,R
    if ($w >= 0 && $w < 1) {
        $t = 0.05; $R = 0.48 + 0.32 * $f;
        if (!$alphablend) {
            $Cr += 0.88 * (1-$f);
            $Cg += 0.88 * (1-$f);
            $Cb += 0.88 * (1-$f);
            $Cr = 1.0 if ($Cr > 1.0);
            $Cg = 1.0 if ($Cg > 1.0);
            $Cb = 1.0 if ($Cb > 1.0);
        } else {
            $A *= $f;
        }
    } elsif ($w >= 1.0 && $w < 2.0) {
        $t = 0.05 + $f*0.33; $R = 0.768 + 0.312*$f;
    } elsif ($w >= 2.0 && $w < 3.0) {
        $t = 0.38 + $f*0.58; $R = 1.08;
    } elsif ($w >= 3.0 && $w < 4.0) {
        $t = 0.96 + $f*0.48; $R = 1.08;
    } elsif ($w >= 4.0 && $w < 5.0) {
        $t= 1.44 + $f*0.46; $R = 1.08;
    } elsif ($w >= 5.0 && $w < 6.0) {
        $t= 1.9 + $f*0.6; $R = 1.08;
    } elsif ($w >= 6.0) {
        my $ff = $w - 6.0;
        $t = 2.5 + $ff*0.50; $R = 1.08;
    }
    #printf( "w=%f, f=%f, C=%.4f\n", $w, $f, $C);
    
    # determine angle of the line to horizontal
    my $tx = 0; my $ty = 0; # core thinkness of a line
    my $Rx = 0; my $Ry = 0; # fading edge of a line
    my $cx = 0; my $cy = 0; # cap of a line
    my $ALW = 0.01;
    my $dx = $x2 - $x1;
    my $dy = $y2 - $y1;
    if (abs($dx) < $ALW) {
        # vertical
        $tx = $t; $ty = 0;
        $Rx = $R; $Ry = 0;
        if ($w > 0.0 && $w < 1.0) {
            $tx *= 8;
        } elsif ($w == 1.0) {
            $tx *= 10;
        }
    } elsif (abs($dy) < $ALW) {
        #horizontal
        $tx = 0; $ty = $t;
        $Rx = 0; $Ry = $R;
        if ($w > 0.0 && $w < 1.0) {
            $ty *= 8;
        } elsif ($w == 1.0) {
            $ty *= 10;
        }
    } else {
        if ($w < 3) { # approximate to make things even faster
            my $m = $dy/$dx;
            # and calculate tx,ty,Rx,Ry
            if ($m > -0.4142 && $m <= 0.4142) {
                # -22.5 < $angle <= 22.5, approximate to 0 (degree)
                $tx = $t * 0.1; $ty = $t;
                $Rx = $R * 0.6; $Ry = $R;
            } elsif ($m > 0.4142 && $m <= 2.4142) {
                # 22.5 < $angle <= 67.5, approximate to 45 (degree)
                $tx = $t * -0.7071; $ty = $t * 0.7071;
                $Rx = $R * -0.7071; $Ry = $R * 0.7071;
            } elsif ($m > 2.4142 || $m <= -2.4142) {
                # 67.5 < $angle <= 112.5, approximate to 90 (degree)
                $tx = $t; $ty = $t*0.1;
                $Rx = $R; $Ry = $R*0.6;
            } elsif ($m > -2.4142 && $m < -0.4142) {
                # 112.5 < angle < 157.5, approximate to 135 (degree)
                $tx = $t * 0.7071; $ty = $t * 0.7071;
                $Rx = $R * 0.7071; $Ry = $R * 0.7071;
            } else {
                # error in determining angle
                printf("error in determining angle: m=%.4f\n", $m);
            }
        } else {  # calculate to exact
            $dx= $y1 - $y2;
            $dy= $x2 - $x1;
            my $L = sqrt($dx*$dx + $dy*$dy);
            $dx /= $L;
            $dy /= $L;
            $cx = -0.6*$dy; $cy=0.6*$dx;
            $tx = $t*$dx; $ty = $t*$dy;
            $Rx = $R*$dx; $Ry = $R*$dy;
        }
    }

    # draw the line by triangle strip
    glBegin(GL_TRIANGLE_STRIP);
    if (!$alphablend) {
        glColor3f($Br, $Bg, $Bb);
    } else {
        glColor4f($Cr, $Cg, $Cb, 0);
    }
    glVertex2f($x1 - $tx - $Rx, $y1 - $ty - $Ry);   # fading edge
    glVertex2f($x2 - $tx - $Rx, $y2 - $ty - $Ry);
    
    if (!$alphablend) {
        glColor3f($Cr, $Cg, $Cb);
    } else {
        glColor4f($Cr, $Cg, $Cb, $A);
    }
    glVertex2f($x1 - $tx, $y1 - $ty); # core
    glVertex2f($x2 - $tx, $y2 - $ty);
    glVertex2f($x1 + $tx, $y1 + $ty);
    glVertex2f($x2 + $tx, $y2 + $ty);
    
    if ((abs($dx) < $ALW || abs($dy) < $ALW) && $w <= 1.0) {
        # printf("skipped one fading edge\n");
    } else {
        if (!$alphablend) {
            glColor3f($Br, $Bg, $Bb);
        } else {
            glColor4f($Cr, $Cg, $Cb, 0);
        }
        glVertex2f($x1 + $tx+ $Rx, $y1 + $ty + $Ry);    # fading edge
        glVertex2f($x2 + $tx+ $Rx, $y2 + $ty + $Ry);
    }
    glEnd();

    # cap
    if ($w < 3) {
        # do not draw cap
    } else {
        # draw cap
        glBegin(GL_TRIANGLE_STRIP);
        if (!$alphablend) {
            glColor3f($Br, $Bg, $Bb);
        } else {
            glColor4f($Cr, $Cg, $Cb, 0);
        }
        glVertex2f($x1 - $Rx + $cx, $y1 - $Ry + $cy);
        glVertex2f($x1 + $Rx + $cx, $y1 + $Ry + $cy);
        glColor3f($Cr, $Cg, $Cb);
        glVertex2f($x1 - $tx - $Rx, $y1 - $ty - $Ry);
        glVertex2f($x1 + $tx + $Rx, $y1 + $ty + $Ry);
        glEnd();
        glBegin(GL_TRIANGLE_STRIP);
        if (!$alphablend) {
            glColor3f($Br, $Bg, $Bb);
        } else {
            glColor4f($Cr, $Cg, $Cb, 0);
        }
        glVertex2f($x2 - $Rx - $cx, $y2 - $Ry - $cy);
        glVertex2f($x2 + $Rx - $cx, $y2 + $Ry - $cy);
        glColor3f($Cr, $Cg, $Cb);
        glVertex2f($x2 - $tx - $Rx, $y2 - $ty - $Ry);
        glVertex2f($x2 + $tx + $Rx, $y2 + $ty + $Ry);
        glEnd();
    }
}


package Slic3r::GUI::Plater::2DToolpaths::Dialog;

use Wx qw(:dialog :id :misc :sizer);
use Wx::Event qw(EVT_CLOSE);
use base 'Wx::Dialog';

sub new {
    my $class = shift;
    my ($parent, $print) = @_;
    my $self = $class->SUPER::new($parent, -1, "Toolpaths", wxDefaultPosition, [500,500], wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER);
    
    my $sizer = Wx::BoxSizer->new(wxVERTICAL);
    $sizer->Add(Slic3r::GUI::Plater::2DToolpaths->new($self, $print), 1, wxEXPAND, 0);
    $self->SetSizer($sizer);
    $self->SetMinSize($self->GetSize);
    
    # needed to actually free memory
    EVT_CLOSE($self, sub {
        $self->EndModal(wxID_OK);
        $self->Destroy;
    });
    
    return $self;
}

1;
