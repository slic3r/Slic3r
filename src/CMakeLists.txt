cmake_minimum_required (VERSION 3.9)
project (slic3r)


option(GUI_BUILD_TESTS "Build tests for Slic3r GUI." ON)
option(SLIC3R_BUILD_TESTS "Build tests for libslic3r." ON)
option(SLIC3R_STATIC "Build and link Slic3r statically." ON)
option(BUILD_EXTRUDE_TIN "Build and link the extrude-tin application." OFF)

# only on newer GCCs: -ftemplate-backtrace-limit=0
set(CMAKE_CXX_FLAGS "-g ${CMAKE_CXX_FLAGS} -Wall -DM_PI=3.14159265358979323846 -D_GLIBCXX_USE_C99 -DHAS_BOOL -DNOGDI -DBOOST_ASIO_DISABLE_KQUEUE")

if(DEFINED ENV{SLIC3R_VAR_REL})
    set(CMAKE_CXX_FLAGS "-DVAR_REL=$ENV{SLIC3R_VAR_REL}")
endif(DEFINED ENV{SLIC3R_VAR_REL})

if(DEFINED ENV{SLIC3R_VAR_ABS})
    set(CMAKE_CXX_FLAGS "-DVAR_ABS")
endif(DEFINED ENV{SLIC3R_VAR_ABS})

if(DEFINED ENV{SLIC3R_VAR_ABS_PATH})
    set(CMAKE_CXX_FLAGS "-DVAR_ABS_PATH=$ENV{SLIC3R_VAR_ABS_PATH}")
endif(DEFINED ENV{SLIC3R_VAR_ABS_PATH})

if($ENV{TRAVIS})
    if($ENV{TRAVIS} STREQUAL "true")
        message(STATUS "Building on Travis-CI.")
        set(IS_TRAVIS_BUILD TRUE)
    endif()
endif()

execute_process(COMMAND git rev-parse --short HEAD OUTPUT_VARIABLE GIT_VERSION ERROR_QUIET)

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wall -DSLIC3R_DEBUG")

if(CMAKE_CXX_COMPILER_VERSION VERSION_GREATER 4.7.0)
    if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS 4.7.3)
       set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-inline-small-functions")
    endif(CMAKE_CXX_COMPILER_VERSION VERSION_LESS 4.7.3)
endif(CMAKE_CXX_COMPILER_VERSION VERSION_GREATER 4.7.0)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

IF(CMAKE_HOST_APPLE)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -stdlib=libc++ -DBOOST_THREAD_DONT_USE_CHRONO -DBOOST_NO_CXX11_RVALUE_REFERENCES -DBOOST_THREAD_USES_MOVE")
    set(CMAKE_EXE_LINKER_FLAGS "-framework IOKit -framework CoreFoundation -lc++")
ELSE(CMAKE_HOST_APPLE)
#    set(CMAKE_EXE_LINKER_FLAGS "-static-libgcc -static-libstdc++ -L.")
ENDIF(CMAKE_HOST_APPLE)


if(SLIC3R_STATIC)
    set(Boost_USE_STATIC_LIBS ON)
    set(Boost_USE_STATIC_RUNTIME ON)
else(SLIC3R_STATIC)
    set(Boost_USE_STATIC_LIBS OFF)
    set(Boost_USE_STATIC_RUNTIME OFF)
endif(SLIC3R_STATIC)

find_package(Threads REQUIRED)

find_package(Boost REQUIRED COMPONENTS system thread filesystem)

set(LIBDIR ${CMAKE_CURRENT_SOURCE_DIR}/../xs/src/)
set(GUI_LIBDIR ${CMAKE_CURRENT_SOURCE_DIR}/GUI/)
set(TESTDIR ${CMAKE_CURRENT_SOURCE_DIR}/test)
set(GUI_TESTDIR ${CMAKE_CURRENT_SOURCE_DIR}/test/GUI/)

include_directories(${LIBDIR})
include_directories(${LIBDIR}/libslic3r)
include_directories(${LIBDIR}/slic3r/GUI/)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/standalone/)
include_directories(${LIBDIR}/admesh/)
include_directories(${LIBDIR}/BSpline/)
include_directories(${LIBDIR}/expat/)
include_directories(${LIBDIR}/poly2tri/)
include_directories(${LIBDIR}/poly2tri/sweep)
include_directories(${LIBDIR}/poly2tri/common)

add_library(ZipArchive STATIC
    ${LIBDIR}/Zip/ZipArchive.cpp
)


add_library(libslic3r STATIC
    ${LIBDIR}/libslic3r/BoundingBox.cpp
    ${LIBDIR}/libslic3r/BridgeDetector.cpp
    ${LIBDIR}/libslic3r/ClipperUtils.cpp
    ${LIBDIR}/libslic3r/ConfigBase.cpp
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
    ${LIBDIR}/libslic3r/Fill/FillGyroid.cpp
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
)
target_compile_features(libslic3r PUBLIC cxx_std_11)



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



