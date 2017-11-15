# Enable C++11 language standard.
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add our own cmake module path.
list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake/modules/)

if (CMAKE_SYSTEM_NAME STREQUAL "Linux")
    # Workaround for an old CMake, which does not understand CMAKE_CXX_STANDARD.
    add_compile_options(-std=c++11 -Wall -Wreorder)
endif()

if (CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_GNUXX)
    # Adding -fext-numeric-literals to enable GCC extensions on definitions of quad float literals, which are required by Boost.
    add_compile_options(-fext-numeric-literals)
endif()

# Where all the bundled libraries reside?
set(LIBDIR ${CMAKE_CURRENT_SOURCE_DIR}/src/)
# For the bundled boost libraries (boost::nowide)
include_directories(${LIBDIR})

# For libslic3r.h
include_directories(${LIBDIR}/libslic3r)
#set(CMAKE_INCLUDE_CURRENT_DIR ON)

if(WIN32)
    # BOOST_ALL_NO_LIB: Avoid the automatic linking of Boost libraries on Windows. Rather rely on explicit linking.
    add_definitions(-D_USE_MATH_DEFINES -D_WIN32 -DBOOST_ALL_NO_LIB)
    include_directories(${PERL_INCLUDE_PATH})
    include_directories(C:/Strawberry/c/include)
    add_definitions(-Isrc -Isrc/libslic3r -O -O2 -U__STRICT_ANSI__)
    add_definitions(-D_USE_MATH_DEFINES -D_WIN32 -DBOOST_ALL_NO_LIB -I${PERL_INCLUDE_PATH} -D_GLIBCXX_USE_C99 )
endif()

add_definitions(-DwxUSE_UNICODE -D_UNICODE -DUNICODE)

# Disable features of exprtk to reduce size.
add_definitions(-Dexprtk_disable_rtl_io_file -Dexprtk_disable_return_statement -Dexprtk_disable_rtl_vecops -Dexprtk_disable_string_capabilities -Dexprtk_disable_enhanced_features)

