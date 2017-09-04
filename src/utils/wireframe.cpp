#include "Config.hpp"
#include "Geometry.hpp"
#include "IO.hpp"
#include "Model.hpp"
#include "WireframePrint.hpp"
#include "TriangleMesh.hpp"
#include "libslic3r.h"
#include <cstdio>
#include <string>
#include <cstring>
#include <iostream>
#include <math.h>
#include <boost/filesystem.hpp>
#include <boost/nowide/args.hpp>
#include <boost/nowide/iostream.hpp>

using namespace Slic3r;

int
main(int argc, char **argv)
{
	// Convert arguments to UTF-8 (needed on Windows).
    // argv then points to memory owned by a.
    boost::nowide::args a(argc, argv);

    //Read the config options from CLI

    //Read config files

    //Check if wireframe is enabled

	return 0;
}
