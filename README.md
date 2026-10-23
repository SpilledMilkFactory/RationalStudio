# Rational Studio

**Rational Studio** is an open-source scientific and graphing calculator written in **C++20** with **Qt Widgets**. It combines scientific expression evaluation, interactive Cartesian graphing, and exact rational arithmetic in a desktop application, with a separate command-line calculator using the same mathematical core.

The project emphasizes modular architecture, numerical reliability, and independently testable C++ components.

| | |
|---|---|
| **Version** | `0.1.0` (active development) |
| **Verified development platform** | Linux |
| **Target platforms** | Linux and Windows |
| **Language** | C++20 |
| **License** | MIT |

> **Status:** The application builds and runs on Linux. Native Windows builds, packaging, and runtime behavior have not yet been independently verified. See [VERIFICATION.md](VERIFICATION.md) for the current test record.

## Features

### Scientific calculator

- Arithmetic, parentheses, operator precedence, exponentiation, roots, and integer factorials.
- Trigonometric, inverse trigonometric, and hyperbolic functions.
- Logarithms, exponentials, and other common mathematical functions.
- Mathematical constants `pi`, `e`, and `tau`.
- Degree and radian modes, limited implicit multiplication, and the previous-answer variable `ans`.
- Live expression preview, calculation history, keyboard entry, and an on-screen keypad.

Examples:

```text
2 + 3 * 4
(2 + 3)^2
sin(pi / 2)
sqrt(144)
log(1000)
3!
2sin(pi / 4)
```

Scientific calculations use IEEE-754 double-precision floating-point arithmetic; they are not arbitrary-precision symbolic calculations.

### Graphing calculator

The graphing workspace plots multiple explicit Cartesian functions of the form `y = f(x)`.

- Editable expressions with individually configurable curve visibility and colors.
- Mouse-driven panning, precision-aware cursor-anchored wheel zooming, validated navigation limits, and a dynamically spaced Cartesian grid.
- Axes, coordinate labels, and point inspection near plotted curves.
- JSON workspace save/load and PNG graph export.
- Trigonometric expressions evaluated using the selected angle mode.

Examples:

```text
y = x^2
y = sin(x)
y = 2x + 3
y = 1/x
y = sqrt(abs(x))
y = floor(x)
```

**Adaptive curve sampling.** Instead of relying solely on fixed two-pixel intervals, the Qt-independent sampler evaluates intermediate points and recursively subdivides intervals where the screen-space approximation error warrants additional detail. Sampling has configurable subdivision and evaluation limits.

**Domain and discontinuity handling.** The sampler rejects unrenderable coordinates and disconnects drawing paths around *detected* undefined regions. Additional terminal-interval consistency checks help avoid artificial connections across sampled finite jump discontinuities and selected vertical asymptotes, including non-grid-aligned reciprocal poles. Unlike the original large-jump rule, the current approach does not automatically reject a segment merely for being steep.

**Steep continuous-curve preservation.** When a finite curve interval remains unresolved at the normal subdivision limit, the sampler can perform additional bounded refinement rather than immediately discarding the geometry. This helps preserve sharply changing continuous functions, including steep hyperbolic tangent and arctangent transitions. The additional refinement is configurable through `maxTerminalRefinement` and remains subject to the sampler's evaluation limit. Intervals that cannot be represented reliably are still omitted.

**Cached curve geometry.** The Qt renderer caches completed `QPainterPath` geometry per compiled expression. Unchanged curves reuse their paths during ordinary repaints and presentation-only changes, including color, label, visibility, and ordering changes. Editing an expression rebuilds the affected curve; viewport, size, angle-mode, and previous-answer changes invalidate geometry as necessary. Cache rebuilds still occur on the GUI thread.

**Important limitation:** Finite sampling cannot establish continuity for every mathematical function. Narrow singularities, unobserved domain gaps, high-frequency oscillations, and extreme nonlinear features may still be missed or approximated imperfectly. Additional terminal refinement improves selected steep continuous curves but does not guarantee their reconstruction. Cached geometry is rebuilt when required inputs change; persistent caching does not eliminate the cost of sampling during navigation.

**Viewport precision and navigation stability.** The Qt-independent `GraphViewport` component uses precision-aware calculations for mouse panning and cursor-anchored zooming. Navigation incorporates fused multiply-add operations where appropriate, preserves viewport state when zoom limits are reached, and avoids unintended zoom-direction reversals when restoring views outside the normal mouse-wheel scale range. Operations that would introduce excessive screen-space error due to floating-point limitations are rejected without partially modifying the viewport.

### Exact rational calculator

- Addition, subtraction, multiplication, and division of fractions.
- Arbitrary-precision integer numerators and denominators using `boost::multiprecision::cpp_int`.
- Fraction normalization, sign handling, zero-denominator validation, and exact rational comparisons in the core.
- Rounded decimal approximations for display.

Example:

```text
5/2 + 7/4 = 17/4
```

Fractions are reduced using the Euclidean greatest common divisor algorithm. Exact rational arithmetic remains subject to available memory and computation time.

## Technology stack

