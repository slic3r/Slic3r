
# Building Slic3r++ on Microsoft Windows

The currently supported way of building Slic3r++ on Windows is with CMake and MS Visual Studio 2017.
You can use the free [Visual Studio 2017 Community Edition](https://www.visualstudio.com/vs/older-downloads/).
CMake installer can be downloaded from [the official website](https://cmake.org/download/).~~

Building with newer versions of MSVS (2019) should work too.


### Building the dependencies package yourself

The dependencies package is built using CMake scripts inside the `deps` subdirectory of Slic3r++ sources.
(This is intentionally not interconnected with the CMake scripts in the rest of the sources.)

Open the preferred Visual Studio command line (64 or 32 bit variant, or just a cmd window) and `cd` into the directory with Slic3r++ sources.
Then `cd` into the `deps` directory and use these commands to build:

    mkdir build
    cd build
    cmake .. -G "Visual Studio 15 Win64" -DDESTDIR="C:\local\destdir-custom"
    msbuild /m ALL_BUILD.vcxproj
	
You can also use the Visual Studio GUI or other generators as mentioned below.

Note that if you're building a 32-bit variant, you will need to change the `"Visual Studio 15 Win64"` to just `"Visual Studio 15"`.

Conversely, if you're using Visual Studio version other than 2017, the version number will need to be changed accordingly (-G "Visual Studio 16 2019" -A "x64" for Visual Studio 2019 Community).

The `DESTDIR` option is the location where the bundle will be installed.
This may be customized. If you leave it empty, the `DESTDIR` will be placed inside the same `build` directory.

Warning: If the `build` directory is nested too deep inside other folders, various file paths during the build become too long and the build might fail due to file writing errors (\*). For this reason, it is recommended to place the `build` directory relatively close to the drive root.

Note that the build variant that you may choose using Visual Studio (i.e. _Release_ or _Debug_ etc.) when building the dependency package is **not relevant**.
The dependency build will by default build _both_ the _Release_ and _Debug_ variants regardless of what you choose in Visual Studio.
You can disable building of the debug variant by passing the

    -DDEP_DEBUG=OFF

option to CMake, this will only produce a _Release_ build.

Refer to the CMake scripts inside the `deps` directory to see which dependencies are built in what versions and how this is done.

* Specifically, the problem arises when building boost. Boost build tool appends all build options into paths of intermediate files, which are not handled correctly by either `b2.exe` or possibly `ninja` (?).


### Building Slic3r++ with Visual Studio

First obtain the Slic3r++ sources via either git or by extracting the source archive.

Then you will need to note down the so-called 'prefix path' to the dependencies, this is the location of the dependencies packages + `\usr\local` appended.

When ready, open the relevant Visual Studio command line and `cd` into the directory with Slic3r++ sources.
Use these commands to prepare Visual Studio solution file:

    mkdir build
    cd build
    cmake .. -G "Visual Studio 15 Win64" -DCMAKE_PREFIX_PATH="C:\local\destdir-custom\usr\local"

Note that the '-G "Visual Studio 15 Win64"' have to be the same as the one you sue for building the dependencies. So replace it the same way your replace it when you built the dependencies (if you did).

If `cmake` has finished without errors, go to the build directory and open the `Slic3r++.sln` solution file in Visual Studio.
Before building, make sure you're building the right project (use one of those starting with `Slic3r++_app_...`) and that you're building with the right configuration, i.e. _Release_ vs. _Debug_. When unsure, choose _Release_.
Note that you won't be able to build a _Debug_ variant against a _Release_-only dependencies package.

#### Installing using the `INSTALL` project

Slic3r++ can be run from the Visual Studio or from Visual Studio's build directory (`src\Release` or `src\Debug`), but for longer-term usage you might want to install somewhere using the `INSTALL` project.
By default, this installs into `C:\Program Files\Slic3r++`.
To customize the install path, use the `-DCMAKE_INSTALL_PREFIX=<path of your choice>` when invoking `cmake`.

### Building from the command line

There are several options for building from the command line:

- [msbuild](https://docs.microsoft.com/en-us/visualstudio/msbuild/msbuild-reference?view=vs-2017&viewFallbackFrom=vs-2013)
- [Ninja](https://ninja-build.org/)
- [nmake](https://docs.microsoft.com/en-us/cpp/build/nmake-reference?view=vs-2017)

To build with msbuild, use the same CMake command as in previous paragraph and then build using

    msbuild /m /P:Configuration=Release ALL_BUILD.vcxproj

To build with Ninja or nmake, replace the `-G` option in the CMake call with `-G Ninja` or `-G "NMake Makefiles"` , respectively.
Then use either `ninja` or `nmake` to start the build.

To install, use `msbuild /P:Configuration=Release INSTALL.vcxproj` , `ninja install` , or `nmake install` .

### building tests

You must use visual studio 2017 or higher, and build all deps & all projects with it (not the same compiler as vs 2013). Also, you have to add the "CRT SDK" to your installation (can be done via the VS intaller).
Tested only with vs 2017. Should work with 2015 and 2019.

