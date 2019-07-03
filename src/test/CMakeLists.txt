project(slic3r_test)
cmake_minimum_required(VERSION 2.6)

include(PrecompiledHeader)

set(TESTDIR ${CMAKE_CURRENT_SOURCE_DIR})
set(TESTFILE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/inputs/)
set(SLIC3R_TEST_SOURCES
    test_data.cpp
    test_data.hpp
	test_harness.cpp
    GUI/test_cli.cpp
#    libslic3r/test_config.cpp # toredo
    libslic3r/test_fill.cpp
    libslic3r/test_flow.cpp
    libslic3r/test_gcodewriter.cpp
    libslic3r/test_geometry.cpp
    libslic3r/test_model.cpp
    libslic3r/test_print.cpp
    libslic3r/test_thin.cpp
	libslic3r/test_denserinfill.cpp
	libslic3r/test_extrusion_entity.cpp
	libslic3r/test_skirt_brim.cpp
)

if (NOT TARGET Catch)
	include (ExternalProject)
	if(IS_TRAVIS_BUILD)     # on travis, use git for fetching instead of wget
		set(FETCH_EXTERNAL_CATCH
			GIT_REPOSITORY https://github.com/catchorg/Catch.git
			GIT_TAG 03d122a35c3f5c398c43095a87bc82ed44642516)
	elseif(WIN32)
		set(FETCH_EXTERNAL_CATCH
			URL https://github.com/catchorg/Catch2/archive/v2.4.2.zip
			URL_HASH MD5=6a2ffb9c69d368ebc1ad13146c5a5e1e)
	else()
		set(FETCH_EXTERNAL_CATCH
			URL https://github.com/catchorg/Catch2/archive/v2.4.2.tar.gz
			URL_HASH MD5=26927b878b1f42633f15a9ef1c4bd8e7)
	endif()
	ExternalProject_Add(Catch-External
		PREFIX ${CMAKE_BINARY_DIR}/external/Catch
		${FETCH_EXTERNAL_CATCH}
		CONFIGURE_COMMAND ""
		BUILD_COMMAND ""
		INSTALL_COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_BINARY_DIR}/external/Catch/src/Catch-External/single_include/catch2/catch.hpp
												 ${CMAKE_BINARY_DIR}/external/Catch/include/catch.hpp
	)
	add_library(Catch INTERFACE)
	add_dependencies(Catch Catch-External)
	target_include_directories(Catch INTERFACE ${CMAKE_BINARY_DIR}/external/Catch/include)
	target_compile_definitions(Catch INTERFACE $<$<CXX_COMPILER_ID:MSVC>:_SILENCE_CXX17_UNCAUGHT_EXCEPTION_DEPRECATION_WARNING>)
endif ()

    configure_file("${TESTDIR}/test_options.hpp.in" "${TESTDIR}/test_options.hpp")
    add_executable(slic3r_test ${SLIC3R_TEST_SOURCES}) 
    add_test(NAME TestSlic3r COMMAND slic3r_test)
    target_compile_features(slic3r_test PUBLIC cxx_std_14)
	# add_library(slic3r_test STATIC ${SLIC3R_TEST_SOURCES})
    # target_link_libraries(slic3r_test PUBLIC libslic3r Catch ${LIBSLIC3R_DEPENDS})
	target_link_libraries(slic3r_test slic3r_lib Catch)
	# libnest2d admesh miniz ${Boost_LIBRARIES} clipper nowide ${EXPAT_LIBRARIES} ${GLEW_LIBRARIES} ${PNG_LIBRARIES} glu-libtess polypartition poly2tri qhull semver tbb

# if (SLIC3R_PCH AND NOT SLIC3R_SYNTAXONLY)
    # add_precompiled_header(slic3r_test libslic3r_gui pchheader.hpp FORCEINCLUDE)
# endif ()
