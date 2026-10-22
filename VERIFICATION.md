# Rational Studio — Verification Report

**Project Version:** 0.1.0  
**Verification Date:** October 22, 2026  
**Platform:** Linux  
**Compiler:** GCC 15.2.0  
**Language Standard:** C++20  
**Overall Status:** PASS — Verified Linux build, automated regression tests, and manual application testing.

This document records the build, automated test, sanitizer, and manual verification results for the current Rational Studio implementation.

The purpose of this report is to establish a reproducible verification baseline and distinguish demonstrated functionality from areas that have not yet undergone independent testing.

Passing the recorded tests does not imply exhaustive mathematical correctness or compatibility with untested platforms.

---

## 1. Build Environment

The application was compiled and tested on a native Linux development workstation.

| Component | Configuration |
|---|---|
| Operating System | Linux |
| C++ Compiler | GCC 15.2.0 |
| Language Standard | C++20 |
| Build System | CMake |
| Build Generator | Ninja |
| GUI Framework | Qt Widgets |
| Exact Integer Arithmetic | Boost.Multiprecision |
| Test Framework | CTest |
| Primary Build Configuration | Debug |

Qt 6 is the project's preferred development target.

The exact Qt library version selected during configuration was not recorded in the supplied CMake output.

Windows compilation and runtime compatibility have not yet been independently verified.

---

## 2. Build Verification

**Result: PASS**

The application was successfully configured and compiled using GCC 15.2.0.

### Configuration

```bash
cmake -S . -B build-issue16 -G Ninja -DCMAKE_BUILD_TYPE=Debug
```

CMake completed compiler detection, dependency discovery, configuration, and generation without reporting errors.

### Compilation

```bash
cmake --build build-issue16 --parallel
```

### Recorded Result

```text
[21/21] Linking CXX executable RationalStudio
```

The build completed successfully.

### Build Targets

| Target | Type | Result |
|---|---|---|
| `RationalCore` | Static library | PASS |
| `RationalCLI` | Executable | PASS |
| `RationalStudio` | Qt desktop executable | PASS |
| `RationalCoreTests` | Test executable | PASS |
| `RationalViewportTests` | Test executable | PASS |
| `RationalCurveSamplerTests` | Test executable | PASS |
| `RationalCurveDomainTests` | Test executable | PASS |

No compiler errors, linker errors, or compiler warnings were reported in the supplied build output.

### Compiler Diagnostics

Earlier development builds identified narrowing-conversion warnings involving integer widget dimensions and Qt floating-point coordinates.

The affected code was updated to use explicit conversions.

An earlier CMake invocation warning caused by incorrect shell argument escaping was also corrected.

The current recorded Debug configuration and build completed without those warnings.

No compiler diagnostic suppressions were introduced to resolve them.

---

## 3. Automated Test Verification

**Result: PASS**

The complete registered CTest suite passed.

### Test Command

```bash
ctest --test-dir build-issue16 --output-on-failure
```

### Recorded Output

```text
Test project /home/chris/Documents/CPP_Projects/RationalStudio/build-issue16

    Start 1: RationalCoreTests
1/4 Test #1: RationalCoreTests ................ Passed  0.00 sec

    Start 2: RationalViewportTests
2/4 Test #2: RationalViewportTests ............ Passed  0.00 sec

    Start 3: RationalCurveSamplerTests
3/4 Test #3: RationalCurveSamplerTests ........ Passed  0.00 sec

    Start 4: RationalCurveDomainTests
4/4 Test #4: RationalCurveDomainTests ......... Passed  0.01 sec

100% tests passed, 0 tests failed out of 4

Total Test time (real) = 0.02 sec
```

### Test Suite Summary

| Test Executable | Coverage | Result |
|---|---|---|
| `RationalCoreTests` | Rational arithmetic and scientific expression evaluation | PASS |
| `RationalViewportTests` | Coordinate transformations and viewport navigation | PASS |
| `RationalCurveSamplerTests` | Adaptive curve sampling and approximation | PASS |
| `RationalCurveDomainTests` | Mathematical domain validation and drawing-path separation | PASS |