add_library(libslic3r STATIC
    ${LIBDIR}/libslic3r/BoundingBox.cpp
    ${LIBDIR}/libslic3r/BoundingBox.hpp
    ${LIBDIR}/libslic3r/BridgeDetector.cpp
    ${LIBDIR}/libslic3r/BridgeDetector.hpp
    ${LIBDIR}/libslic3r/ClipperUtils.cpp
    ${LIBDIR}/libslic3r/ClipperUtils.hpp
    ${LIBDIR}/libslic3r/ConditionalGCode.hpp
    ${LIBDIR}/libslic3r/ConditionalGCode.cpp
    ${LIBDIR}/libslic3r/Config.cpp
    ${LIBDIR}/libslic3r/Config.hpp
    ${LIBDIR}/libslic3r/ExPolygon.cpp
    ${LIBDIR}/libslic3r/ExPolygon.hpp
    ${LIBDIR}/libslic3r/ExPolygonCollection.cpp
    ${LIBDIR}/libslic3r/ExPolygonCollection.hpp
    ${LIBDIR}/libslic3r/Extruder.cpp
    ${LIBDIR}/libslic3r/Extruder.hpp
    ${LIBDIR}/libslic3r/ExtrusionEntity.cpp
    ${LIBDIR}/libslic3r/ExtrusionEntity.hpp
    ${LIBDIR}/libslic3r/ExtrusionEntityCollection.cpp
    ${LIBDIR}/libslic3r/ExtrusionEntityCollection.hpp
    ${LIBDIR}/libslic3r/Fill/Fill.cpp
    ${LIBDIR}/libslic3r/Fill/Fill.hpp
    ${LIBDIR}/libslic3r/Fill/Fill3DHoneycomb.cpp
    ${LIBDIR}/libslic3r/Fill/Fill3DHoneycomb.hpp
    ${LIBDIR}/libslic3r/Fill/FillConcentric.cpp
    ${LIBDIR}/libslic3r/Fill/FillConcentric.hpp
    ${LIBDIR}/libslic3r/Fill/FillHoneycomb.cpp
    ${LIBDIR}/libslic3r/Fill/FillHoneycomb.hpp
    ${LIBDIR}/libslic3r/Fill/FillPlanePath.cpp
    ${LIBDIR}/libslic3r/Fill/FillPlanePath.hpp
    ${LIBDIR}/libslic3r/Fill/FillRectilinear.cpp
    ${LIBDIR}/libslic3r/Fill/FillRectilinear.hpp
    ${LIBDIR}/libslic3r/Flow.cpp
    ${LIBDIR}/libslic3r/Flow.hpp
    ${LIBDIR}/libslic3r/GCode.cpp
    ${LIBDIR}/libslic3r/GCode.hpp
    ${LIBDIR}/libslic3r/GCode/CoolingBuffer.cpp
    ${LIBDIR}/libslic3r/GCode/CoolingBuffer.hpp
    ${LIBDIR}/libslic3r/GCode/SpiralVase.cpp
    ${LIBDIR}/libslic3r/GCode/SpiralVase.hpp
    ${LIBDIR}/libslic3r/GCodeReader.cpp
    ${LIBDIR}/libslic3r/GCodeReader.hpp
    ${LIBDIR}/libslic3r/GCodeSender.cpp
    ${LIBDIR}/libslic3r/GCodeSender.hpp
    ${LIBDIR}/libslic3r/GCodeTimeEstimator.cpp
    ${LIBDIR}/libslic3r/GCodeTimeEstimator.hpp
    ${LIBDIR}/libslic3r/GCodeWriter.cpp
    ${LIBDIR}/libslic3r/GCodeWriter.hpp
    ${LIBDIR}/libslic3r/Geometry.cpp
    ${LIBDIR}/libslic3r/Geometry.hpp
    ${LIBDIR}/libslic3r/IO.cpp
    ${LIBDIR}/libslic3r/IO.hpp
    ${LIBDIR}/libslic3r/IO/AMF.cpp
    ${LIBDIR}/libslic3r/IO/TMF.cpp
    ${LIBDIR}/libslic3r/IO/TMF.hpp
    ${LIBDIR}/libslic3r/Layer.cpp
    ${LIBDIR}/libslic3r/Layer.hpp
    ${LIBDIR}/libslic3r/LayerRegion.cpp
    ${LIBDIR}/libslic3r/LayerRegionFill.cpp
    ${LIBDIR}/libslic3r/LayerHeightSpline.cpp
    ${LIBDIR}/libslic3r/LayerHeightSpline.hpp
    ${LIBDIR}/libslic3r/Line.cpp
    ${LIBDIR}/libslic3r/Line.hpp
    ${LIBDIR}/libslic3r/Model.cpp
    ${LIBDIR}/libslic3r/Model.hpp
    ${LIBDIR}/libslic3r/MotionPlanner.cpp
    ${LIBDIR}/libslic3r/MotionPlanner.hpp
    ${LIBDIR}/libslic3r/MultiPoint.cpp
    ${LIBDIR}/libslic3r/MultiPoint.hpp
    ${LIBDIR}/libslic3r/PerimeterGenerator.cpp
    ${LIBDIR}/libslic3r/PerimeterGenerator.hpp
    ${LIBDIR}/libslic3r/PlaceholderParser.cpp
    ${LIBDIR}/libslic3r/PlaceholderParser.hpp
    ${LIBDIR}/libslic3r/Point.cpp
    ${LIBDIR}/libslic3r/Point.hpp
    ${LIBDIR}/libslic3r/Polygon.cpp
    ${LIBDIR}/libslic3r/Polygon.hpp
    ${LIBDIR}/libslic3r/Polyline.cpp
    ${LIBDIR}/libslic3r/Polyline.hpp
    ${LIBDIR}/libslic3r/PolylineCollection.cpp
    ${LIBDIR}/libslic3r/PolylineCollection.hpp
    ${LIBDIR}/libslic3r/Print.cpp
    ${LIBDIR}/libslic3r/Print.hpp
    ${LIBDIR}/libslic3r/PrintConfig.cpp
    ${LIBDIR}/libslic3r/PrintConfig.hpp
    ${LIBDIR}/libslic3r/PrintObject.cpp
    ${LIBDIR}/libslic3r/PrintRegion.cpp
    ${LIBDIR}/libslic3r/SLAPrint.cpp
    ${LIBDIR}/libslic3r/SLAPrint.hpp
    ${LIBDIR}/libslic3r/SVG.cpp
    ${LIBDIR}/libslic3r/SVG.hpp
    ${LIBDIR}/libslic3r/SlicingAdaptive.cpp
    ${LIBDIR}/libslic3r/SlicingAdaptive.hpp
    ${LIBDIR}/libslic3r/SupportMaterial.hpp
    ${LIBDIR}/libslic3r/Surface.cpp
    ${LIBDIR}/libslic3r/Surface.hpp
    ${LIBDIR}/libslic3r/SurfaceCollection.cpp
    ${LIBDIR}/libslic3r/SurfaceCollection.hpp
    ${LIBDIR}/libslic3r/TriangleMesh.cpp
    ${LIBDIR}/libslic3r/TriangleMesh.hpp
    ${LIBDIR}/libslic3r/libslic3r.h
)

