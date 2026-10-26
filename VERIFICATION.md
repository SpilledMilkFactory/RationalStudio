# Rational Studio — Verification Report

<!-- ISSUE28_VERIFICATION_CURRENT_BEGIN -->

**Project version:** 0.1.0  
**Verification date:** October 26, 2026  
**Recorded environment:** Linux, GCC 15.2.0, C++20, CMake/Ninja, Qt Widgets  
**Current status:** **PASS — Issue #28 (M2-02) scalar variable assignments and dependency tracking.** GCC 15.2.0 Linux GUI-enabled Debug CTest **24/24 PASS** (10.94 s), GUI-enabled Release CTest **24/24 PASS** (5.03 s), and core-only ASan/UBSan CTest **17/17 PASS** (6.66 s). `RationalVariableDefinitionsTests` passed **951 assertions**. Existing GUI behavior was checked manually and reported unchanged; this was an informal developer regression check, not a scripted acceptance session. Issue #28 is core-only: GUI assignment rows, reusable functions, sliders, automatic graph updates, and persistence are not yet implemented. Historical Issue #25 benchmark measurements remain unchanged.

This document records the current Issue #28 variable-assignment and dependency-tracking verification, following Issue #27 named-variable parsing. The prior Issue #27 results, earlier graphing regression history, and Issue #25 measured performance baseline remain as historical evidence. It is a **verification record**, not a development roadmap. Results apply to the configurations actually tested; they do not establish exhaustive mathematical correctness or independently verify future GUI variable-editing features.

## 1. Build environment

| Component | Recorded configuration |
|---|---|
| Operating system | Linux |
| Compiler | GCC 15.2.0 |
| Language standard | C++20 |
| Build system | CMake with Ninja generator |
| Current Issue #28 application builds | GCC 15.2.0 GUI-enabled Debug 24/24 CTests (incremental build); GUI-enabled Release full build 24/24 CTests; core-only ASan/UBSan build 17/17 CTests |
| Historical Issue #27 application builds | GCC 15.2.0 Debug and Release GUI-enabled builds and 23/23 CTests each; core-only ASan/UBSan 16/16 CTests |
| Historical Issue #26 application builds | Debug GUI-enabled full build and tests; Release desktop application target rebuilt |
| Historical Issue #25 application builds | Debug and Release, GUI enabled; optional benchmark targets ON |
| GUI framework | Qt Widgets |
| Exact arithmetic | Boost.Multiprecision |
| Test runner | CTest |
| Sanitizers | AddressSanitizer and UndefinedBehaviorSanitizer; core-only build |

The Issue #26 GUI builds selected Qt 6. Qt 6 is the preferred development target. The exact Qt version selected by the recorded CMake configuration was not displayed in the submitted console output. Native Windows compatibility and Qt 5 compatibility have not been independently verified.

## 2. Debug build — PASS

### Issue #28 current Debug and Release build results

**Debug (GCC 15.2.0, C++20, Ninja, Qt Widgets):**

```bash
cmake -S . -B build-issue28 -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build-issue28 --parallel
ctest --test-dir build-issue28 --output-on-failure
```

```text
[9/9] Linking CXX executable RationalVariableDefinitionsTests
100% tests passed, 0 tests failed out of 24
Total Test time (real) = 10.94 sec
```

The Debug build was incremental in the supplied terminal log; this result
confirms successful configuration, linking, and full CTest execution, not a
fresh-from-empty-tree Debug compilation.

**Release (GCC 15.2.0, C++20, Ninja, Qt Widgets):**

```bash
cmake -S . -B build-issue28-release -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build-issue28-release --parallel
ctest --test-dir build-issue28-release --output-on-failure
```

```text
[78/78] Linking CXX executable RationalStudio
100% tests passed, 0 tests failed out of 24
Total Test time (real) = 5.03 sec
```

The Release build completed successfully; all previously registered GUI,
viewport, graphing, and mathematical regressions passed alongside the new
Qt-independent definition-store suite.

### Issue #27 historical Debug and Release build results

**Debug (GCC 15.2.0, C++20, Ninja, Qt Widgets):**

```bash
cmake -S . -B build-issue27 -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build-issue27 --parallel
ctest --test-dir build-issue27 --output-on-failure
```

```text
[75/75] Linking CXX executable RationalStudio
100% tests passed, 0 tests failed out of 23
Total Test time (real) = 10.35 sec
```

**Release (GCC 15.2.0, C++20, Ninja, Qt Widgets):**

```bash
cmake -S . -B build-issue27-release -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build-issue27-release --parallel
ctest --test-dir build-issue27-release --output-on-failure
```

```text
[75/75] Linking CXX executable RationalStudio
100% tests passed, 0 tests failed out of 23
Total Test time (real) = 5.10 sec
```

Both full GUI-enabled builds succeeded and all registered tests passed. The
previously available graphics/cache/budget/scheduling regressions remain part
of these suites. No new native Windows or Qt 5 verification was performed.

### Issue #26 historical build results

**Commands executed during implementation:**

```bash
cmake --build build-issue26 -j 4
ctest --test-dir build-issue26 --output-on-failure -j 4
cmake --build build-issue26-release --target RationalStudio -j 4
```

The existing GCC 15.2.0 Debug configuration rebuilt RationalStudio, RationalCLI, and the regression executables. The full GUI-enabled Debug suite passed **22/22** tests. The existing Release configuration rebuilt the RationalStudio desktop target successfully; a complete current Release CTest run was not recorded. Both configurations use C++20 and Qt 6 Widgets. Configuration details are retained locally in their `CMakeCache.txt` files.

### Issue #25 historical Debug build

**Commands executed:**

```bash
cmake -S . -B build-issue25 -G Ninja -DCMAKE_BUILD_TYPE=Debug \
    -DRATIONALSTUDIO_BUILD_BENCHMARKS=ON
cmake --build build-issue25 --parallel
```

GCC 15.2.0 successfully configured and built RationalStudio, RationalCLI, the test executables, and both optional benchmarking executables.

**Final recorded Debug build output:**

```text
[71/71] Linking CXX executable RationalStudio
```

No compiler warnings or errors appeared in the submitted final Issue #25 Debug build output; this does not constitute a `-Werror` verification.

### Issue #25 historical Release build — PASS

**Commands executed:**

```bash
cmake -S . -B build-issue25-release -G Ninja -DCMAKE_BUILD_TYPE=Release \
    -DRATIONALSTUDIO_BUILD_BENCHMARKS=ON
cmake --build build-issue25-release --parallel
ctest --test-dir build-issue25-release --output-on-failure
```

**Final recorded Release build output:**

```text
[71/71] Linking CXX executable RationalStudio
```

**Recorded Release CTest summary:**

```text
100% tests passed, 0 tests failed out of 18
Total Test time (real) = 0.68 sec
```

No compiler warnings or errors appeared in the submitted final Issue #25 Release build output. The interactive GUI was manually checked in Debug, not separately in Release.

## 3. Automated regression tests — PASS

### Issue #28 current automated regression results

| Verification | Recorded result |
|---|---|
| GCC 15.2.0 GUI-enabled Debug CTest | **24/24 PASS**, 10.94 s total |
| GCC 15.2.0 GUI-enabled Release CTest | **24/24 PASS**, 5.03 s total |
| GCC 15.2.0 core-only ASan/UBSan CTest | **17/17 PASS**, 6.66 s total |
| `RationalVariableDefinitionsTests` (direct Debug run) | **PASS — 951 assertions** |

Direct test invocation and output:

```bash
./build-issue28/RationalVariableDefinitionsTests
```

```text
PASS: 951 VariableDefinitions assertions
```

The new core suite covers scalar assignment parsing; name/reference validation;
direct and transitive dependency ordering; selective recomputation and changed
values; rejected cycles or malformed edits with transactional preservation;
undefined-value propagation and recovery; revision/binding snapshots; and
reuse of compiled graph expressions with newly resolved bindings. These are
regression checks of the Qt-independent APIs, not an interactive GUI assignment
workflow. The Debug build archive's `LastTest.log` separately confirms the
951-assertion pass and GUI-enabled CTest execution.

