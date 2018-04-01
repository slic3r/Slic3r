package Slic3r::GUI::ColorScheme;
use strict;
use warnings;

use POSIX;
use vars qw(@ISA @EXPORT);
use Exporter 'import';
our @ISA = 'Exporter';
our @EXPORT = qw($DEFAULT_COLORSCHEME $SOLID_BACKGROUNDCOLOR @SELECTED_COLOR @HOVER_COLOR @TOP_COLOR @BOTTOM_COLOR @GRID_COLOR @GROUND_COLOR @COLOR_CUTPLANE @COLOR_PARTS @COLOR_INFILL @COLOR_SUPPORT @COLOR_UNKNOWN @BED_COLOR @BED_GRID @BED_SELECTED @BED_OBJECTS @BED_INSTANCE @BED_DRAGGED @BED_CENTER @BED_SKIRT @BED_CLEARANCE @BED_DARK @BACKGROUND255 @TOOL_DARK @TOOL_SUPPORT @TOOL_STEPPERIM @TOOL_INFILL @TOOL_SHADE @TOOL_COLOR @BACKGROUND_COLOR @SPLINE_L_PEN @SPLINE_O_PEN @SPLINE_I_PEN @SPLINE_R_PEN );

# DEFAULT values
our $DEFAULT_COLORSCHEME   = 1;
our $SOLID_BACKGROUNDCOLOR = 0;
our @SELECTED_COLOR   = (0, 1, 0);
our @HOVER_COLOR      = (0.4, 0.9, 0);           # Hover over Model
our @TOP_COLOR        = (10/255,98/255,144/255); # TOP Backgroud color
our @BOTTOM_COLOR     = (0,0,0);                 # BOTTOM Backgroud color
our @BACKGROUND_COLOR = @TOP_COLOR;              # SOLID background color
our @GRID_COLOR       = (0.2, 0.2, 0.2, 0.4);    # Grid color
our @GROUND_COLOR     = (0.8, 0.6, 0.5, 0.4);    # Ground or Plate color
our @COLOR_CUTPLANE   = (.8, .8, .8, 0.5);
our @COLOR_PARTS      = (1, 0.95, 0.2, 1);       # Perimeter color
our @COLOR_INFILL     = (1, 0.45, 0.45, 1);
our @COLOR_SUPPORT    = (0.5, 1, 0.5, 1);
our @COLOR_UNKNOWN    = (0.5, 0.5, 1, 1);
our @BED_COLOR        = (255, 255, 255);
our @BED_GRID         = (230, 230, 230);
our @BED_SELECTED     = (255, 166, 128);
our @BED_OBJECTS      = (210, 210, 210);
our @BED_INSTANCE     = (255, 128, 128);
our @BED_DRAGGED      = (128, 128, 255);
our @BED_CENTER       = (200, 200, 200);
our @BED_SKIRT        = (150, 150, 150);
our @BED_CLEARANCE    = (0, 0, 200);
our @BACKGROUND255    = (255, 255, 255);
our @TOOL_DARK        = (0, 0, 0);
our @TOOL_SUPPORT     = (0, 0, 0);
our @TOOL_INFILL      = (0, 0, 0.7);
our @TOOL_STEPPERIM   = (0.7, 0, 0);
our @TOOL_SHADE       = (0.95, 0.95, 0.95);
our @TOOL_COLOR       = (0.9, 0.9, 0.9);
our @SPLINE_L_PEN     = (50, 50, 50);
our @SPLINE_O_PEN     = (200, 200, 200);
our @SPLINE_I_PEN     = (255, 0, 0);
our @SPLINE_R_PEN     = (5, 120, 160);
our @BED_DARK         = (0, 0, 0);

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

# create your own theme:
# 1. add new sub and name it according to your scheme
# 2. add that name to Preferences.pm
# 3. Choose your newly created theme in Slic3rs' Preferences (File -> Preferences).