add_library(libslic3r_gui STATIC
    ${LIBDIR}/slic3r/GUI/3DScene.cpp
    ${LIBDIR}/slic3r/GUI/3DScene.hpp
    ${LIBDIR}/slic3r/GUI/GUI.cpp
    ${LIBDIR}/slic3r/GUI/GUI.hpp
)

add_library(admesh STATIC
    ${LIBDIR}/admesh/connect.c
    ${LIBDIR}/admesh/normals.c
    ${LIBDIR}/admesh/shared.c
    ${LIBDIR}/admesh/stl.h
    ${LIBDIR}/admesh/stl_io.c
    ${LIBDIR}/admesh/stlinit.c
    ${LIBDIR}/admesh/util.c
)

add_library(clipper STATIC 
    ${LIBDIR}/clipper.cpp
    ${LIBDIR}/clipper.hpp
)

add_library(ZipArchive STATIC 
    ${LIBDIR}/Zip/ZipArchive.cpp
    ${LIBDIR}/Zip/ZipArchive.hpp
)

add_library(BSpline STATIC 
    ${LIBDIR}/BSpline/BSpline.cpp
    ${LIBDIR}/BSpline/BSpline.h
    ${LIBDIR}/BSpline/BandedMatrix.h
)

add_library(polypartition STATIC 
    ${LIBDIR}/polypartition.cpp
    ${LIBDIR}/polypartition.h
)

add_library(poly2tri STATIC
    ${LIBDIR}/poly2tri/common/shapes.cc
    ${LIBDIR}/poly2tri/common/shapes.h
    ${LIBDIR}/poly2tri/common/utils.h
    ${LIBDIR}/poly2tri/poly2tri.h
    ${LIBDIR}/poly2tri/sweep/advancing_front.cc
    ${LIBDIR}/poly2tri/sweep/advancing_front.h
    ${LIBDIR}/poly2tri/sweep/cdt.cc
    ${LIBDIR}/poly2tri/sweep/cdt.h
    ${LIBDIR}/poly2tri/sweep/sweep.cc
    ${LIBDIR}/poly2tri/sweep/sweep.h
    ${LIBDIR}/poly2tri/sweep/sweep_context.cc
    ${LIBDIR}/poly2tri/sweep/sweep_context.h
)
SET_TARGET_PROPERTIES(admesh PROPERTIES LINKER_LANGUAGE C)
add_library(nowide STATIC
    ${LIBDIR}/boost/nowide/args.hpp
    ${LIBDIR}/boost/nowide/cenv.hpp
    ${LIBDIR}/boost/nowide/config.hpp
    ${LIBDIR}/boost/nowide/convert.hpp
    ${LIBDIR}/boost/nowide/cstdio.hpp
    ${LIBDIR}/boost/nowide/cstdlib.hpp
    ${LIBDIR}/boost/nowide/filebuf.hpp
    ${LIBDIR}/boost/nowide/fstream.hpp
    ${LIBDIR}/boost/nowide/integration/filesystem.hpp
    ${LIBDIR}/boost/nowide/iostream.cpp
    ${LIBDIR}/boost/nowide/iostream.hpp
    ${LIBDIR}/boost/nowide/stackstring.hpp
    ${LIBDIR}/boost/nowide/system.hpp
    ${LIBDIR}/boost/nowide/utf8_codecvt.hpp
    ${LIBDIR}/boost/nowide/windows.hpp
)

# Generate the Slic3r Perl module (XS) typemap file.
set(MyTypemap ${CMAKE_CURRENT_BINARY_DIR}/typemap)
add_custom_command(
        OUTPUT ${MyTypemap}
        DEPENDS ${CMAKE_CURRENT_LIST_DIR}/xsp/my.map
        COMMAND ${PERL_EXECUTABLE} -MExtUtils::Typemaps -MExtUtils::Typemaps::Basic -e "$typemap = ExtUtils::Typemaps->new(file => \"${CMAKE_CURRENT_LIST_DIR}/xsp/my.map\"); $typemap->merge(typemap => ExtUtils::Typemaps::Basic->new); $typemap->write(file => \"${MyTypemap}\")"
        VERBATIM
)

