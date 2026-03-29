cmake_minimum_required(VERSION 3.20)
project(LabyPath
    VERSION 1.0.0
    DESCRIPTION "Path/maze generation and rendering system using CGAL"
    LANGUAGES CXX
)

# ─── C++ Standard ────────────────────────────────────────────────────────────
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# ─── Strict Compiler Warnings ────────────────────────────────────────────────
# Defined as a variable and applied per-target (not globally via
# add_compile_options) so that system/third-party headers are not
# affected when they are included as SYSTEM directories.
set(LABYPATH_WARNING_FLAGS
    -Wall
    -Wextra
    -Wpedantic
    -Wshadow
    -Wnon-virtual-dtor
    -Woverloaded-virtual
    -Wcast-align
    -Wunused
    -Wconversion
    -Wsign-conversion
    -Wdouble-promotion
    -Wformat=2
)

# Treat warnings as errors in CI builds (opt-in)
option(LABYPATH_WERROR "Treat compiler warnings as errors" OFF)
if(LABYPATH_WERROR)
    list(APPEND LABYPATH_WARNING_FLAGS -Werror)
endif()

# ─── Options ─────────────────────────────────────────────────────────────────
option(LABYPATH_BUILD_TESTS "Build unit tests" ON)
option(LABYPATH_ENABLE_PROFILER "Enable easy_profiler integration" OFF)

# ─── Find Dependencies ──────────────────────────────────────────────────────
set(LABYPATH_CGAL_MIN_VERSION 6.1.1)
set(LABYPATH_PROTOBUF_MIN_VERSION 34.1.0)
set(protobuf_MODULE_COMPATIBLE ON)
find_package(CGAL ${LABYPATH_CGAL_MIN_VERSION} REQUIRED CONFIG)
find_package(Boost REQUIRED)
find_package(Protobuf REQUIRED CONFIG)
if(NOT Protobuf_VERSION VERSION_GREATER_EQUAL LABYPATH_PROTOBUF_MIN_VERSION)
    message(FATAL_ERROR
        "Found Protobuf ${Protobuf_VERSION}, but ${LABYPATH_PROTOBUF_MIN_VERSION}+ is required")
endif()
message(STATUS "Found CGAL ${CGAL_VERSION}")
message(STATUS "Found Protobuf ${Protobuf_VERSION}")
# Microsoft Guideline Support Library (GSL) via FetchContent
include(FetchContent)
FetchContent_Declare(GSL
    GIT_REPOSITORY "https://github.com/microsoft/GSL"
    GIT_TAG "v4.2.0"
    GIT_SHALLOW ON
)
FetchContent_MakeAvailable(GSL)