sub getSolarized { # add this name to Preferences.pm
    $DEFAULT_COLORSCHEME   = 0;               # DISABLE default color scheme
    $SOLID_BACKGROUNDCOLOR = 1;               # Switch between SOLID or FADED background color
    @SELECTED_COLOR   = @COLOR_MAGENTA;       # Color of selected Model
    @HOVER_COLOR      = @COLOR_VIOLET;        # Color when hovering over Model
    # @TOP_COLOR        = @COLOR_BASE2;         # FADE Background color - only used if $SOLID_BACKGROUNDCOLOR = 0
    # @BOTTOM_COLOR     = @COLOR_BASE02;        # FADE Background color - only used if $SOLID_BACKGROUNDCOLOR = 0
    @BACKGROUND_COLOR = @COLOR_BASE3;         # SOLID Background color  - REQUIRED for NOT getDefault
    @GRID_COLOR       = (@COLOR_BASE1, 0.4);  # Grid
    @GROUND_COLOR     = (@COLOR_BASE2, 0.4);  # Ground or Plate
    @COLOR_CUTPLANE   = (@COLOR_BASE1, 0.5);  # Cut plane
    @COLOR_PARTS      = (@TOOL_COLOR,  1);    # Perimeter
    @COLOR_INFILL     = (@COLOR_BASE2, 1);    # Infill
    @COLOR_SUPPORT    = (@TOOL_SUPPORT, 1);   # Support
    @COLOR_UNKNOWN    = (@COLOR_CYAN,  1);    # I don't know what that color's for!
    
    # 2DBed.pm and ./plater/2D.pm colors
    @BED_COLOR        = map { ceil($_ * 255) } @COLOR_BASE2;      # do math -> multiply each value by 255 and round up
    @BED_GRID         = map { ceil($_ * 255) } @COLOR_BASE1;      # Bed, Ground or Plate
    @BED_SELECTED     = map { ceil($_ * 255) } @COLOR_YELLOW;     # Selected Model
    @BED_INSTANCE     = map { ceil($_ * 255) } @SELECTED_COLOR;   # Real selected Model
    @BED_OBJECTS      = map { ceil($_ * 255) } @COLOR_PARTS;      # Models on bed
    @BED_DRAGGED      = map { ceil($_ * 255) } @COLOR_CYAN;       # Color while dragging
    @BED_CENTER       = map { ceil($_ * 255) } @COLOR_BASE1;      # Cross hair
    @BED_SKIRT        = map { ceil($_ * 255) } @COLOR_BASE01;     # Brim/Skirt
    @BED_CLEARANCE    = map { ceil($_ * 255) } @COLOR_BLUE;       # not sure what that does
    @BED_DARK         = map { ceil($_ * 255) } @COLOR_BASE01;     # not sure what that does
    @BACKGROUND255    = map { ceil($_ * 255) } @BACKGROUND_COLOR; # Backgroud color, this time RGB
    
    # 2DToolpaths.pm colors : LAYERS Tab
    @TOOL_DARK        = @COLOR_BASE01;  # Brim/Skirt
    @TOOL_SUPPORT     = @COLOR_GREEN;   # Support
    @TOOL_INFILL      = @COLOR_BASE1;   # Infill
    @TOOL_COLOR       = @COLOR_BLUE;    # Real Perimeter
    @TOOL_STEPPERIM   = @COLOR_CYAN;    # Inner Perimeter
    @TOOL_SHADE       = @COLOR_BASE2;   # Shade; model inside
    
    # Colors for *Layer heights...* dialog
    @SPLINE_L_PEN     = map { ceil($_ * 255) } @COLOR_BASE01;  # Line color
    @SPLINE_O_PEN     = map { ceil($_ * 255) } @COLOR_BASE1;   # Original color
    @SPLINE_I_PEN     = map { ceil($_ * 255) } @COLOR_MAGENTA; # Interactive color
    @SPLINE_R_PEN     = map { ceil($_ * 255) } @COLOR_VIOLET;  # Resulting color

}

sub getDefault{
    $DEFAULT_COLORSCHEME   = 1;               # ENABLE default color scheme
    # Define your function here
    # getDefault is just a dummy function and uses the default values from above.
}



1; # REQUIRED at EOF