| Component | Technology |
|---|---|
| Language | C++20 |
| GUI | Qt Widgets |
| Build | CMake; Ninja or another supported generator |
| Exact arithmetic | Boost.Multiprecision |
| Scientific expressions | Custom recursive-descent parser and abstract syntax tree |
| Viewport mathematics | Qt-independent `GraphViewport` |
| Curve approximation | Qt-independent `CurveSampler` |
| Rendering | `QPainter` / `QPainterPath` |
| Regression testing | CTest |

## Architecture

The `RationalCore` static library has **no Qt dependency**. Together with the header-only `CurveGeometryCache` template, the reusable Qt-independent code includes:

| Component | Responsibility |
|---|---|
| `Rational` | Exact fractions, normalization, parsing, arithmetic, comparisons, and decimal conversion. |
| `Expression` | Parsing, operator precedence, mathematical functions, constants, and repeated expression evaluation. |
| `GraphViewport` | Cartesian coordinate transformations, viewport state, precision-aware panning, cursor-anchored zooming, zoom-limit stability, numerical validation, and transactional state preservation. |
| `CurveSampler` | Adaptive approximation, domain and discontinuity checks, drawing-path separation, bounded terminal refinement for steep continuous curves, and evaluation limits. |
| `CurveGeometryCache` | Qt-independent, expression-identity-based cache management, lazy geometry construction, reuse, and invalidation. |

The Qt-specific `GraphCanvas` handles mouse and wheel events, grid and axis painting, coordinate tracing, and rendering the sampler's screen-space geometry. It stores generated `QPainterPath` values in `CurveGeometryCache` so ordinary repaints don't reevaluate unchanged curves. `MainWindow` constructs the scientific, graphing, and exact-fraction interfaces programmatically; no Qt Designer `.ui` files are required.

`RationalCLI` links against `RationalCore` without using Qt.

### Repository layout

```text
RationalStudio/
├── CMakeLists.txt
├── README.md
├── VERIFICATION.md
├── LICENSE.MD
├── .gitignore
├── src/
│   ├── main.cpp
│   ├── cli/
│   │   └── main.cpp
│   ├── core/
│   │   ├── Rational.hpp
│   │   ├── Rational.cpp
│   │   ├── Expression.hpp
│   │   ├── Expression.cpp
│   │   ├── GraphViewport.hpp
│   │   ├── GraphViewport.cpp
│   │   ├── CurveSampler.hpp
│   │   ├── CurveSampler.cpp
│   │   └── CurveGeometryCache.hpp
│   └── gui/
│       ├── MainWindow.hpp
│       ├── MainWindow.cpp
│       ├── GraphCanvas.hpp
│       └── GraphCanvas.cpp
└── tests/
    ├── CoreTests.cpp
    ├── GraphViewportTests.cpp
    ├── CurveSamplerTests.cpp
    ├── CurveDomainTests.cpp
    ├── CurveDiscontinuityTests.cpp
    ├── CurveSteepTests.cpp
    ├── GraphViewportPrecisionTests.cpp
    ├── CurveGeometryCacheTests.cpp
    └── GraphCanvasCacheTests.cpp
```

## Requirements

- C++20-compatible compiler (GCC 15.2.0 used for the recorded Linux verification).
- CMake 3.16 or newer.
- Qt Widgets development packages for the desktop application; Qt 6 is preferred.
- Boost headers with Boost.Multiprecision.
- Ninja or another CMake-supported build tool.

CMake includes discovery support for Qt 5 and Qt 6, but **Qt 5 compatibility has not been independently verified**.

On Ubuntu or Debian-based Linux:

```bash
sudo apt update
sudo apt install \
    build-essential \
    cmake \
    ninja-build \
    qt6-base-dev \
    libboost-dev
```

A non-system Qt installation may require setting `CMAKE_PREFIX_PATH` during configuration.

## Build and run

Clone the public repository:

```bash
git clone https://github.com/SpilledMilkFactory/RationalStudio.git
cd RationalStudio
```

Configure and build a Release version:

```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
```

Launch either application:

```bash
./build/RationalStudio
./build/RationalCLI
```

For a separate Debug build:

```bash
cmake -S . -B build-debug -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build-debug --parallel
./build-debug/RationalStudio
```

### CMake options

| Option | Default | Purpose |
|---|---|---|
| `RATIONALSTUDIO_BUILD_GUI` | `ON` | Build the Qt desktop application. |
| `RATIONALSTUDIO_BUILD_CLI` | `ON` | Build the terminal calculator. |
| `BUILD_TESTING` | `ON` | Build and register the regression tests. |

To build and test the core **without Qt**:

```bash
cmake -S . -B build-core -G Ninja \
    -DCMAKE_BUILD_TYPE=Debug \
    -DRATIONALSTUDIO_BUILD_GUI=OFF
cmake --build build-core --parallel
ctest --test-dir build-core --output-on-failure
```

## Testing

With GUI and testing enabled, CTest registers nine test executables; core-only builds register eight:

| Executable | Focus |
|---|---|
| `RationalCoreTests` | Rational arithmetic and scientific expression evaluation. |
| `RationalViewportTests` | Coordinate transforms, panning, cursor-anchored zoom, and invalid-input handling. |
| `RationalCurveSamplerTests` | Adaptive approximation, sampled geometry, oscillatory functions, and evaluation limits. |
| `RationalCurveDomainTests` | Restricted domains, undefined intervals, non-finite results, and path separation. |
| `RationalCurveDiscontinuityTests` | Finite step jumps, `floor(x)`, reciprocal/tangent poles, sample-position sweeps, and continuous-curve controls. |
| `RationalCurveSteepTests` | Steep continuous curves, bounded terminal refinement, interpolation accuracy, sampling-position sweeps, and discontinuity regression controls. |
| `RationalViewportPrecisionTests` | Coordinate precision, repeated pan/zoom stability, zoom-limit behavior, restored viewport scales, extreme coordinates, and transactional state preservation. |
| `RationalCurveGeometryCacheTests` | Qt-independent cache identity, reuse, invalidation, reordering, and lazy regeneration. |
| `RationalGraphCanvasCacheTests` | Offscreen Qt widget rendering: cached `QPainterPath` reuse and invalidation; only built when the GUI is enabled. |

Run the configured suite:

```bash
ctest --test-dir build --output-on-failure
```

### AddressSanitizer and UndefinedBehaviorSanitizer

An independent core-only sanitizer configuration can be created with:

```bash
cmake -S . -B build-asan -G Ninja \
    -DCMAKE_BUILD_TYPE=Debug \
    -DRATIONALSTUDIO_BUILD_GUI=OFF \
    -DCMAKE_CXX_FLAGS="-fsanitize=address,undefined -fno-omit-frame-pointer" \
    -DCMAKE_EXE_LINKER_FLAGS="-fsanitize=address,undefined"
cmake --build build-asan --parallel
ctest --test-dir build-asan --output-on-failure
```

An offscreen Qt widget regression test validates graph geometry cache behavior. Comprehensive end-to-end GUI interaction tests and pixel-level rendering tests are not yet included.

See [VERIFICATION.md](VERIFICATION.md) for recorded toolchain, test, sanitizer, and manual GUI results.

## Mathematical expression reference

| Operator | Meaning |
|---|---|
| `+`, `-`, `*`, `/` | Basic arithmetic |
| `%` | Floating-point remainder |
| `^` | Exponentiation |
| `!` | Factorial |

Supported constants and special variables are `pi`, `e`, `tau`, `ans`, and graphing variable `x`.

Supported functions:

```text
sin    cos    tan    asin   acos   atan
sec    csc    cot    sinh   cosh   tanh
sqrt   cbrt   ln     log    log10  log2
exp    abs    floor  ceil   round  sign
deg    rad    min    max    pow    atan2
hypot  mod
```

Examples:

```text
sin(pi / 2)
sqrt(25)
max(4, 9)
pow(2, 8)
hypot(3, 4)
2^3^2
```

Expressions are parsed into an abstract syntax tree before evaluation. Exponentiation is right-associative (`2^3^2` evaluates to `512`). Invalid expressions are rejected during parsing; undefined real-number evaluations do not produce drawable graph coordinates.

## Precision and known limitations

- **Scientific arithmetic:** IEEE-754 `double`, subject to normal floating-point rounding and range constraints; no arbitrary-precision decimal or symbolic arithmetic.
- **Rational arithmetic:** Exact integer fractions; decimal displays are rounded approximations (up to 15 places in the current GUI).
- **Graphing:** Explicit Cartesian `y = f(x)` only. Adaptive sampling includes bounded secondary refinement to preserve steep continuous curves. Domain and discontinuity detection rely on finite numerical samples and cannot guarantee correct rendering of every mathematical function.
- **Viewport precision:** Viewport navigation uses double-precision coordinates and precision-aware pan/zoom calculations. Operations that cannot be represented within the configured screen-space error tolerance are rejected without modifying viewport state. Arbitrary-precision navigation is not supported.
- **Rendering:** Unchanged `QPainterPath` geometry is cached and reused across ordinary repaints. Sampling and rebuilding invalidated paths still run on the GUI thread; there is no background graph worker or shared frame-time guarantee.
- **Not yet supported:** Implicit, parametric, or polar graphs; complex arithmetic; matrices; interactive named-variable sliders; symbolic algebra; numerical integration and differentiation; and inequality shading.
- **Platform verification:** Linux verified; native Windows and Qt 5 builds not yet verified.

## Development and contributions

Development work, defects, enhancements, and technical improvements are tracked in the repository's [GitHub Issues](https://github.com/SpilledMilkFactory/RationalStudio/issues). Major goals are grouped through GitHub Milestones and the [public project board](https://github.com/users/SpilledMilkFactory/projects/1), rather than duplicated in this README.

Contributions and technical discussion are welcome. Review existing issues before beginning substantial work, keep mathematical algorithms separate from presentation logic where practical, and add regression tests for changes to numerical behavior.

## License

Rational Studio is licensed under the **MIT License**. See [LICENSE.MD](LICENSE.MD).

Qt, Boost, and other dependencies remain subject to their own licenses; the project's MIT License does not supersede third-party terms.
