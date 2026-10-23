# Rational Studio — Verification Report

**Project version:** 0.1.0  
**Verification date:** October 23, 2026  
**Recorded environment:** Linux, GCC 15.2.0, C++20, CMake/Ninja, Qt Widgets  
**Current status:** **PASS (developer-reported GUI verification)** — Issue #25 GCC 15.2.0 Debug and Release builds (18/18 GUI-enabled CTest each), core-only ASan/UBSan (13/13), 16 benchmark-statistics assertions, 150 sampling measurements and 750 offscreen Qt rendering/navigation measurements recorded; manual GUI regression verification reported successful. Performance timings are baseline measurements, not pass/fail thresholds.

This document records results through the graph rendering and navigation performance benchmark infrastructure associated with GitHub Issue #25. It is a **verification record**, not a development roadmap. Results are scoped to the commands and platforms actually exercised; they do not establish exhaustive mathematical correctness.

## 1. Build environment

| Component | Recorded configuration |
|---|---|
| Operating system | Linux |
| Compiler | GCC 15.2.0 |
| Language standard | C++20 |
| Build system | CMake with Ninja generator |
| Current Issue #25 application builds | Debug and Release, GUI enabled; optional benchmark targets ON |
| GUI framework | Qt Widgets |
| Exact arithmetic | Boost.Multiprecision |
| Test runner | CTest |
| Sanitizers | AddressSanitizer and UndefinedBehaviorSanitizer; core-only build |

Qt 6 is the preferred development target. The exact Qt version selected by the recorded CMake configuration was not displayed in the submitted console output. Native Windows compatibility and Qt 5 compatibility have not been independently verified.

## 2. Debug build — PASS

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

### Release build — PASS

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

## 4. AddressSanitizer / UndefinedBehaviorSanitizer — PASS

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

## 5. Desktop GUI — PASS (developer-reported Issue #25 manual verification)

The current Issue #25 Debug application was launched using:

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

## 6. Terminal application

The `RationalCLI` target built successfully in the current Issue #25 Debug, Release, and core-only sanitizer configurations. A historical manual CLI smoke test produced:

```text
Input:     5/2 7/4
Operation: +
Result:    5/2 + 7/4 = 17/4
```

No separate interactive CLI execution was included in the supplied Issue #25 terminal record. The CLI compiled in the Issue #25 Debug, Release, and sanitizer-enabled configurations; the historical smoke-test result is not represented as a new run.

## 7. Platform and configuration coverage

| Platform / configuration | Build | Tests | Runtime |
|---|---|---|---|
| Linux Debug, Qt GUI enabled (Issue #25) | PASS | 18/18 PASS | Manual GUI regression verification PASS (developer-reported); Qt widget tests PASS |
| Linux Release, Qt GUI enabled (Issue #25) | PASS | 18/18 PASS | Offscreen Qt widget tests PASS; 150 core sampling and 750 Qt benchmark observations recorded; interactive Release GUI launch not recorded |
| Linux Debug, GUI disabled, ASan/UBSan (Issue #25) | PASS | 13/13 PASS | Core regression and statistics test execution PASS; Qt GUI excluded |
| Native Windows | Not tested | Not tested | Not tested |
| Qt 5 | Not tested | Not tested | Not tested |

## 8. Outstanding limitations

The following areas remain outside the current verification scope or are known approximation limits:

- Finite probing cannot detect all discontinuities, arbitrarily narrow domain gaps, or unsampled singularities.
- Bounded terminal refinement improves steep continuous curves, but sufficiently narrow or extreme nonlinear features may still be omitted or approximated imperfectly.
- Highly oscillatory functions may be undersampled.
- Per-curve and shared per-paint callback-count limits bound evaluation counts, not the time spent in any callback or the total frame time. Budget-limited geometry can be truncated or deferred.
- Valid `QPainterPath` geometry is cached; rapid navigation uses timer-coalesced refreshes and transformed cached-path previews. Resizes invalidate stale geometry and outdated refresh requests, while bounded pixel-space grid positions avoid unrepresentable label coordinates. At extreme finite centers, grid markers may be suppressed when floating-point precision is insufficient. Sampling remains on the GUI thread under callback budgets; no frame-time, input-latency, or FPS guarantee is claimed.
- Five offscreen Qt `GraphCanvas` suites cover cache, budget, scheduling, viewport-boundary, and cross-feature regression behavior. Issue #25 additionally records Release offscreen repaint and synthetic navigation benchmarks; the timing tools are not CTests, and full end-to-end GUI automation and rendered-pixel correctness testing have not been established.
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

These generic commands describe a reproducible procedure. Current Issue #25 validation used `build-issue25`, `build-issue25-release`, and `build-issue25-asan`. The performance baseline was generated separately with `bash scripts/run_issue25_benchmarks.sh` against the Release benchmark targets.

## 10. Verification summary

| Area | Result |
|---|---|
| GCC 15.2.0 Debug GUI/CLI/core build — Issue #25 | PASS |
| GCC 15.2.0 Release GUI/CLI/core build — Issue #25 | PASS |
| GUI-enabled Debug CTest suite | **18/18 PASS** |
| GUI-enabled Release CTest suite | **18/18 PASS** |
| Core-only ASan/UBSan CTest suite | **13/13 PASS** |
| Benchmark statistics regression assertions | **16 PASS** |
| Qt-independent sampling measurements | **150 recorded** (Release benchmark; six groups) |
| Offscreen Qt rendering and navigation measurements | **750 recorded** (Release benchmark; thirty groups) |
| Sampling median/p95 and evaluation/build totals | Recorded above from benchmark JSON; host-specific |
| Cold/warm render and wheel/drag preview/settlement median/p95 | Recorded above from benchmark JSON; offscreen synthetic events |
| Existing graphing/viewport/cache/budget/scheduling/boundary regressions | PASS — included in current CTest runs |
| Issue #25 Debug GUI launch | Executed |
| Issue #25 manual GUI regression verification | PASS — developer-reported |
| Issue #25 compiler warnings | None displayed in supplied final build output; not a `-Werror` verification |
| Separate interactive Release GUI verification | Not recorded |
| Previous Issue #23 KDE KIO diagnostics | Historical observation; not reported in Issue #25 supplied output |
| Prior CLI smoke test | PASS — previous baseline |
| Full GUI sanitizer coverage | Not performed |
| End-to-end GUI/pixel-level testing | Not performed |
| Native Windows verification | Not performed |

**Assessment:** Issue #25 is supported by successful GCC 15.2.0 Linux Debug and Release builds, eighteen passing GUI-enabled CTests in both configurations, thirteen passing Qt-independent ASan/UBSan tests, and sixteen passing benchmark-statistics assertions. The developer reports successful manual GUI checks. The Release performance runner recorded 150 sampling and 750 offscreen canvas/navigation measurements; median/p95 values, aggregate callbacks/builds, scenarios, metadata, and source hashes are captured above from local measurement files. This is an initial, environment-dependent benchmark baseline, **not** a claim of higher FPS, guaranteed input latency, or measured speedup over an uninstrumented prior revision. Interactive Release GUI and Qt sanitizer coverage are not recorded, and physical display latency was not measured.
