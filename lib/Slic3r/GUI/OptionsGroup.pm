package Slic3r::GUI::OptionsGroup;
use strict;
use warnings;

use Wx qw(:sizer wxSYS_DEFAULT_GUI_FONT);
use Wx::Event qw(EVT_TEXT EVT_CHECKBOX EVT_CHOICE);
use base 'Wx::StaticBoxSizer';

use constant OPT_LABEL_WIDTH => 180;
use constant OPT_FIELD_WIDTH => 200;
use constant OPT_HGAP_WIDTH => 15;

# not very elegant, but this solution is temporary waiting for a better GUI
our @reload_callbacks = ();
our %fields = ();  # $key => [$control]

sub new {
    my $class = shift;
    my ($parent, %p) = @_;
    
    my $box = Wx::StaticBox->new($parent, -1, $p{title});
    my $self = $class->SUPER::new($box, wxVERTICAL);
    
    my $grid_sizer = Wx::GridBagSizer->new(2, OPT_HGAP_WIDTH);
	$grid_sizer->AddGrowableCol(1,1);
	
    #grab the default font, to fix Windows font issues/keep things consistent
    my $bold_font = Wx::SystemSettings::GetFont(wxSYS_DEFAULT_GUI_FONT);
    $bold_font->SetWeight(&Wx::wxFONTWEIGHT_BOLD);

    my $row = 0;
    foreach my $opt_key (@{$p{options}}) {
        my $opt = $Slic3r::Config::Options->{$opt_key};
        my $label = Wx::StaticText->new($parent, -1, "$opt->{label}", Wx::wxDefaultPosition,
            [$p{label_width} || OPT_LABEL_WIDTH, -1]);
        $label->Wrap($p{label_width} || OPT_LABEL_WIDTH);  # needed to avoid Linux/GTK bug
        
        #set the bold font point size to the same size as all the other labels (for consistency)
        $bold_font->SetPointSize($label->GetFont()->GetPointSize());
        $label->SetFont($bold_font) if $opt->{important};
        my $field;
        if ($opt->{type} =~ /^(i|f|s|s@)$/) {
            my $style = 0;
            if ($opt->{multiline}) {
                $style = &Wx::wxTE_MULTILINE;
            }
            my $size = Wx::Size->new($opt->{width} || OPT_FIELD_WIDTH, $opt->{height} || -1);
            
            my ($get, $set) = $opt->{type} eq 's@' ? qw(serialize deserialize) : qw(get set);
            
            $field = Wx::TextCtrl->new($parent, -1, Slic3r::Config->$get($opt_key),
                Wx::wxDefaultPosition, $size, $style);
                
            if ($opt->{tooltip}) {
            	$field->SetToolTipString($opt->{tooltip});
            }
            EVT_TEXT($parent, $field, sub { Slic3r::Config->$set($opt_key, $field->GetValue) });
            push @reload_callbacks, sub { $field->SetValue(Slic3r::Config->$get($opt_key)) };
        } elsif ($opt->{type} eq 'bool') {
            $field = Wx::CheckBox->new($parent, -1, "");
            $field->SetValue(Slic3r::Config->get($opt_key));
            EVT_CHECKBOX($parent, $field, sub { Slic3r::Config->set($opt_key, $field->GetValue) });
            push @reload_callbacks, sub { $field->SetValue(Slic3r::Config->get($opt_key)) };
        } elsif ($opt->{type} eq 'point') {
            $field = Wx::FlexGridSizer->new(1, 4, 2, 0);
			$field->AddGrowableCol(1,1);
			
			my $label_width = 20;
            my $value = Slic3r::Config->get($opt_key);
            
			$field->Add(Wx::StaticText->new($parent, -1, "X:", Wx::wxDefaultPosition, [$label_width, -1]), -1, wxALIGN_CENTER_VERTICAL|wxALIGN_RIGHT );
			my $x_field = Wx::TextCtrl->new($parent, -1, $value->[0], Wx::wxDefaultPosition, [OPT_FIELD_WIDTH/2 - $label_width, -1]);
			$field->Add($x_field, 1);
			$field->Add(Wx::StaticText->new($parent, -1, "  Y:", Wx::wxDefaultPosition, [$label_width, -1]), -1, wxALIGN_CENTER_VERTICAL|wxALIGN_RIGHT);
			my $y_field = Wx::TextCtrl->new($parent, -1, $value->[1], Wx::wxDefaultPosition, [OPT_FIELD_WIDTH/2 - $label_width, -1]);
			$field->Add($y_field, 1);
			
            my $set_value = sub {
                my ($i, $value) = @_;
                my $val = Slic3r::Config->get($opt_key);
                $val->[$i] = $value;
                Slic3r::Config->set($opt_key, $val);
            };
            EVT_TEXT($parent, $x_field, sub { $set_value->(0, $x_field->GetValue) });
            EVT_TEXT($parent, $y_field, sub { $set_value->(1, $y_field->GetValue) });
            push @reload_callbacks, sub {
                my $value = Slic3r::Config->get($opt_key);
                $x_field->SetValue($value->[0]);
                $y_field->SetValue($value->[1]);
            };
            $fields{$opt_key} = [$x_field, $y_field];
        } elsif ($opt->{type} eq 'select') {
            $field = Wx::Choice->new($parent, -1, Wx::wxDefaultPosition, Wx::wxDefaultSize, $opt->{labels} || $opt->{values});
            EVT_CHOICE($parent, $field, sub {
                Slic3r::Config->set($opt_key, $opt->{values}[$field->GetSelection]);
            });
            push @reload_callbacks, sub {
                my $value = Slic3r::Config->get($opt_key);
                $field->SetSelection(grep $opt->{values}[$_] eq $value, 0..$#{$opt->{values}});
            };
            $reload_callbacks[-1]->();
        } else {
            die "Unsupported option type: " . $opt->{type};
        }
        
        if ($opt->{vertical}) {
        	$grid_sizer->Add($label, Wx::GBPosition->new($row++, 0), Wx::GBSpan->new(1, 2), wxALL, 0);
        	$grid_sizer->Add($field, Wx::GBPosition->new($row, 0), Wx::GBSpan->new(1, 2), wxALL, 0);
        	$row++;
        }else{
        	$grid_sizer->Add($label, Wx::GBPosition->new($row, 0), Wx::GBSpan->new(1, 1), wxALL, 0);
        	$grid_sizer->Add($field, Wx::GBPosition->new($row, 1), Wx::GBSpan->new(1, 1), wxALL, 0);
        	$row++;
        }
        
        $fields{$opt_key} ||= [$field];
    }
    
    $self->Add($grid_sizer, 0, wxEXPAND | wxALL, 15);
    
    return $self;
}

1;