# Generate the Slic3r Perl module (XS) main.xs file.
set(XS_MAIN_XS ${CMAKE_CURRENT_BINARY_DIR}/main.xs)
set(XSP_DIR ${CMAKE_CURRENT_SOURCE_DIR}/xsp)
#FIXME list the dependecies explicitely, add dependency on the typemap.
set(XS_XSP_FILES
    ${XSP_DIR}/BoundingBox.xsp
    ${XSP_DIR}/BridgeDetector.xsp
    ${XSP_DIR}/Clipper.xsp
    ${XSP_DIR}/ConditionalGCode.xsp
    ${XSP_DIR}/Config.xsp
    ${XSP_DIR}/ExPolygon.xsp
    ${XSP_DIR}/ExPolygonCollection.xsp
    ${XSP_DIR}/Extruder.xsp
    ${XSP_DIR}/ExtrusionEntityCollection.xsp
    ${XSP_DIR}/ExtrusionLoop.xsp
    ${XSP_DIR}/ExtrusionPath.xsp
    ${XSP_DIR}/Filler.xsp
    ${XSP_DIR}/Flow.xsp
    ${XSP_DIR}/GCode.xsp
    ${XSP_DIR}/GCodeSender.xsp
    ${XSP_DIR}/GCodeTimeEstimator.xsp
    ${XSP_DIR}/GCodeWriter.xsp
    ${XSP_DIR}/GUI.xsp
    ${XSP_DIR}/GUI_3DScene.xsp
    ${XSP_DIR}/Geometry.xsp
    ${XSP_DIR}/Layer.xsp
    ${XSP_DIR}/LayerHeightSpline.xsp
    ${XSP_DIR}/Line.xsp
    ${XSP_DIR}/Model.xsp
    ${XSP_DIR}/MotionPlanner.xsp
    ${XSP_DIR}/PerimeterGenerator.xsp
    ${XSP_DIR}/PlaceholderParser.xsp
    ${XSP_DIR}/Point.xsp
    ${XSP_DIR}/Polygon.xsp
    ${XSP_DIR}/Polyline.xsp
    ${XSP_DIR}/PolylineCollection.xsp
    ${XSP_DIR}/Print.xsp
    ${XSP_DIR}/SLAPrint.xsp
    ${XSP_DIR}/Surface.xsp
    ${XSP_DIR}/SlicingAdaptive.xsp
    ${XSP_DIR}/SurfaceCollection.xsp
    ${XSP_DIR}/SupportMaterial.xsp
    ${XSP_DIR}/TriangleMesh.xsp
    ${XSP_DIR}/XS.xsp
)
foreach (file ${XS_XSP_FILES})
    if (MSVC)
        # Visual Studio C compiler has issues with FILE pragmas containing quotes.
        set(INCLUDE_COMMANDS "${INCLUDE_COMMANDS}INCLUDE_COMMAND: $^X -MExtUtils::XSpp::Cmd -e xspp -- -t ${CMAKE_CURRENT_LIST_DIR}/xsp/typemap.xspt ${file}\n")
    else ()
        set(INCLUDE_COMMANDS "${INCLUDE_COMMANDS}INCLUDE_COMMAND: $^X -MExtUtils::XSpp::Cmd -e xspp -- -t \"${CMAKE_CURRENT_LIST_DIR}/xsp/typemap.xspt\" \"${file}\"\n")
    endif ()
endforeach ()
configure_file(main.xs.in ${XS_MAIN_XS} @ONLY) # Insert INCLUDE_COMMANDS into main.xs

# Generate the Slic3r Perl module (XS) XS.cpp file.
#FIXME add the dependency on main.xs and typemap.
set(XS_MAIN_CPP ${CMAKE_CURRENT_BINARY_DIR}/XS.cpp)
add_custom_command(
        OUTPUT ${XS_MAIN_CPP}
        DEPENDS ${MyTypemap} ${XS_XSP_FILES} ${CMAKE_CURRENT_LIST_DIR}/xsp/typemap.xspt
        COMMAND COMMAND xsubpp -typemap typemap -output ${XS_MAIN_CPP} -hiertype ${XS_MAIN_XS}
)

# Define the Perl XS shared library.
if(APPLE)
    set(XS_SHARED_LIBRARY_TYPE MODULE)
else()
    set(XS_SHARED_LIBRARY_TYPE SHARED)
