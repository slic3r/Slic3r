cmake_minimum_required (VERSION 2.8)
project (slic3r)

# only on newer GCCs: -ftemplate-backtrace-limit=0
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wall -DM_PI=3.14159265358979323846 -D_GLIBCXX_USE_C99 -DHAS_BOOL -DNOGDI -DBOOST_ASIO_DISABLE_KQUEUE")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wall -DSLIC3R_DEBUG")

if(CMAKE_CXX_COMPILER_VERSION VERSION_GREATER 4.7.0)
    if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS 4.7.3)
       set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-inline-small-functions")
    endif(CMAKE_CXX_COMPILER_VERSION VERSION_LESS 4.7.3)
endif(CMAKE_CXX_COMPILER_VERSION VERSION_GREATER 4.7.0)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

IF(CMAKE_HOST_APPLE)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++ -DBOOST_THREAD_DONT_USE_CHRONO -DBOOST_NO_CXX11_RVALUE_REFERENCES -DBOOST_THREAD_USES_MOVE")
    set(CMAKE_EXE_LINKER_FLAGS "-framework IOKit -framework CoreFoundation -lc++")
ELSE(CMAKE_HOST_APPLE)
    set(CMAKE_EXE_LINKER_FLAGS "-static-libgcc -static-libstdc++")
ENDIF(CMAKE_HOST_APPLE)
set(Boost_USE_STATIC_LIBS ON)
set(Boost_USE_STATIC_RUNTIME ON)
set(CMAKE_FIND_LIBRARY_SUFFIXES ".a")
find_package(Boost COMPONENTS system thread filesystem)

set(LIBDIR ${CMAKE_CURRENT_SOURCE_DIR}/../xs/src/)

include_directories(${LIBDIR})
include_directories(${LIBDIR}/libslic3r)
include_directories(${LIBDIR}/Slic3r/GUI/)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/standalone/)
include_directories(${LIBDIR}/admesh/)
include_directories(${LIBDIR}/BSpline/)
include_directories(${LIBDIR}/expat/)
include_directories(${LIBDIR}/poly2tri/)
include_directories(${LIBDIR}/poly2tri/sweep)
include_directories(${LIBDIR}/poly2tri/common)

add_library(libslic3r STATIC
    ${LIBDIR}/libslic3r/BoundingBox.cpp
    ${LIBDIR}/libslic3r/BridgeDetector.cpp
    ${LIBDIR}/libslic3r/ClipperUtils.cpp
    ${LIBDIR}/libslic3r/Config.cpp
    ${LIBDIR}/libslic3r/ExPolygon.cpp
    ${LIBDIR}/libslic3r/ExPolygonCollection.cpp
    ${LIBDIR}/libslic3r/Extruder.cpp
    ${LIBDIR}/libslic3r/ExtrusionEntity.cpp
    ${LIBDIR}/libslic3r/ExtrusionEntityCollection.cpp
    ${LIBDIR}/libslic3r/Fill/Fill.cpp
    ${LIBDIR}/libslic3r/Fill/Fill3DHoneycomb.cpp
    ${LIBDIR}/libslic3r/Fill/FillConcentric.cpp
    ${LIBDIR}/libslic3r/Fill/FillHoneycomb.cpp
    ${LIBDIR}/libslic3r/Fill/FillPlanePath.cpp
    ${LIBDIR}/libslic3r/Fill/FillRectilinear.cpp
    ${LIBDIR}/libslic3r/Flow.cpp
    ${LIBDIR}/libslic3r/GCode.cpp
    ${LIBDIR}/libslic3r/GCode/CoolingBuffer.cpp
    ${LIBDIR}/libslic3r/GCode/SpiralVase.cpp
    ${LIBDIR}/libslic3r/GCodeReader.cpp
    ${LIBDIR}/libslic3r/GCodeSender.cpp
    ${LIBDIR}/libslic3r/GCodeTimeEstimator.cpp
    ${LIBDIR}/libslic3r/GCodeWriter.cpp
    ${LIBDIR}/libslic3r/Geometry.cpp
    ${LIBDIR}/libslic3r/IO.cpp
    ${LIBDIR}/libslic3r/IO/AMF.cpp
    ${LIBDIR}/libslic3r/IO/TMF.cpp
    ${LIBDIR}/libslic3r/Layer.cpp
    ${LIBDIR}/libslic3r/LayerRegion.cpp
    ${LIBDIR}/libslic3r/LayerRegionFill.cpp
    ${LIBDIR}/libslic3r/LayerHeightSpline.cpp
    ${LIBDIR}/libslic3r/Line.cpp
    ${LIBDIR}/libslic3r/Model.cpp
    ${LIBDIR}/libslic3r/MotionPlanner.cpp
    ${LIBDIR}/libslic3r/MultiPoint.cpp
    ${LIBDIR}/libslic3r/PerimeterGenerator.cpp
    ${LIBDIR}/libslic3r/PlaceholderParser.cpp
    ${LIBDIR}/libslic3r/Point.cpp
    ${LIBDIR}/libslic3r/Polygon.cpp
    ${LIBDIR}/libslic3r/Polyline.cpp
    ${LIBDIR}/libslic3r/PolylineCollection.cpp
    ${LIBDIR}/libslic3r/Print.cpp
    ${LIBDIR}/libslic3r/PrintConfig.cpp
    ${LIBDIR}/libslic3r/PrintObject.cpp
    ${LIBDIR}/libslic3r/PrintRegion.cpp
    ${LIBDIR}/libslic3r/SLAPrint.cpp
    ${LIBDIR}/libslic3r/SlicingAdaptive.cpp
    ${LIBDIR}/libslic3r/Surface.cpp
    ${LIBDIR}/libslic3r/SurfaceCollection.cpp
    ${LIBDIR}/libslic3r/SVG.cpp
    ${LIBDIR}/libslic3r/TriangleMesh.cpp
    ${LIBDIR}/libslic3r/Zip/ZipArchive.cpp
)

