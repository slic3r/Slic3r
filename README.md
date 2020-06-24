
# SuperSlicer

**A PrusaSlicer fork (which is a slic3r fork)** (previously Slic3r++)

Prebuilt Windows, Linux and macos 64b releases are available through the [git releases page](https://github.com/supermerill/SuperSlicer/releases). The linux & macos aren't tested (by me), just compiled, so report me any bugs that can occur on them.
Nightly builds are available through the [git actions page](https://github.com/supermerill/SuperSlicer/actions). Click on the a build for your plateform and then on 'Artifacts (1)' in the top right corner.

SuperSlicer takes 3D models (STL, OBJ, AMF) and converts them into G-code
instructions for FFF printers or PNG layers for mSLA 3D printers. It's
compatible with any modern printer based on the RepRap toolchain, including all
those based on the Marlin, Prusa, Klipper and more firmware.

SuperSlicer is based on [PrusaSlicer](https://github.com/prusa3d/PrusaSlicer) by Prusa Research.
PrusaSlicer is based on [Slic3r](https://github.com/Slic3r/Slic3r) by Alessandro Ranelucci and the RepRap community.

See the [wiki](https://github.com/supermerill/SuperSlicer/wiki) and
the [documentation directory](doc/) for information about compilation.

### What are SuperSlicer main features? main new features/differences?

* Custom-made generated calibration tests.
* **Ironing** top surface & many new settings to fine-tune the top surface quality, like 'only one perimeter on top'.
* A "denser infill" option for supporting the (solid) top layers.
* Better **Thin walls** (anchored inside the print, no more random bits at the ends, embedded in perimeter loops).
* Options to change holes dimensions and/or geometry, to print them at the right size.
* Better overhangs (add perimeters if needed, slice them in opposite direction each layer).
* Brim rework: many more options (inside, outside only, 'ears', per object)
* Some new seam options, to help hide them.
* built-in calibration prints
* built-in object generation script
* Can join perimeters into a big one to avoid travel moves.
* Many other little options and corrections (like the filled concentric pattern).
* It has also all the current slic3rPE/PrusaSlicer features.

### Complete changelog [here](https://github.com/supermerill/SuperSlicer/wiki)

See the wiki for examples.

### What are SuperSlicer / PrusaSlicer / Slic3r's main features?

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

[Command Line Interface](https://github.com/prusa3d/PrusaSlicer/wiki/Command-Line-Interface) wiki page.

## Development

### What language is it written in?

Almost everything are written in C++,

The slicing core is the `libslic3r` library, which can be built and used in a standalone way.
The command line interface is a thin wrapper over `libslic3r`.
You can download a precompiled package from the release page.
it will run without the need for any dependency.

### How to compile

If you want to compile the source yourself, follow the instructions on one of
these documentation pages:
* [Linux](doc/How%20to%20build%20-%20Linux%20et%20al.md)
* [macOS](doc/How%20to%20build%20-%20Mac%20OS.md)
* [Windows](doc/How%20to%20build%20-%20Windows.md)

Also, you can look at the worklow yaml files for [git actions](https://github.com/supermerill/Slic3r/tree/master/.github/workflows), as they describe how to build from source from a "virgin" dev computer.

### Can I help?

Sure! You can do the following to find things that are available to help with:
* Add an issue to the github tracker **if it isn't already present**.

Before sending patches and pull requests contact me (preferably through opening a github issue or commenting on an existing, related, issue) to discuss your proposed
changes: this way we'll ensure nobody wastes their time and no conflicts arise in development.

## Licence and attribution

SuperSlicer is licensed under the _GNU Affero General Public License, version 3_.
The SuperSlicer is based on PrusaSlicer by PrusaResearch.

PrusaSlicer is licensed under the _GNU Affero General Public License, version 3_.
PrusaSlicer is owned by Prusa Research.
PrusaSlicer is originally based on Slic3r by Alessandro Ranellucci.

Slic3r is licensed under the _GNU Affero General Public License, version 3_.
The first author is Alessandro Ranellucci, and many contributors

The _GNU Affero General Public License, version 3_ ensure that if you **use** any part of this software in any way (even behind a web server), your software must be released under the same licence.