Issue #28 adds one Qt-independent test executable: **24 CTest registrations
from 23 executables** (17 core tests and 7 offscreen Qt invocations from
6 widget executables). The GUI-disabled ASan/UBSan suite contains
**17 core-only CTests**; it does not exercise Qt widget code.

### Issue #27 historical automated regression results

| Verification | Recorded result |
|---|---|
| GCC 15.2.0 GUI-enabled Debug CTest | **23/23 PASS**, 10.35 s total |
| GCC 15.2.0 GUI-enabled Release CTest | **23/23 PASS**, 5.10 s total |
| GCC 15.2.0 core-only ASan/UBSan CTest | **16/16 PASS**, 6.25 s total |
| `RationalNamedVariableTests` (direct Debug run) | **PASS — 3,149 assertions** |

Direct test invocation and recorded output:

```bash
./build-issue27/RationalNamedVariableTests
```

```text
PASS: 3149 NamedVariable assertions
```

**New regression coverage:** ASCII identifier validation (1–64 characters),
case-insensitive declarations and bindings, builtin-name reservation, duplicate
and undeclared identifiers, scope isolation, missing/non-finite values, unique
sorted referenced-variable discovery, arithmetic with builtins and `x`/`ans`,
move semantics, and changes to per-evaluation bindings without reparsing.

The new regression adds one Qt-independent executable. The complete GUI-enabled
suite contains **23 CTest registrations from 22 executables** (16 core tests
and 7 offscreen Qt invocations of 6 widget executables, including the 2× DPI
invocation). The sanitizer build runs **16 core-only tests** and excludes Qt.

### Issue #26 historical automated results

**Recorded Debug suite result:**

```text
100% tests passed, 0 tests failed out of 22
Total Test time (real) = 7.93 sec
```

This implementation run used `ctest --test-dir build-issue26 --output-on-failure -j 4`. The subsequently inspected local `build-issue26/Testing/Temporary/LastTest.log` also records all 22 passing tests; the dedicated canvas coverage test took 7.99 seconds in that log. Timings are host observations, not performance acceptance thresholds.

| Executable / CTest | Recorded result |
|---|---|
| `RationalCurveCoverageRegressionTests` | PASS — 4,626 assertions |
| `RationalCurveSamplingJobTests` | PASS — 896,291 assertions |
| `RationalGraphCanvasCoverageTests` | PASS — 33,628 assertions |
| `RationalGraphCanvasCoverageHiDpiTests` | PASS — 493 assertions; `QT_SCALE_FACTOR=2` |
| `RationalGraphCanvasBudgetTests` | PASS — 332 assertions with resumable completion under small budgets |
| `RationalGraphCanvasSchedulingTests` | PASS — 64 assertions |

The configured suite contains **15 Qt-independent tests** and **7 offscreen Qt invocations from 6 widget executables**, totaling **22 CTests from 21 executables**. The high-DPI test invokes the coverage executable a second time. Optional benchmarks and Python recorder tests are not CTests.

**Manual evidence recorder unit tests:**

```bash
python3 -m unittest discover -s tests -p 'test_issue26_gui_verification.py' -v
```

```text
Ran 12 tests in 0.234s

OK
```

These test recorder verdicts, required/optional cases, schema rejection, save/resume behavior, report escaping, and check exit status. They do not operate the desktop GUI or attest that the final rendering fix passed manual testing.

### Issue #25 historical automated results

**Command executed:**

```bash
ctest --test-dir build-issue25 --output-on-failure
```

**Recorded Issue #25 Debug CTest results:**

```text
1/18 Test #1: RationalCoreTests ... Passed 0.00 sec
2/18 Test #2: RationalViewportTests ... Passed 0.00 sec
3/18 Test #3: RationalCurveSamplerTests ... Passed 0.00 sec
4/18 Test #4: RationalCurveDomainTests ... Passed 0.01 sec
5/18 Test #5: RationalCurveDiscontinuityTests ... Passed 0.02 sec
6/18 Test #6: RationalCurveSteepTests ... Passed 0.02 sec
7/18 Test #7: RationalViewportPrecisionTests ... Passed 0.00 sec
8/18 Test #8: RationalCurveGeometryCacheTests ... Passed 0.00 sec
9/18 Test #9: RationalCurveEvaluationBudgetTests ... Passed 0.01 sec
10/18 Test #10: RationalGraphRepaintSchedulerTests ... Passed 0.00 sec
11/18 Test #11: RationalGraphViewportBoundaryTests ... Passed 0.00 sec
12/18 Test #12: RationalGraphRegressionMatrixTests ... Passed 0.01 sec
13/18 Test #13: RationalGraphBenchmarkStatisticsTests ... Passed 0.00 sec
14/18 Test #14: RationalGraphCanvasCacheTests ... Passed 0.07 sec
15/18 Test #15: RationalGraphCanvasBudgetTests ... Passed 0.11 sec
16/18 Test #16: RationalGraphCanvasSchedulingTests ... Passed 0.29 sec
17/18 Test #17: RationalGraphCanvasBoundaryTests ... Passed 0.07 sec
18/18 Test #18: RationalGraphCanvasRegressionMatrixTests ... Passed 0.12 sec

100% tests passed, 0 tests failed out of 18
Total Test time (real) = 0.74 sec
```

**Recorded Issue #25 Release CTest:** 18/18 passed (0.68 seconds total).

The Issue #25 GUI-enabled suite includes **thirteen Qt-independent executables** (including the benchmark-statistics test) and **five offscreen Qt widget regression executables**: eighteen total. The optional **RationalGraphSamplingBench** and **RationalGraphCanvasBench** programs are separate measurement tools, not CTests. The core-only sanitizer build runs thirteen tests and excludes the Qt renderer.

### Test coverage by executable

| Test executable | Verified areas |
|---|---|
| `RationalCoreTests` | Fraction arithmetic and normalization, exact integer handling, scientific parsing/evaluation, operator precedence, functions, malformed inputs, and undefined results. |
| `RationalViewportTests` | Coordinate conversions, inverse transformations, viewport dimensions, pan and cursor-anchored zoom, invalid inputs, and state preservation. |
| `RationalCurveSamplerTests` | Adaptive screen-space subdivision, line/curve approximation, oscillatory probes, finite drawing geometry, and evaluation limits. |
| `RationalCurveDomainTests` | Open/closed function domains, valid regions separated by undefined intervals, exception/non-finite handling, and bounded sampling. |
| `RationalCurveDiscontinuityTests` | Reciprocal and tangent poles, finite step jumps, `floor(x)` boundaries, offset viewport cases, sampled-position sweeps, and continuous controls. |
| `RationalCurveSteepTests` | Steep continuous straight lines, hyperbolic tangent and inverse-tangent transitions, bounded terminal refinement, approximation consistency, sampling-position sweeps, and discontinuity controls. |
| `RationalViewportPrecisionTests` | Coordinate round trips, repeated pan/zoom stability, zoom-limit behavior, restored viewport scales, extreme-coordinate validation, and state preservation after rejected navigation. |
| `RationalCurveGeometryCacheTests` | Qt-independent cache identity, reuse, invalidation, reordering, and lazy geometry regeneration. |
| `RationalGraphCanvasCacheTests` | Offscreen Qt widget integration for `QPainterPath` cache reuse and invalidation during paint operations. |
| `RationalCurveEvaluationBudgetTests` | Exact callback accounting, per-curve and shared limits, zero-budget and invalid-result handling, and deterministic multi-curve budget sweeps. |
| `RationalGraphCanvasBudgetTests` | Offscreen Qt integration for per-paint budget enforcement, deferred uncached curves, sampling diagnostics, and invalidation after budget changes. |
| `RationalGraphRepaintSchedulerTests` | Qt-independent navigation event coalescing, generation tracking, obsolete deferred-work cancellation, and screen-space preview transform regressions. |
| `RationalGraphCanvasSchedulingTests` | Offscreen Qt wheel/drag scheduling integration, path preview reuse, delayed resampling, and superseded navigation cancellation. |
| `RationalGraphViewportBoundaryTests` | Invalid viewport input rejection and state preservation, extreme finite centers, bounded grid geometry, and resize-dimension regressions. |
| `RationalGraphCanvasBoundaryTests` | Offscreen Qt resize and boundary checks, extreme zoom/center rendering, cancellation of stale refreshes, and invalid-view rollback. |
| `RationalGraphRegressionMatrixTests` | Qt-independent cross-subsystem checks for viewport transformations, cursor-anchored zoom, sampled segment accuracy, restricted/invalid domains, discontinuities, and evaluation ceilings. |
| `RationalGraphCanvasRegressionMatrixTests` | Offscreen Qt integration checks for cache reuse, navigation, resizing, invalid views, evaluation budgets, and discontinuity/domain rendering. |
| `RationalGraphBenchmarkStatisticsTests` | Qt-independent checks of percentile summaries, report escaping, benchmark output files, and argument validation; no host-speed thresholds. |
| `RationalCurveCoverageRegressionTests` | Reproduce left-side budget starvation, check distributed strip coverage, shared ceilings, and smooth/restricted-domain controls for the single-call sampler. |
| `RationalCurveSamplingJobTests` | Complete tangent branches across widths and zoom levels, exact callback accounting, budget-independent geometry, pole/step separation, and steep continuous controls. |
| `RationalNamedVariableTests` | Qt-independent identifier validation, scoped named-variable resolution, case-folded bindings, reserved names, missing/non-finite values, reference introspection, and backward-compatible evaluation. |
| `RationalVariableDefinitionsTests` | Qt-independent scalar assignments, dependency ordering and change propagation, circular/malformed redefinition rollback, undefined-value recovery, selective recomputation, revision tracking, and binding snapshots. |
| `RationalGraphCanvasCoverageTests` | Rendered tangent-tail coverage, identical repeated repaints, retained previews through interrupted refreshes, toolbar zoom, resizing, and exhausted shared budgets. |
| `RationalGraphCanvasCoverageHiDpiTests` | Second invocation of the coverage executable with `--dpi-smoke` and `QT_SCALE_FACTOR=2`. |

