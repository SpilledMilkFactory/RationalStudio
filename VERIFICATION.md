# Rational Studio — Verification Report

**Project version:** 0.1.0  
**Verification date:** October 22, 2026  
**Recorded environment:** Linux, GCC 15.2.0, C++20, CMake/Ninja, Qt Widgets  
**Current status:** **PASS** — Issue #21 GCC 15.2.0 Debug build (11/11 CTest), core-only ASan/UBSan (9/9 CTest), 5,944 evaluation-budget assertions and 20 Qt budget assertions. Issue #21 Release verification not recorded; manual GUI verification PASS (reported).

This document records results for the implementation through the bounded curve evaluation and sampling-budget improvements associated with GitHub Issue #21. It is a **verification record**, not a development roadmap. Results are scoped to the commands and platforms actually exercised; they do not establish exhaustive mathematical correctness.

## 1. Build environment

| Component | Recorded configuration |
|---|---|
| Operating system | Linux |
| Compiler | GCC 15.2.0 |
| Language standard | C++20 |
| Build system | CMake with Ninja generator |
| Current Issue #21 application build | Debug, GUI enabled (Release not recorded) |
| GUI framework | Qt Widgets |
| Exact arithmetic | Boost.Multiprecision |
| Test runner | CTest |
| Sanitizers | AddressSanitizer and UndefinedBehaviorSanitizer; core-only build |

Qt 6 is the preferred development target. The exact Qt version selected by the recorded CMake configuration was not displayed in the submitted console output. Native Windows compatibility and Qt 5 compatibility have not been independently verified.

## 2. Debug build — PASS

**Commands executed:**

```bash
cmake -S . -B build-issue21 -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build-issue21 --parallel
```

The full Debug configuration succeeded with GCC 15.2.0 and linked the Qt desktop application, the CLI, and the registered test executables.

**Final recorded build output:**

```text
[41/41] Linking CXX executable RationalStudio
```

The current Issue #21 Debug configuration and build completed without errors. No compiler warnings appeared in the supplied build output; this is not a warnings-as-errors certification.

### Release build — Previous Issue #20 PASS; Issue #21 not recorded

**Commands executed:**

```bash
cmake -S . -B build-issue20-release -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build-issue20-release --parallel
ctest --test-dir build-issue20-release --output-on-failure
```

**Final recorded build output:**

```text
[34/34] Linking CXX executable RationalStudio
```

**Previous Issue #20 Release CTest summary (historical):**

```text
100% tests passed, 0 tests failed out of 9
Total Test time (real) = 0.15 sec
```

These commands and results belong to the **previous Issue #20 baseline**. The submitted Issue #21 terminal output contains no Release configuration, build, or CTest run. Issue #21 Release status remains **not verified**; the previous 9/9 result must not be presented as an Issue #21 result. A separate interactive launch of the Release GUI has not been recorded.

## 3. Automated regression tests — PASS

**Command executed:**

```bash
ctest --test-dir build-issue21 --output-on-failure
```

**Recorded Debug CTest output:**

```text
Start  1: RationalCoreTests
 1/11 Test  #1: RationalCoreTests ....................   Passed    0.00 sec
      Start  2: RationalViewportTests
 2/11 Test  #2: RationalViewportTests ................   Passed    0.00 sec
      Start  3: RationalCurveSamplerTests
 3/11 Test  #3: RationalCurveSamplerTests ............   Passed    0.00 sec
      Start  4: RationalCurveDomainTests
 4/11 Test  #4: RationalCurveDomainTests .............   Passed    0.01 sec
      Start  5: RationalCurveDiscontinuityTests
 5/11 Test  #5: RationalCurveDiscontinuityTests ......   Passed    0.02 sec
      Start  6: RationalCurveSteepTests
 6/11 Test  #6: RationalCurveSteepTests ..............   Passed    0.02 sec
      Start  7: RationalViewportPrecisionTests
 7/11 Test  #7: RationalViewportPrecisionTests .......   Passed    0.00 sec
      Start  8: RationalCurveGeometryCacheTests
 8/11 Test  #8: RationalCurveGeometryCacheTests ......   Passed    0.00 sec
      Start  9: RationalCurveEvaluationBudgetTests
 9/11 Test  #9: RationalCurveEvaluationBudgetTests ...   Passed    0.01 sec
      Start 10: RationalGraphCanvasCacheTests
10/11 Test #10: RationalGraphCanvasCacheTests ........   Passed    0.11 sec
      Start 11: RationalGraphCanvasBudgetTests
11/11 Test #11: RationalGraphCanvasBudgetTests .......   Passed    0.11 sec

100% tests passed, 0 tests failed out of 11
Total Test time (real) = 0.29 sec
```

