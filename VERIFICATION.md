# Rational Studio — Verification Report

**Project version:** 0.1.0  
**Verification date:** October 23, 2026  
**Recorded environment:** Linux, GCC 15.2.0, C++20, CMake/Ninja, Qt Widgets  
**Current status:** **PASS (developer-reported GUI verification)** — Issue #24 GCC 15.2.0 Debug and Release builds (17/17 GUI-enabled CTest each), core-only ASan/UBSan (12/12), 5,021 Qt-independent graph regression-matrix assertions and 92 offscreen Qt graph regression-matrix assertions; manual GUI regression verification reported successful.

This document records results for the implementation through the cross-subsystem graph sampling and viewport regression coverage associated with GitHub Issue #24. It is a **verification record**, not a development roadmap. Results are scoped to the commands and platforms actually exercised; they do not establish exhaustive mathematical correctness.

## 1. Build environment

| Component | Recorded configuration |
|---|---|
| Operating system | Linux |
| Compiler | GCC 15.2.0 |
| Language standard | C++20 |
| Build system | CMake with Ninja generator |
| Current Issue #24 application builds | Debug and Release, GUI enabled |
| GUI framework | Qt Widgets |
| Exact arithmetic | Boost.Multiprecision |
| Test runner | CTest |
| Sanitizers | AddressSanitizer and UndefinedBehaviorSanitizer; core-only build |

Qt 6 is the preferred development target. The exact Qt version selected by the recorded CMake configuration was not displayed in the submitted console output. Native Windows compatibility and Qt 5 compatibility have not been independently verified.

## 2. Debug build — PASS

**Commands executed:**

```bash
cmake -S . -B build-issue24 -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build-issue24 --parallel
```

GCC 15.2.0 configured and built the Qt desktop application, CLI, and regression-test targets successfully. Issue #24 adds tests without changing production rendering algorithms.

**Final recorded Debug build output:**

```text
[62/62] Linking CXX executable RationalStudio
```

No compiler warnings or errors appeared in the supplied Issue #24 Debug build output. This is not a warnings-as-errors certification.

### Release build — PASS

**Commands executed:**

```bash
cmake -S . -B build-issue24-release -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build-issue24-release --parallel
ctest --test-dir build-issue24-release --output-on-failure
```

**Final recorded Release build output:**

```text
[62/62] Linking CXX executable RationalStudio
```

**Recorded Release CTest summary:**

```text
100% tests passed, 0 tests failed out of 17
Total Test time (real) = 0.68 sec
```

No compiler warnings or errors appeared in the supplied Issue #24 Release build output. No separate interactive Release GUI launch was reported; the developer manually verified the Debug GUI.

## 3. Automated regression tests — PASS

**Command executed:**

```bash
ctest --test-dir build-issue24 --output-on-failure
```

**Recorded Issue #24 Debug CTest results:**

```text
1/17 Test #1: RationalCoreTests ... Passed 0.00 sec
2/17 Test #2: RationalViewportTests ... Passed 0.00 sec
3/17 Test #3: RationalCurveSamplerTests ... Passed 0.00 sec
4/17 Test #4: RationalCurveDomainTests ... Passed 0.01 sec
5/17 Test #5: RationalCurveDiscontinuityTests ... Passed 0.02 sec
6/17 Test #6: RationalCurveSteepTests ... Passed 0.02 sec
7/17 Test #7: RationalViewportPrecisionTests ... Passed 0.00 sec
8/17 Test #8: RationalCurveGeometryCacheTests ... Passed 0.00 sec
9/17 Test #9: RationalCurveEvaluationBudgetTests ... Passed 0.01 sec
10/17 Test #10: RationalGraphRepaintSchedulerTests ... Passed 0.00 sec
11/17 Test #11: RationalGraphViewportBoundaryTests ... Passed 0.00 sec
12/17 Test #12: RationalGraphRegressionMatrixTests ... Passed 0.01 sec
13/17 Test #13: RationalGraphCanvasCacheTests ... Passed 0.07 sec
14/17 Test #14: RationalGraphCanvasBudgetTests ... Passed 0.11 sec
15/17 Test #15: RationalGraphCanvasSchedulingTests ... Passed 0.29 sec
16/17 Test #16: RationalGraphCanvasBoundaryTests ... Passed 0.07 sec
17/17 Test #17: RationalGraphCanvasRegressionMatrixTests ... Passed 0.12 sec

100% tests passed, 0 tests failed out of 17
Total Test time (real) = 0.74 sec
```