### Discontinuity regression results

The new test executable was also run directly:

```bash
./build-issue17/RationalCurveDiscontinuityTests
```

**Recorded output:**

```text
PASS: 8663 CurveDiscontinuity assertions
```

The discontinuity suite exercises:

- Selected `1/(x-a)` vertical asymptotes, including shifted/non-grid-aligned positions.
- `tan(x)` near the tested asymptotes at `±pi/2` in radian mode.
- Finite step discontinuities and `floor(x)` integer boundaries.
- A deterministic sweep of 257 sample-phase locations for reciprocal poles and step jumps.
- A translated/scaled viewport case.
- A steep continuous straight line and a smooth parabola, guarding against indiscriminate rejection of legitimate geometry.

A finite-step case was previously observed to bridge a discontinuity with the preceding sampler and passed after the Issue #17 revision. That is targeted regression evidence; it is not a proof of complete discontinuity detection.

### Steep continuous-curve regression results

Issue #18 introduces bounded secondary refinement for finite curve intervals that remain unresolved at the normal subdivision limit.

The new implementation provides additional refinement opportunities for steep continuous functions rather than immediately disconnecting their drawing geometry.

The dedicated regression executable is:

`RationalCurveSteepTests`

It was executed directly using:

```bash
./build-issue18/RationalCurveSteepTests
```

**Recorded output:**

```text
PASS: 6516 CurveSteep assertions
```

### Viewport precision and navigation stability regression results

Issue #19 improves numerical precision and stability in the Qt-independent `GraphViewport` component.

The implementation uses precision-aware navigation calculations, preserves viewport state when zoom limits are reached, and rejects operations that would introduce excessive screen-space error due to floating-point limitations.

The dedicated regression executable is:

`RationalViewportPrecisionTests`

It was executed directly using:

```bash
./build-issue19/RationalViewportPrecisionTests
```

Recorded result:

```text
PASS: 3474 GraphViewportPrecision assertions
```

The regression suite verifies:

- Coordinate round trips across multiple viewport centers and scales.
- Coordinate behavior with odd viewport dimensions and half-pixel centers.
- Repeated cursor-anchored zoom-in and zoom-out cycles.
- Repeated zoom attempts at minimum and maximum wheel-scale limits.
- Restored viewport states outside ordinary mouse-wheel zoom limits.
- Repeated forward and reverse panning operations.
- Invalid navigation input rejection.
- Preservation of viewport state following rejected operations.
- Safe handling of extreme world-coordinate values.
- Rejection of navigation that cannot be represented within the configured screen-space error tolerance.

The tests include a regression case for previously incorrect zoom-direction behavior when a restored viewport scale exceeded the ordinary mouse-wheel limits.

The viewport continues to use IEEE-754 double-precision coordinates. These improvements do not establish arbitrary-precision navigation or guarantee that all extreme coordinate transformations can be represented accurately.

### Curve geometry cache regression results

Issue #20 introduces a Qt-independent geometry-cache policy and caches generated `QPainterPath` geometry in `GraphCanvas`. Valid paths can be reused during ordinary repaints and presentation-only changes, while changes to mathematical inputs or the viewport require regeneration.

The Qt-independent regression executable was run directly:

```bash
./build-issue20/RationalCurveGeometryCacheTests
```

Recorded output:

```text
PASS: 55 CurveGeometryCache assertions
```

The offscreen Qt widget regression executable was also run directly:

```bash
QT_QPA_PLATFORM=offscreen ./build-issue20/RationalGraphCanvasCacheTests
```

Recorded output:

```text
PASS: 49 GraphCanvasCache assertions
```

These are focused regression tests for cache management and Qt widget cache integration. They do not constitute complete end-to-end GUI interaction tests, pixel-level rendering comparisons, or controlled performance benchmarks. No frames-per-second improvement is claimed.

### Bounded curve evaluation and rendering-budget regression results

Issue #21 introduces configurable per-curve and shared per-paint callback ceilings for rebuilding curve geometry. The Qt-independent `CurveEvaluationBudget` accounts for calls across multiple sampling operations; the Qt renderer reports exhausted or truncated work and defers curves not processed in the current paint. This bounds callback counts, **not** the duration of each callback or total frame time.

The Qt-independent budget executable was run directly:

```bash
./build-issue21/RationalCurveEvaluationBudgetTests
```

Recorded output:

```text
PASS: 5944 CurveEvaluationBudget assertions
```

The offscreen Qt widget budget executable was also run directly:

```bash
QT_QPA_PLATFORM=offscreen ./build-issue21/RationalGraphCanvasBudgetTests
```

Recorded output:

```text
PASS: 20 GraphCanvasBudget assertions
```

The focused tests exercise callback accounting, per-curve/shared limits, deterministic multi-curve budget sweeps, deferred cache builds, sampling diagnostics, and cache invalidation after sampling limits change. They do not establish wall-clock execution limits, completeness of budget-truncated geometry, or measured UI performance improvements.

### Graph repaint scheduling regression results

Issue #22 adds `GraphRepaintScheduler` to coalesce rapid navigation updates and cancel obsolete deferred work. While a refresh is pending, `GraphCanvas` can transform existing cached paths as an interim display; the renderer then samples the latest viewport on its scheduled refresh. The target interval is 16 ms. This is not a real-time frame deadline.

The Qt-independent scheduler test was run directly:

```bash
./build-issue22/RationalGraphRepaintSchedulerTests
```

Recorded result:

```text
PASS: 5947 GraphRepaintScheduler assertions
```

The offscreen Qt integration test was also run directly:

```bash
QT_QPA_PLATFORM=offscreen ./build-issue22/RationalGraphCanvasSchedulingTests
```

Recorded result:

```text
PASS: 57 GraphCanvasScheduling assertions
```

The suites cover navigation coalescing, generation tracking, cancellation of superseded deferred requests, transformed geometry previews, and scheduled rebuilding. Passing tests do not establish visually flicker-free drag rendering or measurable FPS improvements.

### Issue #23 viewport boundary and resize regression results

Issue #23 improves graph resizing and viewport boundary handling. `GraphGridLayout` validates grid spacing and constructs safe pixel-space positions; `GraphCanvas` avoids stale rendering on resize and handles extreme but finite viewport centers without unsafe grid-label coordinates. These checks preserve the prior precision-aware viewport and Issue #22 repaint scheduling behavior.

**Qt-independent direct regression command:**

