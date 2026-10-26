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
    src/core/VariableDefinitions.cpp
    src/core/FunctionDefinitions.cpp
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
    # Issue #21 — Shared Mathematical Evaluation Budget
    # -----------------------------------------------------

    add_executable(RationalCurveEvaluationBudgetTests
        tests/CurveEvaluationBudgetTests.cpp
    )

    target_link_libraries(RationalCurveEvaluationBudgetTests PRIVATE 
        RationalCore
    )

    add_test(
        NAME RationalCurveEvaluationBudgetTests
        COMMAND RationalCurveEvaluationBudgetTests
    )

    # -----------------------------------------------------
    # Issue #22 — Graph Rendering & Navigation Scheduling
    # -----------------------------------------------------

    add_executable(RationalGraphRepaintSchedulerTests
        tests/GraphRepaintSchedulerTests.cpp
    )

    target_link_libraries(RationalGraphRepaintSchedulerTests PRIVATE
        RationalCore
    )

    add_test(
        NAME RationalGraphRepaintSchedulerTests
        COMMAND RationalGraphRepaintSchedulerTests
    )

    # -----------------------------------------------------
    # Issue #23 — Viewport Boundaries & Safe Grid Layout
    # -----------------------------------------------------

    add_executable(RationalGraphViewportBoundaryTests
        tests/GraphViewportBoundaryTests.cpp
    )

    target_link_libraries(RationalGraphViewportBoundaryTests PRIVATE
        RationalCore
    )

    add_test(NAME RationalGraphViewportBoundaryTests
             COMMAND RationalGraphViewportBoundaryTests)

    # -----------------------------------------------------------------
    # Issue #24 — Cross-Subsystem Graph Sampling / Viewport Regressions
    # -----------------------------------------------------------------

    add_executable(RationalGraphRegressionMatrixTests
    tests/GraphRegressionMatrixTests.cpp
    )

    target_link_libraries(RationalGraphRegressionMatrixTests PRIVATE
        RationalCore
    )

    add_test(NAME RationalGraphRegressionMatrixTests
             COMMAND RationalGraphRegressionMatrixTests)

    set_tests_properties(RationalGraphRegressionMatrixTests PROPERTIES
        LABELS "graphing;regression;core"
    )

    # -----------------------------------------------------------------------------
    # Issue #25 — Benchmark Statistics/Output Correctness (not a speed threshold)
    # -----------------------------------------------------------------------------

    add_executable(RationalGraphBenchmarkStatisticsTests
        tests/GraphBenchmarkStatisticsTests.cpp
    )
    target_include_directories(RationalGraphBenchmarkStatisticsTests PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/benchmarks"
    )

    add_test(NAME RationalGraphBenchmarkStatisticsTests
             COMMAND RationalGraphBenchmarkStatisticsTests)

    set_tests_properties(RationalGraphBenchmarkStatisticsTests PROPERTIES
        LABELS "graphing;benchmark;core;regression"
    )

    # ---------------------------------------------------------
    # Issue #26 — Wide-Viewport Curve Coverage Regression
    # ---------------------------------------------------------

    add_executable(RationalCurveCoverageRegressionTests
        tests/CurveCoverageRegressionTests.cpp
    )

    target_link_libraries(RationalCurveCoverageRegressionTests PRIVATE
        RationalCore
    )

    add_test(
        NAME RationalCurveCoverageRegressionTests
        COMMAND RationalCurveCoverageRegressionTests
    )

    add_executable(RationalCurveSamplingJobTests tests/CurveSamplingJobTests.cpp)
    
    target_link_libraries(RationalCurveSamplingJobTests PRIVATE
        RationalCore
    )

    add_test(NAME RationalCurveSamplingJobTests COMMAND RationalCurveSamplingJobTests)


    # %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
    # ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ MILESTONE 2 ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    # %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

    # ---------------------------------------------------------
    # Issue #27 — Named Variable Parsing
    # ---------------------------------------------------------

    add_executable(RationalNamedVariableTests
        tests/NamedVariableTests.cpp
    )   

    target_link_libraries(RationalNamedVariableTests PRIVATE
        RationalCore
    )   

    add_test(
        NAME RationalNamedVariableTests
        COMMAND RationalNamedVariableTests
    )   

    set_tests_properties(RationalNamedVariableTests PROPERTIES
        LABELS "core;regression;variables"
    )

    # ---------------------------------------------------------
    # Issue #28 — Named Variable Parsing
    # ---------------------------------------------------------

    add_executable(RationalVariableDefinitionsTests
        tests/VariableDefinitionsTests.cpp
    )   

    target_link_libraries(RationalVariableDefinitionsTests PRIVATE
        RationalCore
    )   

    add_test(
        NAME RationalVariableDefinitionsTests
        COMMAND RationalVariableDefinitionsTests
    )   

    set_tests_properties(
        RationalVariableDefinitionsTests
        PROPERTIES
            LABELS "core;regression;variables"
    )

    # ---------------------------------------------------------
    # Issue #29 — Reusable mathematical functions
    # ---------------------------------------------------------

    add_executable(RationalFunctionDefinitionsTests
        tests/FunctionDefinitionsTests.cpp
    )

    target_link_libraries(RationalFunctionDefinitionsTests PRIVATE
        RationalCore
    )

    add_test(NAME RationalFunctionDefinitionsTests COMMAND RationalFunctionDefinitionsTests)
    
    set_tests_properties(RationalFunctionDefinitionsTests PROPERTIES
        LABELS "core;regression;functions"
    )

    # -----------------------------------------------------
    # Shared Test Compiler Warnings
    # -----------------------------------------------------

    foreach(test_target IN ITEMS
        # Milestone 1:
        RationalCoreTests 
        RationalViewportTests
        RationalCurveSamplerTests
        RationalCurveDomainTests
        RationalCurveDiscontinuityTests
        RationalCurveSteepTests
        RationalViewportPrecisionTests
        RationalCurveGeometryCacheTests
        RationalCurveEvaluationBudgetTests
        RationalGraphRepaintSchedulerTests
        RationalGraphViewportBoundaryTests
        RationalGraphRegressionMatrixTests
        RationalGraphBenchmarkStatisticsTests
        RationalCurveCoverageRegressionTests
        RationalCurveSamplingJobTests
        # Milestone 2:
        RationalNamedVariableTests
        RationalVariableDefinitionsTests
        RationalFunctionDefinitionsTests
    )

        target_compile_options(${test_target} PRIVATE
            $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic>
        )

    endforeach()

