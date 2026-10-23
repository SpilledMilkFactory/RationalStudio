cmake_minimum_required(VERSION 3.16)

project(RationalStudio VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ---------------------------------------------------------
# Mathematical Core (Qt-independent)
# ---------------------------------------------------------

add_library(RationalCore STATIC
    src/core/Rational.cpp
    src/core/Expression.cpp
    src/core/GraphViewport.cpp
    src/core/CurveSampler.cpp
)

target_include_directories(RationalCore PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/src/core"
)

target_compile_options(RationalCore PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic>
)

# CurveGeometryCache.hpp is header-only.
# No additional .cpp file is required.

# ---------------------------------------------------------
# Command-Line Application
# ---------------------------------------------------------

option(
    RATIONALSTUDIO_BUILD_CLI
    "Build the standalone CLI calculator"
    ON
)

if(RATIONALSTUDIO_BUILD_CLI)

    add_executable(RationalCLI
        src/cli/main.cpp
    )

    target_link_libraries(RationalCLI PRIVATE
        RationalCore
    )

endif()

# ---------------------------------------------------------
# Automated Testing — Qt-independent tests
# ---------------------------------------------------------

include(CTest)

if(BUILD_TESTING)

    # -----------------------------------------------------
    # Mathematical Core
    # -----------------------------------------------------

    add_executable(RationalCoreTests
        tests/CoreTests.cpp
    )

    target_link_libraries(RationalCoreTests PRIVATE
        RationalCore
    )

    add_test(
        NAME RationalCoreTests
        COMMAND RationalCoreTests
    )

    # -----------------------------------------------------
    # Issue #14 — Viewport Mathematics
    # -----------------------------------------------------

    add_executable(RationalViewportTests
        tests/GraphViewportTests.cpp
    )

    target_link_libraries(RationalViewportTests PRIVATE
        RationalCore
    )

    add_test(
        NAME RationalViewportTests
        COMMAND RationalViewportTests
    )

    # -----------------------------------------------------
    # Issue #15 — Adaptive Curve Sampling
    # -----------------------------------------------------

    add_executable(RationalCurveSamplerTests
        tests/CurveSamplerTests.cpp
    )

    target_link_libraries(RationalCurveSamplerTests PRIVATE
        RationalCore
    )

    add_test(
        NAME RationalCurveSamplerTests
        COMMAND RationalCurveSamplerTests
    )

    # -----------------------------------------------------
    # Issue #16 — Undefined Function Domains
    # -----------------------------------------------------

    add_executable(RationalCurveDomainTests
        tests/CurveDomainTests.cpp
    )

    target_link_libraries(RationalCurveDomainTests PRIVATE
        RationalCore
    )

    add_test(
        NAME RationalCurveDomainTests
        COMMAND RationalCurveDomainTests
    )

    # -----------------------------------------------------
    # Issue #17 — Discontinuities and Asymptotes
    # -----------------------------------------------------

    add_executable(RationalCurveDiscontinuityTests
        tests/CurveDiscontinuityTests.cpp
    )

    target_link_libraries(
        RationalCurveDiscontinuityTests PRIVATE
        RationalCore
    )

    add_test(
        NAME RationalCurveDiscontinuityTests
        COMMAND RationalCurveDiscontinuityTests
    )

    # -----------------------------------------------------
    # Issue #18 — Steep Continuous Curves
    # -----------------------------------------------------

    add_executable(RationalCurveSteepTests
        tests/CurveSteepTests.cpp
    )

    target_link_libraries(RationalCurveSteepTests PRIVATE
        RationalCore
    )

    add_test(
        NAME RationalCurveSteepTests
        COMMAND RationalCurveSteepTests
    )

    # -----------------------------------------------------
    # Issue #19 — Viewport Precision
    # -----------------------------------------------------

    add_executable(RationalViewportPrecisionTests
        tests/GraphViewportPrecisionTests.cpp
    )

    target_link_libraries(RationalViewportPrecisionTests PRIVATE
        RationalCore
    )

    add_test(
        NAME RationalViewportPrecisionTests
        COMMAND RationalViewportPrecisionTests
    )

    # -----------------------------------------------------
    # Issue #20 — Curve Geometry Cache
    # Qt-independent regression tests
    # -----------------------------------------------------

    add_executable(RationalCurveGeometryCacheTests
        tests/CurveGeometryCacheTests.cpp
    )

    target_link_libraries(RationalCurveGeometryCacheTests PRIVATE
        RationalCore
    )

    add_test(
        NAME RationalCurveGeometryCacheTests
        COMMAND RationalCurveGeometryCacheTests
    )

    # -----------------------------------------------------
    # Shared Test Compiler Warnings
    # -----------------------------------------------------

    foreach(test_target IN ITEMS
        RationalCoreTests
        RationalViewportTests
        RationalCurveSamplerTests
        RationalCurveDomainTests
        RationalCurveDiscontinuityTests
        RationalCurveSteepTests
        RationalViewportPrecisionTests
        RationalCurveGeometryCacheTests
    )

        target_compile_options(${test_target} PRIVATE
            $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic>
        )

    endforeach()

endif()

# ---------------------------------------------------------
# Qt Desktop Application
# ---------------------------------------------------------

option(
    RATIONALSTUDIO_BUILD_GUI
    "Build the Qt desktop application"
    ON
)

if(RATIONALSTUDIO_BUILD_GUI)

    find_package(
        QT NAMES Qt6 Qt5
        REQUIRED COMPONENTS Widgets
    )

    find_package(
        Qt${QT_VERSION_MAJOR}
        REQUIRED COMPONENTS Widgets
    )

    set(CMAKE_AUTOMOC ON)

    # -----------------------------------------------------
    # Rational Studio Desktop Executable
    # -----------------------------------------------------

    add_executable(RationalStudio
        src/main.cpp

        src/gui/MainWindow.hpp
        src/gui/MainWindow.cpp

        src/gui/GraphCanvas.hpp
        src/gui/GraphCanvas.cpp
    )

    target_link_libraries(RationalStudio PRIVATE
        RationalCore
        Qt${QT_VERSION_MAJOR}::Widgets
    )

    target_compile_options(RationalStudio PRIVATE
        $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic>
    )

    set_target_properties(RationalStudio PROPERTIES
        WIN32_EXECUTABLE ON
        MACOSX_BUNDLE ON
    )

    # ---------------------------------------------------------
    # Issue #20 — GraphCanvas Geometry Cache
    # Qt-specific offscreen integration tests
    # --------------------------------------------------------- 

    if(BUILD_TESTING)   

        add_executable(RationalGraphCanvasCacheTests
            tests/GraphCanvasCacheTests.cpp
            src/gui/GraphCanvas.hpp
            src/gui/GraphCanvas.cpp
        )   

        target_include_directories(RationalGraphCanvasCacheTests PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/src/gui"
        )   

        target_link_libraries(RationalGraphCanvasCacheTests PRIVATE
            RationalCore
            Qt${QT_VERSION_MAJOR}::Widgets
        )   

        target_compile_options(RationalGraphCanvasCacheTests PRIVATE
            $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic>
        )   

        add_test(
            NAME RationalGraphCanvasCacheTests
            COMMAND RationalGraphCanvasCacheTests
        )   

        set_tests_properties(
            RationalGraphCanvasCacheTests
            PROPERTIES
                ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
        )   

    endif()

endif()