```bash
./build-issue23/RationalGraphViewportBoundaryTests
```

Recorded result:

```text
PASS: 1884 GraphViewportBoundary assertions
```

**Qt offscreen direct regression command:**

```bash
QT_QPA_PLATFORM=offscreen ./build-issue23/RationalGraphCanvasBoundaryTests
```

Recorded result:

```text
PASS: 80 GraphCanvasBoundary assertions
```

The targeted suites cover invalid view changes and state rollback, extreme finite centers and scales, grid-position safety, repeated dimensions/resize transitions, and renderer invalidation. They do **not** prove arbitrary-precision coordinate behavior, exhaustive geometry correctness, or the absence of every GUI defect. Manual verification of resizing and interaction was also reported successful (Section 5).

### Issue #24 graph sampling and viewport regression matrix results

Issue #24 adds two complementary automated regression matrices without modifying production graph-rendering algorithms. The Qt-independent matrix exercises transformations, cursor-anchored zoom, emitted-segment approximation accuracy, restricted domains, selected finite and infinite discontinuities, and bounded evaluation work. The offscreen Qt matrix checks the combined behavior of viewport navigation, resizing, cached geometry, invalid-view rollback, and rendering controls.

**Qt-independent regression matrix:**

```bash
./build-issue24/RationalGraphRegressionMatrixTests
```

Recorded output:

```text
PASS: 5021 GraphRegressionMatrix assertions
```

**Offscreen Qt widget regression matrix:**

```bash
QT_QPA_PLATFORM=offscreen ./build-issue24/RationalGraphCanvasRegressionMatrixTests
```

Recorded output:

```text
PASS: 92 GraphCanvasRegressionMatrix assertions
```

These suites complement the existing focused tests. The reported assertions cover representative mathematical functions, defined tolerances, selected invalid-domain and discontinuity cases, and numerical callback limits; they do not establish exhaustive mathematical correctness, pixel-identical output, end-to-end GUI automation, or a frame-time guarantee. The developer separately reports successful manual GUI verification (Section 5).

### Issue #25 graph rendering and navigation benchmark results

The Issue #25 Release runner completed **150 mathematical-sampling measurements** and **750 Qt offscreen canvas/navigation measurements**. These are genuine host measurements imported from the benchmark JSON/CSV files when the documentation updater ran; they are **not CTest success thresholds**.

**Recorded benchmark setup:** 960 × 600 logical-pixel viewport, 1/4/8 active curves, 25 recorded trials per scenario, 5 warmup trials, 12 synthetic events per navigation burst, GCC 15.2.0 Release build, `QT_QPA_PLATFORM=offscreen` for Qt.

| Environment field | Recorded value |
|---|---|
| Date (UTC) | 2026-10-23T17:35:26+00:00 |
| Git commit | e0c19bf140905e7f5c1fde08bca578a2711c81b8 |
| Git worktree | M CMakeLists.txt |
| OS | Linux-7.0.0-38-generic-x86_64-with-glibc2.43 |
| CPU | AMD Ryzen 7 5800X 8-Core Processor |
| Available hardware threads | 16 |
| Governor | powersave |
| Display session | wayland |
| Sampling compiler | GCC 15.2.0 |
| Sampling build type | Release (NDEBUG) |
| Qt benchmark build type | Release (NDEBUG) |
| Trials per group | 25 |
| Events per navigation burst | 12 |

**graph_sampling — measured scenario aggregates**

| Measurement | Scenario | Curves | N | Median (ms) | p95 (ms) | Total callbacks | Total builds | Truncated runs |
|---|---|---:|---:|---:|---:|---:|---:|---:|
| sampling_callbacks_plus_geometry | default_view | 1 | 25 | 0.0050 | 0.0065 | 6425 | 25 | 0 |
| sampling_callbacks_plus_geometry | default_view | 4 | 25 | 0.0235 | 0.0255 | 31050 | 100 | 0 |
| sampling_callbacks_plus_geometry | default_view | 8 | 25 | 0.0940 | 0.0985 | 156350 | 200 | 0 |
| sampling_callbacks_plus_geometry | zoomed_view | 1 | 25 | 0.0025 | 0.0025 | 4025 | 25 | 0 |
| sampling_callbacks_plus_geometry | zoomed_view | 4 | 25 | 0.0146 | 0.0146 | 21350 | 100 | 0 |
| sampling_callbacks_plus_geometry | zoomed_view | 8 | 25 | 0.1175 | 0.1604 | 176700 | 200 | 0 |

- Source CSV: `benchmark-results/issue25/graph_sampling.csv` (SHA-256 `1a78ed43754d2a87191ecc5c9bb56a617995d06769cbf5e33992c5036abd02e6`)
- Source JSON: `benchmark-results/issue25/graph_sampling.json` (SHA-256 `305be7cbba06b2c6b0626a767775f72b089806f06726d9a42d7b886431c282f3`)

**graph_canvas — measured scenario aggregates**

| Measurement | Scenario | Curves | N | Median (ms) | p95 (ms) | Total callbacks | Total builds | Truncated runs |
|---|---|---:|---:|---:|---:|---:|---:|---:|
| cold_invalidated_render | 960x600 | 1 | 25 | 0.6166 | 0.8056 | 6301 | 25 | 0 |
| warm_cached_render | 960x600 | 1 | 25 | 0.5868 | 0.7175 | 0 | 0 | 0 |
| input_to_first_preview | wheel_zoom_burst | 1 | 25 | 0.6322 | 0.7729 | 0 | 0 | 0 |
| navigation_event_dispatch_total | wheel_zoom_burst | 1 | 25 | 0.0255 | 0.0315 | 0 | 0 | 0 |
| preview_render_total | wheel_zoom_burst | 1 | 25 | 8.0470 | 9.2648 | 0 | 0 | 0 |
| last_event_to_geometry_settled | wheel_zoom_burst | 1 | 25 | 9.9845 | 10.3864 | 4025 | 25 | 0 |
| input_to_first_preview | mouse_drag_burst | 1 | 25 | 0.6465 | 0.7213 | 0 | 0 | 0 |
| navigation_event_dispatch_total | mouse_drag_burst | 1 | 25 | 0.0268 | 0.0377 | 0 | 0 | 0 |
| preview_render_total | mouse_drag_burst | 1 | 25 | 7.1867 | 8.7479 | 0 | 0 | 0 |
| last_event_to_geometry_settled | mouse_drag_burst | 1 | 25 | 10.6316 | 10.9026 | 6425 | 25 | 0 |
| cold_invalidated_render | 960x600 | 4 | 25 | 2.0677 | 2.4290 | 30998 | 100 | 0 |
| warm_cached_render | 960x600 | 4 | 25 | 1.9911 | 2.4238 | 0 | 0 | 0 |
| input_to_first_preview | wheel_zoom_burst | 4 | 25 | 2.0518 | 2.1282 | 0 | 0 | 0 |
| navigation_event_dispatch_total | wheel_zoom_burst | 4 | 25 | 0.0273 | 0.0324 | 0 | 0 | 0 |
| preview_render_total | wheel_zoom_burst | 4 | 25 | 29.2878 | 32.9129 | 0 | 0 | 0 |
| last_event_to_geometry_settled | wheel_zoom_burst | 4 | 25 | 3.8000 | 4.5081 | 22350 | 100 | 0 |
| input_to_first_preview | mouse_drag_burst | 4 | 25 | 1.9915 | 2.0824 | 0 | 0 | 0 |
| navigation_event_dispatch_total | mouse_drag_burst | 4 | 25 | 0.0302 | 0.0372 | 0 | 0 | 0 |
| preview_render_total | mouse_drag_burst | 4 | 25 | 23.5251 | 24.7977 | 0 | 0 | 0 |
| last_event_to_geometry_settled | mouse_drag_burst | 4 | 25 | 6.2567 | 6.6141 | 31050 | 100 | 0 |
| cold_invalidated_render | 960x600 | 8 | 25 | 3.0269 | 3.0933 | 147651 | 200 | 0 |
| warm_cached_render | 960x600 | 8 | 25 | 2.6691 | 2.7858 | 0 | 0 | 0 |
| input_to_first_preview | wheel_zoom_burst | 8 | 25 | 2.7690 | 2.8100 | 0 | 0 | 0 |
| navigation_event_dispatch_total | wheel_zoom_burst | 8 | 25 | 0.0300 | 0.0340 | 0 | 0 | 0 |
| preview_render_total | wheel_zoom_burst | 8 | 25 | 38.5885 | 39.0943 | 0 | 0 | 0 |
| last_event_to_geometry_settled | wheel_zoom_burst | 8 | 25 | 6.3814 | 6.6292 | 142950 | 200 | 0 |
| input_to_first_preview | mouse_drag_burst | 8 | 25 | 2.6894 | 2.7225 | 0 | 0 | 0 |
| navigation_event_dispatch_total | mouse_drag_burst | 8 | 25 | 0.0338 | 0.0383 | 0 | 0 | 0 |
| preview_render_total | mouse_drag_burst | 8 | 25 | 31.6884 | 32.3934 | 0 | 0 | 0 |
| last_event_to_geometry_settled | mouse_drag_burst | 8 | 25 | 8.6816 | 8.9484 | 157475 | 200 | 0 |

