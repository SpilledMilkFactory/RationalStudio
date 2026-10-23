# Rational Studio — Verification Report

**Project version:** 0.1.0  
**Verification date:** October 23, 2026  
**Recorded environment:** Linux, GCC 15.2.0, C++20, CMake/Ninja, Qt Widgets  
**Current status:** **PASS (developer-attested manual fix)** — Issue #22 GCC 15.2.0 Debug and Release builds (13/13 CTest each), core-only ASan/UBSan (10/10), 5,947 scheduler and 57 offscreen Qt scheduling assertions; manual flicker fix and retest reported.

This document records results for the implementation through the graph repaint scheduling improvements associated with GitHub Issue #22. It is a **verification record**, not a development roadmap. Results are scoped to the commands and platforms actually exercised; they do not establish exhaustive mathematical correctness.

## 1. Build environment

| Component | Recorded configuration |
|---|---|
| Operating system | Linux |
| Compiler | GCC 15.2.0 |
| Language standard | C++20 |
| Build system | CMake with Ninja generator |
| Current Issue #22 application builds | Debug and Release, GUI enabled |
| GUI framework | Qt Widgets |
| Exact arithmetic | Boost.Multiprecision |
| Test runner | CTest |
| Sanitizers | AddressSanitizer and UndefinedBehaviorSanitizer; core-only build |

Qt 6 is the preferred development target. The exact Qt version selected by the recorded CMake configuration was not displayed in the submitted console output. Native Windows compatibility and Qt 5 compatibility have not been independently verified.

## 2. Debug build — PASS

**Commands executed:**

```bash
cmake -S . -B build-issue22 -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build-issue22 --parallel
```

GCC 15.2.0 configured and built the Qt application, CLI, and test executables successfully.

**Final recorded build output:**

```text
[48/48] Linking CXX executable RationalStudio
```

The Qt test `tests/GraphCanvasSchedulingTests.cpp` produced three `-Wdeprecated-declarations` warnings for the older `QMouseEvent` constructor (lines 116, 120, and 124). They are source-compatibility warnings in the test, not compilation failures or demonstrated runtime errors. The new panning flicker is separately reported as a GUI regression.

### Release build — PASS

**Commands executed:**

```bash
cmake -S . -B build-issue22-release -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build-issue22-release --parallel
ctest --test-dir build-issue22-release --output-on-failure
```

**Final recorded build output:**

```text
[48/48] Linking CXX executable RationalStudio
```

**Recorded Release CTest summary:**

```text
100% tests passed, 0 tests failed out of 13
Total Test time (real) = 0.39 sec
```

The same three Qt mouse-event deprecation warnings appeared in the Release build. A separate interactive Release GUI launch is not recorded.

## 3. Automated regression tests — PASS

**Command executed:**

```bash
ctest --test-dir build-issue22 --output-on-failure
```

**Recorded Debug CTest output:**

```text
1/13 Test #1: RationalCoreTests ... Passed 0.00 sec
2/13 Test #2: RationalViewportTests ... Passed 0.00 sec
3/13 Test #3: RationalCurveSamplerTests ... Passed 0.00 sec
4/13 Test #4: RationalCurveDomainTests ... Passed 0.01 sec
5/13 Test #5: RationalCurveDiscontinuityTests ... Passed 0.02 sec
6/13 Test #6: RationalCurveSteepTests ... Passed 0.02 sec
7/13 Test #7: RationalViewportPrecisionTests ... Passed 0.00 sec
8/13 Test #8: RationalCurveGeometryCacheTests ... Passed 0.00 sec
9/13 Test #9: RationalCurveEvaluationBudgetTests ... Passed 0.01 sec
10/13 Test #10: RationalGraphRepaintSchedulerTests ... Passed 0.00 sec
11/13 Test #11: RationalGraphCanvasCacheTests ... Passed 0.07 sec
12/13 Test #12: RationalGraphCanvasBudgetTests ... Passed 0.11 sec
13/13 Test #13: RationalGraphCanvasSchedulingTests ... Passed 0.19 sec

100% tests passed, 0 tests failed out of 13
Total Test time (real) = 0.44 sec
```

**Recorded Issue #22 Release CTest:** 13/13 passed (0.39 seconds total).

The Issue #22 GUI-enabled suite includes three Qt offscreen widget regression executables (cache, budget, and scheduling), for 13 tests total. The GUI-disabled sanitizer configuration excludes the three Qt tests and registers ten Qt-independent tests.

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

## 4. AddressSanitizer / UndefinedBehaviorSanitizer — PASS

**Configuration and build:**

```bash
cmake -S . -B build-issue22-asan -G Ninja \
    -DCMAKE_BUILD_TYPE=Debug \
    -DRATIONALSTUDIO_BUILD_GUI=OFF \
    -DCMAKE_CXX_FLAGS="-fsanitize=address,undefined -fno-omit-frame-pointer" \
    -DCMAKE_EXE_LINKER_FLAGS="-fsanitize=address,undefined"
cmake --build build-issue22-asan --parallel
```

**Final build output:**

```text
[27/27] Linking CXX executable RationalCoreTests
```

**Test command:**

```bash
ctest --test-dir build-issue22-asan --output-on-failure
```

**Recorded output:**