set(UI_TEST_SOURCES
    ${GUI_TESTDIR}/testableframe.cpp
    ${GUI_TESTDIR}/test_harness_gui.cpp
    ${GUI_TESTDIR}/test_field_checkbox.cpp
)
set(SLIC3R_TEST_SOURCES
    ${TESTDIR}/test_harness.cpp
)
add_executable(slic3r slic3r.cpp)
#set_target_properties(slic3r PROPERTIES LINK_SEARCH_START_STATIC 1)
#set_target_properties(slic3r PROPERTIES LINK_SEARCH_END_STATIC 1)


set(Boost_USE_STATIC_LIBS OFF)
set(Boost_USE_STATIC_RUNTIME OFF)

find_library(bsystem_l boost_system log)
add_library(bsystem SHARED IMPORTED)
set_target_properties(bsystem PROPERTIES IMPORTED_LOCATION ${bsystem_l})
find_library(bthread_l boost_thread)
add_library(bthread SHARED IMPORTED)
set_target_properties(bthread PROPERTIES IMPORTED_LOCATION ${bthread_l})
include_directories(${Boost_INCLUDE_DIRS})

if(${SLIC3R_STATIC})
    set(wxWidgets_USE_STATIC ON)
else(${SLIC3R_STATIC})
    set(wxWidgets_USE_STATIC OFF)
endif(${SLIC3R_STATIC})

set(wxWidgets_USE_UNICODE ON)

find_package(wxWidgets COMPONENTS net gl html aui adv core base)

IF(CMAKE_HOST_UNIX)
    #set(Boost_LIBRARIES bsystem bthread bfilesystem)
ENDIF(CMAKE_HOST_UNIX)

# Libraries that Libslic3r itself depends on.
set(LIBSLIC3R_DEPENDS
    admesh
    BSpline
    clipper
    expat
    polypartition
    poly2tri
    ZipArchive
    ${Boost_LIBRARIES}
)


IF(wxWidgets_FOUND)
    MESSAGE("wx found!")
    INCLUDE("${wxWidgets_USE_FILE}")

    if (NOT GIT_VERSION STREQUAL "")
        if (MSVC)
            set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /DSLIC3R_BUILD_COMMIT=${GIT_VERSION} ")
        else(MSVC)
            execute_process(COMMAND git rev-parse --short HEAD OUTPUT_VARIABLE GIT_VERSION)
            set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DSLIC3R_BUILD_COMMIT=${GIT_VERSION}")
            string(REGEX REPLACE "\n$" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
        endif(MSVC)
    endif(NOT GIT_VERSION STREQUAL "")

    include_directories(${GUI_LIBDIR})
    include_directories(${wxWidgets_INCLUDE})

    add_library(slic3r_gui STATIC 
    ${GUI_LIBDIR}/Dialogs/AboutDialog.cpp
    ${GUI_LIBDIR}/Dialogs/PresetEditor.cpp
    ${GUI_LIBDIR}/Dialogs/PrintEditor.cpp
    ${GUI_LIBDIR}/Dialogs/PrinterEditor.cpp
    ${GUI_LIBDIR}/Dialogs/MaterialEditor.cpp
    ${GUI_LIBDIR}/GUI.cpp
    ${GUI_LIBDIR}/MainFrame.cpp
    ${GUI_LIBDIR}/Plater.cpp
    ${GUI_LIBDIR}/Plater/Plate2D.cpp
    ${GUI_LIBDIR}/Plater/PlaterObject.cpp
    ${GUI_LIBDIR}/ProgressStatusBar.cpp
    ${GUI_LIBDIR}/Settings.cpp
    ${GUI_LIBDIR}/misc_ui.cpp
    )
    target_compile_features(slic3r_gui PUBLIC cxx_std_14)
    #only build GUI lib if building with wx
    target_link_libraries (slic3r slic3r_gui ${wxWidgets_LIBRARIES})
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DUSE_WX")

    if (GUI_BUILD_TESTS)
        enable_testing()
        if (NOT TARGET Catch)
            include (ExternalProject)
            if(IS_TRAVIS_BUILD)     # on travis, use git for fetching instead of wget
                set(FETCH_EXTERNAL_CATCH
                    GIT_REPOSITORY https://github.com/philsquared/Catch.git
                    GIT_TAG 19ab2117c5bac2f376f8da4a4b25e183137bcec0)
            elseif(WIN32)
                set(FETCH_EXTERNAL_CATCH
                    URL https://github.com/catchorg/Catch2/archive/v2.0.1.zip
                    URL_HASH MD5=1abca1b324b99b1631e999119b172620)
            else()
                set(FETCH_EXTERNAL_CATCH
                    URL https://github.com/catchorg/Catch2/archive/v2.0.1.tar.gz
                    URL_HASH MD5=2080f4696579351d9323b3b5a8c3c71b)
            endif()
            ExternalProject_Add(Catch-External
                PREFIX ${CMAKE_BINARY_DIR}/external/Catch
                ${FETCH_EXTERNAL_CATCH}
                CONFIGURE_COMMAND ""
                BUILD_COMMAND ""
                INSTALL_COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_BINARY_DIR}/external/Catch/src/Catch-External/single_include/catch.hpp
                                                         ${CMAKE_BINARY_DIR}/external/Catch/include/catch.hpp
            )
            add_library(Catch INTERFACE)
            add_dependencies(Catch Catch-External)

            target_include_directories(Catch INTERFACE ${CMAKE_BINARY_DIR}/external/Catch/include)
            target_compile_definitions(Catch INTERFACE $<$<CXX_COMPILER_ID:MSVC>:_SILENCE_CXX17_UNCAUGHT_EXCEPTION_DEPRECATION_WARNING>)
        endif()
        add_executable(gui_test ${UI_TEST_SOURCES}) 
        add_test(NAME TestGUI COMMAND gui_test)
        target_link_libraries(gui_test PUBLIC libslic3r slic3r_gui Catch ${wxWidgets_LIBRARIES})
    endif()