- Source CSV: `benchmark-results/issue25/graph_canvas.csv` (SHA-256 `d6150bf4c9ff4852daf4474860ecd4a0b54313d0143b23e4129a3c7b77590e22`)
- Source JSON: `benchmark-results/issue25/graph_canvas.json` (SHA-256 `b66efe965b07f389da426542310480c2ac6e96ff22a83af55d406fa89ea08a43`)

- Generated detailed Markdown baseline: `benchmark-results/issue25/RESULTS.md` (SHA-256 `415d570762776131013cde73ba40ff03969331ff927fd6b8c0d27f1a54359d73`).
- These paths identify locally generated files; whether raw files are committed is a separate repository decision. Scenario aggregates above remain in this verification report even if raw files are not tracked.

**Interpretation boundaries:** Sampling totals count mathematical callbacks, not instructions or CPU cycles. Cold Qt rendering includes sampling and rasterization; warm rendering is cached path rasterization. Offscreen `QWidget::render()` does not measure desktop compositor presentation, actual mouse-to-monitor latency, or FPS. The navigation settlement metric includes timer and event-loop waiting. Measurements are machine-, load-, and build-dependent; no before/after speedup can be established without a comparably instrumented earlier run.

### Issue #26 wide-view sampling and rendering regression results

The original GUI failure was caused by evaluation-budget exhaustion during left-to-right adaptive sampling of repeated tangent poles. An intermediate strip-based workaround spread geometry across the viewport but could still exhaust a strip partway through a branch and cache missing tails. The final renderer uses resumable `CurveSamplingJob` builds, retaining completed geometry until a complete replacement is ready.

The current implementation provides:

- Bounded callbacks per curve per paint and across each paint, with unfinished sampling resumed on paced later paints; cached probes prevent duplicate callback work in a retried interval.
- World-space coarse interval limits, monotonicity-aware offscreen pruning, geometric error checks for steep chords, additional refinement around invalid probes, and clipped drawing coordinates.
- Surrounding sampled geometry covering twice the widget width and height for nearby navigation previews, retained through wheel input, toolbar zoom, resize, and repeated interruption of an unfinished refresh.
- Separate branch stroking and device-scale-aware raster caches for unchanged completed curves and pending previews; later curves retain their previews even when earlier curves consume the shared paint allowance.

**Coverage:** Core tests use widths 350, 900, and 1600, height 617, a translated view, and scales 58, 15, 1.38, 0.5, and 0.1 pixels per unit. They check analytically expected tangent intersections with both viewport edges, finite ordered geometry, no sampled chords across tangent poles, exact callback accounting, and identical completed vertices under tiny versus large work allowances. Separate controls check shifted reciprocal poles, finite jumps, and connected steep `tanh` transitions.

Offscreen widget tests render `tan(x)`, `1/(x-0.133)`, and `floor(x)` together at widths 350, 900, and 1600 and scales 58, 9, 1.38, 0.5, and 0.1. They check expected tail pixels near the top and bottom, identical repeated cached images, zero redundant geometry builds, interrupted refresh previews, toolbar zoom, resizing, and a four-callback shared budget. A separate 2× display-scale invocation checks widths/scales selected by `--dpi-smoke`.

The rendered checks are targeted pixel-presence and same-run image comparisons, not a reference-image oracle for every curve or evidence of physical-display latency. The initial dense-view test exceeded its 60-second timeout before raster caching; the final implementation passed the same test in approximately eight seconds. This development observation is not a controlled benchmark or an FPS claim. Issue #25 benchmark data above have not been regenerated for the new renderer.

## 4. AddressSanitizer / UndefinedBehaviorSanitizer — PASS

### Issue #28 current complete core-only sanitizer suite — PASS

```bash
cmake -S . -B build-issue28-asan -G Ninja \
    -DCMAKE_BUILD_TYPE=Debug \
    -DRATIONALSTUDIO_BUILD_GUI=OFF \
    -DRATIONALSTUDIO_BUILD_BENCHMARKS=OFF \
    -DCMAKE_CXX_FLAGS="-fsanitize=address,undefined -fno-omit-frame-pointer" \
    -DCMAKE_EXE_LINKER_FLAGS="-fsanitize=address,undefined"
cmake --build build-issue28-asan --parallel
ctest --test-dir build-issue28-asan --output-on-failure
```

```text
[42/42] Linking CXX executable RationalNamedVariableTests
100% tests passed, 0 tests failed out of 17
Total Test time (real) = 6.66 sec
```

All 17 registered Qt-independent tests passed in the ASan/UBSan configuration.
No sanitizer diagnostics appeared in the supplied run. Qt widgets and the
full GUI renderer were deliberately excluded from the sanitizer build.

### Issue #27 historical complete core-only sanitizer suite — PASS

```bash
cmake -S . -B build-issue27-asan -G Ninja \
    -DCMAKE_BUILD_TYPE=Debug \
    -DRATIONALSTUDIO_BUILD_GUI=OFF \
    -DRATIONALSTUDIO_BUILD_BENCHMARKS=OFF \
    -DCMAKE_CXX_FLAGS="-fsanitize=address,undefined -fno-omit-frame-pointer" \
    -DCMAKE_EXE_LINKER_FLAGS="-fsanitize=address,undefined"
cmake --build build-issue27-asan --parallel
ctest --test-dir build-issue27-asan --output-on-failure
```

```text
[39/39] Linking CXX executable RationalCurveSamplerTests
100% tests passed, 0 tests failed out of 16
Total Test time (real) = 6.25 sec
```

All 16 registered Qt-independent tests passed with AddressSanitizer and
UndefinedBehaviorSanitizer instrumentation. No sanitizer diagnostics were
reported in the supplied terminal output. This does **not** establish
sanitizer coverage of Qt widgets or the GUI render pipeline.

### Historical sanitizer evidence (Issues #25–#26)

**Configuration and build:**

```bash
cmake -S . -B build-issue25-asan -G Ninja \
    -DCMAKE_BUILD_TYPE=Debug \
    -DRATIONALSTUDIO_BUILD_GUI=OFF \
    -DRATIONALSTUDIO_BUILD_BENCHMARKS=OFF \
    -DCMAKE_CXX_FLAGS="-fsanitize=address,undefined -fno-omit-frame-pointer" \
    -DCMAKE_EXE_LINKER_FLAGS="-fsanitize=address,undefined"
cmake --build build-issue25-asan --parallel
```

**Final recorded build output:**

```text
[33/33] Linking CXX executable RationalCurveGeometryCacheTests
```

**Test command:**

```bash
ctest --test-dir build-issue25-asan --output-on-failure
```

**Recorded output:**

