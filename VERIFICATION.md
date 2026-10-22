# Rational Studio — Verification Report

**Project version:** 0.1.0  
**Verification date:** October 22, 2026  
**Recorded environment:** Linux, GCC 15.2.0, C++20, CMake/Ninja, Qt Widgets  
**Current status:** **PASS** — Debug build, five automated test suites, core-only ASan/UBSan suite, and reported manual GUI checks.

This document records results for the implementation through the discontinuity-handling changes associated with GitHub Issue #17. It is a **verification record**, not a development roadmap. Results are scoped to the commands and platforms actually exercised; they do not establish exhaustive mathematical correctness.

## 1. Build environment

| Component | Recorded configuration |
|---|---|
| Operating system | Linux |
| Compiler | GCC 15.2.0 |
| Language standard | C++20 |
| Build system | CMake with Ninja generator |
| Standard build | Debug, GUI enabled |
| GUI framework | Qt Widgets |
| Exact arithmetic | Boost.Multiprecision |
| Test runner | CTest |
| Sanitizers | AddressSanitizer and UndefinedBehaviorSanitizer; core-only build |

Qt 6 is the preferred development target. The exact Qt version selected by the recorded CMake configuration was not displayed in the submitted console output. Native Windows compatibility and Qt 5 compatibility have not been independently verified.

## 2. Debug build — PASS

**Commands executed:**

```bash
cmake -S . -B build-issue17 -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build-issue17 --parallel
```

CMake successfully configured and generated the build. GCC compiled and linked the mathematical core, test executables, CLI, and Qt desktop application.

**Final build output:**

```text
[23/23] Linking CXX executable RationalStudio
```

No configuration errors, compiler errors, linker errors, or compiler warnings appeared in the supplied output. This confirms that the earlier narrowing-conversion and malformed-shell-command warnings were not reproduced in this build; it is not a warning-as-error test.

## 3. Automated regression tests — PASS

**Command executed:**

```bash
ctest --test-dir build-issue17 --output-on-failure
```

**Recorded result:**

```text
Start 1: RationalCoreTests
1/5 Test #1: RationalCoreTests ................. Passed  0.00 sec

Start 2: RationalViewportTests
2/5 Test #2: RationalViewportTests ............. Passed  0.00 sec

Start 3: RationalCurveSamplerTests
3/5 Test #3: RationalCurveSamplerTests ......... Passed  0.00 sec

Start 4: RationalCurveDomainTests
4/5 Test #4: RationalCurveDomainTests .......... Passed  0.01 sec

Start 5: RationalCurveDiscontinuityTests
5/5 Test #5: RationalCurveDiscontinuityTests ... Passed  0.02 sec

100% tests passed, 0 tests failed out of 5
Total Test time (real) = 0.04 sec
```

### Test coverage by executable

| Test executable | Verified areas |
|---|---|
| `RationalCoreTests` | Fraction arithmetic and normalization, exact integer handling, scientific parsing/evaluation, operator precedence, functions, malformed inputs, and undefined results. |
| `RationalViewportTests` | Coordinate conversions, inverse transformations, viewport dimensions, pan and cursor-anchored zoom, invalid inputs, and state preservation. |
| `RationalCurveSamplerTests` | Adaptive screen-space subdivision, line/curve approximation, oscillatory probes, finite drawing geometry, and evaluation limits. |
| `RationalCurveDomainTests` | Open/closed function domains, valid regions separated by undefined intervals, exception/non-finite handling, and bounded sampling. |
| `RationalCurveDiscontinuityTests` | Reciprocal and tangent poles, finite step jumps, `floor(x)` boundaries, offset viewport cases, sampled-position sweeps, and continuous controls. |

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

## 4. AddressSanitizer / UndefinedBehaviorSanitizer — PASS

**Configuration and build:**

```bash
cmake -S . -B build-issue17-asan -G Ninja \
    -DCMAKE_BUILD_TYPE=Debug \
    -DRATIONALSTUDIO_BUILD_GUI=OFF \
    -DCMAKE_CXX_FLAGS="-fsanitize=address,undefined -fno-omit-frame-pointer" \
    -DCMAKE_EXE_LINKER_FLAGS="-fsanitize=address,undefined"
cmake --build build-issue17-asan --parallel
```

**Final build output:**

```text
[17/17] Linking CXX executable RationalCoreTests
```

**Test command:**

```bash
ctest --test-dir build-issue17-asan --output-on-failure
```

**Recorded result:**