All four test executables completed successfully.

---

## 4. Mathematical Core Verification

**Result: PASS**

The mathematical core regression tests validate rational arithmetic and scientific expression evaluation.

### Rational Arithmetic

Test coverage includes:

- Addition.
- Subtraction.
- Multiplication.
- Division.
- Fraction normalization.
- Signed rational values.
- Zero-denominator validation.
- Arbitrary-precision integer arithmetic.
- Rational comparisons.
- Decimal conversion and rounding.

### Scientific Expressions

Test coverage includes:

- Mathematical expression parsing.
- Operator precedence.
- Parentheses.
- Exponentiation.
- Factorials.
- Implicit multiplication.
- Trigonometric functions.
- Logarithms.
- Mathematical constants.
- Multi-argument functions.
- Variable evaluation.
- Previous-answer references.

### Error Handling

Test coverage includes:

- Invalid rational-number input.
- Malformed mathematical expressions.
- Division by zero.
- Undefined mathematical results.
- Invalid mathematical domains.

The mathematical core can be compiled and tested independently of Qt.

---

## 5. Viewport Verification

**Result: PASS**

The graphing viewport uses a Qt-independent implementation for coordinate transformations and navigation mathematics.

Automated regression tests cover:

- World-to-screen coordinate conversion.
- Screen-to-world coordinate conversion.
- Coordinate round trips.
- Viewport dimension changes.
- Horizontal panning.
- Vertical panning.
- Cursor-anchored zooming.
- Scale validation.
- Invalid-input rejection.
- Preservation of viewport state after rejected operations.
- Selected extreme numerical inputs.

The dedicated viewport test executable completed successfully.

The viewport implementation is integrated into `GraphCanvas`, which delegates mathematical coordinate transformations and navigation operations to the independent component.

---

## 6. Adaptive Curve Sampling Verification

**Result: PASS**

The graphing engine uses adaptive screen-space curve sampling rather than relying exclusively on fixed horizontal sampling intervals.

Automated regression tests cover:

- Straight-line approximation.
- Nonlinear mathematical functions.
- Recursive subdivision.
- Midpoint and quarter-point probing.
- Screen-space approximation checks.
- Oscillatory functions.
- Finite drawing-coordinate validation.
- Evaluation safety limits.
- Invalid sampling configurations.

The sampler generates screen-space drawing geometry independently of Qt.

### Verification Scope

The tests establish expected behavior for the selected mathematical functions and sampling configurations.

They do not prove that arbitrary mathematical functions will be reconstructed perfectly.

Highly oscillatory functions and narrow mathematical features may require additional sampling or more sophisticated analysis.

---

## 7. Undefined Function Domain Verification

**Result: PASS**

The adaptive curve sampler performs interior validity checks before accepting sampled curve segments.

The implementation treats non-finite mathematical evaluations and exceptions as invalid samples.

Detected undefined regions cause drawing paths to be disconnected rather than joined across known invalid sample locations.

### Automated Test Coverage

The dedicated domain regression suite covers:

- Closed domain boundaries.
- Open domain boundaries.
- Functions with bounded valid regions.
- Undefined intervals separating valid branches.
- Narrow undefined regions.
- Exception-producing mathematical evaluations.
- NaN and infinity handling.
- Extreme transformed coordinates.
- Evaluation safety limits.

Representative mathematical cases include:

```text
sqrt(x)
ln(x)
1/sqrt(x)
acos(x)
sqrt((x+0.537)*(0.423-x))
sqrt((x-0.131)*(x-0.147))
```

### Regression Verification

A targeted regression test covers a narrow undefined interval embedded within an otherwise valid curve.

The previous sampler could connect drawing geometry across this interval.

The updated implementation detects the invalid region in the specified regression case and preserves the separation between valid curve branches.

### Direct Test Execution

```bash
./build-issue16/RationalCurveDomainTests
```

### Recorded Result

```text
PASS: 2383 CurveDomain assertions
```

The regression suite completed successfully.

### Limitations

Domain validation relies on sampled mathematical evaluations.