```text
1/13 Test #1: RationalCoreTests ... Passed 0.02 sec
2/13 Test #2: RationalViewportTests ... Passed 0.01 sec
3/13 Test #3: RationalCurveSamplerTests ... Passed 0.02 sec
4/13 Test #4: RationalCurveDomainTests ... Passed 0.04 sec
5/13 Test #5: RationalCurveDiscontinuityTests ... Passed 0.09 sec
6/13 Test #6: RationalCurveSteepTests ... Passed 0.07 sec
7/13 Test #7: RationalViewportPrecisionTests ... Passed 0.01 sec
8/13 Test #8: RationalCurveGeometryCacheTests ... Passed 0.01 sec
9/13 Test #9: RationalCurveEvaluationBudgetTests ... Passed 0.03 sec
10/13 Test #10: RationalGraphRepaintSchedulerTests ... Passed 0.01 sec
11/13 Test #11: RationalGraphViewportBoundaryTests ... Passed 0.01 sec
12/13 Test #12: RationalGraphRegressionMatrixTests ... Passed 0.05 sec
13/13 Test #13: RationalGraphBenchmarkStatisticsTests ... Passed 0.03 sec

100% tests passed, 0 tests failed out of 13
Total Test time (real) = 0.41 sec
```

All thirteen Qt-independent CTests passed in the sanitizer-enabled configuration, including the new benchmark-statistics test. No ASan or UBSan failures were reported.

**Scope:** The Qt desktop application, all five Qt widget regression executables, and the two optional non-CTest benchmark tools are excluded from this sanitizer build. The results do not establish Qt renderer or interactive GUI sanitizer coverage.

### Issue #26 incremental sampling sanitizer result

**Commands executed during implementation:**

```bash
cmake --build build-issue26-asan --target RationalCurveSamplingJobTests -j 4
ASAN_OPTIONS=detect_leaks=0 ./build-issue26-asan/RationalCurveSamplingJobTests
```

```text
PASS: 896291 sampling job assertions
```

The core-only Debug configuration used `-fsanitize=address,undefined -fno-omit-frame-pointer`. No ASan or UBSan failure was reported for this direct executable run. Leak detection was disabled for this invocation. This is a focused current result, separate from the historical **13/13** Issue #25 sanitizer suite above; the full current **15-test** core-only suite and Qt renderer sanitizer tests were not recorded. The original Issue #26 sanitizer build directory is no longer present locally at documentation-update time; the command/result above are retained from the implementation session.

## 5. Desktop GUI — historical passes and Issue #26 retest status

### Issue #28 interactive GUI regression status

**PASS — developer-reported informal manual check.** The developer launched
and inspected the existing GUI after the Issue #28 core changes, revisiting
usual graphing and calculator regression scenarios, and reported no visible
behavior changes or failures. No formal Issue #28 checklist, automated desktop
interaction session, or separate interactive Release GUI run was supplied.
The Qt offscreen regression tests passed in both 24-test GUI-enabled suites.
Issue #28 does **not** add variable-assignment controls to the GUI.

### Issue #27 historical interactive GUI status

Issue #27 interactive GUI smoke test: PASS — developer-attested after verifying existing graphing/navigation and calculator behavior; not an automated GUI acceptance test.

The named-variable interface remains a **Qt-independent core API** in this
issue. The passing offscreen `GraphCanvas` tests validate existing widget
regressions, but do not demonstrate GUI assignment syntax or slider support.
The historical Issue #26 failed GRA-03 JSON/Markdown record is retained. The
developer separately reported a successful visual retest of that fix during
Issue #26 closeout; an updated machine-readable post-fix session is not in this
attached verification document. Neither observation is attributed to new
Issue #27 interactive testing.

The historical Issue #25 Debug application was launched using:

```bash
./build-issue25/RationalStudio
```

The developer reports successful completion of the prescribed Issue #19 graphical verification and regression checks. The inspected graphs displayed the expected behavior, with no functional failures reported.

The previously completed graphical regression checks included:

| Expression / behavior | Verification goal |
|---|---|
| `y = 1/x` | Separate reciprocal branches near zero. |
| `y = 1/(x-0.133)` and `y = 1/(x+0.327)` | Avoid false joins across shifted poles. |
| `y = tan(x)` | Inspect branch separation near tangent poles (radian mode). |
| `y = floor(x)` and `y = sign(x)` | Inspect finite jump behavior. |
| `y = 500*x` | Preserve a steep continuous straight line. |
| `y = -2500*x` | Preserve a steep continuous line with negative slope. |
| `y = tanh(1000*x)` | Preserve continuity through a steep transition at the origin. |
| `y = atan(1000*x)` | Preserve continuity through a steep inverse-tangent transition. |
| `y = tanh(1000*(x-0.137))` | Preserve a steep continuous transition shifted away from the origin. |
| `y = x^5` and `y = exp(40*x)` | Inspect smooth but strongly varying continuous functions. |
| `y = x^2` and `y = sin(x)` | Preserve ordinary smooth curves. |
| `y = sqrt(x)` | Preserve restricted-domain behavior. |

The manual regression review also covered graph panning, cursor-anchored zoom, resizing, multiple functions, visibility toggles, coordinate tracing, workspace save/load, PNG export, and the non-graph calculator workspaces. No failures were reported.

### Issue #19 navigation verification

The Issue #19 Debug application was launched on Linux.

The prescribed navigation checks include:

- Cursor-anchored zoom at off-center positions.
- Repeated zoom-in and zoom-out operations.
- Zoom-limit behavior without viewport drift.
- Horizontal and vertical mouse panning.
- Repeated panning and zooming.
- Viewport reset and resizing.
- Saved-workspace viewport restoration.
- Existing graph rendering and PNG export regression checks.

The developer reports completing all prescribed Issue #19 navigation and graphical regression checks successfully, with no functional failures observed.

### Issue #20 cached-geometry manual verification

The developer reports completing the prescribed Issue #20 interactive GUI verification successfully after launching:

```bash
./build-issue20/RationalStudio
```

The verification focused on correct graph output and invalidation behavior around ordinary repaints, expression changes, visibility controls, panning/zooming, viewport resizing, angle-mode and previous-answer changes, workspace persistence, and PNG export. No functional failures were reported. This is a reported manual result, not an automated performance measurement.

Separately, `RationalGraphCanvasCacheTests` passed as an offscreen Qt widget test (49 assertions); it exercises cache behavior but is not an end-to-end GUI automation suite.

### Issue #21 sampling-budget verification

The Issue #21 Debug application was launched using `./build-issue21/RationalStudio`.

The developer explicitly confirms completing the prescribed Issue #21 interactive budget and graphing regression checks successfully, with no functional failures reported.

The recommended manual coverage comprises normal functions, steep continuous transitions, discontinuity regressions, multiple demanding functions, expression/visibility changes, pan/zoom, resizing, persistence, and PNG export. An application launch alone does not verify every interaction or demonstrate a measured frame-time improvement.

Separately, `RationalGraphCanvasBudgetTests` passed as an offscreen Qt widget regression suite with 20 assertions; it is not complete end-to-end GUI automation.

### Issue #22 navigation scheduling manual verification

The developer initially observed visible line flashing during graph panning. After a subsequent fix, the developer explicitly confirmed the panning flicker was resolved and repeated the Issue #22 interactive GUI checks successfully. The fix and retest are operator-attested and are not inferred from the original CTest run.

The original Qt offscreen scheduling suite passed 57 assertions, but it does not replace manual continuous-drag visual verification. Temporary previews may omit newly exposed graph regions until resampling completes; sustained flashing or blank-path transitions should be investigated.

### Issue #23 viewport boundary and resize manual verification

The developer reports completing Issue #23 interactive GUI verification successfully, including resizing, extreme zoom values, invalid viewport behavior, and regression checks for previously implemented graphing features. No functional failures or corrupted geometry were reported. The previous Issue #22 panning flicker was not reported to recur.

During the Debug GUI launch, the console printed four `kf.kio.widgets.kdirmodel: No node found for item that was just removed` messages referring to `CMakeLists.txt`, `LICENSE.MD`, `README.md`, and `VERIFICATION.md`. These are KDE KIO file-model runtime diagnostics; the report includes no accompanying crash or failed user-facing workflow. Their root cause was not established, and their appearance is not evidence that graph geometry was corrupted. They are recorded for traceability, not marked as a failed regression test.

The direct `RationalGraphCanvasBoundaryTests` offscreen suite passed 80 assertions. The full GUI was **not** subjected to GUI-enabled ASan/UBSan or pixel-diff visual testing; manual checks are developer-reported observations.

### Issue #24 manual GUI regression verification