**Previous Issue #20 Release CTest result:** 9/9 passed (0.15 seconds total). **Issue #21 Release CTest:** not recorded.

The Issue #21 GUI-enabled suite includes two Qt offscreen widget regression executables (cache and budget), for 11 tests total. The GUI-disabled sanitizer configuration excludes both Qt tests and registers nine Qt-independent tests.

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

## 4. AddressSanitizer / UndefinedBehaviorSanitizer — PASS

**Configuration and build:**

```bash
cmake -S . -B build-issue21-asan -G Ninja \
    -DCMAKE_BUILD_TYPE=Debug \
    -DRATIONALSTUDIO_BUILD_GUI=OFF \
    -DCMAKE_CXX_FLAGS="-fsanitize=address,undefined -fno-omit-frame-pointer" \
    -DCMAKE_EXE_LINKER_FLAGS="-fsanitize=address,undefined"
cmake --build build-issue21-asan --parallel
```

**Final build output:**

```text
[25/25] Linking CXX executable RationalCoreTests
```

**Test command:**

```bash
ctest --test-dir build-issue21-asan --output-on-failure
```

**Recorded output:**

```text
Start 1: RationalCoreTests
1/9 Test #1: RationalCoreTests .................... Passed    0.02 sec
    Start 2: RationalViewportTests
2/9 Test #2: RationalViewportTests ................ Passed    0.01 sec
    Start 3: RationalCurveSamplerTests
3/9 Test #3: RationalCurveSamplerTests ............ Passed    0.02 sec
    Start 4: RationalCurveDomainTests
4/9 Test #4: RationalCurveDomainTests ............. Passed    0.03 sec
    Start 5: RationalCurveDiscontinuityTests
5/9 Test #5: RationalCurveDiscontinuityTests ...... Passed    0.10 sec
    Start 6: RationalCurveSteepTests
6/9 Test #6: RationalCurveSteepTests .............. Passed    0.07 sec
    Start 7: RationalViewportPrecisionTests
7/9 Test #7: RationalViewportPrecisionTests ....... Passed    0.02 sec
    Start 8: RationalCurveGeometryCacheTests
8/9 Test #8: RationalCurveGeometryCacheTests ...... Passed    0.01 sec
    Start 9: RationalCurveEvaluationBudgetTests
9/9 Test #9: RationalCurveEvaluationBudgetTests ... Passed    0.03 sec

100% tests passed, 0 tests failed out of 9
Total Test time (real) = 0.32 sec
```

All nine Qt-independent tests passed in the sanitizer-enabled configuration. No ASan or UBSan failures were reported in the captured output.

**Scope:** `RATIONALSTUDIO_BUILD_GUI=OFF` excludes the desktop application and both `RationalGraphCanvasCacheTests` and `RationalGraphCanvasBudgetTests`. The nine core-only tests do **not** establish sanitizer coverage for Qt rendering or GUI interactions.

## 5. Desktop GUI — PASS (reported manual verification and offscreen cache tests)

The current Issue #21 Debug application was launched using:

```bash
./build-issue21/RationalStudio
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

**Scope:** Manual functional checks and offscreen cache tests are not complete end-to-end GUI automation, pixel-comparison testing, accessibility validation, or performance benchmarking. The recorded results do not establish mathematically perfect graph rendering.

## 6. Terminal application

The `RationalCLI` target built successfully in the current Issue #21 Debug and core-only sanitizer configurations. The previously verified Issue #20 Release build also included the CLI. A historical manual CLI smoke test produced:

```text
Input:     5/2 7/4
Operation: +
Result:    5/2 + 7/4 = 17/4
```

No separate interactive CLI execution was included in the supplied Issue #21 terminal record. The CLI compiled in the Issue #21 Debug and sanitizer-enabled configurations; the Issue #21 Release configuration was not recorded. The historical smoke-test result is not represented as a new run.

## 7. Platform and configuration coverage

| Platform / configuration | Build | Tests | Runtime |
|---|---|---|---|
| Linux Debug, Qt GUI enabled | PASS | 11/11 PASS | GUI launched; Qt offscreen cache and budget tests PASS; manual Issue #21: PASS — developer attested successful interactive GUI checks |
| Linux Release, Qt GUI enabled (Issue #21) | Not recorded | Not recorded | Previous Issue #20 Release 9/9 PASS; no Issue #21 Release run provided |
| Linux Debug, GUI disabled, ASan/UBSan | PASS | 9/9 PASS | Core test execution PASS; Qt GUI excluded |
| Native Windows | Not tested | Not tested | Not tested |
| Qt 5 | Not tested | Not tested | Not tested |

## 8. Outstanding limitations

The following areas remain outside the current verification scope or are known approximation limits:

- Finite probing cannot detect all discontinuities, arbitrarily narrow domain gaps, or unsampled singularities.
- Bounded terminal refinement improves steep continuous curves, but sufficiently narrow or extreme nonlinear features may still be omitted or approximated imperfectly.
- Highly oscillatory functions may be undersampled.
- Per-curve and shared per-paint callback-count limits bound evaluation counts, not the time spent in any callback or the total frame time. Budget-limited geometry can be truncated or deferred.
- Valid per-expression `QPainterPath` geometry is cached across ordinary repaints; invalidated curves rebuild on the GUI thread under callback budgets and may be deferred. No measured frame-time guarantee or FPS improvement is claimed.
- Offscreen Qt `GraphCanvas` cache and budget regression tests exist; full end-to-end GUI automation and rendered-pixel correctness tests have not been established.
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

These commands describe a reproducible procedure. Issue #21 Debug and sanitizer results came from `build-issue21` and `build-issue21-asan`. The latest Release results remain the previous Issue #20 run from `build-issue20-release`; Issue #21 Release verification is not yet recorded.

## 10. Verification summary

| Area | Result |
|---|---|
| CMake/Ninja Debug configuration — Issue #21 | PASS |
| CMake/Ninja Release configuration — Issue #21 | Not recorded (previous Issue #20 Release PASS) |
| GCC 15.2.0 GUI/CLI/core compilation — Issue #21 | PASS — Debug |
| GUI-enabled Debug CTest suite | **11/11 PASS** |
| GUI-enabled Release CTest suite — Issue #21 | Not recorded |
| Core-only ASan/UBSan CTest suite | **9/9 PASS** |
| Evaluation-budget regression assertions | **5,944 PASS** |
| Offscreen Qt `GraphCanvas` budget assertions | **20 PASS** |
| Prior cache-policy regression assertions | **55 PASS** — Issue #20 baseline |
| Prior offscreen Qt cache assertions | **49 PASS** — Issue #20 baseline |
| Issue #21 Debug GUI launch | Executed |
| Issue #21 manual GUI sampling-budget checks | PASS — developer attested successful interactive GUI checks |
| Offscreen Qt cache and budget regression suites | PASS — focused tests, not full GUI automation |
| Prior CLI smoke test | PASS — previous baseline |
| Full GUI sanitizer coverage | Not performed |
| End-to-end GUI / pixel-level testing | Not performed |
| Native Windows verification | Not performed |

**Assessment:** Issue #21 is supported by a successful GCC 15.2.0 Linux Debug build, 11 passing GUI-enabled CTest executables, nine passing core-only ASan/UBSan tests, 5,944 budget-accounting assertions, and 20 passing offscreen Qt budget assertions. The Debug application launched; the developer attests successful Issue #21 manual GUI verification. Per-curve and per-paint evaluation ceilings bound callback counts rather than execution time; geometry may be truncated or deferred, and no FPS or frame-time gain is claimed. No Issue #21 Release build/test was included in the submitted record (the previous Issue #20 Release verification remains historical). Full GUI sanitizer testing, end-to-end GUI automation, and Windows verification remain outstanding.
