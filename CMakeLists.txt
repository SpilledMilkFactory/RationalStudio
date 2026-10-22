cmake_minimum_required(VERSION 3.16)

project(RationalStudio VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ---------------------------------------------------------
# Mathematical Core
# ---------------------------------------------------------

# The mathematical core is Qt-independent.
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
# Automated Testing
# ---------------------------------------------------------

include(CTest)

if(BUILD_TESTING)

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

    add_executable(RationalViewportTests
        tests/GraphViewportTests.cpp
    )

    target_link_libraries(RationalViewportTests PRIVATE
        RationalCore
    )

    target_compile_options(RationalViewportTests PRIVATE
        $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic>
    )

    add_test(
        NAME RationalViewportTests
        COMMAND RationalViewportTests
    )

    # Adaptive curve sampling tests.
    add_executable(RationalCurveSamplerTests
        tests/CurveSamplerTests.cpp
    )

    target_link_libraries(RationalCurveSamplerTests PRIVATE
        RationalCore
    )

    target_compile_options(RationalCurveSamplerTests PRIVATE
        $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic>
    )

    add_test(
        NAME RationalCurveSamplerTests
        COMMAND RationalCurveSamplerTests
    )

    # ---------------------------------------------------------
    # Undefined Function Domain Tests
    # GitHub Issue #16
    # ---------------------------------------------------------

    add_executable(RationalCurveDomainTests
        tests/CurveDomainTests.cpp
    )

    target_link_libraries(RationalCurveDomainTests PRIVATE
        RationalCore
    )

    target_compile_options(RationalCurveDomainTests PRIVATE
        $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic>
    )

    add_test(
        NAME RationalCurveDomainTests
        COMMAND RationalCurveDomainTests
    )

    # ---------------------------------------------------------
    # Finite Discontinuity and Asymptote Regression Tests
    # GitHub Issue #17
    # --------------------------------------------------------- 

    add_executable(RationalCurveDiscontinuityTests
        tests/CurveDiscontinuityTests.cpp
    )   

    target_link_libraries(RationalCurveDiscontinuityTests PRIVATE
        RationalCore
    )   

    target_compile_options(RationalCurveDiscontinuityTests PRIVATE
        $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic>
    )   

    add_test(
        NAME RationalCurveDiscontinuityTests
        COMMAND RationalCurveDiscontinuityTests
    )

    # ---------------------------------------------------------
    # Preserve Continuous Curves With Steep Gradients
    # GitHub Issue #18
    # ---------------------------------------------------------

    add_executable(RationalCurveSteepTests
        tests/CurveSteepTests.cpp
    )   

    target_link_libraries(RationalCurveSteepTests PRIVATE
        RationalCore
    )   

    target_compile_options(RationalCurveSteepTests PRIVATE
        $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic>
    )   

    add_test(
        NAME RationalCurveSteepTests
        COMMAND RationalCurveSteepTests
    )

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

endif()
