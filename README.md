# Rational Studio

**Rational Studio** is an open-source scientific and graphing calculator developed in **C++20** using **Qt Widgets**.

The application combines scientific expression evaluation, interactive Cartesian graphing, and arbitrary-precision rational arithmetic within a unified desktop interface.

Rational Studio emphasizes mathematical correctness, modular architecture, reusable C++ components, automated regression testing, and intuitive mathematical interaction.

**Current Version:** 0.1.0  
**Development Platform:** Linux  
**Target Platforms:** Linux and Windows  
**License:** MIT

> **Development Status:** Rational Studio is under active development. The application is functional and has been built and tested on Linux. Windows compatibility remains a development target and has not yet been independently verified.

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

Scientific calculations use IEEE-754 double-precision floating-point arithmetic.

### Graphing Calculator

The graphing workspace provides interactive visualization of explicit Cartesian functions.

Current capabilities include:

- Simultaneous rendering of multiple functions.
- Editable mathematical expressions.
- Individual curve visibility controls.
- Color-coded function rendering.
- Interactive mouse-driven panning.
- Cursor-anchored scroll-wheel zooming.
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

Graph expressions are parsed into an abstract syntax tree before evaluation, avoiding repeated parsing during curve sampling.

#### Adaptive Curve Sampling

The graphing engine uses adaptive screen-space sampling to approximate mathematical curves.

Rather than relying exclusively on fixed horizontal sampling intervals, the renderer evaluates intermediate points and recursively subdivides intervals when their screen-space approximation error exceeds a configurable tolerance.

The implementation includes:

- Coarse initial sampling intervals.
- Recursive subdivision based on approximation error.
- Midpoint and quarter-point evaluation.
- Additional sampling in regions requiring greater geometric detail.
- Finite-coordinate validation.
- Configurable evaluation and subdivision limits.

The mathematical sampling engine is independent of Qt.

#### Undefined Domain Handling

The graphing engine performs additional validity checks when sampling functions with restricted mathematical domains.

Invalid evaluations are excluded from generated drawing geometry, and connections are omitted across detected undefined regions.

Supported behavior includes handling sampled undefined regions in functions such as:

```text
sqrt(x)
ln(x)
1/sqrt(x)
acos(x)
sqrt((x-0.131)*(x-0.147))
```

The sampler also treats exceptions thrown during mathematical evaluation as undefined samples.

**Limitations:** Domain detection is based on finite sampling. Arbitrarily narrow undefined regions, certain discontinuities, asymptotes, and highly oscillatory features may not be detected or reconstructed accurately.

More advanced discontinuity classification and rendering optimizations are not yet implemented.

### Exact Rational Calculator

The rational calculator provides exact arithmetic using arbitrary-precision integers.

Supported functionality includes:

- Rational addition, subtraction, multiplication, and division.
- Automatic fraction reduction.
- Signed numerator and denominator handling.
- Zero-denominator validation.
- Arbitrary-precision integer arithmetic.
- Rational comparisons within the mathematical core.
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
| Coordinate Mathematics | Custom GraphViewport component |
| Curve Sampling | Custom adaptive CurveSampler component |
| Graph Rendering | Qt QPainter |
| Automated Testing | CTest |
| Primary Development Environment | Linux |

The numerical and graph-sampling components are independent of Qt, allowing them to be compiled and tested without initializing the graphical application.

---

## Project Structure

```text
RationalStudio/
├── CMakeLists.txt
├── README.md
├── VERIFICATION.md
├── LICENSE.MD
├── .gitignore
│
├── src/
│   ├── main.cpp
│   │
│   ├── core/
│   │   ├── Rational.hpp
│   │   ├── Rational.cpp
│   │   ├── Expression.hpp
│   │   ├── Expression.cpp
│   │   ├── GraphViewport.hpp
│   │   ├── GraphViewport.cpp
│   │   ├── CurveSampler.hpp
│   │   └── CurveSampler.cpp
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
    ├── CoreTests.cpp
    ├── GraphViewportTests.cpp
    ├── CurveSamplerTests.cpp
    └── CurveDomainTests.cpp
```

