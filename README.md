![](var/Slic3r_128px.png) Slic3r [![Build Status](https://travis-ci.org/slic3r/Slic3r.svg?branch=master)](https://travis-ci.org/slic3r/Slic3r) [![Build status](https://ci.appveyor.com/api/projects/status/8iqmeat6cj158vo6?svg=true)](https://ci.appveyor.com/project/lordofhyphens/slic3r) [![Build Status](http://osx-build.slic3r.org:8080/buildStatus/icon?job=Slic3r)](http://osx-build.slic3r.org:8080/job/Slic3r)![Coverity Status](https://scan.coverity.com/projects/17257/badge.svg)
======

We have automated builds for Windows (64-bit) and OSX (>= 10.7). [Get a fresh build now](http://dl.slic3r.org/dev/) and stay up-to-date with the development!

The MacOS X build server is kindly sponsored by: <img width=150 src=https://cloud.githubusercontent.com/assets/31754/22719818/09998c92-ed6d-11e6-9fa0-09de638f3a36.png />

### So, what's this Slic3r?

Slic3r is mainly a **toolpath generator** for 3D printers: it reads 3D models (STL, OBJ, AMF, 3MF) and it converts them into **G-code** instructions for 3D printers. But it does much more than that, see the [features list](#features) below.

Slic3r was born in **2011** within the RepRap community and thanks to its high configurability became the swiss-army knife for 3D printing. It served as a platform for implementing several **new (experimental) ideas that later became technology standards**, such as multiple extruders, brim, variable-height layers, per-object settings, modifiers, post-processing scripts, G-code macros and more. Despite being based on volunteer efforts, Slic3r is still pushing the boundaries of 3D printing.

Slic3r is:

* **Open:** it is totally **open source** and it's **independent from any commercial company** or printer manufacturer. We want to keep 3D printing open and free.
* **Compatible:** it supports all the known G-code dialects (Marlin, Repetier, Mach3, LinuxCNC, Machinekit, Smoothie, Makerware, Sailfish).
* **Advanced:** many configuration options allow for fine-tuning and full control. While novice users often need just few options, Slic3r is mostly used by advanced users.
* **Community-driven:** new features or issues are discussed in the [GitHub repository](https://github.com/slic3r/Slic3r/issues). Join our collaborative effort and help improve it!
* **Robust:** the codebase includes more than 1,000 unit and regression tests, collected in 6 years of development.
* **Modular:** the core of Slic3r is libslic3r, a C++ library that provides a granular API and reusable components.
* **Embeddable:** a complete and powerful command line interface allows Slic3r to be used from the shell or integrated with server-side applications.
* **Powerful:** see the list below!

See the [project homepage](https://slic3r.org/) at slic3r.org for more information.

### <a name="features"></a>Features

(Most of these are also available in the command line interface.)

* **G-code generation** for FFF/FDM printers;
* **conversion** between STL, OBJ, AMF, 3MF and POV formats;
* **auto-repair** of non-manifold meshes (and ability to re-export them);
* **SVG export** of slices;
* built-in **USB/serial** host controller, supporting **multiple simultaneous printers** each one with a spool queue;
* **OctoPrint integration** (send to printer);
* built-in **projector and host for DLP printers**;
* tool for **cutting meshes** in multiple solid parts with visual preview (also in batch using a grid);
* tool for **extruding 2.5D TIN meshes**.

### What language is it written in?

The core parts of Slic3r are written in C++11, with multithreading. The graphical interface is in the process of being ported to C++14.

### How to install?

You can download a precompiled package from [slic3r.org](https://slic3r.org/) (releases) or from [dl.slicr3r.org](https://dl.slic3r.org/dev/) (automated builds).

If you want to compile the source yourself follow the instructions on one of these wiki pages:
* [Linux](https://github.com/slic3r/Slic3r/wiki/Running-Slic3r-from-git-on-GNU-Linux)
* [Windows](https://github.com/slic3r/Slic3r/wiki/Running-Slic3r-from-git-on-Windows)
* [Mac OSX](https://github.com/slic3r/Slic3r/wiki/Running-Slic3r-from-git-on-OS-X)

### Can I help?

Sure, but please read the
[CONTRIBUTING](https://github.com/slic3r/Slic3r/blob/master/.github/CONTRIBUTING.md)
document first!

### Directory structure

* `package/`: the scripts used for packaging the executables
* `src/`: the C++ source of the `slic3r` executable and the CMake definition file for compiling it
* `src/GUI`: The C++ GUI.
* `src/test`: New test suite for libslic3r and the GUI. Implemented with [Catch2](https://github.com/catchorg/Catch2)
* `t/`: the test suite (deprecated)
* `utils/`: various useful scripts
* `xs/src/libslic3r/`: C++ sources for libslic3r
* `xs/t/`: test suite for libslic3r (deprecated)
* `xs/xsp/`: bindings for calling libslic3r from Perl (XS) (deprecated)

### Acknowledgements

The main author of Slic3r is Alessandro Ranellucci (@alranel, *Sound* in IRC, [@alranel](http://twitter.com/alranel) on Twitter), who started the project in 2011.

Joseph Lenox (@lordofhyphens, *LoH* in IRC, [@LenoxPlay](http://twitter.com/LenoxPlay) on Twitter) is the current co-maintainer.

Contributions by Henrik Brix Andersen, Vojtech Bubnik, Nicolas Dandrimont, Mark Hindess, Petr Ledvina, Y. Sapir, Mike Sheldrake, Kliment Yanev and numerous others. Original manual by Gary Hodgson. Slic3r logo designed by Corey Daniels, <a href="http://www.famfamfam.com/lab/icons/silk/">Silk Icon Set</a> designed by Mark James, stl and gcode file icons designed by Akira Yasuda.

### How can I invoke Slic3r using the command line?

The command line is documented in the relevant [manual page](https://manual.slic3r.org/advanced/command-line).