The developer reports completing the Issue #24 manual GUI regression pass successfully after launching the Debug application. No visible graphing, panning, zooming, resizing, or curve-rendering regression was reported. This is a developer-attested interactive check, not a pixel-comparison or automated end-to-end GUI test.

Separately, the new `RationalGraphCanvasRegressionMatrixTests` offscreen suite passed 92 assertions. The combined results provide focused integration coverage but do not guarantee all possible Qt event orders, graph functions, or window-system environments.

### Issue #25 manual GUI and benchmark verification

The developer reports successfully completing the Issue #25 manual GUI regression pass after running the application. No failures were reported for the existing graph-rendering, panning, zooming, caching, or resizing functionality. This report distinguishes the developer-attested GUI check from synthetic offscreen benchmark events.

The Release benchmark runner completed successfully with 150 sampling measurements and 750 canvas/navigation measurements and generated five local output files (`graph_sampling.csv`, `graph_sampling.json`, `graph_canvas.csv`, `graph_canvas.json`, and `RESULTS.md`). Aggregate median/p95 values are recorded in the Issue #25 benchmark section above, using the files present at documentation-update time. These measurements are a reproducible **initial baseline**, not evidence of a performance improvement relative to Issue #24.

**Scope:** Manual functional checks and offscreen widget tests are not complete end-to-end GUI automation, pixel-comparison testing, accessibility validation, or controlled display/compositor performance benchmarking. The recorded results do not establish mathematically perfect graph rendering.

### Issue #26 manual GUI evidence and final-fix retest status

The local recorder report was updated at **2026-10-24T01:17:42+00:00** (October 23 in the development timezone), for commit `a4e6150063c0ce2cf3dbdebeb2b0f93448c5ecb5` with the recorder files untracked. It identifies Linux, KDE/Wayland, and `build-issue26/RationalStudio`.

**Recorded manual assessment: FAIL — 24 passed, 1 failed, 0 skipped, 0 not run.** Of 22 required checks, 21 passed and `GRA-03` failed; all three optional checks passed. The failed observation describes tangent branches clustered on the left when zoomed out, with screenshot references `GRA-03-Zoom-Out-Fail.png` and `GRA-03-Zoom-Out-Fail-Zoomed_In.png` in `build-issue26/`.

The other recorded passes cover launch, ordinary/restricted-domain/steep curves, tracing, panning and prior flicker, zoom, navigation, resizing, editing, eight demanding curves, persistence, PNG export, other calculator workspaces, and settled-view checks. Optional passes include malformed workspace handling, an interactive Release build, and an additional display scale/monitor. These observations predate the final rendering fix and are not evidence of a post-fix manual PASS.

The final fix has the passing automated and focused rendered-pixel coverage in Section 3. **A fresh operator-attested manual retest of the final implementation has not been recorded.** Keep the original failed session as historical evidence and use a new recorder output file for a retest, as described in `docs/Issue26GraphingManualQA.md`.

**Local evidence fingerprints:**

- `verification-results/issue26/manual_gui.json` — SHA-256 `29b50cb4edd60105fbbf87ee0e1fd7824813efa43a19af3b2e09aa784d3170bd`.
- `verification-results/issue26/manual_gui.md` — SHA-256 `802603754c20a5426c2004b19eb0ad337911ab5c659ae0923471e9450023ca52`.

Benchmark sources/results and verification reports remain local and excluded from Git by the current `.gitignore`. The Issue #25 source fingerprints above match the inspected local files. Their historical aggregates are retained here; neither those measurements nor the original manual evidence have been overwritten for this update.

## 6. Terminal application

The `RationalCLI` target built successfully in the historical Issue #25 Debug, Release, and core-only sanitizer configurations and the current Issue #26 full Debug build. The current Release rebuild targeted the desktop application only. A historical manual CLI smoke test produced:

```text
Input:     5/2 7/4
Operation: +
Result:    5/2 + 7/4 = 17/4
```

No separate interactive CLI execution was included in the supplied Issue #25 terminal record. The CLI compiled in the Issue #25 Debug, Release, and sanitizer-enabled configurations; the historical smoke-test result is not represented as a new run.

## 7. Platform and configuration coverage

