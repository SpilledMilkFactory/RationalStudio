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
    src/core/ParametricSampler.cpp
    src/core/ImplicitContour.cpp
    src/core/IntersectionDetection.cpp
    src/core/GraphFeatureDetection.cpp
    src/core/NumericalDifferentiation.cpp
    src/core/NumericalIntegration.cpp
    src/core/NumericalRootFinding.cpp
    src/core/InequalityRegion.hpp
    src/core/InequalityRegion.cpp
    src/core/GraphTrace.cpp
    src/core/VariableSliderScale.cpp
    src/core/ComplexMath.cpp
    src/core/MatrixMath.cpp
    src/core/DescriptiveStatistics.cpp
    src/core/StatisticalRegression.cpp
    src/core/AdvancedMathInput.cpp
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

# M3 / Issue #45: independent optional Release numerical benchmarks.
# Timing data is NOT a CTest pass/fail assertion.
option(RATIONALSTUDIO_BUILD_ADVANCED_GRAPH_BENCHMARKS
    "Build optional M3 advanced graph sampling/trace benchmark" OFF)
if(RATIONALSTUDIO_BUILD_ADVANCED_GRAPH_BENCHMARKS)
    add_executable(RationalAdvancedGraphBench benchmarks/AdvancedGraphBench.cpp)
    target_link_libraries(RationalAdvancedGraphBench PRIVATE RationalCore)
    target_compile_options(RationalAdvancedGraphBench PRIVATE
        $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic>)
endif()

# ---------------------------------------------------------
# Automated Testing — Qt-independent tests
# ---------------------------------------------------------

include(CTest)