```text
Start 1: RationalCoreTests
1/5 Test #1: RationalCoreTests ................. Passed  0.03 sec

Start 2: RationalViewportTests
2/5 Test #2: RationalViewportTests ............. Passed  0.01 sec

Start 3: RationalCurveSamplerTests
3/5 Test #3: RationalCurveSamplerTests ......... Passed  0.02 sec

Start 4: RationalCurveDomainTests
4/5 Test #4: RationalCurveDomainTests .......... Passed  0.04 sec

Start 5: RationalCurveDiscontinuityTests
5/5 Test #5: RationalCurveDiscontinuityTests ... Passed  0.09 sec

100% tests passed, 0 tests failed out of 5
Total Test time (real) = 0.19 sec
```

All registered tests passed in the sanitizer-enabled configuration, and no ASan or UBSan failures were reported in the captured output.

**Scope:** `RATIONALSTUDIO_BUILD_GUI=OFF` excludes `RationalStudio`. These results apply to the Qt-independent code exercised by the five tests; they do **not** establish sanitizer coverage for Qt rendering or GUI interaction paths.

## 5. Desktop GUI — PASS (manual verification)

The application was launched using:

```bash
./build-issue17/RationalStudio
```

The developer reports completing the prescribed manual GUI checks successfully.

The graphing cases exercised included:

| Expression / behavior | Verification goal |
|---|---|
| `y = 1/x` | Separate reciprocal branches near zero. |
| `y = 1/(x-0.133)` and `y = 1/(x+0.327)` | Avoid false joins across shifted poles. |
| `y = tan(x)` | Inspect branch separation near tangent poles (radian mode). |
| `y = floor(x)` and `y = sign(x)` | Inspect finite jump behavior. |
| `y = 500*x` | Preserve a steep continuous straight line. |
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

No separate interactive CLI execution was included in the latest Issue #17 terminal record; the historical smoke-test result is not represented as a new run.

## 7. Platform and configuration coverage

| Platform / configuration | Build | Tests | Runtime |
|---|---|---|---|
| Linux Debug, Qt GUI enabled | PASS | 5/5 PASS | GUI manual checks PASS |
| Linux Debug, GUI disabled, ASan/UBSan | PASS | 5/5 PASS | Core test execution PASS |
| Linux Release, current Issue #17 code | Not documented in user's terminal record | Not documented | Not documented |
| Native Windows | Not tested | Not tested | Not tested |
| Qt 5 | Not tested | Not tested | Not tested |

## 8. Outstanding limitations

The following areas remain outside the current verification scope or are known approximation limits:

- Finite probing cannot detect all discontinuities, arbitrarily narrow domain gaps, or unsampled singularities.
- Strongly curved continuous segments may still be conservatively omitted near the sampling limit.
- Highly oscillatory functions may be undersampled.
- Per-curve evaluation limits are not a shared frame-time performance guarantee.
- Curve geometry is recomputed during painting; there is no persistent drawing cache.
- Automated Qt GUI interaction and rendered-pixel correctness tests have not been established.
- The complete Qt application has not undergone recorded sanitizer testing.
- Current Issue #17 Release and native Windows results have not been provided.

## 9. Reproduction

On Linux with the documented dependencies installed, use fresh build directories:

```bash
# Full Debug build and tests
cmake -S . -B build-debug -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build-debug --parallel
ctest --test-dir build-debug --output-on-failure
./build-debug/RationalStudio

# Core-only ASan/UBSan build and tests
cmake -S . -B build-asan -G Ninja \
    -DCMAKE_BUILD_TYPE=Debug \
    -DRATIONALSTUDIO_BUILD_GUI=OFF \
    -DCMAKE_CXX_FLAGS="-fsanitize=address,undefined -fno-omit-frame-pointer" \
    -DCMAKE_EXE_LINKER_FLAGS="-fsanitize=address,undefined"
cmake --build build-asan --parallel
ctest --test-dir build-asan --output-on-failure
```

These commands describe a reproducible procedure. The recorded results above came from `build-issue17` and `build-issue17-asan`, not from the generic directories used here.

## 10. Verification summary

| Area | Result |
|---|---|
| CMake/Ninja Debug configuration | PASS |
| GCC 15.2.0 GUI/CLI/core compilation | PASS |
| Five registered CTest suites | **5/5 PASS** |
| Discontinuity assertions | **8,663 PASS** |
| Core-only AddressSanitizer / UndefinedBehaviorSanitizer suite | **5/5 PASS** |
| Manual desktop GUI regression testing | PASS (reported) |
| Prior CLI smoke test | PASS (previous baseline) |
| Automated Qt GUI testing | Not performed |
| Current Release verification | Not recorded |
| Native Windows verification | Not performed |

**Assessment:** The Issue #17 implementation is supported by a successful Linux Debug build, five passing CTest suites, a passing core-only ASan/UBSan suite, and reported successful manual GUI checks. The remaining numerical, performance, GUI-automation, and cross-platform limitations are explicitly documented above.
