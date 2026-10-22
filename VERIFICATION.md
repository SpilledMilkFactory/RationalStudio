# Rational Studio — Verification Report

**Project version:** 0.1.0  
**Verification date:** October 22, 2026  
**Recorded environment:** Linux, GCC 15.2.0, C++20, CMake/Ninja, Qt Widgets  
**Current status:** **PASS** — Debug and Release builds, six automated test suites, core-only ASan/UBSan verification, and reported successful manual GUI checks.

This document records results for the implementation through the steep continuous-curve preservation changes associated with GitHub Issue #18. It is a **verification record**, not a development roadmap. Results are scoped to the commands and platforms actually exercised; they do not establish exhaustive mathematical correctness.

## 1. Build environment

| Component | Recorded configuration |
|---|---|
| Operating system | Linux |
| Compiler | GCC 15.2.0 |
| Language standard | C++20 |
| Build system | CMake with Ninja generator |
| Application builds | Debug and Release, GUI enabled |
| GUI framework | Qt Widgets |
| Exact arithmetic | Boost.Multiprecision |
| Test runner | CTest |
| Sanitizers | AddressSanitizer and UndefinedBehaviorSanitizer; core-only build |

Qt 6 is the preferred development target. The exact Qt version selected by the recorded CMake configuration was not displayed in the submitted console output. Native Windows compatibility and Qt 5 compatibility have not been independently verified.

## 2. Debug build — PASS

**Commands executed:**

```bash
cmake -S . -B build-issue18 -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build-issue18 --parallel
```

CMake successfully configured and generated the build. GCC compiled and linked the mathematical core, test executables, CLI, and Qt desktop application.

**Final build output:**

```text
[25/25] Linking CXX executable RationalStudio
```

No configuration errors, compiler errors, linker errors, or compiler warnings appeared in the supplied output. This confirms that the earlier narrowing-conversion and malformed-shell-command warnings were not reproduced in this build; it is not a warning-as-error test.

### Release build — PASS

The current implementation was also compiled and tested in a separate Release configuration using GCC 15.2.0.

Commands executed:

```bash
cmake -S . -B build-issue18-release -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build-issue18-release --parallel
ctest --test-dir build-issue18-release --output-on-failure
```
**Recorded result:**

```text
100% tests passed, 0 tests failed out of 6
Total Test time (real) = 0.02 sec
```


## 3. Automated regression tests — PASS

**Command executed:**

```bash
ctest --test-dir build-issue18 --output-on-failure
```

**Recorded result:**

```text
Start 1: RationalCoreTests
1/6 Test #1: RationalCoreTests ................. Passed  0.00 sec

Start 2: RationalViewportTests
2/6 Test #2: RationalViewportTests ............. Passed  0.00 sec

Start 3: RationalCurveSamplerTests
3/6 Test #3: RationalCurveSamplerTests ......... Passed  0.00 sec

Start 4: RationalCurveDomainTests
4/6 Test #4: RationalCurveDomainTests .......... Passed  0.01 sec

Start 5: RationalCurveDiscontinuityTests
5/6 Test #5: RationalCurveDiscontinuityTests ... Passed  0.02 sec

Start 6: RationalCurveSteepTests
6/6 Test #6: RationalCurveSteepTests ........... Passed  0.02 sec

100% tests passed, 0 tests failed out of 6
Total Test time (real) = 0.06 sec
```

### Test coverage by executable

| Test executable | Verified areas |
|---|---|
| `RationalCoreTests` | Fraction arithmetic and normalization, exact integer handling, scientific parsing/evaluation, operator precedence, functions, malformed inputs, and undefined results. |
| `RationalViewportTests` | Coordinate conversions, inverse transformations, viewport dimensions, pan and cursor-anchored zoom, invalid inputs, and state preservation. |
| `RationalCurveSamplerTests` | Adaptive screen-space subdivision, line/curve approximation, oscillatory probes, finite drawing geometry, and evaluation limits. |
| `RationalCurveDomainTests` | Open/closed function domains, valid regions separated by undefined intervals, exception/non-finite handling, and bounded sampling. |
| `RationalCurveDiscontinuityTests` | Reciprocal and tangent poles, finite step jumps, `floor(x)` boundaries, offset viewport cases, sampled-position sweeps, and continuous controls. |
| `RationalCurveSteepTests` | Steep continuous straight lines, hyperbolic tangent and inverse-tangent transitions, bounded terminal refinement, approximation consistency, sampling-position sweeps, and discontinuity controls. |

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

**Important:** The inserted Markdown contains nested code blocks. Copy the content beginning with `### Steep continuous-curve regression results` and ending with the final paragraph into your file as ordinary Markdown.


## 4. AddressSanitizer / UndefinedBehaviorSanitizer — PASS

**Configuration and build:**

```bash
cmake -S . -B build-issue18-asan -G Ninja \
    -DCMAKE_BUILD_TYPE=Debug \
    -DRATIONALSTUDIO_BUILD_GUI=OFF \
    -DCMAKE_CXX_FLAGS="-fsanitize=address,undefined -fno-omit-frame-pointer" \
    -DCMAKE_EXE_LINKER_FLAGS="-fsanitize=address,undefined"
cmake --build build-issue18-asan --parallel
```

