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

**Cached curve geometry.** The Qt renderer caches completed `QPainterPath` geometry per compiled expression. Unchanged curves reuse their paths during ordinary repaints and presentation-only changes, including color, label, visibility, and ordering changes. Editing an expression rebuilds the affected curve; viewport, size, angle-mode, and previous-answer changes invalidate geometry as necessary. Completed curves and retained previews also use device-scale-aware raster caches, avoiding repeated stroking of dense unchanged geometry. Disconnected branches are stroked separately. Cache rebuilds still occur on the GUI thread.

**Bounded sampling evaluations.** The graph renderer applies configurable per-curve-per-paint and shared per-paint callback limits when rebuilding cached geometry. A Qt-independent `CurveEvaluationBudget` tracks evaluations across multiple curves. `CurveSamplingJob` resumes unfinished builds on later paints, memoizes probes in an unfinished interval, and publishes replacement paths only after the build completes. Exhausting a paint budget no longer caches an incomplete curve as its final geometry. The single-call `sampleExplicitCurve` API still reports truncation when its configured budget is exhausted. These limits bound callback counts, not the duration of an individual evaluation or overall frame time.

**Coalesced navigation redraws.** Rapid mouse-wheel zooming and mouse-drag panning use a short (16 ms target) coalescing interval. During that interval, existing cached curve paths can be shown through a temporary screen-space transform without resampling each intermediate viewport. At the next scheduled refresh, replacement geometry starts building for the latest viewport. Previously completed paths remain available while that work yields to later paints, including toolbar zoom and resize changes. Sampling covers twice the widget width and height to provide surrounding geometry for nearby pan/zoom previews; this finite margin cannot cover arbitrary navigation jumps. Budget-deferred paints are timer-paced and obsolete queued requests are cancelled when newer inputs supersede them. Sampling remains on the Qt GUI thread; no hard frame-time or latency guarantee is implied.