### Mathematical Core

The `RationalCore` static library provides reusable numerical and graphing functionality independently of Qt.

#### Rational

Implements exact rational-number representation using arbitrary-precision integer numerators and denominators.

Responsibilities include:

- Fraction normalization.
- Arithmetic operations.
- Rational comparisons.
- Input validation.
- String parsing.
- Decimal conversion and rounding.

#### Expression

Implements scientific expression parsing and evaluation.

Responsibilities include:

- Mathematical expression tokenization.
- Recursive-descent parsing.
- Abstract syntax tree construction.
- Operator precedence and associativity.
- Mathematical function evaluation.
- Variable evaluation for graphing.
- Error handling for malformed expressions.

Expressions are parsed once and can be evaluated repeatedly with different variable values.

#### GraphViewport

Encapsulates Cartesian viewport mathematics without Qt dependencies.

Responsibilities include:

- World-to-screen coordinate transformations.
- Screen-to-world coordinate transformations.
- Viewport dimensions and scale.
- Viewport center coordinates.
- Mouse-driven panning calculations.
- Cursor-anchored zoom calculations.
- Input validation and state preservation.

The component provides a single authoritative representation of viewport state.

Invalid viewport operations are rejected without modifying previously valid state.

#### CurveSampler

Implements adaptive sampling for explicit Cartesian functions.

Responsibilities include:

- Screen-space approximation.
- Recursive curve subdivision.
- Intermediate mathematical evaluations.
- Finite-coordinate validation.
- Handling sampled undefined mathematical regions.
- Drawing-path separation.
- Per-call evaluation limits.

The sampler produces screen-space drawing geometry without depending on Qt-specific types.

This allows its mathematical behavior to be tested independently of the graphical renderer.

### Graph Rendering

The `GraphCanvas` widget handles presentation and interaction using Qt.

Its responsibilities include:

- Grid and axis rendering.
- Mathematical curve drawing.
- Mouse event handling.
- Viewport interaction.
- Coordinate tracing.
- Integration with the mathematical graphing components.

`GraphCanvas` delegates coordinate transformations to `GraphViewport` and mathematical curve sampling to `CurveSampler`.

The resulting geometry is rendered using Qt's `QPainter` and `QPainterPath` APIs.

### Application Interface

The `MainWindow` class manages three primary workspaces:

1. Scientific Calculator.
2. Graphing Calculator.
3. Exact Fractions.

The interface is constructed programmatically using Qt Widgets rather than Qt Designer-generated `.ui` files.

### Command-Line Application

`RationalCLI` provides a standalone terminal interface for exact rational arithmetic.

It uses the same `RationalCore` library as the desktop application.

The terminal executable demonstrates that numerical functionality can operate independently of the graphical interface.

---

## Requirements

### Development Dependencies

- A C++20-compatible compiler.
- CMake 3.16 or newer.
- Qt Widgets development libraries.
- Boost headers providing Boost.Multiprecision.
- Ninja or another supported native build tool.

Qt 6 is the preferred development target.

The CMake configuration also includes Qt 5 discovery support, although Qt 5 compatibility has not yet been independently verified.

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

```bash
git clone https://github.com/SpilledMilkFactory/RationalStudio.git
cd RationalStudio
```

### Configure a Release Build

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

The mathematical core, graph-sampling engine, tests, and terminal application can be compiled independently of Qt.

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

The test suite currently includes four independently registered test executables.

| Test Executable | Primary Responsibility |
|---|---|
| `RationalCoreTests` | Rational arithmetic and scientific expression evaluation |
| `RationalViewportTests` | Coordinate transformations, viewport navigation, and validation |
| `RationalCurveSamplerTests` | Adaptive curve sampling and screen-space approximation |
| `RationalCurveDomainTests` | Mathematical domain validity and drawing-path separation |

### Run the Complete Test Suite

```bash
ctest --test-dir build --output-on-failure
```