The implementation cannot guarantee detection of every arbitrarily narrow undefined region or every discontinuity.

Finite results on opposite sides of an asymptote may require additional analysis to determine whether connecting them is mathematically appropriate.

These limitations do not invalidate the verified regression cases but remain relevant to graphing accuracy.

---

## 8. AddressSanitizer and UndefinedBehaviorSanitizer

**Result: PASS — Qt-Independent Components**

The mathematical core and associated test executables were compiled and tested using AddressSanitizer and UndefinedBehaviorSanitizer.

### Configuration

```bash
cmake -S . -B build-issue16-asan \
    -G Ninja \
    -DCMAKE_BUILD_TYPE=Debug \
    -DRATIONALSTUDIO_BUILD_GUI=OFF \
    -DCMAKE_CXX_FLAGS="-fsanitize=address,undefined -fno-omit-frame-pointer" \
    -DCMAKE_EXE_LINKER_FLAGS="-fsanitize=address,undefined"
```

### Compilation

```bash
cmake --build build-issue16-asan --parallel
```

### Recorded Result

```text
[15/15] Linking CXX executable RationalCLI
```

The sanitizer-enabled build completed successfully.

### Test Execution

```bash
ctest --test-dir build-issue16-asan --output-on-failure
```

### Recorded Output

```text
Test project /home/chris/Documents/CPP_Projects/RationalStudio/build-issue16-asan

    Start 1: RationalCoreTests
1/4 Test #1: RationalCoreTests ................ Passed  0.04 sec

    Start 2: RationalViewportTests
2/4 Test #2: RationalViewportTests ............ Passed  0.01 sec

    Start 3: RationalCurveSamplerTests
3/4 Test #3: RationalCurveSamplerTests ........ Passed  0.02 sec

    Start 4: RationalCurveDomainTests
4/4 Test #4: RationalCurveDomainTests ......... Passed  0.04 sec

100% tests passed, 0 tests failed out of 4

Total Test time (real) = 0.11 sec
```

All four test executables passed under the sanitizer-enabled configuration.

No sanitizer failures were reported in the supplied output.

### Sanitizer Scope

The sanitizer configuration explicitly disables the Qt GUI using:

```text
RATIONALSTUDIO_BUILD_GUI=OFF
```

Consequently, these results validate the Qt-independent components exercised by the test suite.

They do not establish sanitizer coverage for the Qt desktop application or its rendering operations.

---

## 9. Desktop Application Verification

**Result: PASS — Manual Functional Verification**

The Qt desktop application compiled and launched successfully on Linux.

Manual testing was reported successful for the prescribed graphical verification procedures.

### Application Workspaces

The application provides three primary workspaces:

1. Scientific Calculator.
2. Graphing Calculator.
3. Exact Fractions.

### Graphing Verification

Manual graphing checks included the prescribed representative functions with restricted mathematical domains, alongside regression checks of existing graphing interactions.

Representative expressions included:

```text
y = sqrt(x)
y = ln(x)
y = 1/sqrt(x)
y = acos(x)
y = sqrt((x+0.537)*(0.423-x))
y = sqrt((x-0.131)*(x-0.147))
y = sqrt(-1)
y = sin(x)
```

The manual verification was reported successful.

### Interaction Regression Testing

The prescribed manual verification also covered existing application interactions, including:

- Mouse panning.
- Scroll-wheel zooming.
- Window resizing.
- Coordinate inspection.
- Multiple simultaneous graph expressions.
- Curve visibility controls.
- Graph workspace saving and loading.
- PNG image export.

No functional failures were reported during these checks.

### Scope and Limitations

These results represent manual functional verification performed in the development environment.

They do not constitute automated Qt GUI testing, exhaustive rendering validation, accessibility verification, or comprehensive performance benchmarking.

---

## 10. Terminal Application Verification

**Result: PASS — Previously Verified Baseline**

The standalone terminal calculator was previously compiled and manually verified.

### Input

```text
5/2 7/4
+
```

### Expected Output

```text
5/2 + 7/4 = 17/4
```

### Observed Output