**Wide-view curve coverage (Issue #26).** Interactive sampling limits coarse intervals in world space so repeated tangent poles are not skipped merely because the view is zoomed out. Monotonicity checks, screen-space geometric error, offscreen pruning, and additional refinement near unrenderable probes preserve visible steep tails without joining the tested opposite sides of poles. Coordinates are clipped before rasterization. Coverage tests check expected tangent intersections and rendered pixels, rather than only whether each horizontal region contains some geometry.

**Viewport resizing and boundary safety.** Grid lines and labels are positioned in bounded screen space, with checks for finite coordinates and representable grid spacing. During rapid resizing, obsolete scheduled redraws are cancelled and geometry is rebuilt for the current widget dimensions. Invalid dimensions and non-finite viewport settings are rejected or skipped without painting corrupted geometry. Very large but finite viewport centers may intentionally suppress grid markers when floating-point precision cannot support reliable positions; zoom and mathematical precision limitations still apply.

**Performance benchmarking (Issue #25).** Optional, non-CTest benchmark executables measure sampling callback and geometry-assembly cost, cold invalidated and warm cached offscreen Qt repaint times, and wheel/drag navigation event-to-preview and event-to-refreshed-geometry delays across one, four, and eight active curves. Repeated Release runs emit CSV and JSON with median and 95th-percentile timings. These measurements are environment-dependent; offscreen raster timing is not physical display latency or a frame-rate guarantee.

**Important limitation:** Finite sampling cannot establish continuity for every mathematical function. Narrow singularities, unobserved domain gaps, high-frequency oscillations, and extreme nonlinear features may still be missed or approximated imperfectly. Additional terminal refinement improves selected steep continuous curves but does not guarantee their reconstruction. Cached geometry is rebuilt when required inputs change; persistent caching does not eliminate the cost of sampling during navigation.

**Named-variable parsing (Issue #27).** The Qt-independent expression compiler can accept explicitly declared named variables using `VariableScope` and evaluate them with per-call `VariableBindings`, without reparsing when values change. Identifiers are ASCII `[A-Za-z_][A-Za-z0-9_]*` (maximum 64 characters), case-insensitive, and cannot shadow `x`, `ans`, constants, or builtin functions. Unknown/unbound variables remain invalid/undefined. **The parser itself does not own assigned values; subsequent Issues #28–#30 add a separate assignment/dependency owner, reusable function definitions, and session-local sliders. Automatic graph binding and workspace persistence remain future work.**

**Scalar assignments and dependencies (Issue #28).** The Qt-independent `VariableDefinitions` component stores assignments such as `a = 2`, `b = a + 3`, and `c = b^2`, compiles edited definitions against explicit identifier scopes, tracks direct and transitive dependencies, and recomputes only the affected subgraph after a successful edit. Malformed or circular redefinitions are rejected without replacing prior values or edges. Evaluation-domain errors propagate as undefined results; subsequent edits can restore valid values. Recomputed names and names with changed numeric/undefined values are reported separately to support future selective graph invalidation. Variables must be defined before they can be referenced; scalar assignments do not depend on `x` or `ans`. **Graphing assignment rows, function editing, automatic graph updates, and persistence are outside this core-only feature; the GUI gains session-local sliders in Issue #30.**

**Reusable function definitions (Issue #29).** The Qt-independent `FunctionDefinitions` owner compiles reusable functions such as `f(x) = x^2 + 3`, supports calls through an explicit `FunctionScope`, and resolves previously defined functions with checked arity. Function parameters have lexical scope, and user functions may capture named scalar values from `VariableBindings` without textual substitution. Rejected malformed, recursive, or cyclic definitions preserve the previous published function scope. Accepted redefinitions recompile dependent definitions and publish immutable snapshots. Previously compiled external expressions retain their old snapshots until recompiled against the latest function scope; changing scalar values only requires fresh bindings. **GUI function-definition rows, automatic graph invalidation, and persistence are planned for later Milestone 2 issues; basic session-local sliders are introduced in Issue #30.**

**Interactive variable sliders (Issue #30).** The graph sidebar includes session-local Qt Widgets controls for adding named scalar variables and changing their numeric values. Each slider exposes editable minimum, maximum, and step settings and updates the existing `VariableDefinitions` owner and any scalar dependents. A Qt-independent `VariableSliderScale` bounds the tick count to 100,000, rejects non-finite/invalid ranges, and preserves an exactly reachable maximum even when the final interval is shorter than the requested step. The Qt editor supports values within ±1e9 and steps of at least 1e-9. **Issue #31 connects these values to graph rendering; slider configurations are not yet included in saved graph workspaces (Issue #35).**

<!-- ISSUE31_README_UPDATED -->

**Automatic variable-driven graph updates (Issue #31).** The graph editor compiles expressions against variables currently declared in the session slider panel. Curve sampling and point tracing consume immutable per-job snapshots of current `VariableBindings`. On an accepted slider edit, the scalar assignment owner reports changed numeric results (including transitive scalar dependents); only curves referencing those names directly or through compiled user-function captures have their geometry invalidated. Unrelated compiled expression identities, finished paths, and in-progress sampling jobs remain intact. Creating a variable can repair previously invalid graph rows; removing it invalidates affected rows without reparsing unrelated valid ones. The existing bounded sampling and repaint scheduling remain in force. GUI authoring of reusable functions and persistence of sliders are still outside this issue; no measured frame-rate or latency improvement is claimed.

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
| `Expression` | Parsing, operator precedence, mathematical functions, constants, named-variable nodes, explicit scope validation, and repeated evaluation with per-call bindings. |
| `VariableScope` / `VariableBindings` | Qt-independent declared identifier sets and normalized finite runtime values used by the assignment owner. |
| `VariableDefinitions` | Qt-independent assignment parsing, compiled scalar definitions, dependency graph, circular-reference rejection, selective re-evaluation, undefined propagation, and bindings snapshots. |
| `FunctionScope` / `FunctionDefinitions` | Explicit compiled-function lookup, lexical positional parameters, checked calls, transitive scalar dependencies, transactional redefinition, recursion rejection, and immutable scope publication. |
| `VariableSliderScale` | Qt-free validation and mapping of finite slider ranges and steps onto bounded integer ticks, including a reachable terminal endpoint. |
| `GraphVariableDependencies` | Qt-independent matching of changed scalar names against direct expression and transitive function-captured dependencies. |
| `GraphViewport` | Cartesian coordinate transformations, viewport state, precision-aware panning, cursor-anchored zooming, zoom-limit stability, numerical validation, and transactional state preservation. |
| `CurveSampler` | Adaptive approximation, domain and discontinuity checks, drawing-path separation, bounded terminal refinement for steep continuous curves, and evaluation limits. |
| `CurveSamplingJob` | Incremental visible-geometry builds, bounded work per advance, retained probes, and completion reporting. |
| `CurveEvaluationBudget` | Qt-independent callback accounting shared across curve-sampling operations, with explicit exhaustion reporting. |
| `GraphRepaintScheduler` | Qt-independent navigation coalescing, generation tracking, and cancellation of obsolete deferred repaint requests. |
| `GraphGridLayout` | Qt-independent validation of grid steps and pixel-space grid positions at viewport boundaries and extreme finite centers. |
| `CurveGeometryCache` | Qt-independent, expression-identity-based cache management, lazy geometry construction, reuse, and invalidation. |

The Qt-specific `VariableSliderPanel` provides interactive session-local scalar controls backed by `VariableDefinitions`; it notifies `MainWindow` of value and scope edits, and `GraphCanvas` selectively invalidates only affected curves. Slider configurations are not yet persisted. The Qt-specific `GraphCanvas` handles mouse and wheel events, grid and axis painting, coordinate tracing, and rendering the sampler's screen-space geometry. It stores generated `QPainterPath` values in `CurveGeometryCache` so ordinary repaints don't reevaluate unchanged curves. Geometry cache rebuilds advance resumable sampling jobs under per-curve-per-paint and shared callback limits. Interactive navigation coalesces refreshes and temporarily transforms completed paths; replacement geometry is published after completion. Per-curve raster caches reuse unchanged rendering, account for device scale, and preserve previews while builds are deferred. Resizes cancel stale refresh work, and bounded pixel-space grid positions prevent overflowed labels at extreme coordinates. `MainWindow` constructs the scientific, graphing, and exact-fraction interfaces programmatically; no Qt Designer `.ui` files are required.

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
│   │   ├── VariableDefinitions.hpp
│   │   ├── VariableDefinitions.cpp
│   │   ├── FunctionDefinitions.hpp
│   │   ├── FunctionDefinitions.cpp
│   │   ├── VariableSliderScale.hpp
│   │   ├── VariableSliderScale.cpp
│   │   ├── GraphVariableDependencies.hpp
│   │   ├── GraphViewport.hpp
│   │   ├── GraphViewport.cpp
│   │   ├── CurveSampler.hpp
│   │   ├── CurveSampler.cpp
│   │   ├── CurveGeometryCache.hpp
│   │   ├── GraphRepaintScheduler.hpp
│   │   └── GraphGridLayout.hpp
│   └── gui/
│       ├── MainWindow.hpp
│       ├── MainWindow.cpp
│       ├── VariableSliderPanel.hpp
│       ├── VariableSliderPanel.cpp
│       ├── GraphCanvas.hpp
│       └── GraphCanvas.cpp
├── benchmarks/                   # Local optional benchmark sources; excluded by .gitignore
│   ├── BenchmarkMetrics.hpp
│   ├── GraphSamplingBench.cpp
│   └── GraphCanvasBench.cpp
├── scripts/
│   ├── run_issue25_benchmarks.sh
│   ├── summarize_issue25.py
│   └── issue26_gui_verification.py
└── tests/
    ├── CoreTests.cpp
    ├── GraphViewportTests.cpp
    ├── CurveSamplerTests.cpp
    ├── CurveDomainTests.cpp
    ├── CurveDiscontinuityTests.cpp
    ├── CurveSteepTests.cpp
    ├── GraphViewportPrecisionTests.cpp
    ├── CurveGeometryCacheTests.cpp
    ├── GraphCanvasCacheTests.cpp
    ├── CurveEvaluationBudgetTests.cpp
    ├── GraphCanvasBudgetTests.cpp
    ├── GraphRepaintSchedulerTests.cpp
    ├── GraphCanvasSchedulingTests.cpp
    ├── GraphViewportBoundaryTests.cpp
    ├── GraphCanvasBoundaryTests.cpp
    ├── GraphRegressionMatrixTests.cpp
    ├── GraphCanvasRegressionMatrixTests.cpp
    ├── GraphBenchmarkStatisticsTests.cpp
    ├── CurveCoverageRegressionTests.cpp
    ├── CurveSamplingJobTests.cpp
    ├── NamedVariableTests.cpp
    ├── VariableDefinitionsTests.cpp
    ├── FunctionDefinitionsTests.cpp
    ├── VariableSliderScaleTests.cpp
    ├── VariableSliderPanelTests.cpp
    ├── GraphVariableDependencyTests.cpp
    ├── GraphVariableRefreshTests.cpp
    ├── GraphCanvasCoverageTests.cpp
    └── test_issue26_gui_verification.py
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

**Cross-subsystem regression coverage (Issue #24).** Dedicated Qt-independent and offscreen Qt matrices exercise viewport transformations and cursor-anchored zoom together with sampled curve accuracy, undefined domains, finite jumps and shifted poles, and explicit callback evaluation ceilings. The matrices complement rather than replace the existing targeted regression suites. They verify bounded numerical behavior and widget integration, not every mathematically possible function, pixel-identical images, or a guaranteed frame rate.

With GUI and testing enabled, CTest registers **29 tests from 28 executables**: twenty Qt-independent tests and nine offscreen Qt invocations from eight widget test executables. The coverage executable also runs separately at 2× display scale. Core-only builds register twenty tests:

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
| `RationalCurveEvaluationBudgetTests` | Exact callback accounting, per-curve and shared budgets, zero-budget behavior, invalid callbacks, and deterministic multi-curve cap sweeps. |
| `RationalGraphCanvasBudgetTests` | Qt offscreen per-paint budget enforcement, deferral of uncached curves, diagnostics, and cache invalidation after limit changes. |
| `RationalGraphRepaintSchedulerTests` | Qt-independent coalescing, generation control, stale deferred-work cancellation, and deterministic event-burst sweeps. |
| `RationalGraphCanvasSchedulingTests` | Offscreen Qt integration for wheel-event coalescing, transformed-path preview, deferred regeneration, and cancellation of superseded viewport refreshes. |
| `RationalGraphViewportBoundaryTests` | Transactional rejection of invalid viewport inputs, extreme finite centers, stable pixel-space grid layout, and repeated resize dimensions. |
| `RationalGraphCanvasBoundaryTests` | Offscreen Qt resize transitions, extreme zoom/center safety, stale-refresh cancellation, and invalid-view rollback. |
| `RationalGraphRegressionMatrixTests` | End-to-end Qt-free regression matrix of viewport transforms, zoom anchoring, sampled chord accuracy, restricted domains, discontinuities, and callback budgets. |
| `RationalGraphCanvasRegressionMatrixTests` | Qt offscreen regression matrix of caching, navigation, resizing, invalid views, evaluation budgets, and domain/discontinuity rendering. |
| `RationalGraphBenchmarkStatisticsTests` | Qt-independent benchmark statistics, CSV/JSON output, and argument validation helpers; no performance thresholds. |
| `RationalCurveCoverageRegressionTests` | Reproduce left-side budget starvation, check distributed strip coverage, shared ceilings, and smooth/restricted-domain controls for the single-call sampler. |
| `RationalCurveSamplingJobTests` | Complete tangent branches across widths and zoom levels, exact callback accounting, budget-independent geometry, pole/step separation, and steep continuous controls. |
| `RationalNamedVariableTests` | Scope declarations, identifier validation, reserved names, case-folded binding lookup, referenced-variable introspection, re-evaluation without reparsing, and legacy API regressions. |
| `RationalVariableDefinitionsTests` | Scalar assignment grammar, cycle detection and transactional rollback, dependency ordering, selective recomputation, undefined propagation/recovery, revisions, binding snapshots, and existing compiled-graph evaluation. |
| `RationalFunctionDefinitionsTests` | Function definitions and nested calls, argument validation, lexical parameters, scalar captures, transitive dependencies, safe redefinition/recursion rejection, and compiled-expression compatibility. |
| `RationalVariableSliderScaleTests` | Qt-free finite-range/step validation, bounded tick counts, endpoint mapping, rounding, and deterministic numerical sweeps. |
| `RationalVariableSliderPanelTests` | Offscreen Qt slider creation, duplicate-name rejection, range editing, input synchronization, scalar dependency propagation, and safe deletion. |
| `RationalGraphVariableDependencyTests` | Qt-free dependency selection, transitive assignment changes, nested function scalar captures, and unaffected-expression preservation. |
| `RationalGraphVariableRefreshTests` | Offscreen GraphCanvas and real MainWindow integration: named-variable graph parsing, selective cache reuse, value edits, declaration, removal, and restoration. |
| `RationalGraphCanvasCoverageTests` | Rendered tangent-tail coverage, identical repeated repaints, retained previews through interrupted refreshes, toolbar zoom, resizing, and exhausted shared budgets. |
| `RationalGraphCanvasCoverageHiDpiTests` | Second invocation of the coverage executable with `--dpi-smoke` and `QT_SCALE_FACTOR=2`. |

Run the configured suite:

```bash
ctest --test-dir build --output-on-failure
```

### Graph performance benchmarks (Issue #25)

The optional `RATIONALSTUDIO_BUILD_BENCHMARKS` CMake option (default `OFF`)
adds `RationalGraphSamplingBench` and, when the GUI is enabled,
`RationalGraphCanvasBench`. The executables are **not CTests** and contain no
absolute timing pass/fail thresholds. A separate
`RationalGraphBenchmarkStatisticsTests` CTest verifies percentile calculations,
CSV/JSON escaping, and report generation without testing host speed.

Build and record measurements using a Release configuration:

```bash
cmake -S . -B build-issue25-release -G Ninja \
    -DCMAKE_BUILD_TYPE=Release \
    -DRATIONALSTUDIO_BUILD_BENCHMARKS=ON
cmake --build build-issue25-release --parallel
bash scripts/run_issue25_benchmarks.sh
```

Results appear in `benchmark-results/issue25/` as raw CSV, grouped JSON,
and `RESULTS.md`. The harness measures one, four and eight active curves with
representative smooth, steep, discontinuous and restricted-domain functions.
The sampling benchmark records callback counts and sampling/geometry assembly
cost. The offscreen Qt benchmark records cold, invalidated painting versus
warm cached painting, and synthetic wheel and drag burst responsiveness.
Control CPU load and governor, record the toolchain/Qt/display context, run
repeated trials, and review median/p95. No before/after FPS claim is supported
without a comparably measured earlier baseline. Synthetic Qt events and
offscreen paints do not measure actual monitor presentation latency.

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

**Recorded Issue #26 results.** The Debug GUI-enabled suite passed **22/22** tests. The incremental sampling job passed **896,291 assertions**, including a direct core-only ASan/UBSan run. Debug and Release desktop applications were rebuilt; a complete Issue #26 Release CTest run and a complete current core-only sanitizer suite were not recorded. The manual evidence recorder separately passed **12 Python unit tests**:

```bash
python3 -m unittest discover -s tests -p 'test_issue26_gui_verification.py' -v
```

Six offscreen Qt widget executables now include focused rendered-pixel coverage and identical-repaint checks. They do not replace full desktop interaction testing. The local Issue #26 manual report retains the original **24 passed / 1 failed** result (`GRA-03`, missing tangent branches); a manual retest of the final fix has not been recorded. See [the manual checklist](docs/Issue26GraphingManualQA.md) for recorder usage; its Issue #25 baseline counts describe the original manual-only scope, while the current test counts above include the rendering fix.

Benchmark sources/results and manual evidence remain local and excluded from Git. The optional benchmark build requires the local `benchmarks/` sources. Historical Issue #25 benchmark aggregates remain recorded in `VERIFICATION.md`; they have not been rerun for the current renderer.

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
- **Rendering:** Valid paths are cached, and rapid navigation coalesces geometry refreshes with temporary affine previews. Resize changes invalidate stale paths and obsolete timers. Grid lines/labels use bounded pixel-space positions; extremely large finite viewport centers may suppress unreliable grid markers. Invalidated paths rebuild incrementally under per-curve-per-paint and shared callback budgets; complete replacements may take multiple paints. Retained previews include finite surrounding coverage and can omit newly exposed regions after sufficiently large navigation changes. The single-call sampler can still return explicitly truncated geometry. Sampling and other mathematical callbacks remain on the GUI thread. Coalescing and callback ceilings do not guarantee frame time, input latency, or increased FPS.
- **Not yet supported:** Implicit, parametric, or polar graphs; complex arithmetic; matrices; GUI-defined reusable functions and workspace persistence for sliders; symbolic algebra; numerical integration and differentiation; and inequality shading.
- **Platform verification:** Linux verified; native Windows and Qt 5 builds not yet verified.

## Development and contributions

Development work, defects, enhancements, and technical improvements are tracked in the repository's [GitHub Issues](https://github.com/SpilledMilkFactory/RationalStudio/issues). Major goals are grouped through GitHub Milestones and the [public project board](https://github.com/users/SpilledMilkFactory/projects/1), rather than duplicated in this README.

Contributions and technical discussion are welcome. Review existing issues before beginning substantial work, keep mathematical algorithms separate from presentation logic where practical, and add regression tests for changes to numerical behavior.

## License

Rational Studio is licensed under the **MIT License**. See [LICENSE.MD](LICENSE.MD).

Qt, Boost, and other dependencies remain subject to their own licenses; the project's MIT License does not supersede third-party terms.