if(BUILD_TESTING)
    # M4-09 / Issue #54: cross-service correctness and domain qualification.
    # Timings are not used as pass/fail assertions.
    foreach(suite IN ITEMS NumericalAccuracyMatrix NumericalDomainMatrix)
        add_executable(Rational${suite}Tests tests/${suite}Tests.cpp)
        target_link_libraries(Rational${suite}Tests PRIVATE RationalCore)
        target_compile_options(Rational${suite}Tests PRIVATE
            $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic>)
        add_test(NAME Rational${suite}Tests COMMAND Rational${suite}Tests)
        set_tests_properties(Rational${suite}Tests PROPERTIES
            LABELS "core;advanced-math;accuracy;domain;regression")
    endforeach()
    # M4-08 / Issue #53: typed advanced-input parsing (Qt-independent).
    add_executable(RationalAdvancedMathInputTests tests/AdvancedMathInputTests.cpp)
    target_link_libraries(RationalAdvancedMathInputTests PRIVATE RationalCore)
    target_compile_options(RationalAdvancedMathInputTests PRIVATE
        $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic>)
    add_test(NAME RationalAdvancedMathInputTests COMMAND RationalAdvancedMathInputTests)
    set_tests_properties(RationalAdvancedMathInputTests PROPERTIES
        LABELS "core;advanced-math;regression")
    # M4-07 / Issue #52 — bounded real polynomial/linear least-squares regression.
    add_executable(RationalStatisticalRegressionTests tests/StatisticalRegressionTests.cpp)
    target_link_libraries(RationalStatisticalRegressionTests PRIVATE RationalCore)
    target_compile_options(RationalStatisticalRegressionTests PRIVATE
        $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic>)
    add_test(NAME RationalStatisticalRegressionTests COMMAND RationalStatisticalRegressionTests)
    set_tests_properties(RationalStatisticalRegressionTests PROPERTIES
        LABELS "core;statistics;regression;least-squares")

    # M4-06 / Issue #51 — finite real descriptive statistics.
    add_executable(RationalDescriptiveStatisticsTests tests/DescriptiveStatisticsTests.cpp)
    target_link_libraries(RationalDescriptiveStatisticsTests PRIVATE RationalCore)
    target_compile_options(RationalDescriptiveStatisticsTests PRIVATE
        $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic>)
    add_test(NAME RationalDescriptiveStatisticsTests COMMAND RationalDescriptiveStatisticsTests)
    set_tests_properties(RationalDescriptiveStatisticsTests PROPERTIES
        LABELS "core;statistics;descriptive;regression")

    # M4-05 / Issue #50 — bounded real matrix and vector operations.
    add_executable(RationalMatrixMathTests tests/MatrixMathTests.cpp)
    target_link_libraries(RationalMatrixMathTests PRIVATE RationalCore)
    target_compile_options(RationalMatrixMathTests PRIVATE
        $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic>)
    add_test(NAME RationalMatrixMathTests COMMAND RationalMatrixMathTests)
    set_tests_properties(RationalMatrixMathTests PROPERTIES
        LABELS "core;matrix;vector;linear-algebra;regression")

    # M4-04 / Issue #49 — Implement complex-number arithmetic
    add_executable(RationalComplexMathTests tests/ComplexMathTests.cpp)
    target_link_libraries(RationalComplexMathTests PRIVATE RationalCore)
    target_compile_options(RationalComplexMathTests PRIVATE
        $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic>)
    add_test(NAME RationalComplexMathTests COMMAND RationalComplexMathTests)
    set_tests_properties(RationalComplexMathTests PROPERTIES
        LABELS "core;complex;regression")


    # M4-03 / Issue #48 — Qt-independent numerical equation/root solving.
    add_executable(RationalNumericalRootFindingTests
        tests/NumericalRootFindingTests.cpp)
    target_link_libraries(RationalNumericalRootFindingTests PRIVATE RationalCore)
    target_compile_options(RationalNumericalRootFindingTests PRIVATE
        $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic>)
    add_test(NAME RationalNumericalRootFindingTests
        COMMAND RationalNumericalRootFindingTests)
    set_tests_properties(RationalNumericalRootFindingTests PROPERTIES
        LABELS "core;equations;roots;regression")

    # -----------------------------------------------------
    # M4-02 / Issue #47: adaptive numerical integration.
    # -----------------------------------------------------

    add_executable(RationalNumericalIntegrationTests
        tests/NumericalIntegrationTests.cpp)
    target_link_libraries(RationalNumericalIntegrationTests PRIVATE RationalCore)
    target_compile_options(RationalNumericalIntegrationTests PRIVATE
        $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic>)
    add_test(NAME RationalNumericalIntegrationTests COMMAND RationalNumericalIntegrationTests)
    set_tests_properties(RationalNumericalIntegrationTests PROPERTIES
        LABELS "core;calculus;integration;regression")

    # -----------------------------------------------------
    # M4-01 / Issue #46 — Numerical Differentiation
    # -----------------------------------------------------

    add_executable(RationalNumericalDifferentiationTests
        tests/NumericalDifferentiationTests.cpp
    )

    target_link_libraries(RationalNumericalDifferentiationTests
        PRIVATE
        RationalCore
    )

    target_compile_options(RationalNumericalDifferentiationTests PRIVATE
        $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic>
    )

    add_test(
        NAME RationalNumericalDifferentiationTests
        COMMAND RationalNumericalDifferentiationTests
    )

    set_tests_properties(
        RationalNumericalDifferentiationTests
        PROPERTIES
        LABELS "core;calculus;differentiation;regression"
    )

    # M3 / Issue #45 — cross-representation analytical and resource matrices.
    foreach(suite IN ITEMS AdvancedGraphAccuracyMatrix AdvancedGraphResourceMatrix)
        add_executable(Rational${suite}Tests tests/${suite}Tests.cpp)
        target_link_libraries(Rational${suite}Tests PRIVATE RationalCore)
        target_compile_options(Rational${suite}Tests PRIVATE
            $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic>)
        add_test(NAME Rational${suite}Tests COMMAND Rational${suite}Tests)
        set_tests_properties(Rational${suite}Tests PROPERTIES
            LABELS "core;graphing;advanced;accuracy;regression")
    endforeach()


    # M3 / Issue #44 — Qt-free nearest-segment tracing and parameter metadata.
    add_executable(RationalGraphTraceTests tests/GraphTraceTests.cpp)
    target_link_libraries(RationalGraphTraceTests PRIVATE RationalCore)
    target_compile_options(RationalGraphTraceTests PRIVATE
        $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic>)
    add_test(NAME RationalGraphTraceTests COMMAND RationalGraphTraceTests)
    set_tests_properties(RationalGraphTraceTests PROPERTIES LABELS "core;trace;graphing")

    # M3 / Issue #42 — Qt-free inequality parsing and span classification.
    add_executable(RationalInequalityRegionTests tests/InequalityRegionTests.cpp)
    target_link_libraries(RationalInequalityRegionTests PRIVATE RationalCore)
    target_compile_options(RationalInequalityRegionTests PRIVATE
        $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic>)
    add_test(NAME RationalInequalityRegionTests COMMAND RationalInequalityRegionTests)
    set_tests_properties(RationalInequalityRegionTests PROPERTIES LABELS "core;inequality;graphing")

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
    # Issue #28 — Variable assignments and dependency tracking
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

    # ---------------------------------------------------------
    # Issue #30 — Interactive variable sliders
    # ---------------------------------------------------------

    add_executable(RationalVariableSliderScaleTests
        tests/VariableSliderScaleTests.cpp
    )

    target_link_libraries(RationalVariableSliderScaleTests PRIVATE
        RationalCore
    )

    add_test(NAME RationalVariableSliderScaleTests
             COMMAND RationalVariableSliderScaleTests)

    set_tests_properties(RationalVariableSliderScaleTests PROPERTIES
        LABELS "core;regression;sliders"
    )

    # ---------------------------------------------------------
    # Issue #31 — variable/graph dependency introspection
    # ---------------------------------------------------------
    add_executable(RationalGraphVariableDependencyTests
        tests/GraphVariableDependencyTests.cpp
    )

    target_link_libraries(RationalGraphVariableDependencyTests PRIVATE
        RationalCore
    )

    add_test(NAME RationalGraphVariableDependencyTests
        COMMAND RationalGraphVariableDependencyTests
    )

    set_tests_properties(RationalGraphVariableDependencyTests PROPERTIES
        LABELS "core;regression;graphing;variables"
    )

    # ---------------------------------------------------------
    # Issue #32 — Mathematical Input Symbols / Incomplete Edits
    # ---------------------------------------------------------
    add_executable(RationalMathExpressionInputTests
        tests/MathExpressionInputTests.cpp
    )
    target_link_libraries(RationalMathExpressionInputTests PRIVATE RationalCore)
    add_test(NAME RationalMathExpressionInputTests COMMAND RationalMathExpressionInputTests)
    set_tests_properties(RationalMathExpressionInputTests PROPERTIES
        LABELS "core;regression;expression;input"
    )

    # ---------------------------------------------------------
    # Issue #33 — structured expression diagnostics (Qt independent)
    # ---------------------------------------------------------
    add_executable(RationalExpressionDiagnosticTests
        tests/ExpressionDiagnosticTests.cpp
    )
    target_link_libraries(RationalExpressionDiagnosticTests PRIVATE RationalCore)
    add_test(NAME RationalExpressionDiagnosticTests COMMAND RationalExpressionDiagnosticTests)
    set_tests_properties(RationalExpressionDiagnosticTests PROPERTIES
        LABELS "core;regression;expression;diagnostics"
    )

    # ---------------------------------------------------------
    # Issue #36 — Cross-owner regressions and dynamic evaluation
    # ---------------------------------------------------------
    add_executable(RationalVariableFunctionInteractionTests
        tests/VariableFunctionInteractionTests.cpp
    )
    target_link_libraries(RationalVariableFunctionInteractionTests PRIVATE RationalCore)
    add_test(NAME RationalVariableFunctionInteractionTests
             COMMAND RationalVariableFunctionInteractionTests)
    set_tests_properties(RationalVariableFunctionInteractionTests PROPERTIES
        LABELS "core;regression;variables;functions;integration")

    add_executable(RationalDynamicEvaluationMatrixTests
        tests/DynamicEvaluationMatrixTests.cpp
    )
    target_link_libraries(RationalDynamicEvaluationMatrixTests PRIVATE RationalCore)
    add_test(NAME RationalDynamicEvaluationMatrixTests
             COMMAND RationalDynamicEvaluationMatrixTests)
    set_tests_properties(RationalDynamicEvaluationMatrixTests PROPERTIES
        LABELS "core;regression;variables;functions;dynamic")

    # -----------------------------------------------------
    # M3 / Issue #37 — Qt-free adaptive parametric sampling
    # -----------------------------------------------------
    add_executable(RationalParametricSamplerTests
        tests/ParametricSamplerTests.cpp
    )
    target_link_libraries(RationalParametricSamplerTests PRIVATE RationalCore)
    add_test(NAME RationalParametricSamplerTests COMMAND RationalParametricSamplerTests)
    set_tests_properties(RationalParametricSamplerTests PROPERTIES
        LABELS "core;regression;graphing;parametric")

    # -----------------------------------------------------
    # M3 / Issue #38 — Polar input adapter and numerical reference cases
    # -----------------------------------------------------
    add_executable(RationalPolarExpressionTests
        tests/PolarExpressionTests.cpp
    )
    target_link_libraries(RationalPolarExpressionTests PRIVATE RationalCore)
    add_test(NAME RationalPolarExpressionTests COMMAND RationalPolarExpressionTests)
    set_tests_properties(RationalPolarExpressionTests PROPERTIES
        LABELS "core;regression;graphing;polar")

    # -----------------------------------------------------
    # M3 / Issue #39 — Qt-free bounded implicit contour extraction
    # -----------------------------------------------------
    add_executable(RationalImplicitContourTests tests/ImplicitContourTests.cpp)
    target_link_libraries(RationalImplicitContourTests PRIVATE RationalCore)
    add_test(NAME RationalImplicitContourTests COMMAND RationalImplicitContourTests)
    set_tests_properties(RationalImplicitContourTests PROPERTIES
        LABELS "core;regression;graphing;implicit")

    # -----------------------------------------------------
    # M3 / Issue #40 — bounded numerical Cartesian intersections
    # -----------------------------------------------------
    add_executable(RationalIntersectionDetectionTests tests/IntersectionDetectionTests.cpp)
    target_link_libraries(RationalIntersectionDetectionTests PRIVATE RationalCore)
    add_test(NAME RationalIntersectionDetectionTests COMMAND RationalIntersectionDetectionTests)
    set_tests_properties(RationalIntersectionDetectionTests PROPERTIES
        LABELS "core;regression;graphing;intersections")

    # M3 / Issue #41 — bounded Cartesian feature inspection
    add_executable(RationalGraphFeatureDetectionTests tests/GraphFeatureDetectionTests.cpp)
    target_link_libraries(RationalGraphFeatureDetectionTests PRIVATE RationalCore)
    add_test(NAME RationalGraphFeatureDetectionTests COMMAND RationalGraphFeatureDetectionTests)
    set_tests_properties(RationalGraphFeatureDetectionTests PROPERTIES
        LABELS "core;regression;graphing;features")

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
        RationalVariableSliderScaleTests
        RationalGraphVariableDependencyTests
        RationalMathExpressionInputTests
        RationalExpressionDiagnosticTests
        RationalVariableFunctionInteractionTests
        RationalDynamicEvaluationMatrixTests
        RationalParametricSamplerTests
        RationalPolarExpressionTests
        RationalImplicitContourTests
        RationalIntersectionDetectionTests
        RationalGraphFeatureDetectionTests
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

    # Optional Issue #45 offscreen Qt cold-to-ready/warm-render timing.
    # Not registered as CTest: wall-clock observations are advisory only.
    if(RATIONALSTUDIO_BUILD_ADVANCED_GRAPH_BENCHMARKS)
        add_executable(RationalAdvancedGraphCanvasBench
            benchmarks/AdvancedGraphCanvasBench.cpp
            src/gui/GraphCanvas.hpp src/gui/GraphCanvas.cpp)
        target_include_directories(RationalAdvancedGraphCanvasBench PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/src/gui")
        target_link_libraries(RationalAdvancedGraphCanvasBench PRIVATE
            RationalCore Qt${QT_VERSION_MAJOR}::Widgets)
        target_compile_options(RationalAdvancedGraphCanvasBench PRIVATE
            $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic>)
    endif()

    # -----------------------------------------------------
    # Rational Studio Desktop Executable
    # -----------------------------------------------------

    add_executable(RationalStudio
        src/main.cpp

        src/gui/MainWindow.hpp
        src/gui/MainWindow.cpp
            src/gui/AdvancedMathPage.hpp src/gui/AdvancedMathPage.cpp

        src/gui/GraphCanvas.hpp
        src/gui/GraphCanvas.cpp
    )

    target_sources(RationalStudio PRIVATE
        src/gui/MathExpressionEdit.hpp
        src/gui/MathExpressionEdit.cpp
        src/gui/VariableSliderPanel.hpp
        src/gui/VariableSliderPanel.cpp
    )

    target_include_directories(RationalStudio PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/src/gui"
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
        # M4-08 / Issue #53 — offscreen advanced mathematics interface and
        # MainWindow tab integration. One GUI CTest; existing GUI tests retained.
        add_executable(RationalAdvancedMathGuiTests
            tests/AdvancedMathGuiTests.cpp
            src/gui/AdvancedMathPage.hpp src/gui/AdvancedMathPage.cpp
            src/gui/MainWindow.hpp src/gui/MainWindow.cpp
            src/gui/GraphCanvas.hpp src/gui/GraphCanvas.cpp
            src/gui/MathExpressionEdit.hpp src/gui/MathExpressionEdit.cpp
            src/gui/VariableSliderPanel.hpp src/gui/VariableSliderPanel.cpp
            src/gui/GraphWorkspacePersistence.hpp)
        target_include_directories(RationalAdvancedMathGuiTests PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/src/gui")
        target_link_libraries(RationalAdvancedMathGuiTests PRIVATE
            RationalCore Qt${QT_VERSION_MAJOR}::Widgets)
        target_compile_options(RationalAdvancedMathGuiTests PRIVATE
            $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic>)
        add_test(NAME RationalAdvancedMathGuiTests COMMAND RationalAdvancedMathGuiTests)
        set_tests_properties(RationalAdvancedMathGuiTests PROPERTIES
            ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
            LABELS "gui;advanced-math;regression;qt;offscreen"
            TIMEOUT 120)

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

    # ---------------------------------------------------------
    # Issue #30 — interactive variable sliders
    # ---------------------------------------------------------
        add_executable(RationalVariableSliderPanelTests
            tests/VariableSliderPanelTests.cpp

            src/gui/VariableSliderPanel.hpp
            src/gui/VariableSliderPanel.cpp

            src/gui/MainWindow.hpp
            src/gui/MainWindow.cpp
            src/gui/AdvancedMathPage.hpp src/gui/AdvancedMathPage.cpp
            src/gui/MathExpressionEdit.hpp
            src/gui/MathExpressionEdit.cpp

            src/gui/GraphCanvas.hpp
            src/gui/GraphCanvas.cpp
        )
        target_include_directories(RationalVariableSliderPanelTests PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/src/gui"
        )
        target_link_libraries(RationalVariableSliderPanelTests PRIVATE
            RationalCore Qt${QT_VERSION_MAJOR}::Widgets
        )
        target_compile_options(RationalVariableSliderPanelTests PRIVATE
            $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic>
        )
        add_test(NAME RationalVariableSliderPanelTests
                 COMMAND RationalVariableSliderPanelTests)
        set_tests_properties(RationalVariableSliderPanelTests PROPERTIES
            ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
            LABELS "gui;regression;sliders;qt;offscreen"
            TIMEOUT 30
        )



        # ---------------------------------------------------------
        # Issue #31 — variable-dependent graph refresh integration
        # ---------------------------------------------------------
        add_executable(RationalGraphVariableRefreshTests
            tests/GraphVariableRefreshTests.cpp
            src/gui/MathExpressionEdit.hpp
            src/gui/MathExpressionEdit.cpp
            src/gui/MainWindow.hpp
            src/gui/MainWindow.cpp
            src/gui/AdvancedMathPage.hpp src/gui/AdvancedMathPage.cpp
            src/gui/GraphCanvas.hpp
            src/gui/GraphCanvas.cpp
            src/gui/VariableSliderPanel.hpp
            src/gui/VariableSliderPanel.cpp
        )

        target_include_directories(RationalGraphVariableRefreshTests PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/src/gui"
        )

        target_link_libraries(RationalGraphVariableRefreshTests PRIVATE
            RationalCore
            Qt${QT_VERSION_MAJOR}::Widgets
        )

        target_compile_options(RationalGraphVariableRefreshTests PRIVATE
            $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic>
        )

        add_test(NAME RationalGraphVariableRefreshTests
            COMMAND RationalGraphVariableRefreshTests
        )

        set_tests_properties(RationalGraphVariableRefreshTests PROPERTIES
            ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
            LABELS "gui;regression;graphing;variables;qt;offscreen"
            TIMEOUT 60
        )

        # ---------------------------------------------------------
        # Issue #32 — Mathematical Expression Editor Integration
        # ---------------------------------------------------------
        add_executable(RationalMathExpressionEditorTests
            tests/MathExpressionEditorTests.cpp
            src/gui/MathExpressionEdit.hpp
            src/gui/MathExpressionEdit.cpp
            src/gui/MainWindow.hpp
            src/gui/MainWindow.cpp
            src/gui/AdvancedMathPage.hpp src/gui/AdvancedMathPage.cpp
            src/gui/GraphCanvas.hpp
            src/gui/GraphCanvas.cpp
            src/gui/VariableSliderPanel.hpp
            src/gui/VariableSliderPanel.cpp
        )
        target_include_directories(RationalMathExpressionEditorTests PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/src/gui"
        )
        target_link_libraries(RationalMathExpressionEditorTests PRIVATE
            RationalCore Qt${QT_VERSION_MAJOR}::Widgets
        )
        target_compile_options(RationalMathExpressionEditorTests PRIVATE
            $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic>
        )
        add_test(NAME RationalMathExpressionEditorTests COMMAND RationalMathExpressionEditorTests)
        set_tests_properties(RationalMathExpressionEditorTests PROPERTIES
            ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
            LABELS "gui;regression;expression;input;qt;offscreen"
            TIMEOUT 60
        )
        # ---------------------------------------------------------
        # Issue #33 — inline expression feedback in real MainWindow
        # ---------------------------------------------------------
        add_executable(RationalExpressionInlineFeedbackTests
            tests/ExpressionInlineFeedbackTests.cpp
            src/gui/MathExpressionEdit.hpp
            src/gui/MathExpressionEdit.cpp
            src/gui/MainWindow.hpp
            src/gui/MainWindow.cpp
            src/gui/AdvancedMathPage.hpp src/gui/AdvancedMathPage.cpp
            src/gui/GraphCanvas.hpp
            src/gui/GraphCanvas.cpp
            src/gui/VariableSliderPanel.hpp
            src/gui/VariableSliderPanel.cpp
        )
        target_include_directories(RationalExpressionInlineFeedbackTests PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/src/gui"
        )
        target_link_libraries(RationalExpressionInlineFeedbackTests PRIVATE
            RationalCore Qt${QT_VERSION_MAJOR}::Widgets
        )
        target_compile_options(RationalExpressionInlineFeedbackTests PRIVATE
            $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic>
        )
        add_test(NAME RationalExpressionInlineFeedbackTests
                 COMMAND RationalExpressionInlineFeedbackTests)
        set_tests_properties(RationalExpressionInlineFeedbackTests PROPERTIES
            ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
            LABELS "gui;regression;expression;diagnostics;offscreen;qt"
            TIMEOUT 60
        )
        # ---------------------------------------------------------
        # Issue #34 — Mathematical keyboard shortcuts and row navigation
        # ---------------------------------------------------------
        add_executable(RationalMathKeyboardShortcutTests
            tests/MathKeyboardShortcutTests.cpp
            src/gui/MathExpressionEdit.hpp
            src/gui/MathExpressionEdit.cpp
            src/gui/MainWindow.hpp
            src/gui/MainWindow.cpp
            src/gui/AdvancedMathPage.hpp src/gui/AdvancedMathPage.cpp
            src/gui/GraphCanvas.hpp
            src/gui/GraphCanvas.cpp
            src/gui/VariableSliderPanel.hpp
            src/gui/VariableSliderPanel.cpp
        )
        target_include_directories(RationalMathKeyboardShortcutTests PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/src/gui"
        )
        target_link_libraries(RationalMathKeyboardShortcutTests PRIVATE
            RationalCore Qt${QT_VERSION_MAJOR}::Widgets
        )
        target_compile_options(RationalMathKeyboardShortcutTests PRIVATE
            $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic>
        )
        add_test(NAME RationalMathKeyboardShortcutTests
                 COMMAND RationalMathKeyboardShortcutTests)
        set_tests_properties(RationalMathKeyboardShortcutTests PROPERTIES
            ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
            LABELS "gui;regression;keyboard;expression;qt;offscreen"
            TIMEOUT 60
        )
        # ---------------------------------------------------------
        # Issue #35 — Transactional graph workspace persistence (v1/v2)
        # ---------------------------------------------------------
        add_executable(RationalGraphWorkspacePersistenceTests
            tests/GraphWorkspacePersistenceTests.cpp
            src/gui/MainWindow.hpp
            src/gui/MainWindow.cpp
            src/gui/AdvancedMathPage.hpp src/gui/AdvancedMathPage.cpp
            src/gui/MathExpressionEdit.hpp
            src/gui/MathExpressionEdit.cpp
            src/gui/GraphCanvas.hpp
            src/gui/GraphCanvas.cpp
            src/gui/VariableSliderPanel.hpp
            src/gui/VariableSliderPanel.cpp
            src/gui/GraphWorkspacePersistence.hpp
        )
        target_include_directories(RationalGraphWorkspacePersistenceTests PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/src/gui"
        )
        target_link_libraries(RationalGraphWorkspacePersistenceTests PRIVATE
            RationalCore Qt${QT_VERSION_MAJOR}::Widgets
        )
        target_compile_options(RationalGraphWorkspacePersistenceTests PRIVATE
            $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic>
        )
        add_test(NAME RationalGraphWorkspacePersistenceTests
                 COMMAND RationalGraphWorkspacePersistenceTests)
        set_tests_properties(RationalGraphWorkspacePersistenceTests PROPERTIES
            ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
            LABELS "gui;regression;persistence;variables;functions;qt;offscreen"
            TIMEOUT 60
        )
        # ---------------------------------------------------------
        # M3 / Issue #37 — Parametric graph and v3 workspace integration
        # ---------------------------------------------------------
        add_executable(RationalParametricGraphTests
            tests/ParametricGraphTests.cpp
            src/gui/MainWindow.hpp src/gui/MainWindow.cpp
            src/gui/AdvancedMathPage.hpp src/gui/AdvancedMathPage.cpp
            src/gui/GraphCanvas.hpp src/gui/GraphCanvas.cpp
            src/gui/MathExpressionEdit.hpp src/gui/MathExpressionEdit.cpp
            src/gui/VariableSliderPanel.hpp src/gui/VariableSliderPanel.cpp
            src/gui/GraphWorkspacePersistence.hpp
        )
        target_include_directories(RationalParametricGraphTests PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/src/gui")
        target_link_libraries(RationalParametricGraphTests PRIVATE
            RationalCore Qt${QT_VERSION_MAJOR}::Widgets)
        target_compile_options(RationalParametricGraphTests PRIVATE
            $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic>)
        add_test(NAME RationalParametricGraphTests COMMAND RationalParametricGraphTests)
        set_tests_properties(RationalParametricGraphTests PROPERTIES
            ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
            LABELS "gui;regression;parametric;graphing;qt;offscreen"
            TIMEOUT 90)
        # ---------------------------------------------------------
        # M3 / Issue #38 — Polar graph integration and v3 persistence
        # ---------------------------------------------------------
        add_executable(RationalPolarGraphTests
            tests/PolarGraphTests.cpp
            src/gui/MainWindow.hpp src/gui/MainWindow.cpp
            src/gui/AdvancedMathPage.hpp src/gui/AdvancedMathPage.cpp
            src/gui/GraphCanvas.hpp src/gui/GraphCanvas.cpp
            src/gui/MathExpressionEdit.hpp src/gui/MathExpressionEdit.cpp
            src/gui/VariableSliderPanel.hpp src/gui/VariableSliderPanel.cpp
            src/gui/GraphWorkspacePersistence.hpp
        )
        target_include_directories(RationalPolarGraphTests PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/src/gui")
        target_link_libraries(RationalPolarGraphTests PRIVATE
            RationalCore Qt${QT_VERSION_MAJOR}::Widgets)
        target_compile_options(RationalPolarGraphTests PRIVATE
            $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic>)
        add_test(NAME RationalPolarGraphTests COMMAND RationalPolarGraphTests)
        set_tests_properties(RationalPolarGraphTests PROPERTIES
            ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
            LABELS "gui;regression;polar;graphing;qt;offscreen"
            TIMEOUT 90)
        # ---------------------------------------------------------
        # M3 / Issue #39 — Implicit contour integration and persistence
        # ---------------------------------------------------------
        add_executable(RationalImplicitGraphTests
            tests/ImplicitGraphTests.cpp
            src/gui/MainWindow.hpp src/gui/MainWindow.cpp
            src/gui/AdvancedMathPage.hpp src/gui/AdvancedMathPage.cpp
            src/gui/GraphCanvas.hpp src/gui/GraphCanvas.cpp
            src/gui/MathExpressionEdit.hpp src/gui/MathExpressionEdit.cpp
            src/gui/VariableSliderPanel.hpp src/gui/VariableSliderPanel.cpp
            src/gui/GraphWorkspacePersistence.hpp
        )
        target_include_directories(RationalImplicitGraphTests PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/src/gui")
        target_link_libraries(RationalImplicitGraphTests PRIVATE
            RationalCore Qt${QT_VERSION_MAJOR}::Widgets)
        target_compile_options(RationalImplicitGraphTests PRIVATE
            $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic>)
        add_test(NAME RationalImplicitGraphTests COMMAND RationalImplicitGraphTests)
        set_tests_properties(RationalImplicitGraphTests PROPERTIES
            ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
            LABELS "gui;regression;implicit;graphing;qt;offscreen"
            TIMEOUT 120)
        # ---------------------------------------------------------
        # M3 / Issue #40 — numerical intersections GUI integration
        # ---------------------------------------------------------
        add_executable(RationalIntersectionGraphTests
            tests/IntersectionGraphTests.cpp
            src/gui/MainWindow.hpp src/gui/MainWindow.cpp
            src/gui/AdvancedMathPage.hpp src/gui/AdvancedMathPage.cpp
            src/gui/GraphCanvas.hpp src/gui/GraphCanvas.cpp
            src/gui/MathExpressionEdit.hpp src/gui/MathExpressionEdit.cpp
            src/gui/VariableSliderPanel.hpp src/gui/VariableSliderPanel.cpp
            src/gui/GraphWorkspacePersistence.hpp
        )
        target_include_directories(RationalIntersectionGraphTests PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/src/gui")
        target_link_libraries(RationalIntersectionGraphTests PRIVATE
            RationalCore Qt${QT_VERSION_MAJOR}::Widgets)
        target_compile_options(RationalIntersectionGraphTests PRIVATE
            $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic>)
        add_test(NAME RationalIntersectionGraphTests COMMAND RationalIntersectionGraphTests)
        set_tests_properties(RationalIntersectionGraphTests PROPERTIES
            ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
            LABELS "gui;regression;intersections;graphing;qt;offscreen"
            TIMEOUT 90)
        # M3 / Issue #42 — inequality shading Qt integration and v3 persistence
        add_executable(RationalInequalityGraphTests
            tests/InequalityGraphTests.cpp
            src/gui/MainWindow.hpp src/gui/MainWindow.cpp
            src/gui/AdvancedMathPage.hpp src/gui/AdvancedMathPage.cpp
            src/gui/GraphCanvas.hpp src/gui/GraphCanvas.cpp
            src/gui/MathExpressionEdit.hpp src/gui/MathExpressionEdit.cpp
            src/gui/VariableSliderPanel.hpp src/gui/VariableSliderPanel.cpp
            src/gui/GraphWorkspacePersistence.hpp
        )
        target_include_directories(RationalInequalityGraphTests PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/src/gui")
        target_link_libraries(RationalInequalityGraphTests PRIVATE
            RationalCore Qt${QT_VERSION_MAJOR}::Widgets)
        target_compile_options(RationalInequalityGraphTests PRIVATE
            $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic>)
        add_test(NAME RationalInequalityGraphTests COMMAND RationalInequalityGraphTests)
        set_tests_properties(RationalInequalityGraphTests PROPERTIES
            ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
            LABELS "gui;regression;inequality;graphing;qt;offscreen"
            TIMEOUT 120)
        # M3 / Issue #44 — offscreen trace for all supported representations.
        add_executable(RationalGraphTracingTests
            tests/GraphTracingTests.cpp
            src/gui/GraphCanvas.hpp src/gui/GraphCanvas.cpp)
        target_include_directories(RationalGraphTracingTests PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/src/gui")
        target_link_libraries(RationalGraphTracingTests PRIVATE
            RationalCore Qt${QT_VERSION_MAJOR}::Widgets)
        target_compile_options(RationalGraphTracingTests PRIVATE
            $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic>)
        add_test(NAME RationalGraphTracingTests COMMAND RationalGraphTracingTests)
        set_tests_properties(RationalGraphTracingTests PROPERTIES
            ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
            LABELS "gui;trace;graphing;qt;offscreen"
            TIMEOUT 120)
        # M3 / Issue #43 — representation chooser, contextual labels, and
        # mixed graph type controls (Qt-only UI, no mathematical core changes).
        add_executable(RationalGraphRepresentationControlsTests
            tests/GraphRepresentationControlsTests.cpp
            src/gui/MainWindow.hpp src/gui/MainWindow.cpp
            src/gui/AdvancedMathPage.hpp src/gui/AdvancedMathPage.cpp
            src/gui/GraphCanvas.hpp src/gui/GraphCanvas.cpp
            src/gui/MathExpressionEdit.hpp src/gui/MathExpressionEdit.cpp
            src/gui/VariableSliderPanel.hpp src/gui/VariableSliderPanel.cpp
            src/gui/GraphWorkspacePersistence.hpp
        )
        target_include_directories(RationalGraphRepresentationControlsTests PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/src/gui")
        target_link_libraries(RationalGraphRepresentationControlsTests PRIVATE
            RationalCore Qt${QT_VERSION_MAJOR}::Widgets)
        target_compile_options(RationalGraphRepresentationControlsTests PRIVATE
            $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic>)
        add_test(NAME RationalGraphRepresentationControlsTests
            COMMAND RationalGraphRepresentationControlsTests)
        set_tests_properties(RationalGraphRepresentationControlsTests PROPERTIES
            ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
            LABELS "gui;regression;representation;graphing;qt;offscreen"
            TIMEOUT 120)
        # M3 / Issue #41 — feature inspection Qt integration
        add_executable(RationalGraphFeatureGraphTests
            tests/GraphFeatureGraphTests.cpp
            src/gui/MainWindow.hpp src/gui/MainWindow.cpp
            src/gui/AdvancedMathPage.hpp src/gui/AdvancedMathPage.cpp
            src/gui/GraphCanvas.hpp src/gui/GraphCanvas.cpp
            src/gui/MathExpressionEdit.hpp src/gui/MathExpressionEdit.cpp
            src/gui/VariableSliderPanel.hpp src/gui/VariableSliderPanel.cpp
            src/gui/GraphWorkspacePersistence.hpp
        )
        target_include_directories(RationalGraphFeatureGraphTests PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/src/gui")
        target_link_libraries(RationalGraphFeatureGraphTests PRIVATE
            RationalCore Qt${QT_VERSION_MAJOR}::Widgets)
        target_compile_options(RationalGraphFeatureGraphTests PRIVATE
            $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic>)
        add_test(NAME RationalGraphFeatureGraphTests COMMAND RationalGraphFeatureGraphTests)
        set_tests_properties(RationalGraphFeatureGraphTests PROPERTIES
            ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
            LABELS "gui;regression;features;graphing;qt;offscreen"
            TIMEOUT 90)
    endif()

endif()