| Platform / configuration | Build | Tests | Runtime |
|---|---|---|---|
| Linux Debug, Qt GUI enabled (Issue #28) | PASS (incremental build) | **24/24 PASS** | Qt offscreen regressions PASS; informal manual GUI check PASS (developer-reported) |
| Linux Release, Qt GUI enabled (Issue #28) | PASS | **24/24 PASS** | Qt offscreen regressions PASS; interactive Release launch not separately recorded |
| Linux Debug, GUI disabled, ASan/UBSan (Issue #28) | PASS | **17/17 PASS** | Entire Qt-independent suite passed; GUI renderer excluded |
| Linux Debug, Qt GUI enabled (Issue #27) | PASS | **23/23 PASS** | Qt offscreen regression tests passed; Issue #27 manual smoke-test status recorded in Section 5 |
| Linux Release, Qt GUI enabled (Issue #27) | PASS | **23/23 PASS** | Qt offscreen regression tests passed; interactive Release launch not separately recorded |
| Linux Debug, GUI disabled, ASan/UBSan (Issue #27) | PASS | **16/16 PASS** | Entire Qt-independent suite passed; Qt GUI excluded |
| Linux Debug, Qt GUI enabled (Issue #25) | PASS | 18/18 PASS | Manual GUI regression verification PASS (developer-reported); Qt widget tests PASS |
| Linux Release, Qt GUI enabled (Issue #25) | PASS | 18/18 PASS | Offscreen Qt widget tests PASS; 150 core sampling and 750 Qt benchmark observations recorded; interactive Release GUI launch not recorded |
| Linux Debug, GUI disabled, ASan/UBSan (Issue #25) | PASS | 13/13 PASS | Core regression and statistics test execution PASS; Qt GUI excluded |
| Linux Debug, Qt GUI enabled (Issue #26) | PASS | 22/22 PASS | Offscreen pixel/preview/repaint regressions and 2× display-scale test PASS; final manual retest not recorded |
| Linux Release desktop target (Issue #26) | PASS | Full current suite not run | Final interactive Release retest not recorded |
| Linux Debug, GUI disabled, ASan/UBSan (Issue #26) | Target PASS | Direct sampling job: 896,291 assertions PASS | Leak detection disabled; full current core suite and Qt GUI excluded from this result |
| Python manual evidence recorder (Issue #26) | Not applicable | 12/12 unit tests PASS | Historical manual report: 24 PASS / 1 FAIL; final manual retest not recorded |
| Native Windows | Not tested | Not tested | Not tested |
| Qt 5 | Not tested | Not tested | Not tested |

## 8. Outstanding limitations

The following areas remain outside the current verification scope or are known approximation limits:

- Issue #27 provides explicitly scoped named-variable expression parsing. Issue #28 adds core-only scalar assignment/dependency tracking and transactional cycle rejection using `VariableDefinitions`. GUI assignment rows, reusable function definitions, sliders, automatic dependency-driven graph refresh, and persistence of definitions are not yet implemented and remain future Milestone 2 issues. Forward references require prior definitions; scalar assignments cannot depend on `x` or `ans`.
- Finite probing cannot detect all discontinuities, arbitrarily narrow domain gaps, or unsampled singularities.
- Bounded terminal refinement improves steep continuous curves, but sufficiently narrow or extreme nonlinear features may still be omitted or approximated imperfectly.
- Highly oscillatory functions may be undersampled.
- Per-curve and shared per-paint callback-count limits bound evaluation counts, not the time spent in any callback or the total frame time. The single-call sampler can return truncated geometry. Current GUI sampling jobs resume unfinished work; complete replacement paths may take multiple paints.
- Valid `QPainterPath` geometry is cached; rapid navigation uses timer-coalesced refreshes and transformed cached-path previews. Resizes invalidate stale geometry and outdated refresh requests, while bounded pixel-space grid positions avoid unrepresentable label coordinates. At extreme finite centers, grid markers may be suppressed when floating-point precision is insufficient. Sampling remains on the GUI thread under callback budgets; no frame-time, input-latency, or FPS guarantee is claimed.
- Six offscreen Qt `GraphCanvas` executables cover cache, budget, scheduling, viewport boundaries, cross-feature behavior, and selected rendered-pixel/repaint comparisons, with a second coverage invocation at 2× display scale. Full desktop interaction automation and exhaustive rendered-pixel correctness are not established. Issue #25 timing measurements remain historical; benchmark tools are not CTests.
- A panning-flicker regression was reported during initial Issue #22 manual verification; the developer subsequently attested that it was corrected and retested.
- Retained preview geometry has finite surrounding coverage; sufficiently large navigation jumps can expose regions requiring fresh sampling. No arbitrary-zoom or universal flicker-free rendering guarantee is established. The original Issue #26 failed manual check still requires a recorded post-fix retest.
- The complete Qt application has not undergone recorded sanitizer testing.
- Native Windows builds, Qt 5 compatibility, and a post-fix interactive Release GUI retest remain unverified. The original Issue #26 optional Release check passed before the final fix.

## 9. Reproduction

On Linux with the documented dependencies installed, use fresh build directories:

```bash
# Full Debug build and tests
cmake -S . -B build-debug -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build-debug --parallel
ctest --test-dir build-debug --output-on-failure
./build-debug/RationalStudio

# Full Release build and tests
cmake -S . -B build-release -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build-release --parallel
ctest --test-dir build-release --output-on-failure

# Core-only ASan/UBSan build and tests
cmake -S . -B build-asan -G Ninja \
    -DCMAKE_BUILD_TYPE=Debug \
    -DRATIONALSTUDIO_BUILD_GUI=OFF \
    -DCMAKE_CXX_FLAGS="-fsanitize=address,undefined -fno-omit-frame-pointer" \
    -DCMAKE_EXE_LINKER_FLAGS="-fsanitize=address,undefined"
cmake --build build-asan --parallel
ctest --test-dir build-asan --output-on-failure
```

These generic commands describe a reproducible procedure. Current Issue #28 validation used `build-issue28`, `build-issue28-release`, and `build-issue28-asan` for GUI-enabled Debug, Release, and core-only sanitizer CTest suites, respectively. Historical Issue #27 validation used `build-issue27`, `build-issue27-release`, and `build-issue27-asan` for the prior complete Debug, Release, and core-only sanitizer suites, respectively. Historical Issue #26 implementation validation used `build-issue26`, `build-issue26-release` (desktop target), and `build-issue26-asan` (direct sampling-job sanitizer target). The latter directory was subsequently removed locally. Historical Issue #25 validation used `build-issue25`, `build-issue25-release`, and `build-issue25-asan`. The performance baseline was generated separately with `bash scripts/run_issue25_benchmarks.sh` against the Release benchmark targets.

## 10. Verification summary

| Area | Result |
|---|---|
| GCC 15.2.0 Debug GUI/CLI/core incremental build — Issue #28 | **PASS** |
| GCC 15.2.0 Release GUI/CLI/core full build — Issue #28 | **PASS** |
| GUI-enabled Debug CTest suite — Issue #28 | **24/24 PASS** |
| GUI-enabled Release CTest suite — Issue #28 | **24/24 PASS** |
| Core-only ASan/UBSan CTest suite — Issue #28 | **17/17 PASS** |
| Variable-definitions regression assertions — Issue #28 | **951 PASS** |
| Issue #28 interactive GUI regression check | PASS — informal developer-attested smoke check, not scripted end-to-end QA |
| GCC 15.2.0 Debug GUI/CLI/core build — Issue #27 | **PASS** |
| GCC 15.2.0 Release GUI/CLI/core build — Issue #27 | **PASS** |
| GUI-enabled Debug CTest suite — Issue #27 | **23/23 PASS** |
| GUI-enabled Release CTest suite — Issue #27 | **23/23 PASS** |
| Core-only ASan/UBSan CTest suite — Issue #27 | **16/16 PASS** |
| Named-variable regression assertions — Issue #27 | **3,149 PASS** |
| Issue #27 interactive GUI smoke test | PASS — developer-attested; not independently automated |
| GCC 15.2.0 Debug GUI/CLI/core build — Issue #25 | PASS |
| GCC 15.2.0 Release GUI/CLI/core build — Issue #25 | PASS |
| GUI-enabled Debug CTest suite — Issue #26 | **22/22 PASS** |
| GUI-enabled Release CTest suite — Issue #25 historical | **18/18 PASS**; full Issue #26 suite not recorded |
| Core-only ASan/UBSan CTest suite — Issue #25 historical | **13/13 PASS** |
| Issue #26 Debug full build / Release desktop rebuild | PASS / PASS |
| Issue #26 incremental sampling assertions | **896,291 PASS**, including direct ASan/UBSan run with leak detection disabled |
| Issue #26 canvas / 2× display-scale assertions | **33,628 PASS / 493 PASS** |
| Manual evidence recorder unit tests | **12/12 PASS** |
| Issue #26 original manual GUI session | **24 PASS / 1 FAIL** (`GRA-03`); final-fix manual retest not recorded |
| Benchmark statistics regression assertions | **16 PASS** |
| Qt-independent sampling measurements | **150 recorded** (Release benchmark; six groups) |
| Offscreen Qt rendering and navigation measurements | **750 recorded** (Release benchmark; thirty groups) |
| Sampling median/p95 and evaluation/build totals | Recorded above from benchmark JSON; host-specific |
| Cold/warm render and wheel/drag preview/settlement median/p95 | Recorded above from benchmark JSON; offscreen synthetic events |
| Existing graphing/viewport/cache/budget/scheduling/boundary regressions | PASS — included in current CTest runs |
| Issue #25 Debug GUI launch | Executed |
| Issue #25 manual GUI regression verification | PASS — developer-reported |
| Issue #25 compiler warnings | None displayed in supplied final build output; not a `-Werror` verification |
| Interactive Release GUI verification | Original Issue #26 optional check PASS; final-fix retest not recorded |
| Previous Issue #23 KDE KIO diagnostics | Historical observation; not reported in Issue #25 supplied output |
| Prior CLI smoke test | PASS — previous baseline |
| Full GUI sanitizer coverage | Not performed |
| Targeted rendered-pixel and identical-repaint tests | PASS — Issue #26; full desktop end-to-end automation not performed |
| Native Windows verification | Not performed |

**Current assessment — Issue #28:** GCC 15.2.0 GUI-enabled Debug **24/24** and Release **24/24** CTests passed; the full core-only ASan/UBSan suite passed **17/17**. The new `VariableDefinitions` regression executable passed **951 assertions**. These results support Qt-independent scalar assignments, explicit dependencies, selective updates, undefined propagation, and transactional rejection of malformed or circular redefinitions. Existing GUI behavior was checked informally by the developer with no reported regressions. The GUI does not yet offer interactive assignment rows or sliders; dependency-driven graph updates, function definitions, and variable persistence remain future work. No new performance claims are made; Issue #25 benchmark measurements are preserved as historical data.

**Historical assessment — Issue #27:** GCC 15.2.0 Debug and Release GUI-enabled builds and all 23 CTests passed in each configuration; all 16 core-only ASan/UBSan CTests passed; the named-variable suite passed 3,149 assertions. Explicit named-variable declaration, scope validation, binding resolution, and legacy expression compatibility are supported by this verification. No GUI assignment, slider, dependency tracking, or function-definition implementation is claimed. The original Issue #26 failed manual session and Issue #25 benchmark measurements remain historical evidence.

**Historical assessment — Issue #26:** The Issue #26 fix is supported by a successful Linux Debug full build, **22/22 passing GUI-enabled CTests**, **896,291 passing incremental sampling assertions** including a direct ASan/UBSan run, **12 passing recorder unit tests**, and a successful Release desktop rebuild. Targeted core and rendered-pixel checks cover tangent tails across tested zooms and widths, callback accounting, deterministic completion, retained previews, same-run image stability, resizing, and 2× display scaling. The original manual report still records the tangent failure; no post-fix manual PASS is inferred. Historical Issue #25 test results and 150 sampling / 750 offscreen benchmark measurements remain intact and are not current-renderer speedup evidence. Full current Release and sanitizer suites, full Qt sanitizer coverage, native Windows/Qt 5 behavior, and physical display latency are outside the recorded current verification.
