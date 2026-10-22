# Rational Studio — Verification Report

**Project Version:** 0.1.0  
**Verification Date:** October 22, 2026  
**Platform:** Linux  
**Overall Status:** Initial application build and functional smoke tests passed.

This document records the build and test results for the initial Rational Studio application.

It distinguishes automated test coverage from manually verified functionality and identifies areas that have not yet undergone independent verification.

---

## 1. Build Environment

The application was built and tested on a native Linux development workstation.

| Component | Configuration |
|---|---|
| Operating System | Linux |
| Compiler | GCC 15.2.0 |
| Language Standard | C++20 |
| Build System | CMake |
| GUI Framework | Qt Widgets |
| Exact Integer Arithmetic | Boost.Multiprecision |
| Test Framework | CTest |

The project uses Qt 6 for its primary development target.

The exact Qt library version selected by CMake was not recorded in the captured build output.

Windows compilation and runtime compatibility have not yet been independently verified.

---

## 2. Build Verification

**Result: PASS**

The initial application was successfully configured, compiled, and linked.

The following targets were built:

| Target | Type | Result |
|---|---|---|
| `RationalCore` | Static library | PASS |
| `RationalCLI` | Executable | PASS |
| `RationalCoreTests` | Test executable | PASS |
| `RationalViewportTests` | Test executable | PASS |
| `RationalCurveSamplerTests` | Test executable | PASS |
| `RationalStudio` | Qt desktop executable | PASS |

Qt's automatic Meta-Object Compiler generation completed successfully.

No compiler errors or linker errors were reported.

### Build Diagnostics

The initial build produced two narrowing-conversion warnings in `GraphCanvas.cpp`.

Both originated from implicit conversions between integer widget dimensions and Qt floating-point coordinates.

The affected statements were updated to use explicit `qreal` conversions.

The initial CMake invocation also produced an extra-path warning caused by shell argument escaping.

The configuration command was corrected.

**Verification note:** These corrections have been applied, but a clean post-correction build has not yet been included in the recorded terminal output.

No compiler diagnostics were disabled to address these warnings.

---

## 3. Automated Test Verification

**Result: PASS**

CTest completed successfully.

### Recorded Output

```text
Test project: RationalStudio/build

    Start 1: RationalCoreTests

1/1 Test #1: RationalCoreTests ........ Passed

100% tests passed, 0 tests failed out of 1
```

The mathematical test executable contains 91 assertions.

### Test Coverage

The existing test suite exercises:

**Rational Arithmetic**
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

**Scientific Expressions**
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

**Error Handling**
- Invalid fractions.
- Malformed mathematical expressions.
- Division by zero.
- Undefined real-number results.
- Invalid mathematical domains.

### Automated Test Limitations

The current test suite focuses on the Qt-independent mathematical core.

Dedicated automated tests have not yet been implemented for:

- Qt user-interface interactions.
- Graph-rendering correctness.
- Coordinate transformations.
- Viewport navigation.
- Discontinuity handling.
- Graph workspace serialization.
- Image export.
- Rendering performance.

Passing the existing mathematical tests does not establish correctness for these additional components.

---

## 4. Desktop Application Verification

**Result: PASS — Manual Smoke Test**

The Qt desktop application compiled and launched successfully on Linux.

Manual interaction confirmed that the application and its principal calculator functionality were operational.

The application provides three primary workspaces:

1. Scientific Calculator.
2. Graphing Calculator.
3. Exact Fractions.

The initial GUI layout and basic application interactions were manually exercised.

### Scope

This verification establishes that the initial graphical application launches and operates in its development environment.

It does not constitute exhaustive functional, accessibility, rendering, or performance testing.

No automated Qt GUI tests have been recorded for this version.

---

## 5. Terminal Application Verification

**Result: PASS**

The standalone terminal calculator compiled and executed successfully.

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

The terminal application uses the same mathematical core as the Qt desktop application.

---

## 6. Sanitizer Verification

No AddressSanitizer or UndefinedBehaviorSanitizer run using the current GCC 15.2.0 development environment is documented in the captured terminal output.

The Qt application has not yet undergone documented sanitizer verification.

Sanitizer coverage should be evaluated separately from standard compilation and functional testing.

---

## 7. Platform Verification

| Platform | Compilation | Runtime | Status |
|---|---|---|---|
| Linux | PASS | PASS | Initial baseline verified |
| Windows | Not tested | Not tested | Unverified |

The implementation uses C++20, CMake, and Qt APIs intended to support cross-platform development.

Windows support has not yet been demonstrated through a native build or runtime test.

---

## 8. Reproduction Instructions

The following commands can be used to reproduce a Release build using Ninja on a Linux system with the required dependencies installed.

### Configure

```bash
cmake -S . -B build-release -G Ninja -DCMAKE_BUILD_TYPE=Release
```

### Compile

```bash
cmake --build build-release --parallel
```

### Run Automated Tests

```bash
ctest --test-dir build-release --output-on-failure
```

### Launch the Desktop Application

```bash
./build-release/RationalStudio
```

### Launch the Terminal Application

```bash
./build-release/RationalCLI
```

These commands provide a reproducible verification procedure but do not imply that every configuration has been independently executed.

---

## 9. Verification Summary

| Verification Area | Result |
|---|---|
| CMake configuration | PASS |
| C++ compilation | PASS |
| Static library linkage | PASS |
| Desktop executable linkage | PASS |
| CLI executable linkage | PASS |
| Automated mathematical tests | PASS |
| Desktop application smoke test | PASS |
| Terminal calculator smoke test | PASS |
| Post-warning-fix clean rebuild | Pending recorded verification |
| Automated GUI tests | Not performed |
| Dedicated graph correctness tests | Not performed |
| GCC 15.2.0 sanitizer tests | Not performed |
| Windows build verification | Not performed |

### Overall Assessment

Rational Studio v0.1.0 has a functional Linux application baseline.

The numerical library, desktop application, and terminal calculator compiled successfully. The mathematical regression suite passed, and both application executables were manually exercised.

The initial build provides a reproducible foundation for further development.

This verification record does not imply complete mathematical correctness, exhaustive GUI testing, or confirmed compatibility with untested platforms.
