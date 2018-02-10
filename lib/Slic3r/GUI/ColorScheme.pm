package Slic3r::GUI::ColorScheme;
use strict;
use warnings;

use vars qw(@ISA @EXPORT);
use Exporter 'import';
our @ISA = 'Exporter';
our @EXPORT = qw(@SELECTED_COLOR @HOVER_COLOR @TOP_COLOR @BOTTOM_COLOR @GRID_COLOR @GROUND_COLOR @COLOR_CUTPLANE @COLOR_PARTS @COLOR_INFILL @COLOR_SUPPORT @COLOR_UNKNOWN @BED_COLOR @BED_GRID @BED_SELECTED @BED_OBJECTS);

# DEFAULT values
our @SELECTED_COLOR  = (0, 1, 0);
our @HOVER_COLOR     = (0.4, 0.9, 0);           # Hover over Model
our @TOP_COLOR       = (10/255,98/255,144/255); # Backgroud color
our @BOTTOM_COLOR    = (0,0,0);                 # Backgroud color
our @GRID_COLOR      = (0.2, 0.2, 0.2, 0.4);    # Grid color
our @GROUND_COLOR    = (0.8, 0.6, 0.5, 0.4);    # Ground or Plate color
our @COLOR_CUTPLANE  = (.8, .8, .8, 0.5);
our @COLOR_PARTS     = (1, 0.95, 0.2, 1);       # Perimeter color
our @COLOR_INFILL    = (1, 0.45, 0.45, 1);
our @COLOR_SUPPORT   = (0.5, 1, 0.5, 1);
our @COLOR_UNKNOWN   = (0.5, 0.5, 1, 1);
our @BED_COLOR       = (255, 255, 255);
our @BED_GRID        = (230, 230, 230);
our @BED_SELECTED    = (255, 166, 128);
our @BED_OBJECTS     = (210, 210, 210);

# S O L A R I Z E
# # http://ethanschoonover.com/solarized
our @COLOR_BASE03    = (0.00000,0.16863,0.21176);
our @COLOR_BASE02    = (0.02745,0.21176,0.25882);
our @COLOR_BASE01    = (0.34510,0.43137,0.45882);
our @COLOR_BASE00    = (0.39608,0.48235,0.51373);
our @COLOR_BASE0     = (0.51373,0.58039,0.58824);
our @COLOR_BASE1     = (0.57647,0.63137,0.63137);
our @COLOR_BASE2     = (0.93333,0.90980,0.83529);
our @COLOR_BASE3     = (0.99216,0.96471,0.89020);
our @COLOR_YELLOW    = (0.70980,0.53725,0.00000);
our @COLOR_ORANGE    = (0.79608,0.29412,0.08627);
our @COLOR_RED       = (0.86275,0.19608,0.18431);
our @COLOR_MAGENTA   = (0.82745,0.21176,0.50980);
our @COLOR_VIOLET    = (0.42353,0.44314,0.76863);
our @COLOR_BLUE      = (0.14902,0.54510,0.82353);
our @COLOR_CYAN      = (0.16471,0.63137,0.59608);
our @COLOR_GREEN     = (0.52157,0.60000,0.00000);

sub getSOLARIZEDColorScheme {
    @SELECTED_COLOR = @COLOR_MAGENTA;
    @HOVER_COLOR    = @COLOR_VIOLET;        # Hover over Model
    @TOP_COLOR      = @COLOR_BASE3;         # Backgroud color
    @BOTTOM_COLOR   = @COLOR_BASE3;         # Backgroud color
    @GRID_COLOR     = (@COLOR_BASE02, 0.4); # Grid color
    @GROUND_COLOR   = (@COLOR_BASE2,  0.4);  # Ground or Plate color
    @COLOR_CUTPLANE = (@COLOR_BASE1,  0.5);
    @COLOR_PARTS    = (@COLOR_BLUE,   1);     # Perimeter color
    @COLOR_INFILL   = (@COLOR_BASE2,  1);
    @COLOR_SUPPORT  = (@COLOR_ORANGE, 1);
    @COLOR_UNKNOWN  = (@COLOR_CYAN,   1);
    @BED_COLOR      = map { $_ * 255 } @COLOR_BASE2;
    @BED_GRID       = map { $_ * 255 } @COLOR_BASE02;
    @BED_SELECTED   = map { $_ * 255 } @SELECTED_COLOR;
    @BED_OBJECTS    = map { $_ * 255 } @COLOR_PARTS;
}

1;