endif()

#################################################### BENCHMARK #####################################################

# ---------------------------------------------------------
# Qt-independent benchmark 
# ---------------------------------------------------------

option(RATIONALSTUDIO_BUILD_BENCHMARKS
    "Build optional graphing benchmark executables"
    OFF
)

if(RATIONALSTUDIO_BUILD_BENCHMARKS)
    add_executable(RationalGraphSamplingBench
        benchmarks/GraphSamplingBench.cpp
    )

    target_include_directories(RationalGraphSamplingBench PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/benchmarks"
    )

    target_link_libraries(RationalGraphSamplingBench PRIVATE RationalCore)
    
    target_compile_options(RationalGraphSamplingBench PRIVATE
        $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic>
    )

endif()

####################################################################################################################

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

    

    if(BUILD_TESTING)

    # ---------------------------------------------------------
    # Issue #20 — GraphCanvas Geometry Cache
    # Qt-specific offscreen integration tests
    # ---------------------------------------------------------

        add_executable(RationalGraphCanvasCoverageTests
            tests/GraphCanvasCoverageTests.cpp src/gui/GraphCanvas.cpp)
        target_include_directories(RationalGraphCanvasCoverageTests PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/src/gui")
        target_link_libraries(RationalGraphCanvasCoverageTests PRIVATE
            RationalCore Qt${QT_VERSION_MAJOR}::Widgets)
        add_test(NAME RationalGraphCanvasCoverageTests COMMAND RationalGraphCanvasCoverageTests)
        set_tests_properties(RationalGraphCanvasCoverageTests PROPERTIES
            ENVIRONMENT "QT_QPA_PLATFORM=offscreen" TIMEOUT 60)

        add_test(NAME RationalGraphCanvasCoverageHiDpiTests
            COMMAND RationalGraphCanvasCoverageTests --dpi-smoke)
        set_tests_properties(RationalGraphCanvasCoverageHiDpiTests PROPERTIES
            ENVIRONMENT "QT_QPA_PLATFORM=offscreen;QT_SCALE_FACTOR=2" TIMEOUT 60)

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
    # ---------------------------------------------------------
    # Issue #21 — bounded GraphCanvas sampling work per repaint
    # ---------------------------------------------------------

        add_executable(RationalGraphCanvasBudgetTests
            tests/GraphCanvasBudgetTests.cpp
            src/gui/GraphCanvas.hpp
            src/gui/GraphCanvas.cpp
        )
        target_include_directories(RationalGraphCanvasBudgetTests PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/src/gui"
        )
        target_link_libraries(RationalGraphCanvasBudgetTests PRIVATE
            RationalCore
            Qt${QT_VERSION_MAJOR}::Widgets
        )
        target_compile_options(RationalGraphCanvasBudgetTests PRIVATE
            $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic>
        )
        add_test(
            NAME RationalGraphCanvasBudgetTests
            COMMAND RationalGraphCanvasBudgetTests
        )
        set_tests_properties(RationalGraphCanvasBudgetTests PROPERTIES
            ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
        )

    # ---------------------------------------------------------
    # Issue #22 — interactive repaint coalescing in GraphCanvas
    # ---------------------------------------------------------

        add_executable(RationalGraphCanvasSchedulingTests
            tests/GraphCanvasSchedulingTests.cpp
            src/gui/GraphCanvas.hpp
            src/gui/GraphCanvas.cpp
        )
        target_include_directories(RationalGraphCanvasSchedulingTests PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/src/gui"
        )
        target_link_libraries(RationalGraphCanvasSchedulingTests PRIVATE
            RationalCore
            Qt${QT_VERSION_MAJOR}::Widgets
        )
        target_compile_options(RationalGraphCanvasSchedulingTests PRIVATE
            $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic>
        )
        add_test(
            NAME RationalGraphCanvasSchedulingTests
            COMMAND RationalGraphCanvasSchedulingTests
        )
        set_tests_properties(RationalGraphCanvasSchedulingTests PROPERTIES
            ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
        )
    # ---------------------------------------------------------
    # Issue #23 — offscreen resizing / boundary integration
    # ---------------------------------------------------------
        add_executable(RationalGraphCanvasBoundaryTests
            tests/GraphCanvasBoundaryTests.cpp
            src/gui/GraphCanvas.hpp
            src/gui/GraphCanvas.cpp
        )
        target_include_directories(RationalGraphCanvasBoundaryTests PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/src/gui"
        )
        target_link_libraries(RationalGraphCanvasBoundaryTests PRIVATE
            RationalCore
            Qt${QT_VERSION_MAJOR}::Widgets
        )
        target_compile_options(RationalGraphCanvasBoundaryTests PRIVATE
            $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic>
        )
        add_test(NAME RationalGraphCanvasBoundaryTests
                 COMMAND RationalGraphCanvasBoundaryTests)
        set_tests_properties(RationalGraphCanvasBoundaryTests PROPERTIES
            ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
        )

    # ---------------------------------------------------------
    # Issue #24 — offscreen graph regression matrix
    # ---------------------------------------------------------

        add_executable(RationalGraphCanvasRegressionMatrixTests
            tests/GraphCanvasRegressionMatrixTests.cpp
            src/gui/GraphCanvas.hpp
            src/gui/GraphCanvas.cpp
        )
        target_include_directories(RationalGraphCanvasRegressionMatrixTests PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/src/gui"
        )
        target_link_libraries(RationalGraphCanvasRegressionMatrixTests PRIVATE
            RationalCore
            Qt${QT_VERSION_MAJOR}::Widgets
        )
        target_compile_options(RationalGraphCanvasRegressionMatrixTests PRIVATE
            $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic>
        )
        add_test(NAME RationalGraphCanvasRegressionMatrixTests
                 COMMAND RationalGraphCanvasRegressionMatrixTests)
        set_tests_properties(RationalGraphCanvasRegressionMatrixTests PROPERTIES
            ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
            LABELS "graphing;regression;qt;offscreen"
            TIMEOUT 30
        )

    # ---------------------------------------------------------
    # Issue #25 — Qt offscreen benchmark
    # ---------------------------------------------------------

        if(RATIONALSTUDIO_BUILD_BENCHMARKS)
            add_executable(RationalGraphCanvasBench
                benchmarks/GraphCanvasBench.cpp
                src/gui/GraphCanvas.hpp
                src/gui/GraphCanvas.cpp
            )
            target_include_directories(RationalGraphCanvasBench PRIVATE
                "${CMAKE_CURRENT_SOURCE_DIR}/benchmarks"
                "${CMAKE_CURRENT_SOURCE_DIR}/src/gui"
            )
            target_link_libraries(RationalGraphCanvasBench PRIVATE
                RationalCore Qt${QT_VERSION_MAJOR}::Widgets
            )
            target_compile_options(RationalGraphCanvasBench PRIVATE
                $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic>
            )
        endif()

    endif()

endif()
