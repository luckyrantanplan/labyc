cmake_minimum_required(VERSION 3.20)
project(LabyPath
    VERSION 1.0.0
    DESCRIPTION "Path/maze generation and rendering system using CGAL"
    LANGUAGES CXX
)

# ─── C++ Standard ────────────────────────────────────────────────────────────
set(CMAKE_CXX_STANDARD 17)
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
find_package(CGAL REQUIRED)
find_package(Boost REQUIRED)
find_package(Protobuf REQUIRED)

# Mark CGAL and Boost as SYSTEM so their headers don't trigger our strict
# warnings during template instantiation (GCC may inline system-header code
# into project translation units). Requires CMake ≥ 3.25.
if(CMAKE_VERSION VERSION_GREATER_EQUAL "3.25")
    set_target_properties(CGAL       PROPERTIES SYSTEM TRUE)
    set_target_properties(Boost::boost PROPERTIES SYSTEM TRUE)
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
    src/PolyVertexList.cpp
    src/Polyline.cpp
    src/Ribbon.cpp
    src/SVGShapeToGrid.cpp
    src/SegmentPS.cpp
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
    src/SVGParser/Common.cpp
    src/SVGParser/Context.cpp
    src/SVGParser/Loader.cpp
    src/SVGParser/ShapeContext.cpp
    src/SVGParser/Stylable.cpp
    src/SVGParser/UseContext.cpp

    # SVGWriter
    src/SVGWriter/DocumentSVG.cpp

    # basic
    src/basic/AugmentedPolygonSet.cpp
    src/basic/CircleIntersection.cpp
    src/basic/Color.cpp
    src/basic/LinearGradient.cpp
    src/basic/NumericRange.cpp
    src/basic/PairInteger.cpp
    src/basic/PolygonTools.cpp
    src/basic/ProfileActivate.cpp
    src/basic/RandomInteger.cpp
    src/basic/RandomUniDist.cpp
    src/basic/RangeHelper.cpp
    src/basic/SimplifyLines.cpp

    # flatteningOverlap
    src/flatteningOverlap/Family.cpp
    src/flatteningOverlap/Node.cpp
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
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)
# Protobuf include dirs marked as SYSTEM to suppress third-party warnings
target_include_directories(labypath_lib SYSTEM PUBLIC ${Protobuf_INCLUDE_DIRS})

target_link_libraries(labypath_lib
    PUBLIC
        CGAL::CGAL
        Boost::boost
        protobuf::libprotobuf
        pthread
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

# Suppress warnings in third-party code (agg)
set_source_files_properties(
    src/agg/agg_arc.cpp
    src/agg/agg_curve.cpp
    PROPERTIES COMPILE_FLAGS "-Wno-conversion -Wno-sign-conversion -Wno-shadow -Wno-float-conversion -Wno-double-promotion"
)

# svgpp still pulls Boost's deprecated Spirit Phoenix compatibility header on
# modern Boost releases. Limit the compatibility define to the SVG parsing
# translation units so project code continues to surface its own deprecations.
set_source_files_properties(
    src/SVGShapeToGrid.cpp
    src/SVGParser/Common.cpp
    src/SVGParser/Context.cpp
    src/SVGParser/Loader.cpp
    src/SVGParser/ShapeContext.cpp
    src/SVGParser/Stylable.cpp
    src/SVGParser/UseContext.cpp
    PROPERTIES COMPILE_DEFINITIONS "BOOST_ALLOW_DEPRECATED_HEADERS"
)

# Suppress -Wmaybe-uninitialized false positives from svgpp headers (system library)
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