endif()
add_library(XS ${XS_SHARED_LIBRARY_TYPE}
    ${XS_MAIN_CPP}
    ${LIBDIR}/libslic3r/utils.cpp
    ${LIBDIR}/slic3r/GUI/wxPerlIface.cpp
    ${LIBDIR}/perlglue.cpp
    ${LIBDIR}/ppport.h
    ${LIBDIR}/xsinit.h
    ${CMAKE_CURRENT_LIST_DIR}/xsp/my.map
    # mytype.map is empty. Is it required by Build.PL or the Perl xspp module?
    ${CMAKE_CURRENT_LIST_DIR}/xsp/mytype.map
    # Used by Perl xsubpp to generate XS.cpp
    ${CMAKE_CURRENT_LIST_DIR}/xsp/typemap.xspt
)
if(APPLE)
    set_target_properties(XS PROPERTIES BUNDLE TRUE)
    # Ignore undefined symbols of the perl interpreter, they will be found in the caller image.
    target_link_libraries(XS "-undefined dynamic_lookup")
endif()
target_link_libraries(XS libslic3r admesh ZipArchive BSpline libslic3r_gui clipper nowide polypartition poly2tri)

# Add the OpenGL and GLU libraries.
if (SLIC3R_GUI)
    if (MSVC)
        target_link_libraries(XS OpenGL32.Lib GlU32.Lib)
    elseif (MINGW)
        target_link_libraries(XS -lopengl32)
    elseif (APPLE)
        target_link_libraries(XS "-framework OpenGL")
    else ()
        target_link_libraries(XS -lGL -lGLU)
    endif ()
endif ()

target_include_directories(XS PRIVATE src src/libslic3r) # Local include directories
target_compile_definitions(XS PRIVATE -DSLIC3RXS)
set_target_properties(XS PROPERTIES PREFIX "") # Prevent cmake from generating libXS.so instead of XS.so

if (APPLE)
#    add_compile_options(-stdlib=libc++)
#    add_definitions(-DBOOST_THREAD_DONT_USE_CHRONO -DBOOST_NO_CXX11_RVALUE_REFERENCES -DBOOST_THREAD_USES_MOVE)
    target_link_libraries(XS "-framework IOKit" "-framework CoreFoundation" -lc++)
elseif (MSVC)
    target_link_libraries(XS )
else ()
    target_link_libraries(XS -lstdc++)
endif ()

# Windows specific stuff
if (WIN32)
    target_compile_definitions(XS PRIVATE -DNOGDI -DNOMINMAX -DHAS_BOOL)
endif ()

## Configuration flags
if (SLIC3R_GUI)
    message("Slic3r will be built with GUI support")
    target_compile_definitions(XS PRIVATE -DSLIC3R_GUI)
endif ()

if (CMAKE_BUILD_TYPE MATCHES DEBUG)
    target_compile_definitions(XS PRIVATE -DSLIC3R_DEBUG -DDEBUG -D_DEBUG)
else ()
    target_compile_definitions(XS PRIVATE -DNDEBUG)
endif ()

# Perl specific stuff
find_package(PerlLibs REQUIRED)
set(PerlEmbed_DEBUG 1)
find_package(PerlEmbed REQUIRED)
target_include_directories(XS PRIVATE ${PERL_INCLUDE_PATH})
target_compile_options(XS PRIVATE ${PerlEmbed_CCFLAGS})
# If the Perl is compiled with optimization off, disable optimization over the whole project.
if (WIN32 AND ";${PerlEmbed_CCFLAGS};" MATCHES ";[-/]Od;")
    message(STATUS "Perl compiled without optimization. Disabling optimization for the Slic3r build.")
    message("Old CMAKE_CXX_FLAGS_RELEASE: ${CMAKE_CXX_FLAGS_RELEASE}")
    message("Old CMAKE_CXX_FLAGS_RELWITHDEBINFO: ${CMAKE_CXX_FLAGS_RELEASE}")
    message("Old CMAKE_CXX_FLAGS: ${CMAKE_CXX_FLAGS_RELEASE}")
    set(CMAKE_CXX_FLAGS_RELEASE "/MD /Od /Zi /EHsc /DNDEBUG")
    set(CMAKE_C_FLAGS_RELEASE "/MD /Od /Zi /DNDEBUG")
    set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "/MD /Od /Zi /EHsc /DNDEBUG")
    set(CMAKE_C_FLAGS_RELWITHDEBINFO "/MD /Od /Zi /DNDEBUG")
    set(CMAKE_CXX_FLAGS "/MD /Od /Zi /EHsc /DNDEBUG")
    set(CMAKE_C_FLAGS "/MD /Od /Zi /DNDEBUG")