### Mathematical Core Tests

The mathematical core tests cover:

- Rational arithmetic and normalization.
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

### Viewport Tests

The viewport tests cover:

- World-to-screen transformations.
- Screen-to-world transformations.
- Coordinate round trips.
- Viewport resizing.
- Mouse-driven panning.
- Cursor-anchored zoom.
- Scale validation.
- Invalid-input rejection.
- State preservation after rejected operations.

### Adaptive Sampling Tests

The curve-sampling tests cover:

- Straight-line approximation.
- Nonlinear curve sampling.
- Adaptive subdivision behavior.
- Oscillatory mathematical functions.
- Drawing-coordinate validation.
- Screen-space approximation checks.
- Sampling evaluation limits.

### Domain Validation Tests

The domain tests cover:

- Open and closed domain boundaries.
- Functions with bounded valid domains.
- Functions containing undefined intervals.
- Narrow invalid regions between valid branches.
- Exception-producing mathematical evaluations.
- NaN and infinity handling.
- Rejection of unrenderable coordinates.
- Evaluation safety limits.

The domain suite includes a targeted regression case for a narrow undefined interval that previously resulted in an incorrect curve connection.

### Sanitizer Testing

The Qt-independent components can also be compiled and tested using AddressSanitizer and UndefinedBehaviorSanitizer.

Example configuration:

```bash
cmake -S . -B build-asan \
    -G Ninja \
    -DCMAKE_BUILD_TYPE=Debug \
    -DRATIONALSTUDIO_BUILD_GUI=OFF \
    -DCMAKE_CXX_FLAGS="-fsanitize=address,undefined -fno-omit-frame-pointer" \
    -DCMAKE_EXE_LINKER_FLAGS="-fsanitize=address,undefined"
```

Compile:

```bash
cmake --build build-asan --parallel
```

Run:

```bash
ctest --test-dir build-asan --output-on-failure
```

This configuration tests the Qt-independent components. It does not provide sanitizer coverage for the graphical application.

### Test Coverage Limitations

The current automated suite primarily validates mathematical computation, coordinate transformations, and generated graph geometry.

Automated Qt GUI interaction tests and direct rendering-output verification are not yet implemented.

The test suite does not establish that every mathematical discontinuity or arbitrarily narrow undefined region can be detected.

See [VERIFICATION.md](VERIFICATION.md) for recorded build and test results.

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

Non-finite or mathematically undefined real-number results are represented internally as NaN by the expression-evaluation interface.

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

The renderer uses adaptive screen-space sampling and intermediate mathematical evaluations to approximate curves.

Detected invalid evaluations are excluded from generated geometry, and the sampler attempts to preserve gaps across sampled undefined regions.

However, several limitations remain:

- Arbitrarily narrow undefined intervals may escape detection.
- Vertical asymptote handling remains heuristic.
- Steep continuous curves can be affected by conservative jump detection.
- Highly oscillatory functions may be under-sampled.
- Sampling and rendering performance depend on function complexity.
- Curve geometry is currently regenerated during rendering rather than maintained in a persistent rendering cache.

Advanced capabilities such as implicit equations, parametric functions, polar coordinates, interactive variables, symbolic mathematics, and numerical calculus are not currently implemented.

---

## Development and Contributions

Rational Studio is under active development.

Feature requests, defects, technical improvements, and other development tasks are managed through [GitHub Issues](https://github.com/SpilledMilkFactory/RationalStudio/issues).

Repository milestones and the public GitHub Project track the organization and progress of development work.

Contributions and technical discussions are welcome.

Before beginning significant changes, contributors are encouraged to review existing issues to avoid duplicating work.

New functionality should preserve the separation between numerical computation and presentation logic wherever practical.

Changes to mathematical functionality should include appropriate regression tests.

---

## License

Rational Studio is licensed under the **MIT License**.

See [LICENSE.MD](LICENSE.MD) for the full license text.

Qt, Boost, and other third-party dependencies retain their respective licenses. The project's MIT License does not replace or modify those terms.