```text
5/2 + 7/4 = 17/4
```

The observed result matched the expected result.

The CLI continues to use the shared `RationalCore` mathematical library.

The current test run confirms that the CLI target builds successfully. The supplied verification session did not include a separate new interactive CLI test.

---

## 11. Platform Verification

| Platform | Compilation | Runtime | Status |
|---|---|---|---|
| Linux | PASS | PASS | Verified |
| Windows | Not tested | Not tested | Unverified |

The application uses C++20, CMake, and Qt APIs intended to support cross-platform development.

Windows support has not yet been demonstrated through native compilation, packaging, or runtime testing.

The current verified implementation targets the Linux development environment.

---

## 12. Remaining Verification Limitations

The following areas have not yet undergone comprehensive verification:

- Automated Qt GUI interaction testing.
- Direct validation of rendered image output.
- Exhaustive mathematical discontinuity detection.
- Classification of vertical asymptotes.
- Preservation of every steep continuous curve.
- Highly oscillatory and sub-pixel mathematical features.
- Rendering performance with large numbers of active curves.
- Persistent curve-geometry caching.
- Automated Linux and Windows CI builds.
- Native Windows compilation and runtime behavior.
- Sanitizer testing of the complete Qt GUI.
- Release-build verification of the current graphing changes.

These are limitations of the current verification scope rather than evidence of known failures in every listed area.

---

## 13. Reproduction Instructions

The following commands reproduce the recorded Debug and sanitizer-enabled configurations on a compatible Linux development system.

### Debug Configuration

```bash
cmake -S . -B build-debug -G Ninja -DCMAKE_BUILD_TYPE=Debug
```

### Compile

```bash
cmake --build build-debug --parallel
```

### Run Automated Tests

```bash
ctest --test-dir build-debug --output-on-failure
```

### Launch the Desktop Application

```bash
./build-debug/RationalStudio
```

### Sanitizer Configuration

```bash
cmake -S . -B build-asan \
    -G Ninja \
    -DCMAKE_BUILD_TYPE=Debug \
    -DRATIONALSTUDIO_BUILD_GUI=OFF \
    -DCMAKE_CXX_FLAGS="-fsanitize=address,undefined -fno-omit-frame-pointer" \
    -DCMAKE_EXE_LINKER_FLAGS="-fsanitize=address,undefined"
```

### Compile and Test

```bash
cmake --build build-asan --parallel

ctest --test-dir build-asan --output-on-failure
```

The commands above use generic build-directory names.

The recorded verification results in this report were obtained using the development directories `build-issue16` and `build-issue16-asan`.

---

## 14. Verification Summary

| Verification Area | Result |
|---|---|
| CMake Debug configuration | PASS |
| GCC 15.2.0 compilation | PASS |
| Static library linkage | PASS |
| Qt desktop executable linkage | PASS |
| CLI executable linkage | PASS |
| Mathematical core tests | PASS |
| Viewport regression tests | PASS |
| Adaptive curve sampling tests | PASS |
| Undefined-domain regression tests | PASS |
| Complete Debug CTest suite | 4/4 PASS |
| AddressSanitizer test suite | 4/4 PASS |
| UndefinedBehaviorSanitizer test suite | 4/4 PASS |
| Manual Qt GUI verification | PASS |
| Previous CLI functional verification | PASS |
| Automated GUI tests | Not performed |
| Current graphing Release build | Not verified |
| Windows build verification | Not performed |
| Windows runtime verification | Not performed |

### Overall Assessment

Rational Studio v0.1.0 has a functional and verified Linux development baseline.

The current implementation successfully compiled using GCC 15.2.0. All four registered CTest executables passed under both the standard Debug and sanitizer-enabled configurations.

Manual verification of the Qt graphical application was also reported successful.

The graphing engine now includes independently testable viewport mathematics, adaptive screen-space curve sampling, and improved validation of sampled undefined mathematical regions.

The recorded results establish a verified foundation for continued development.

They do not imply exhaustive mathematical correctness, complete GUI test coverage, or compatibility with platforms that have not yet been tested.
