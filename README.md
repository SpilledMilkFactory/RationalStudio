# Rational Studio

**Rational Studio** is an open-source, cross-platform scientific and graphing calculator developed in **C++20** using **Qt Widgets**.

The application combines scientific expression evaluation, interactive Cartesian graphing, and arbitrary-precision rational arithmetic within a unified desktop interface.

Rational Studio is designed with an emphasis on mathematical correctness, modular architecture, reusable C++ components, and responsive user interaction.

**Current Version:** 0.1.0  
**Development Platform:** Linux  
**Target Platforms:** Linux and Windows  
**License:** MIT

> **Development Status:** Rational Studio is under active development. The initial application is functional and has been built and tested on Linux. Windows compatibility is a development target and has not yet been independently verified.

---

## Features

### Scientific Calculator

The scientific calculator provides an expression-based interface for evaluating mathematical operations.

Current capabilities include:

- Standard arithmetic with operator precedence.
- Parenthesized mathematical expressions.
- Exponentiation and roots.
- Integer factorials.
- Trigonometric and inverse trigonometric functions.
- Hyperbolic functions.
- Common, natural, and binary logarithms.
- Mathematical constants, including π, e, and τ.
- Degree and radian calculation modes.
- Limited implicit multiplication.
- Live expression evaluation.
- Calculation history.
- Previous-result referencing through `ans`.
- Keyboard and on-screen keypad input.

Example expressions:

```text
2 + 3 * 4
(2 + 3)^2
sin(pi / 2)
sqrt(144)
log(1000)
3!
2sin(pi / 4)
```

Scientific calculations currently use IEEE-754 double-precision floating-point arithmetic.

### Graphing Calculator

The graphing workspace provides interactive visualization of explicit Cartesian functions.

Current capabilities include:

- Simultaneous rendering of multiple functions.
- Editable mathematical expressions.
- Individual curve visibility controls.
- Color-coded function rendering.
- Interactive mouse-driven panning.
- Scroll-wheel zooming.
- Automatically adjusted grid spacing.
- Cartesian axes and coordinate labels.
- Coordinate inspection near plotted curves.
- Graph workspace persistence using JSON.
- Graph image export to PNG.
- Degree and radian support for trigonometric expressions.

Example graph expressions:

```text
y = x^2
y = sin(x)
y = 2x + 3
y = 1/x
y = sqrt(abs(x))
y = sin(x)^2 + cos(x)^2
```

Graph expressions are parsed into an abstract syntax tree before evaluation, avoiding repeated parsing during curve rendering.

The current renderer uses fixed-interval sampling with heuristic discontinuity detection.

### Exact Rational Calculator

The rational calculator provides exact arithmetic using arbitrary-precision integers.

Supported functionality includes:

- Rational addition, subtraction, multiplication, and division.
- Automatic fraction reduction.
- Signed numerator and denominator handling.
- Zero-denominator validation.
- Arbitrary-precision integer arithmetic.
- Rational comparisons.
- Rounded decimal representation.

Example:

```text
5/2 + 7/4 = 17/4
```

Fractions are normalized using the Euclidean greatest common divisor algorithm.

The implementation uses `boost::multiprecision::cpp_int` to avoid the integer overflow limitations of primitive integer types, subject to available memory and computational resources.

---

## Technology Stack

| Component | Technology |
|---|---|
| Programming Language | C++20 |
| Desktop GUI | Qt Widgets |
| Build System | CMake |
| Exact Integer Arithmetic | Boost.Multiprecision |
| Expression Evaluation | Custom recursive-descent parser |
| Graph Rendering | Qt QPainter |
| Automated Testing | CTest |
| Primary Development Environment | Linux |

The mathematical core is independent of Qt, allowing numerical functionality to be compiled, tested, and reused without the graphical interface.

---

## Project Structure