```text
1/10 Test #1: RationalCoreTests ... Passed 0.02 sec
2/10 Test #2: RationalViewportTests ... Passed 0.01 sec
3/10 Test #3: RationalCurveSamplerTests ... Passed 0.02 sec
4/10 Test #4: RationalCurveDomainTests ... Passed 0.03 sec
5/10 Test #5: RationalCurveDiscontinuityTests ... Passed 0.10 sec
6/10 Test #6: RationalCurveSteepTests ... Passed 0.07 sec
7/10 Test #7: RationalViewportPrecisionTests ... Passed 0.02 sec
8/10 Test #8: RationalCurveGeometryCacheTests ... Passed 0.02 sec
9/10 Test #9: RationalCurveEvaluationBudgetTests ... Passed 0.03 sec
10/10 Test #10: RationalGraphRepaintSchedulerTests ... Passed 0.01 sec

100% tests passed, 0 tests failed out of 10
Total Test time (real) = 0.33 sec
```

All ten Qt-independent tests passed in the sanitizer-enabled configuration. No ASan or UBSan failures were reported in the captured output.

**Scope:** `RATIONALSTUDIO_BUILD_GUI=OFF` excludes the desktop application and all three Qt widget tests: `RationalGraphCanvasCacheTests`, `RationalGraphCanvasBudgetTests`, and `RationalGraphCanvasSchedulingTests`. The ten core-only tests do **not** establish sanitizer coverage for Qt rendering or GUI interactions.

## 5. Desktop GUI — PASS (developer-attested post-fix verification)

The current Issue #22 Debug application was launched using:

```bash
./build-issue22/RationalStudio
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

**Scope:** Manual functional checks and offscreen widget tests are not complete end-to-end GUI automation, pixel-comparison testing, accessibility validation, or performance benchmarking. The recorded results do not establish mathematically perfect graph rendering.

## 6. Terminal application

The `RationalCLI` target built successfully in the current Issue #22 Debug, Release, and core-only sanitizer configurations. A historical manual CLI smoke test produced:

```text
Input:     5/2 7/4
Operation: +
Result:    5/2 + 7/4 = 17/4
```

No separate interactive CLI execution was included in the supplied Issue #22 terminal record. The CLI compiled in the Issue #22 Debug, Release, and sanitizer-enabled configurations; the historical smoke-test result is not represented as a new run.

## 7. Platform and configuration coverage

| Platform / configuration | Build | Tests | Runtime |
|---|---|---|---|
| Linux Debug, Qt GUI enabled | PASS | 13/13 PASS | PASS (reported after flicker fix and manual retest); offscreen scheduling test PASS |
| Linux Release, Qt GUI enabled (Issue #22) | PASS | 13/13 PASS | Offscreen scheduling test PASS; interactive Release GUI not recorded |
| Linux Debug, GUI disabled, ASan/UBSan | PASS | 10/10 PASS | Core test execution PASS; Qt GUI excluded |
| Native Windows | Not tested | Not tested | Not tested |
| Qt 5 | Not tested | Not tested | Not tested |

## 8. Outstanding limitations

The following areas remain outside the current verification scope or are known approximation limits:

- Finite probing cannot detect all discontinuities, arbitrarily narrow domain gaps, or unsampled singularities.
- Bounded terminal refinement improves steep continuous curves, but sufficiently narrow or extreme nonlinear features may still be omitted or approximated imperfectly.
- Highly oscillatory functions may be undersampled.
- Per-curve and shared per-paint callback-count limits bound evaluation counts, not the time spent in any callback or the total frame time. Budget-limited geometry can be truncated or deferred.
- Valid `QPainterPath` geometry is cached; rapid navigation uses timer-coalesced refreshes and transformed cached-path previews. Sampling remains on the GUI thread under callback budgets; no frame-time, input-latency, or FPS guarantee is claimed.
- Offscreen Qt `GraphCanvas` cache, budget, and scheduling tests exist; full end-to-end GUI automation and rendered-pixel correctness tests have not been established.
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

These generic commands describe a reproducible procedure. The current Issue #22 results came from `build-issue22`, `build-issue22-release`, and `build-issue22-asan`.

## 10. Verification summary

| Area | Result |
|---|---|
| GCC 15.2.0 Debug GUI/CLI/core build — Issue #22 | PASS |
| GCC 15.2.0 Release GUI/CLI/core build — Issue #22 | PASS |
| GUI-enabled Debug CTest suite | **13/13 PASS** |
| GUI-enabled Release CTest suite | **13/13 PASS** |
| Core-only ASan/UBSan CTest suite | **10/10 PASS** |
| Graph repaint scheduler regression assertions | **5,947 PASS** |
| Offscreen Qt GraphCanvas scheduling assertions | **57 PASS** |
| Existing core/cache/budget regression suites | PASS — included in current CTest runs |
| Issue #22 Debug GUI launch | Executed |
| Issue #22 visual panning verification | PASS (reported after flicker fix and manual retest) |
| Qt mouse-event test constructor warnings | Three deprecation warnings (Debug and Release); not build failures |
| Prior CLI smoke test | PASS — previous baseline |
| Full GUI sanitizer coverage | Not performed |
| End-to-end GUI / pixel-level testing | Not performed |
| Native Windows verification | Not performed |

**Assessment:** Issue #22 compiled with GCC 15.2.0 in Debug and Release, and passed all 13 GUI-enabled CTest executables in both builds plus ten core-only ASan/UBSan tests. Direct scheduler and offscreen scheduling runs passed 5,947 and 57 assertions respectively. Three Qt `QMouseEvent` deprecation warnings occurred in the test source; these are unrelated to any proven runtime defect. The developer initially observed visible flashing during panning, and subsequently attests the defect was corrected and the GUI retested successfully. The post-fix source and test results should be reviewed before merging. Coalescing and evaluation budgets are not wall-clock guarantees; automated tests do not establish pixel-level rendering correctness, GUI sanitizer coverage, or native Windows support.