**Final build output:**

```text
[19/19] Linking CXX executable RationalCoreTests
```

**Test command:**

```bash
ctest --test-dir build-issue18-asan --output-on-failure
```

**Recorded result:**

```text
Start 1: RationalCoreTests
1/6 Test #1: RationalCoreTests ................. Passed  0.03 sec

Start 2: RationalViewportTests
2/6 Test #2: RationalViewportTests ............. Passed  0.01 sec

Start 3: RationalCurveSamplerTests
3/6 Test #3: RationalCurveSamplerTests ......... Passed  0.03 sec

Start 4: RationalCurveDomainTests
4/6 Test #4: RationalCurveDomainTests .......... Passed  0.04 sec

Start 5: RationalCurveDiscontinuityTests
5/6 Test #5: RationalCurveDiscontinuityTests ... Passed  0.10 sec

Start 6: RationalCurveSteepTests
6/6 Test #6: RationalCurveSteepTests ........... Passed  0.07 sec

100% tests passed, 0 tests failed out of 6
Total Test time (real) = 0.28 sec
```

All registered tests passed in the sanitizer-enabled configuration, and no ASan or UBSan failures were reported in the captured output.

**Scope:** `RATIONALSTUDIO_BUILD_GUI=OFF` excludes `RationalStudio`. These results apply to the Qt-independent code exercised by the six test executables; they do **not** establish sanitizer coverage for Qt rendering or GUI interaction paths.

## 5. Desktop GUI — PASS (manual verification)

The application was launched using:

```bash
./build-issue18/RationalStudio
```

The developer reports successful completion of the prescribed Issue #18 graphical verification and regression checks. The inspected graphs displayed the expected behavior, with no functional failures reported.

The graphing cases exercised included:

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

**Scope:** Manual functional checks are not automated GUI tests, pixel-comparison tests, accessibility validation, or performance benchmarks. The passing GUI checks do not mean every graph is rendered mathematically perfectly.

## 6. Terminal application

The `RationalCLI` target built successfully in the current Debug and core-only sanitizer configurations. A previously recorded manual CLI smoke test produced:

```text
Input:     5/2 7/4
Operation: +
Result:    5/2 + 7/4 = 17/4
```

No separate interactive CLI execution was included in the latest Issue #18 terminal record. The CLI target compiled successfully in the Debug, Release, and sanitizer-enabled configurations; the historical smoke-test result is not represented as a new run.

## 7. Platform and configuration coverage

| Platform / configuration | Build | Tests | Runtime |
|---|---|---|---|
| Linux Debug, Qt GUI enabled | PASS | 6/6 PASS | GUI manual checks PASS |
| Linux Release, Qt GUI enabled | PASS | 6/6 PASS | GUI runtime not separately documented |
| Linux Debug, GUI disabled, ASan/UBSan | PASS | 6/6 PASS | Core test execution PASS |
| Native Windows | Not tested | Not tested | Not tested |
| Qt 5 | Not tested | Not tested | Not tested |

## 8. Outstanding limitations

The following areas remain outside the current verification scope or are known approximation limits:

- Finite probing cannot detect all discontinuities, arbitrarily narrow domain gaps, or unsampled singularities.
- Bounded terminal refinement improves steep continuous curves, but sufficiently narrow or extreme nonlinear features may still be omitted or approximated imperfectly.
- Highly oscillatory functions may be undersampled.
- Per-curve evaluation limits are not a shared frame-time performance guarantee.
- Curve geometry is recomputed during painting; there is no persistent drawing cache.
- Automated Qt GUI interaction and rendered-pixel correctness tests have not been established.
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

These commands describe a reproducible procedure. The current verification results were obtained using `build-issue18`, `build-issue18-release`, and `build-issue18-asan`, rather than the generic directory names shown above.

## 10. Verification summary

| Area | Result |
|---|---|
| CMake/Ninja Debug configuration | PASS |
| CMake/Ninja Release configuration | PASS |
| GCC 15.2.0 GUI/CLI/core compilation | PASS — Debug and Release |
| Six registered CTest suites — Debug | **6/6 PASS** |
| Six registered CTest suites — Release | **6/6 PASS** |
| Steep-curve regression assertions | **6,516 PASS** |
| Core-only AddressSanitizer / UndefinedBehaviorSanitizer suite | **6/6 PASS** |
| Manual desktop GUI regression testing | PASS (reported) |
| Prior CLI smoke test | PASS (previous baseline) |
| Automated Qt GUI testing | Not performed |
| Native Windows verification | Not performed |

**Assessment:** The Issue #18 implementation is supported by successful Linux Debug and Release builds using GCC 15.2.0, six passing CTest suites in both configurations, six passing core-only ASan/UBSan test suites, and reported successful manual GUI verification. Dedicated steep-curve regression tests confirm improved reconstruction of the tested high-gradient continuous functions while preserving previously verified discontinuity safeguards. Numerical approximation limitations, automated GUI testing, and native Windows verification remain outstanding.