```text
RationalStudio/
├── CMakeLists.txt
├── README.md
├── VERIFICATION.md
├── LICENSE
├── .gitignore
│
├── src/
│   ├── main.cpp
│   │
│   ├── core/
│   │   ├── Rational.hpp
│   │   ├── Rational.cpp
│   │   ├── Expression.hpp
│   │   └── Expression.cpp
│   │
│   ├── gui/
│   │   ├── MainWindow.hpp
│   │   ├── MainWindow.cpp
│   │   ├── GraphCanvas.hpp
│   │   └── GraphCanvas.cpp
│   │
│   └── cli/
│       └── main.cpp
│
└── tests/
    └── CoreTests.cpp
```

### Mathematical Core

The `RationalCore` static library provides the application's numerical functionality independently of Qt.

**Rational**

Implements exact rational-number representation and arithmetic using arbitrary-precision integer numerators and denominators.

Responsibilities include:

- Fraction normalization.
- Arithmetic operations.
- Rational comparisons.
- Input validation.
- String parsing and decimal conversion.

**Expression**

Implements scientific expression parsing and evaluation.

Responsibilities include:

- Mathematical expression tokenization.
- Recursive-descent parsing.
- Abstract syntax tree construction.
- Operator precedence and associativity.
- Mathematical function evaluation.
- Variable evaluation for graphing.
- Error handling for malformed expressions.

This separation allows mathematical operations to be tested without initializing the Qt application.

### Graph Rendering

The `GraphCanvas` widget handles the graphical representation of mathematical functions.

Its responsibilities include:

- Coordinate transformations.
- Cartesian grid rendering.
- Axis and coordinate labels.
- Curve sampling and drawing.
- Mouse-based viewport navigation.
- Function tracing.

Rendering is implemented using Qt's `QPainter` API.

### Application Interface

The `MainWindow` class manages three primary workspaces:

1. Scientific Calculator.
2. Graphing Calculator.
3. Exact Fractions.

The interface is constructed programmatically using Qt Widgets rather than Qt Designer-generated `.ui` files.

### Command-Line Application

`RationalCLI` provides a standalone terminal interface for exact rational arithmetic.

It uses the same `RationalCore` library as the desktop application.

The terminal executable demonstrates that the numerical implementation can operate independently of the graphical interface.

---

## Requirements

### Development Dependencies

- C++20-compatible compiler.
- CMake 3.16 or newer.
- Qt Widgets development libraries.
- Boost headers providing Boost.Multiprecision.
- Ninja or another supported native build tool.

Qt 6 is the preferred development target.

The CMake configuration also supports Qt 5 discovery, although Qt 5 compatibility has not yet been independently verified.

### Ubuntu / Debian-Based Linux

Install the required development packages:

```bash
sudo apt update

sudo apt install \
    build-essential \
    cmake \
    ninja-build \
    qt6-base-dev \
    libboost-dev
```

Depending on the Qt installation, an explicit `CMAKE_PREFIX_PATH` may be required during configuration.

---

## Building

### Clone the Repository

Clone the repository using GitHub's **Code → Clone** option, then navigate to the project directory.

### Configure a Release Build

From the project root:

```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
```

### Compile

```bash
cmake --build build --parallel
```

### Run the Desktop Application

```bash
./build/RationalStudio
```

### Run the Terminal Calculator

```bash
./build/RationalCLI
```

### Configure a Debug Build

For development and debugging:

```bash
cmake -S . -B build-debug -G Ninja -DCMAKE_BUILD_TYPE=Debug
```

Compile:

```bash
cmake --build build-debug --parallel
```

Run:

```bash
./build-debug/RationalStudio
```

Separate build directories prevent Debug and Release artifacts from being mixed.

---

## Build Configuration

The following CMake options are available:

| Option | Default | Description |
|---|---|---|
| `RATIONALSTUDIO_BUILD_GUI` | `ON` | Build the Qt desktop application |
| `RATIONALSTUDIO_BUILD_CLI` | `ON` | Build the terminal calculator |
| `BUILD_TESTING` | `ON` | Build and register automated tests |

### Building Without Qt

The mathematical core and terminal application can be compiled independently of Qt.

```bash
cmake -S . -B build-core \
    -G Ninja \
    -DCMAKE_BUILD_TYPE=Release \
    -DRATIONALSTUDIO_BUILD_GUI=OFF
```

