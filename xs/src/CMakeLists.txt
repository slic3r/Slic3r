cmake_minimum_required (VERSION 2.8)
project (slic3r)
set(workaround "")
if(CMAKE_CXX_COMPILER_VERSION VERSION_GREATER 4.7.0)
if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS 4.7.3)
set(workaround "-fno-inline-small-functions")
endif(CMAKE_CXX_COMPILER_VERSION VERSION_LESS 4.7.3)
endif(CMAKE_CXX_COMPILER_VERSION VERSION_GREATER 4.7.0)

set(CMAKE_CXX_FLAGS "-D_GLIBCXX_USE_C99 -DHAS_BOOL -DNOGDI -DBOOST_ASIO_DISABLE_KQUEUE -std=c++14 ${workaround}")
set(CMAKE_INCLUDE_CURRENT_DIR ON)

IF(CMAKE_HOST_APPLE)
    set(CMAKE_EXE_LINKER_FLAGS "-static-libgcc -static-libstdc++ -framework IOKit -framework CoreFoundation")
ELSE(CMAKE_HOST_APPLE)
    set(CMAKE_EXE_LINKER_FLAGS "-static-libgcc -static-libstdc++")
ENDIF(CMAKE_HOST_APPLE)
set(Boost_USE_STATIC_LIBS ON)
set(Boost_USE_STATIC_RUNTIME ON)
set(CMAKE_FIND_LIBRARY_SUFFIXES ".a")
find_package(Boost COMPONENTS system thread)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/libslic3r)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/Slic3r/GUI/)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/standalone/)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/admesh/)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/poly2tri/)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/poly2tri/sweep)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/poly2tri/common)
add_library(slic3r_gui STATIC slic3r/GUI/3DScene.cpp slic3r/GUI/GUI.cpp)
add_library(libslic3r STATIC libslic3r/BoundingBox.cpp 
                        libslic3r/ExPolygon.cpp 
                        libslic3r/GCode.cpp 
                        libslic3r/LayerRegion.cpp 
                        libslic3r/PerimeterGenerator.cpp 
                        libslic3r/Polyline.cpp 
                        libslic3r/SurfaceCollection.cpp 
                        libslic3r/BridgeDetector.cpp 
                        libslic3r/Extruder.cpp 
                        libslic3r/GCodeSender.cpp 
                        libslic3r/Line.cpp 
                        libslic3r/PlaceholderParser.cpp 
                        libslic3r/PrintConfig.cpp 
                        libslic3r/Surface.cpp 
                        libslic3r/ClipperUtils.cpp 
                        libslic3r/ExtrusionEntityCollection.cpp 
                        libslic3r/GCodeWriter.cpp 
                        libslic3r/Model.cpp 
                        libslic3r/Point.cpp 
                        libslic3r/Print.cpp 
                        libslic3r/SVG.cpp 
                        libslic3r/SVGExport.cpp 
                        libslic3r/Config.cpp 
                        libslic3r/ExtrusionEntity.cpp 
                        libslic3r/Geometry.cpp 
                        libslic3r/MotionPlanner.cpp 
                        libslic3r/Polygon.cpp 
                        libslic3r/PrintObject.cpp 
                        libslic3r/TriangleMesh.cpp 
                        libslic3r/ExPolygonCollection.cpp 
                        libslic3r/Flow.cpp 
                        libslic3r/Layer.cpp 
                        libslic3r/MultiPoint.cpp 
                        libslic3r/PolylineCollection.cpp 
                        libslic3r/PrintRegion.cpp)
add_library(admesh STATIC admesh/util.c admesh/stl_io.c admesh/stlinit.c admesh/shared.c admesh/normals.c admesh/connect.c)
add_library(clipper STATIC clipper.cpp)
add_library(polypartition STATIC polypartition.cpp)
add_library(poly2tri STATIC poly2tri/sweep/sweep.cc poly2tri/sweep/sweep_context.cc poly2tri/sweep/cdt.cc poly2tri/sweep/advancing_front.cc poly2tri/common/shapes.cc)
add_executable(slic3r slic3r.cpp)
set_target_properties(slic3r PROPERTIES LINK_SEARCH_START_STATIC 1)
set_target_properties(slic3r PROPERTIES LINK_SEARCH_END_STATIC 1)
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

IF(wxWidgets_FOUND)
    MESSAGE("wx found!")
    INCLUDE("${wxWidgets_USE_FILE}")
    target_link_libraries (slic3r slic3r_gui libslic3r admesh clipper polypartition poly2tri bsystem bthread ${wxWidgets_LIBRARIES})
ELSE(wxWidgets_FOUND)
    # For convenience. When we cannot continue, inform the user
    MESSAGE("wx not found!")
    target_link_libraries (slic3r slic3r_gui libslic3r admesh clipper polypartition poly2tri bsystem bthread)
ENDIF(wxWidgets_FOUND)