**Recorded Issue #24 Release CTest:** 17/17 passed (0.68 seconds total).

The Issue #24 GUI-enabled suite includes twelve Qt-independent executables and five offscreen Qt widget executables (cache, budget, scheduling, boundary, and the new cross-feature regression matrix). The Qt-disabled ASan/UBSan configuration runs twelve tests; it does not exercise the five Qt widget suites.

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

## 4. AddressSanitizer / UndefinedBehaviorSanitizer — PASS

**Configuration and build:**

```bash
cmake -S . -B build-issue24-asan -G Ninja \
    -DCMAKE_BUILD_TYPE=Debug \
    -DRATIONALSTUDIO_BUILD_GUI=OFF \
    -DCMAKE_CXX_FLAGS="-fsanitize=address,undefined -fno-omit-frame-pointer" \
    -DCMAKE_EXE_LINKER_FLAGS="-fsanitize=address,undefined"
cmake --build build-issue24-asan --parallel
```

**Final recorded build output:**

```text
[31/31] Linking CXX executable RationalCurveDiscontinuityTests
```

**Test command:**

```bash
ctest --test-dir build-issue24-asan --output-on-failure
```

**Recorded output:**

```text
1/12 Test #1: RationalCoreTests ... Passed 0.02 sec
2/12 Test #2: RationalViewportTests ... Passed 0.01 sec
3/12 Test #3: RationalCurveSamplerTests ... Passed 0.02 sec
4/12 Test #4: RationalCurveDomainTests ... Passed 0.04 sec
5/12 Test #5: RationalCurveDiscontinuityTests ... Passed 0.09 sec
6/12 Test #6: RationalCurveSteepTests ... Passed 0.07 sec
7/12 Test #7: RationalViewportPrecisionTests ... Passed 0.01 sec
8/12 Test #8: RationalCurveGeometryCacheTests ... Passed 0.02 sec
9/12 Test #9: RationalCurveEvaluationBudgetTests ... Passed 0.03 sec
10/12 Test #10: RationalGraphRepaintSchedulerTests ... Passed 0.01 sec
11/12 Test #11: RationalGraphViewportBoundaryTests ... Passed 0.01 sec
12/12 Test #12: RationalGraphRegressionMatrixTests ... Passed 0.06 sec

100% tests passed, 0 tests failed out of 12
Total Test time (real) = 0.40 sec
```

All twelve Qt-independent tests passed in the sanitizer-enabled configuration. The submitted output reports no ASan or UBSan failures.

**Scope:** `RATIONALSTUDIO_BUILD_GUI=OFF` excludes the desktop application and all five Qt widget regression executables. The sanitizer results do **not** establish memory-safety coverage for the Qt renderer or interactive GUI.

## 5. Desktop GUI — PASS (developer-reported Issue #24 manual verification)

The current Issue #24 Debug application was launched using:

```bash
./build-issue24/RationalStudio
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

**Scope:** Manual functional checks and offscreen widget tests are not complete end-to-end GUI automation, pixel-comparison testing, accessibility validation, or performance benchmarking. The recorded results do not establish mathematically perfect graph rendering.

## 6. Terminal application

The `RationalCLI` target built successfully in the current Issue #24 Debug, Release, and core-only sanitizer configurations. A historical manual CLI smoke test produced:

```text
Input:     5/2 7/4
Operation: +
Result:    5/2 + 7/4 = 17/4
```

No separate interactive CLI execution was included in the supplied Issue #24 terminal record. The CLI compiled in the Issue #24 Debug, Release, and sanitizer-enabled configurations; the historical smoke-test result is not represented as a new run.

## 7. Platform and configuration coverage

| Platform / configuration | Build | Tests | Runtime |
|---|---|---|---|
| Linux Debug, Qt GUI enabled (Issue #24) | PASS | 17/17 PASS | Manual GUI regression verification PASS (developer-reported); five offscreen Qt suites PASS |
| Linux Release, Qt GUI enabled (Issue #24) | PASS | 17/17 PASS | Offscreen Qt suites PASS; interactive Release GUI launch not recorded |
| Linux Debug, GUI disabled, ASan/UBSan (Issue #24) | PASS | 12/12 PASS | Core test execution PASS; Qt GUI excluded |
| Native Windows | Not tested | Not tested | Not tested |
| Qt 5 | Not tested | Not tested | Not tested |

## 8. Outstanding limitations

The following areas remain outside the current verification scope or are known approximation limits:

- Finite probing cannot detect all discontinuities, arbitrarily narrow domain gaps, or unsampled singularities.
- Bounded terminal refinement improves steep continuous curves, but sufficiently narrow or extreme nonlinear features may still be omitted or approximated imperfectly.
- Highly oscillatory functions may be undersampled.
- Per-curve and shared per-paint callback-count limits bound evaluation counts, not the time spent in any callback or the total frame time. Budget-limited geometry can be truncated or deferred.
- Valid `QPainterPath` geometry is cached; rapid navigation uses timer-coalesced refreshes and transformed cached-path previews. Resizes invalidate stale geometry and outdated refresh requests, while bounded pixel-space grid positions avoid unrepresentable label coordinates. At extreme finite centers, grid markers may be suppressed when floating-point precision is insufficient. Sampling remains on the GUI thread under callback budgets; no frame-time, input-latency, or FPS guarantee is claimed.
- Five offscreen Qt `GraphCanvas` suites cover cache, budget, scheduling, viewport-boundary, and cross-feature regression behavior. Full end-to-end GUI automation and rendered-pixel correctness testing have not been established.
- A panning-flicker regression was reported during initial Issue #22 manual verification; the developer subsequently attested that it was corrected and retested.
- The complete Qt application has not undergone recorded sanitizer testing.
- Native Windows builds, Qt 5 compatibility, and separate runtime verification of the Release GUI remain unverified.

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

These generic commands describe a reproducible procedure. The current Issue #24 results came from `build-issue24`, `build-issue24-release`, and `build-issue24-asan`.

## 10. Verification summary

| Area | Result |
|---|---|
| GCC 15.2.0 Debug GUI/CLI/core build — Issue #24 | PASS |
| GCC 15.2.0 Release GUI/CLI/core build — Issue #24 | PASS |
| GUI-enabled Debug CTest suite | **17/17 PASS** |
| GUI-enabled Release CTest suite | **17/17 PASS** |
| Core-only ASan/UBSan CTest suite | **12/12 PASS** |
| Qt-independent graph regression matrix assertions | **5,021 PASS** |
| Offscreen Qt `GraphCanvas` regression matrix assertions | **92 PASS** |
| Existing sampling/viewport/cache/budget/scheduler/boundary tests | PASS — included in current CTest runs |
| Issue #24 Debug GUI launch | Executed |
| Issue #24 manual GUI regression verification | PASS — developer-reported |
| Issue #24 compiler warnings | None displayed in supplied output; not a `-Werror` verification |
| Separate interactive Release GUI verification | Not recorded |
| Previous Issue #23 KDE KIO diagnostics | Historical observation; not reported in current Issue #24 console output |
| Prior CLI smoke test | PASS — previous baseline |
| Full GUI sanitizer coverage | Not performed |
| End-to-end GUI / pixel-level testing | Not performed |
| Native Windows verification | Not performed |

**Assessment:** Issue #24 is supported by successful GCC 15.2.0 Linux Debug and Release builds, seventeen passing GUI-enabled CTest executables in each configuration, twelve passing Qt-independent ASan/UBSan tests, 5,021 cross-subsystem core regression assertions, and 92 offscreen Qt widget regression assertions. The developer reports successful manual GUI regression verification. Tests address viewport transformations, zoom anchoring, curve-segment approximation accuracy, selected undefined domains and discontinuities, and evaluation ceilings. This milestone adds regression coverage without changing production rendering algorithms. The results do not establish exhaustive mathematical correctness, pixel-level equality, full GUI sanitizer coverage, native Windows support, or guaranteed frame time.