ELSE(wxWidgets_FOUND)
    # For convenience. When we cannot continue, inform the user
    MESSAGE("wx not found!")
    #skip gui when no wx included
ENDIF(wxWidgets_FOUND)

target_link_libraries (slic3r libslic3r ${LIBSLIC3R_DEPENDS})

if (BUILD_EXTRUDE_TIN)
    add_executable(extrude-tin utils/extrude-tin.cpp)
    set_target_properties(extrude-tin PROPERTIES LINK_SEARCH_START_STATIC 1)
    set_target_properties(extrude-tin PROPERTIES LINK_SEARCH_END_STATIC 1)
endif() 

# Windows needs a compiled component for Boost.nowide
IF (WIN32)
    add_library(boost-nowide STATIC 
        ${LIBDIR}/boost/nowide/iostream.cpp
    )
    target_link_libraries(slic3r boost-nowide)

    # Windows findwxwidgets doesn't get everything needed for wxMSW
    target_link_libraries(slic3r uxtheme)    
    # MinGW apparently has some multiple definitions of UUID-related items
    # deal with it.
    if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
       set(CMAKE_EXE_LINKER_FLAGS "-Wl,--allow-multiple-definition")
    endif()
    if (BUILD_EXTRUDE_TIN)
        target_link_libraries(extrude-tin boost-nowide)
    endif()
ENDIF(WIN32)


if (SLIC3R_BUILD_TESTS)
    enable_testing()
    if (NOT TARGET Catch)
        include (ExternalProject)
        if(IS_TRAVIS_BUILD)     # on travis, use git for fetching instead of wget
            set(FETCH_EXTERNAL_CATCH
                GIT_REPOSITORY https://github.com/philsquared/Catch.git
                GIT_TAG 19ab2117c5bac2f376f8da4a4b25e183137bcec0)
        elseif(WIN32)
            set(FETCH_EXTERNAL_CATCH
                URL https://github.com/catchorg/Catch2/archive/v2.0.1.zip
                URL_HASH MD5=1abca1b324b99b1631e999119b172620)
        else()
            set(FETCH_EXTERNAL_CATCH
                URL https://github.com/catchorg/Catch2/archive/v2.0.1.tar.gz
                URL_HASH MD5=2080f4696579351d9323b3b5a8c3c71b)
        endif()
        ExternalProject_Add(Catch-External
            PREFIX ${CMAKE_BINARY_DIR}/external/Catch
            ${FETCH_EXTERNAL_CATCH}
            CONFIGURE_COMMAND ""
            BUILD_COMMAND ""
            INSTALL_COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_BINARY_DIR}/external/Catch/src/Catch-External/single_include/catch.hpp
                                                     ${CMAKE_BINARY_DIR}/external/Catch/include/catch.hpp
        )
        add_library(Catch INTERFACE)
        add_dependencies(Catch Catch-External)
        target_include_directories(Catch INTERFACE ${CMAKE_BINARY_DIR}/external/Catch/include)
        target_compile_definitions(Catch INTERFACE $<$<CXX_COMPILER_ID:MSVC>:_SILENCE_CXX17_UNCAUGHT_EXCEPTION_DEPRECATION_WARNING>)
    endif()
    add_executable(slic3r_test ${SLIC3R_TEST_SOURCES}) 
    add_test(NAME TestSlic3r COMMAND slic3r_test)
    target_link_libraries(slic3r_test PUBLIC libslic3r Catch ${LIBSLIC3R_DEPENDS})
endif()

if (BUILD_EXTRUDE_TIN)
    target_link_libraries (extrude-tin libslic3r ${LIBSLIC3R_DEPENDS})
endif()
