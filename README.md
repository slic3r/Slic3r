_Q: Oh cool, a new fork of slic3r?_

A: Yeah!

Slic3r
======
Prebuilt Windows 64b is available through the [git releases page](https://github.com/supermerill/Slic3r/releases).

<img width=256 src=https://cloud.githubusercontent.com/assets/31754/22719818/09998c92-ed6d-11e6-9fa0-09de638f3a36.png />

Slic3r++ takes 3D models (STL, OBJ, AMF) and converts them into G-code
instructions for FFF printers or PNG layers for mSLA 3D printers. It's
compatible with any modern printer based on the RepRap toolchain, including all
those based on the Marlin, Prusa, Sprinter and Repetier firmware. It also works
with Mach3, LinuxCNC and Machinekit controllers.

Slic3r++ is based on [Slic3r](https://github.com/Slic3r/Slic3r) by Alessandro Ranelucci and the RepRap community.

See the [project homepage](https://www.prusa3d.com/slic3r-prusa-edition/) and
the [documentation directory](doc/) for more information.

### What language is it written in?

Almost everything are written in C++,

The slicing core is the `libslic3r` library, which can be built and used in a standalone way.
The command line interface is a thin wrapper over `libslic3r`.

### What are this fork main features/differences?

* **Ironing** top surface & many new settings to fine-tune the top surface quality.
* A "denser infill" option for supporting the (solid) top layers.
* Better overhangs (add perimeters if needed, slice them in opposite direction each layer).
* Better Thin walls (anchored inside the print, no more random bits at the ends).
* Can join perimeters into a big one to avoid travel moves.
* Many other little options and corrections (like the filled concentric pattern).
* It has also all the current slic3rPE/PrusaSlicer features.

#### Complete changelog [here](https://github.com/supermerill/Slic3r/wiki)

See the wiki for examples.

### What are Slic3r's main features?

Key features are:

* **multi-platform** (Linux/Mac/Win) and packaged as standalone-app with no dependencies required
* complete **command-line interface** to use it with no GUI
* multi-material **(multiple extruders)** object printing
* multiple G-code flavors supported (RepRap, Makerbot, Mach3, Machinekit etc.)
* ability to plate **multiple objects having distinct print settings**
* **multithread** processing
* **STL auto-repair** (tolerance for broken models)
* wide automated unit testing

Other major features are:

* combine infill every 'n' perimeters layer & varying density to speed up printing
* **3D preview** (including multi-material files)
* **multiple layer heights** in a single print
* **spiral vase** mode for bumpless vases
* fine-grained configuration of speed, acceleration, extrusion width
* several infill patterns including honeycomb, spirals, Hilbert curves, gyroid
* support material, raft, brim, skirt
* **standby temperature** and automatic wiping for multi-extruder printing
* [customizable **G-code macros**](https://github.com/prusa3d/PrusaSlicer/wiki/Slic3r-Prusa-Edition-Macro-Language) and output filename with variable placeholders
* support for **post-processing scripts**
* **cooling logic** controlling fan speed and dynamic print speed

### Development

You can download a precompiled package from the release page.
it will run without the need for any dependency.


If you want to compile the source yourself, follow the instructions on one of
these documentation pages:
* [Linux](doc/How%20to%20build%20-%20Linux%20et%20al.md)
* [macOS](doc/How%20to%20build%20-%20Mac%20OS.md)
* [Windows](doc/How%20to%20build%20-%20Windows.md)
### Can I help?

Sure! You can do the following to find things that are available to help with:
* Add an issue to the github tracker if it isn't already present.

Before sending patches and pull requests contact me (preferably through opening a github issue or commenting on an existing, related, issue) to discuss your proposed
changes: this way we'll ensure nobody wastes their time and no conflicts arise in development.

Slic3r++ is licensed under the _GNU Affero General Public License, version 3_.
The Slic3r++ is originally based on Slic3r by Alessandro Ranellucci.

Slic3r is licensed under the _GNU Affero General Public License, version 3_.
The first author is Alessandro Ranellucci, and many contributors
Then the Prusa team
Then Durand remi for this fork

Please refer to the [Command Line Interface](https://github.com/prusa3d/PrusaSlicer/wiki/Command-Line-Interface) wiki page.