add_library(BSpline STATIC
    ${LIBDIR}/BSpline/BSpline.cpp
)

add_library(admesh STATIC
    ${LIBDIR}/admesh/connect.c
    ${LIBDIR}/admesh/normals.c
    ${LIBDIR}/admesh/shared.c
    ${LIBDIR}/admesh/stl_io.c
    ${LIBDIR}/admesh/stlinit.c
    ${LIBDIR}/admesh/util.c
)
set_property(TARGET admesh PROPERTY C_STANDARD 99)

add_library(clipper STATIC ${LIBDIR}/clipper.cpp)
add_library(expat STATIC
    ${LIBDIR}/expat/xmlparse.c
    ${LIBDIR}/expat/xmlrole.c
    ${LIBDIR}/expat/xmltok.c
)
add_library(polypartition STATIC ${LIBDIR}/polypartition.cpp)
add_library(poly2tri STATIC
    ${LIBDIR}/poly2tri/common/shapes.cc
    ${LIBDIR}/poly2tri/sweep/advancing_front.cc
    ${LIBDIR}/poly2tri/sweep/cdt.cc
    ${LIBDIR}/poly2tri/sweep/sweep_context.cc
    ${LIBDIR}/poly2tri/sweep/sweep.cc
)

add_executable(slic3r slic3r.cpp)
set_target_properties(slic3r PROPERTIES LINK_SEARCH_START_STATIC 1)
set_target_properties(slic3r PROPERTIES LINK_SEARCH_END_STATIC 1)

add_executable(extrude-tin utils/extrude-tin.cpp)
set_target_properties(extrude-tin PROPERTIES LINK_SEARCH_START_STATIC 1)
set_target_properties(extrude-tin PROPERTIES LINK_SEARCH_END_STATIC 1)

set(wxWidgets_USE_STATIC)
SET(wxWidgets_USE_LIBS)

set(Boost_USE_STATIC_LIBS ON)
set(Boost_USE_STATIC_RUNTIME ON)
set(CMAKE_FIND_LIBRARY_SUFFIXES ".a")
find_library(bsystem_l boost_system)
add_library(bsystem STATIC IMPORTED)
set_target_properties(bsystem PROPERTIES IMPORTED_LOCATION ${bsystem_l})
find_library(bthread_l boost_thread)
add_library(bthread STATIC IMPORTED)
set_target_properties(bthread PROPERTIES IMPORTED_LOCATION ${bthread_l})
include_directories(${Boost_INCLUDE_DIRS})

#find_package(wxWidgets)
#disable wx for the time being - we're not building any of the gui yet
IF(CMAKE_HOST_UNIX)
    #set(Boost_LIBRARIES bsystem bthread bfilesystem)
ENDIF(CMAKE_HOST_UNIX)

target_link_libraries (slic3r libslic3r admesh BSpline clipper expat polypartition poly2tri ${Boost_LIBRARIES})

IF(wxWidgets_FOUND)
    MESSAGE("wx found!")
    INCLUDE("${wxWidgets_USE_FILE}")
    add_library(slic3r_gui STATIC ${LIBDIR}/slic3r/GUI/3DScene.cpp ${LIBDIR}/slic3r/GUI/GUI.cpp)
    #only build GUI lib if building with wx
    target_link_libraries (slic3r slic3r-gui ${wxWidgets_LIBRARIES})
ELSE(wxWidgets_FOUND)
    # For convenience. When we cannot continue, inform the user
    MESSAGE("wx not found!")
    #skip gui when no wx included
ENDIF(wxWidgets_FOUND)

# Windows needs a compiled component for Boost.nowide
IF (WIN32)
    add_library(boost-nowide STATIC 
        ${LIBDIR}/boost/nowide/iostream.cpp
    )

    target_link_libraries(slic3r boost-nowide)
    target_link_libraries(extrude-tin boost-nowide)
ENDIF(WIN32)

target_link_libraries (extrude-tin libslic3r admesh BSpline clipper expat polypartition poly2tri ${Boost_LIBRARIES})