# SVG++ is header-only. Pin the upstream 1.3.1 release so we don't depend on
# the distro's older libsvgpp-dev package and its deprecated Boost Phoenix shim.
FetchContent_Declare(svgpp
    URL "https://github.com/svgpp/svgpp/archive/refs/tags/v1.3.1.tar.gz"
    URL_HASH "SHA256=be8a89df72d01cf062cc9815dd64c9576b4d20910d6d7aee7f0ea26484dc5e76"
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
FetchContent_GetProperties(svgpp)
if(NOT svgpp_POPULATED)
    FetchContent_Populate(svgpp)
endif()

set(LABYPATH_SVGPP_INCLUDE_DIR "${svgpp_SOURCE_DIR}/include")
set(LABYPATH_RAPIDXML_NS_INCLUDE_DIR "${svgpp_SOURCE_DIR}/third_party")
if(NOT EXISTS "${LABYPATH_SVGPP_INCLUDE_DIR}/svgpp/svgpp.hpp")
    message(FATAL_ERROR "Pinned SVG++ archive is missing include/svgpp/svgpp.hpp")
endif()
if(NOT EXISTS "${LABYPATH_RAPIDXML_NS_INCLUDE_DIR}/rapidxml_ns/rapidxml_ns.hpp")
    message(FATAL_ERROR "Pinned SVG++ archive is missing third_party/rapidxml_ns/rapidxml_ns.hpp")
endif()

# Mark CGAL and Boost as SYSTEM so their headers don't trigger our strict
# warnings during template instantiation (GCC may inline system-header code
# into project translation units). Requires CMake ≥ 3.25.
if(CMAKE_VERSION VERSION_GREATER_EQUAL "3.25")
    set_target_properties(CGAL       PROPERTIES SYSTEM TRUE)
    set_target_properties(Boost::boost PROPERTIES SYSTEM TRUE)
    set_target_properties(protobuf::libprotobuf PROPERTIES SYSTEM TRUE)
endif()

# FFTW3 (optional – needed for noise generation)
find_library(FFTW3_LIBRARY NAMES fftw3)
find_path(FFTW3_INCLUDE_DIR NAMES fftw3.h)
if(FFTW3_LIBRARY AND FFTW3_INCLUDE_DIR)
    set(FFTW3_FOUND TRUE)
    message(STATUS "Found FFTW3: ${FFTW3_LIBRARY}")
else()
    set(FFTW3_FOUND FALSE)
    message(STATUS "FFTW3 not found – FFT features will be disabled")
endif()

# easy_profiler (optional)
if(LABYPATH_ENABLE_PROFILER)
    find_package(easy_profiler QUIET)
    if(easy_profiler_FOUND)
        message(STATUS "Found easy_profiler")
    else()
        message(WARNING "easy_profiler not found – profiling disabled")
        set(LABYPATH_ENABLE_PROFILER OFF)
    endif()
endif()

# ─── Library Sources ─────────────────────────────────────────────────────────
set(LABYPATH_SOURCES
    # Core
    src/GeomData.cpp
    src/GridIndex.cpp
    src/MessageIO.cpp
    src/OrientedRibbon.cpp
    src/PolyConvex.cpp 
    src/Polyline.cpp
    src/Ribbon.cpp
    src/SVGShapeToGrid.cpp 
    src/SkeletonGrid.cpp
    src/SkeletonOffset.cpp
    src/SkeletonRadial.cpp
    src/Smoothing.cpp
    src/VoronoiMedialSkeleton.cpp

    # AlternaRoute
    src/AlternaRoute/AlternateRoute.cpp
    src/AlternaRoute/StrokeArrangement.cpp

    # Anisotrop
    src/Anisotrop/Cell.cpp
    src/Anisotrop/Net.cpp
    src/Anisotrop/Placement.cpp
    src/Anisotrop/QueueCost.cpp
    src/Anisotrop/QueueElement.cpp
    src/Anisotrop/Routing.cpp
    src/Anisotrop/SpatialIndex.cpp

    # Rendering
    src/Rendering/GraphicRendering.cpp
    src/Rendering/PenStroke.cpp

    # SVGParser 
    src/SVGParser/Context.cpp
    src/SVGParser/Loader.cpp
    src/SVGParser/ShapeContext.cpp 
    src/SVGParser/UseContext.cpp

    # basic 
    src/basic/CircleIntersection.cpp 
    src/basic/LinearGradient.cpp
    src/basic/NumericRange.cpp 
    src/basic/PolygonTools.cpp
    src/basic/ProfileActivate.cpp   
    src/basic/SimplifyLines.cpp

    # flatteningOverlap
    src/flatteningOverlap/Family.cpp 
    src/flatteningOverlap/NodeRendering.cpp
    src/flatteningOverlap/PathRendering.cpp

    # generator
    src/generator/HqNoise.cpp
    src/generator/StreamLine.cpp

    # agg
    src/agg/agg_arc.cpp
    src/agg/agg_curve.cpp

    # Protobuf generated
    src/protoc/AllConfig.pb.cc
)

# FFTW3 is used directly via fftw3.h (no vendored wrapper)

add_library(labypath_lib STATIC ${LABYPATH_SOURCES})

target_compile_options(labypath_lib PRIVATE ${LABYPATH_WARNING_FLAGS})

target_include_directories(labypath_lib
    SYSTEM PUBLIC
        ${LABYPATH_SVGPP_INCLUDE_DIR}
        ${LABYPATH_RAPIDXML_NS_INCLUDE_DIR}
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(labypath_lib
    PUBLIC
        CGAL::CGAL
        Boost::boost
        protobuf::libprotobuf
        pthread
        Microsoft.GSL::GSL
)

if(FFTW3_FOUND)
    target_include_directories(labypath_lib SYSTEM PUBLIC ${FFTW3_INCLUDE_DIR})
    target_link_libraries(labypath_lib PUBLIC ${FFTW3_LIBRARY})
    target_compile_definitions(labypath_lib PUBLIC LABYPATH_HAS_FFTW3)
endif()

if(LABYPATH_ENABLE_PROFILER)
    target_link_libraries(labypath_lib PUBLIC easy_profiler)
    target_compile_definitions(labypath_lib PUBLIC BUILD_WITH_EASY_PROFILER)
endif()

# Suppress warnings in generated protobuf code
set_source_files_properties(
    src/protoc/AllConfig.pb.cc
    PROPERTIES COMPILE_FLAGS "-Wno-conversion -Wno-sign-conversion -Wno-shadow -Wno-unused-parameter"
)


# Suppress -Wmaybe-uninitialized false positives from svgpp headers.
# GCC inlines svgpp::arc_to_bezier iterator code and reports uninitialized members
# that are actually set in the iterator's advance() method.
set_source_files_properties(
    src/SVGParser/Loader.cpp
    PROPERTIES COMPILE_FLAGS "-Wno-maybe-uninitialized"
)

# ─── Main Executable ─────────────────────────────────────────────────────────
add_executable(labypath src/Main.cpp)
target_link_libraries(labypath PRIVATE labypath_lib)

# ─── Unit Tests ──────────────────────────────────────────────────────────────
if(LABYPATH_BUILD_TESTS)
    enable_testing()
    find_package(GTest REQUIRED)

    add_executable(labypath_tests
        tests/test_polyline.cpp
        tests/test_edgeinfo.cpp
        tests/test_vertexinfo.cpp
        tests/test_ribbon.cpp
        tests/test_geomdata.cpp
        tests/test_smoothing.cpp
        tests/test_color.cpp
        tests/test_pairinteger.cpp
        tests/test_numericrange.cpp
        tests/test_lineargradient.cpp
        tests/test_circleintersection.cpp
        tests/test_polygontools.cpp
        tests/test_orientedribbon.cpp
        tests/test_segmentps.cpp
        tests/test_queuecost.cpp
        tests/test_randominteger.cpp
        tests/test_randomunidist.cpp
        tests/test_hqnoiseutils.cpp
        tests/test_simplifylines.cpp
        tests/test_poissongenerator.cpp
        tests/test_flatteningoverlap.cpp
        tests/test_polyconvex.cpp
        tests/test_graph_coloring.cpp
        tests/test_smoke_svg.cpp
        tests/test_iterators.cpp
        tests/test_net.cpp
        tests/test_queueelement.cpp
        tests/test_documentsvg.cpp
    )

    target_compile_options(labypath_tests PRIVATE ${LABYPATH_WARNING_FLAGS})

    target_link_libraries(labypath_tests
        PRIVATE
            labypath_lib
            GTest::gtest
            GTest::gtest_main
    )

    include(GoogleTest)
    gtest_discover_tests(labypath_tests)
endif()

# ─── Install Rules ───────────────────────────────────────────────────────────
install(TARGETS labypath RUNTIME DESTINATION bin)