endif()
# The following line will add -fPIC on Linux to make the XS.so rellocable.
add_definitions(${PerlEmbed_CCCDLFLAGS})
if (WIN32)
    target_link_libraries(XS ${PERL_LIBRARY})
    add_definitions(-DNOGDI -I${PERL_INCLUDE_PATH})
    include_directories(src src/libslic3r ${PERL_INCLUDE_PATH})
endif()

## REQUIRED packages

# Find and configure boost
if(SLIC3R_STATIC)
    # Use static boost libraries.
    set(Boost_USE_STATIC_LIBS ON)
    # Use boost libraries linked statically to the C++ runtime.
    # set(Boost_USE_STATIC_RUNTIME ON)
endif()
find_package(Boost REQUIRED COMPONENTS system filesystem thread log locale)
if(Boost_FOUND)
    include_directories(${Boost_INCLUDE_DIRS})
    target_link_libraries(XS ${Boost_LIBRARIES})
    if (APPLE)
        # BOOST_ASIO_DISABLE_KQUEUE : prevents a Boost ASIO bug on OS X: https://svn.boost.org/trac/boost/ticket/5339
        add_definitions(-DBOOST_ASIO_DISABLE_KQUEUE)
    endif()
    if(NOT SLIC3R_STATIC)
        add_definitions(-DBOOST_LOG_DYN_LINK)
    endif()
endif()

## OPTIONAL packages

# Find expat or use bundled version
# Always use the system libexpat on Linux.
if (NOT SLIC3R_STATIC OR CMAKE_SYSTEM_NAME STREQUAL "Linux")
    find_package(EXPAT)
endif ()
if (NOT EXPAT_FOUND)
    add_library(expat STATIC
        ${LIBDIR}/expat/xmlparse.c
        ${LIBDIR}/expat/xmlrole.c
        ${LIBDIR}/expat/xmltok.c
    )
    set(EXPAT_FOUND 1)
    set(EXPAT_INCLUDE_DIRS ${LIBDIR}/expat/)
    set(EXPAT_LIBRARIES expat)
endif ()
include_directories(${EXPAT_INCLUDE_DIRS})
target_link_libraries(XS ${EXPAT_LIBRARIES})

# Install the XS.pm and XS.{so,dll,bundle} into the local-lib directory.
set(PERL_LOCAL_LIB_DIR "${PROJECT_SOURCE_DIR}/local-lib/lib/perl5/${PerlEmbed_ARCHNAME}")
add_custom_command(
    TARGET XS
    POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory "${PERL_LOCAL_LIB_DIR}/auto/Slic3r/XS/"
        COMMAND ${CMAKE_COMMAND} -E copy "$<TARGET_FILE:XS>" "${PERL_LOCAL_LIB_DIR}/auto/Slic3r/XS/"
        COMMAND ${CMAKE_COMMAND} -E make_directory "${PERL_LOCAL_LIB_DIR}/Slic3r/"
        COMMAND ${CMAKE_COMMAND} -E copy "${PROJECT_SOURCE_DIR}/xs/lib/Slic3r/XS.pm" "${PERL_LOCAL_LIB_DIR}/Slic3r/"
)
if(APPLE)
    add_custom_command(
        TARGET XS
        POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E rename "${PERL_LOCAL_LIB_DIR}/auto/Slic3r/XS/XS" "${PERL_LOCAL_LIB_DIR}/auto/Slic3r/XS/XS.bundle"
    )
endif()

# Create a slic3r executable if not on a build server
if (NOT (DEFINED ENV{TRAVIS} OR DEFINED ENV{APPVEYOR}))
    add_executable(slic3r ${PROJECT_SOURCE_DIR}/src/slic3r.cpp)
    target_include_directories(XS PRIVATE src src/libslic3r)
    target_link_libraries(slic3r libslic3r libslic3r_gui ZipArchive BSpline admesh ${Boost_LIBRARIES} clipper ${EXPAT_LIBRARIES} polypartition poly2tri ${wxWidgets_LIBRARIES})

    if (APPLE)
        target_link_libraries(slic3r "-framework IOKit" "-framework CoreFoundation" -lc++)
    elseif (NOT MSVC)
        target_link_libraries(slic3r -lstdc++)
    endif ()
        
        
endif()

# Installation
install(TARGETS XS DESTINATION lib/slic3r/auto/Slic3r/XS)
install(FILES lib/Slic3r/XS.pm DESTINATION lib/slic3r/Slic3r)