Compile:

```bash
cmake --build build-core --parallel
```

Run tests:

```bash
ctest --test-dir build-core --output-on-failure
```

---

## Testing

Rational Studio uses CTest for automated regression testing.

Run the test suite:

```bash
ctest --test-dir build --output-on-failure
```

The initial implementation includes **91 mathematical assertions** within the `RationalCoreTests` executable.

Current automated tests cover:

- Rational arithmetic.
- Fraction normalization.
- Arbitrary-precision integer handling.
- Decimal conversion and rounding.
- Mathematical expression parsing.
- Operator precedence.
- Exponentiation and factorials.
- Trigonometric calculations.
- Mathematical constants and functions.
- Variable evaluation.
- Invalid-expression rejection.
- Undefined numerical results.

The current automated suite focuses on the Qt-independent mathematical core.

Dedicated graph-rendering tests and automated GUI interaction tests are not yet included.

See `VERIFICATION.md` for documented build and test results.

---

## Mathematical Expressions

### Operators

| Operator | Description |
|---|---|
| `+` | Addition |
| `-` | Subtraction |
| `*` | Multiplication |
| `/` | Division |
| `%` | Floating-point remainder |
| `^` | Exponentiation |
| `!` | Factorial |

### Constants and Variables

| Identifier | Description |
|---|---|
| `pi` | Pi |
| `e` | Euler's number |
| `tau` | Two times pi |
| `ans` | Previous scientific result |
| `x` | Graphing variable |

### Supported Functions

```text
sin    cos    tan
asin   acos   atan
sec    csc    cot

sinh   cosh   tanh

sqrt   cbrt
ln     log    log10    log2
exp    abs

floor  ceil   round   sign

deg    rad

min    max    pow
atan2  hypot  mod
```

Function arguments use parentheses.

Examples:

```text
sin(pi / 2)
sqrt(25)
max(4, 9)
pow(2, 8)
hypot(3, 4)
```

### Expression Evaluation

Mathematical expressions are parsed into an abstract syntax tree.

The parser implements operator precedence and right-associative exponentiation.

For example:

```text
2^3^2 = 512
```

Expressions containing `x` can be evaluated repeatedly using different variable values.

Malformed expressions are rejected during parsing.

Mathematically undefined real-number results are represented internally as NaN.

---

## Numerical Precision and Limitations

### Scientific Arithmetic

Scientific calculations use IEEE-754 double-precision floating-point values.

Consequently:

- Results are subject to floating-point rounding.
- Some mathematical operations produce non-finite results.
- Arbitrary-precision decimal arithmetic is not currently supported.
- Symbolic algebra is not currently supported.
- Complex-number arithmetic is not currently supported.

### Rational Arithmetic

Rational numbers use arbitrary-precision integer numerators and denominators.

Arithmetic operations preserve exact fractional results.

Decimal conversion is rounded to a configurable number of places, with the graphical interface displaying up to 15 decimal places by default.

### Graphing

The current graphing implementation supports explicit Cartesian functions of the form:

```text
y = f(x)
```

The renderer uses fixed-interval sampling and heuristic discontinuity detection.

It does not guarantee accurate representation of every discontinuity, asymptote, or narrow mathematical feature.

Advanced capabilities such as implicit equations, parametric functions, polar coordinates, interactive variables, symbolic mathematics, and numerical calculus are not currently implemented.

---

## Development and Contributions

Rational Studio is under active development.

Development tasks, feature requests, defects, and technical improvements are managed through the repository's **GitHub Issues** system.

Issues are categorized using labels to distinguish enhancements, defects, documentation work, and other maintenance activities.

Contributions and technical discussions are welcome.

Before beginning significant changes, contributors are encouraged to review existing issues to avoid duplicating work.

New functionality should preserve the separation between numerical computation and presentation logic wherever practical.

Changes to mathematical functionality should include appropriate regression tests.

---

## License

Rational Studio is licensed under the **MIT License**.

See the `LICENSE` file for the full license text.

Qt, Boost, and other third-party dependencies retain their respective licenses. The project's MIT License does not replace or modify those terms.
