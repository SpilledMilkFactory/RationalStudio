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

The graphing workspace supports explicit Cartesian functions `y = f(x)`, finite-interval parametric `x(t), y(t)`, polar radius equations `r = f(θ)`, and bounded implicit zero contours such as `x²+y²=25` (M3-01–M3-03). Issue #40 adds bounded Cartesian pairwise intersection search with visible-domain and numerical accuracy reporting. Issue #41 adds requested numerical roots, extrema and stationary-inflection inspection for valid visible Cartesian curves.

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

**Interactive variable sliders (Issue #30).** The graph sidebar includes session-local Qt Widgets controls for adding named scalar variables and changing their numeric values. Each slider exposes editable minimum, maximum, and step settings and updates the existing `VariableDefinitions` owner and any scalar dependents. A Qt-independent `VariableSliderScale` bounds the tick count to 100,000, rejects non-finite/invalid ranges, and preserves an exactly reachable maximum even when the final interval is shorter than the requested step. The Qt editor supports values within ±1e9 and steps of at least 1e-9. **Issue #31 connects these values to graph rendering; Issue #35 persists slider configurations in version-2 graph workspaces.**

<!-- ISSUE31_README_UPDATED -->

**Automatic variable-driven graph updates (Issue #31).** The graph editor compiles expressions against variables currently declared in the session slider panel. Curve sampling and point tracing consume immutable per-job snapshots of current `VariableBindings`. On an accepted slider edit, the scalar assignment owner reports changed numeric results (including transitive scalar dependents); only curves referencing those names directly or through compiled user-function captures have their geometry invalidated. Unrelated compiled expression identities, finished paths, and in-progress sampling jobs remain intact. Creating a variable can repair previously invalid graph rows; removing it invalidates affected rows without reparsing unrelated valid ones. The existing bounded sampling and repaint scheduling remain in force. GUI authoring of reusable functions and persistence of sliders are still outside this issue; no measured frame-rate or latency improvement is claimed.

<!-- ISSUE32_README_UPDATED -->

**Mathematical expression editing (Issue #32).** Scientific and graph inputs use `MathExpressionEdit`, a Qt Widgets single-line editor with cursor-aware insertion, selected-expression wrapping, paired parentheses, and an on-screen graph symbol strip (`π`, `√`, `²`, `×`, `÷`, `−`). Supported Unicode symbols are translated at the expression-compiler boundary by the Qt-independent, header-only `MathExpressionInput` helper; the visible text and caret are not rewritten. Graph rows show immediate parser-backed valid, invalid, or incomplete-edit feedback, while geometric redraw remains debounced. Scientific preview also recognizes the supported symbols. Parenthesized roots such as `√(9)` are supported; arbitrary mathematical typesetting, syntax highlighting within individual tokens, and a full equation-layout editor are outside this issue. **Issue #32 does not change evaluation grammar, graph sampling, caching, sliders, or persistence.**

<!-- ISSUE33_README_UPDATED -->

**Inline expression diagnostics (Issue #33).** The Qt-independent `Expression` parser publishes an optional `ExpressionDiagnostic` describing invalid input using a category, readable message, source byte offset, and token extent while preserving the existing `error()` string API. `MathExpressionInput::sourceColumn` maps offsets from normalized parser input back to visible UTF-8 character columns when the editor contains supported symbols such as `π`, `√`, or `²`. Scientific and graph editors display actionable errors in their existing inline hint rows and editor tooltips, clear obsolete messages after recovery, and distinguish incomplete edits from malformed syntax where supported. Invalid graph rows remain outside the render curve set, while unrelated valid rows remain available. This is **not** a grammar expansion, an exhaustive syntax predictor, a full token underlining editor, or a guarantee of every domain error at parse time. The graph repaint debounce and selective geometry caching remain unchanged.

<!-- ISSUE34_README_UPDATED -->

**Keyboard mathematical input (Issue #34).** The shared Qt `MathExpressionEdit` accepts editor-local Alt-key shortcuts: `Alt+P` inserts `π`, `Alt+R` inserts `√(...)`, `Alt+2`/`Alt+3` insert squared/cubed notation, `Alt+M`/`Alt+D` insert multiplication/division glyphs, and `Alt+S`/`Alt+C`/`Alt+T` insert sine/cosine/tangent templates. Selected text is wrapped by roots, function calls, and powers as in Issue #32. Graph editors also support `Alt+↑`/`Alt+↓` to move between expression rows (wrapping at either end) and `Ctrl+Enter` to append and focus a new row, up to the existing 40-row limit. Scientific `Enter` still calculates. Ordinary Qt editing, clipboard and undo shortcuts are retained; unrecognized shortcuts are forwarded to Qt. Shortcuts require focus in a mathematical editor; operating-system/global shortcuts may take precedence on some desktop configurations. No parser, variable dependency, sampling, caching or workspace format changes were made.

<!-- ISSUE35_README_UPDATED -->

**Graph workspace persistence (Issue #35).** JSON workspaces now use version 2 and preserve graph rows (expression text, visibility and color), viewport, angle mode, slider names/current values/minimum/maximum/steps, non-slider scalar assignment sources, and reusable mathematical function definitions (parameter lists and bodies). Variable and function sources are saved in dependency-first order. Load validates the entire document using the existing mathematical owners before replacing the current session; invalid, duplicate, cyclic or unresolved definitions are rejected. Save uses QSaveFile for atomic replacement. Original version-1 graph workspaces remain readable, without inventing missing variables or functions. Restored function scopes are connected to graph compilation, including nested function references to scalar variables. The file-based save/load API supports offscreen integration testing without file dialogs. This feature does not introduce a GUI function definition editor or arbitrary symbolic algebra.

<!-- ISSUE36_README_UPDATED -->

**Variable, function and dynamic-evaluation regressions (Issue #36).** Two Qt-independent CTest executables (`RationalVariableFunctionInteractionTests` and `RationalDynamicEvaluationMatrixTests`) validate the complete compiled-expression/variable/function interaction path using explicit numerical oracles. Coverage includes direct and transitive resolution, case-normalized names, dependency-first recomputation, exact changed-name reporting, transactional rollback on malformed/cyclic definitions, function arity/recursion safeguards, immutable function-scope and variable-binding snapshots, undefined-domain propagation and recovery, lexical parameter shadowing, angle-mode updates, selective graph invalidation, and a fixed 240-update × 17-point numerical matrix. This issue adds tests only; it does **not** change parser grammar, runtime semantics, GUI interaction, persistence schema, or rendering algorithms. The suites run in GUI-enabled builds and in the Qt-free sanitizer build.

<!-- ISSUE37_README_UPDATED -->

**Parametric equation graphing (Issue #37; Milestone 3).** The graph sidebar now supports paired `x(t)` and `y(t)` coordinate expressions with editable finite parameter intervals `t min` and `t max` (bounded to ±1e8, with `min < max`). The parameter `t` is local to each sampled curve and may be combined with declared scalar variables and reusable functions. A typical circle uses `x(t) = cos(t)`, `y(t) = sin(t)`, interval `[0, 2π]` in radian mode; in degree mode use `[0, 360]`. The new Qt-free `ParametricSampler` adaptively approximates (X(t),Y(t)) with screen-space error checks, bounded recursion and shared evaluation-budget accounting; invalid points disconnect strokes rather than being painted as real coordinates. The existing `GraphCanvas` owns both Cartesian and parametric curves and reuses its Qt path/raster caching, navigation coalescing, incremental sampling, viewport clipping, and selection of curves affected by variable changes. Editing either coordinate or the parameter interval invalidates its cached geometry. Mixed workspaces containing parametric rows use a version-3 JSON representation with per-row kind and parameter bounds; Cartesian-only workspaces still save as version 2, and legacy versions 1 and 2 remain loadable. Parametric tracing, polar, implicit, and inequality graphing are planned for later M3 issues. Like the Cartesian sampler, finite sampling does not guarantee detection of every singularity or high-frequency feature. No performance improvement is claimed without a controlled benchmark.

<!-- ISSUE38_README_UPDATED -->

**Polar coordinate graphing (Issue #38; Milestone 3).** The graph sidebar supports `r = 2cos(θ)` or `r = 2cos(theta)` with configurable finite angular bounds (±1e8, minimum < maximum). Polar radius functions are compiled once as paired Cartesian coordinate expressions using `x = r cos(θ)` and `y = r sin(θ)`. The Qt-independent `PolarExpression` adapter maps the local θ identifier onto the existing sampling parameter `t` and preserves the scalar/function scope and current RAD/DEG selection. Negative radii are represented mathematically rather than clamped. Polar geometry shares the Issue #37 adaptive `ParametricSampler`, per-paint sampling budgets, `GraphCanvas` drawing, raster/path caching, and selective slider dependency updates. Mixed Cartesian/parametric/polar sessions use version-3 workspace JSON with `kind: polar`, radius source, and angular interval; existing versions 1–3 remain readable. Polar input is not a separate expression grammar; unsupported identifiers still fail parser validation. Angle bounds are interpreted in the current angle mode (e.g. 0 to 2π radians or 0 to 360 degrees); switching mode does not automatically rescale entered bounds. Finite sampling may miss narrow features, and advanced polar tracing is deferred to M3-08. No controlled performance improvement is claimed.

<!-- ISSUE39_README_UPDATED -->

**Implicit equation visualization (Issue #39; Milestone 3).** The graph sidebar adds a `+ Implicit` row for equations `F(x,y)=G(x,y)` (e.g., `x²+y²=25`). The Qt-independent `ImplicitContour` module compiles the normalized difference `F-G` using existing expression and variable/function scopes, and performs deterministic, bounded marching squares over the visible graph region. The Cartesian `x` and per-evaluation local `y` are sampled independently; slider definitions remain session-owned. The incremental contour job consumes the existing per-curve and shared per-paint `CurveEvaluationBudget`, yields without publishing incomplete contours as finished paths, and produces the existing screen-space `CurveVertex`/`QPainterPath` geometry. Invalid/non-finite cells produce no segments. Ambiguous saddle cells use a deterministic bilinear-center tie-breaker. Geometry, raster caching, navigation previews, and variable-driven invalidation remain shared with other graph types; presentation-only edits preserve cached geometry. Version-3 JSON saves implicit rows using `kind: implicit`, and workspaces from versions 1–3 remain loadable. Loading validates the equation against staged variable/function owners before replacing the workspace. Marching squares does not guarantee recovery of tangential zero sets, arbitrarily narrow contours, asymptotic discontinuities, or sub-grid features; sampling work remains bounded, not real-time guaranteed. General-purpose region filling and implicit tracing are outside this issue.

<!-- ISSUE40_README_UPDATED -->

**Numerical intersection detection (Issue #40; Milestone 3).** The new Qt-independent `IntersectionDetection` service searches pairs of compiled explicit Cartesian functions `f(x)` and `g(x)` on a finite X-domain by bounded sign-change bracketing and refinement, with a local-minimum heuristic for sampled tangencies. Each candidate must satisfy a configurable absolute Y-residual tolerance; results record the approximate `(x,y)`, actual residual and final bracket width. Non-finite callback results are excluded; close functions, discontinuities, high-frequency zeros, and nonsampled tangencies can still defeat finite search. Coincident sampled functions are reported separately rather than producing arbitrary isolated roots. Searches are limited by sample count, total callback budget, result count, and—in the GUI—pair count. The graph toolbar's **Intersections** button searches visible, valid, explicit Cartesian rows over the current viewport X-domain and displays numerical tolerances, results and incompleteness warnings; it does not modify cached plot geometry or workspace persistence. **Parametric, polar, and implicit pair detection are not implemented by this action**, and complete root certification or guaranteed real-time latency is not claimed.

<!-- ISSUE41_README_UPDATED -->

**Graph roots, extrema and critical-point inspection (Issue #41; Milestone 3).**
The graph toolbar's **Features** action inspects valid, visible explicit Cartesian
rows in the current finite X viewport. The Qt-independent `GraphFeatureDetection`
service reuses the Issue #40 numerical zero finder for isolated X-axis roots,
then scans and refines sampled local minima, maxima (including supported
nondifferentiable cusps) and stationary-inflection candidates. Reported entries
include approximate `(x,y)` coordinates, numerical residual or slope estimate,
search-bracket width, search-domain bounds and tolerances. Total callback
counts, candidate/result limits, scanned-row limits and displayed-output caps
are bounded. The action is explicitly requested and does not mutate plot
geometry, create workspace objects or persist results; edits, scalar changes,
angle-mode changes and viewport navigation mark previous reports stale.
Derivative estimates use finite differences; the classifications are **heuristic**,
not symbolic or certified. Finite sampling can miss narrow/oscillatory features,
false candidates can occur at discontinuities, and an observed local extremum is
not necessarily global. Endpoints are not classified as local extrema. Constant
zero functions are identified as sampled X-axis coincidence rather than an
arbitrary set of roots. Parametric, polar and implicit representations remain
outside this action; advanced tracing is reserved for M3-08. The synchronous
search bounds mathematical callback counts, not wall-clock latency.

<!-- ISSUE42_README_UPDATED -->

**Inequality region graphing (Issue #42; Milestone 3).** The graph sidebar provides a `+ Inequality` row for vertical comparisons `y > f(x)`, `y >= f(x)`, `y < f(x)`, and `y <= f(x)` (also Unicode `≤` / `≥` and reversed scalar comparisons such as `x² < y`). The existing scalar Expression compiler owns boundary semantics, named sliders, functions and angle mode; no comparison operators were added to the general-purpose expression grammar. A translucent color-matched fill marks the sampled vertical region above or below the boundary. Strict comparisons use dashed boundary strokes, inclusive comparisons solid strokes. The ordinary shared Cartesian CurveSamplingJob still builds and caches the boundary line, while a separate 2-pixel-column raster uses a maximum of 2,048 scalar callbacks per paint across all visible inequality rows, publishing a completed region only after all columns have been processed. Finished rasters are reused on unchanged repaints and invalidated on relevant variable, expression, or viewport changes. Mixed workspaces save inequalities with `kind: inequality` in the existing version-3 schema; loads validate both the comparator and compiled scalar expression transactionally, and versions 1–3 remain readable. This first supported scope covers **vertical Cartesian inequalities only**: generalized implicit two-dimensional relations, nonvertical `x > f(y)` regions, symbolic set operations, arbitrary Boolean combinations, and certified boundary topology are not implemented. Finite column sampling can miss narrow oscillations and cannot prove continuity across undefined points. Non-finite boundary samples produce no shade in their columns. Work is bounded by callback counts, not worst-case expression runtime or memory, and high-DPI region rasters are rendered at logical-widget resolution.

<!-- ISSUE43_README_UPDATED -->

**Multiple-representation graph controls (Issue #43; Milestone 3).** The graph sidebar now uses a two-column, width-aware graph creation grid for Cartesian, parametric, polar, implicit, and inequality rows, instead of a crowded single horizontal toolbar. Existing object names and creation handlers are retained for backward-compatible Qt tests and keyboard flows. Each action displays an explanatory tooltip, and the collapsible **Type guide** provides short syntax examples without occupying permanent sidebar space. Cartesian, implicit, and inequality rows have consistent readable representation badges; parametric and polar rows retain their paired/angle-specific controls and now present interval minimum and maximum inputs in two vertical rows for narrow viewports. New rows receive keyboard focus from creation buttons; visibility and remove controls expose accessible names. This is a GUI/presentation-only enhancement: expression parsing, curve sampling, renderer/cache behavior, interval interpretation, version-3 workspace persistence, and analytical features remain unchanged. Existing support for five representations is retained, including strict/inclusive inequality boundaries.

<!-- ISSUE44_README_UPDATED -->

**Advanced graph tracing and coordinate inspection (Issue #44; Milestone 3).** Pointer hover now inspects the nearest completed, visible Cartesian, parametric, polar, implicit, or inequality-boundary curve in the shared graph canvas. The Qt-independent `GraphTrace` helper finds a bounded nearest point along existing sampled screen-space segments, without joining disconnected strokes or triggering a new sampling job. Parametric sampling retains each emitted vertex's original `t`, enabling interpolated parameter inspection. Parametric readouts include `t` and evaluated Cartesian `(x,y)`; polar readouts include angular `θ` with RAD/DEG units, signed radius `r`, and Cartesian coordinates. Implicit zero contours display approximate `(x,y)` and sampled equation residual `|F-G|`. Inequality-boundary readouts distinguish strict/excluded from inclusive boundaries. The existing **Features** action additionally publishes optional, approximately positioned root/extremum/inflection markers that can be inspected by hovering; these are cleared on edits and navigation. Inspection uses completed cached geometry and does not publish unfinished sampling paths. Values come from finite sampling and parameter interpolation, not symbolic or certified inverse solving; self-intersections can have multiple valid parameters, unsupported/narrow features can be missed, and the bounded vertex scan can omit segments in extremely dense graphs. The shared renderer, mathematical grammar, and workspace schema are unchanged, and no controlled performance improvement is claimed.

<!-- ISSUE45_README_UPDATED -->

**Advanced graph correctness and performance qualification (Issue #45; Milestone 3).** Two Qt-independent CTest matrices add analytical reference cases and bounded resource checks across Cartesian, parametric, polar, implicit, and vertical inequality representations, including signed polar radii, domain gaps, intersection and feature candidates, and sampled-geometry tracing. A fixed-seed reference sample supplements deterministic numerical oracles; tolerances are explicit and finite sampling is not a completeness proof. Two optional Release benchmark executables (`RationalAdvancedGraphBench` and `RationalAdvancedGraphCanvasBench`, enabled by `RATIONALSTUDIO_BUILD_ADVANCED_GRAPH_BENCHMARKS=ON`) report median, p95, and maximum observed timings for mathematical sampling/tracing and offscreen Qt cold-to-ready/warm-cache drawing. Benchmark measurements are advisory and are **not** CTest timing pass/fail gates; they do not represent display latency or guarantee throughput on other hardware. An opt-in `scripts/compare_issue45_benchmarks.py` helper compares CSV medians against a user-selected reference/threshold on comparable hardware, without imposing machine-independent latency limits. Existing per-representation GUI regressions remain required, with no application mathematical grammar, renderer, or workspace schema changes. See VERIFICATION.md for qualification evidence and measured benchmark output, if available.

<!-- ISSUE46_README_UPDATED -->

**Numerical differentiation (Issue #46; Milestone 4).** The Qt-independent
`NumericalDifferentiation` service evaluates a compiled explicit `f(x)` or a
finite-valued callback at a requested finite coordinate. It uses successively
halved central finite differences with second-order Richardson extrapolation;
when the domain excludes one side, an optional three-point one-sided stencil
and extrapolation provide a **one-sided**, not a guaranteed two-sided,
derivative estimate. `DerivativeResult` reports the method, approximate value,
inter-level error estimate, final step, evaluation count, and explicit status.
The configurable sample and evaluation limits prevent unbounded mathematical
callback counts. A left/right secant comparison rejects some cusps, jumps, and
non-smooth candidates, but **finite differences cannot prove differentiability**.
The error estimate is a convergence heuristic, **not a certified error bound**;
catastrophic cancellation, oscillatory functions, narrow discontinuities,
non-finite domains, and pathological functions may yield inaccurate answers or
non-convergence. The compiled-expression overload honors current scalar
bindings, reusable function snapshots, `ans`, and RAD/DEG semantics. This
issue adds a reusable **core API only**, not `derivative(...)` expression grammar,
scientific-calculator buttons, graph overlays, new persistence fields, or
symbolic/automatic differentiation. Those UI decisions are deferred to M4-08.

<!-- ISSUE47_README_UPDATED -->

**Numerical integration (Issue #47; Milestone 4).** The Qt-independent
`NumericalIntegration` service approximates definite integrals of finite-valued
callbacks or compiled explicit `f(x)` expressions over finite intervals using
adaptive embedded Gauss–Kronrod 7/15 quadrature. Largest-estimated-error
intervals are subdivided subject to configurable evaluation, subdivision,
and depth limits. `IntegrationResult` reports a converged/failed status,
result, estimated absolute error, evaluation count, and subdivision count.
Reversed limits negate the integral; equal finite bounds return zero without
calling the integrand. Non-finite samples or throwing callbacks fail explicitly;
partial/unconverged results are not published as valid integrals. Compiled
expressions honor current variable binding snapshots, reusable function
snapshots, `ans`, and RAD/DEG semantics. Embedded-error estimates are **not
certified error bounds**; narrow singularities, discontinuities, highly
oscillatory functions, and difficult cancellation can evade detection or
convergence. This core-only issue introduces no `integral(...)` parser syntax,
scientific-calculator controls, graph overlays, or workspace changes;
user-facing access remains planned for M4-08.

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
| `ExpressionDiagnostic` | Structured parser errors: category, source offset/length, and readable message; legacy error-string compatibility. |
| `Expression` | Parsing, operator precedence, mathematical functions, constants, named-variable nodes, explicit scope validation, and repeated evaluation with per-call bindings. |
| `VariableScope` / `VariableBindings` | Qt-independent declared identifier sets and normalized finite runtime values used by the assignment owner. |
| `VariableDefinitions` | Qt-independent assignment parsing, compiled scalar definitions, dependency graph, circular-reference rejection, selective re-evaluation, undefined propagation, and bindings snapshots. |
| `FunctionScope` / `FunctionDefinitions` | Explicit compiled-function lookup, lexical positional parameters, checked calls, transitive scalar dependencies, transactional redefinition, recursion rejection, and immutable scope publication. |
| `VariableSliderScale` | Qt-free validation and mapping of finite slider ranges and steps onto bounded integer ticks, including a reachable terminal endpoint. |
| `MathExpressionInput` | Qt-independent translation of supported UTF-8 mathematical symbols and incomplete-edit hints; the Expression parser remains authoritative. |
| `GraphWorkspacePersistence` | Qt JSON version-1/2/3 workspace codec, mixed Cartesian/parametric/polar/implicit graph persistence, bounded schema and dependency validation, and deterministic definition ordering for safe round trips. |
| `GraphVariableDependencies` | Qt-independent matching of changed scalar names against direct expression and transitive function-captured dependencies. |
| `GraphViewport` | Cartesian coordinate transformations, viewport state, precision-aware panning, cursor-anchored zooming, zoom-limit stability, numerical validation, and transactional state preservation. |
| `PolarExpression` | Qt-independent polar radius/angle normalization to paired Cartesian coordinate expressions, preserving degree/radian evaluation. |
| `ImplicitContour` | Qt-independent normalization of F(x,y)=G(x,y) into a zero level set, bounded incremental marching squares, deterministic saddle disambiguation, and shared screen-space contour geometry. |
| `IntersectionDetection` | Qt-independent, tolerance-aware finite-domain Cartesian pair intersection search, bounded callback accounting, heuristic tangency refinement, and incomplete/coincident result status. |
| `GraphFeatureDetection` | Qt-independent bounded Cartesian roots, local extrema and stationary-inflection heuristics with numerical residuals and explicit incompleteness status. |
| `InequalityRegion` | Qt-independent inequality syntax normalization, comparator direction and inclusivity, plus finite-world-range/viewport clipping of vertical shade spans. |
| `ParametricSampler` | Qt-free adaptive parameter-interval sampling to shared screen-space geometry, with bounded work, invalid-point disconnection, and incremental jobs. |
| `CurveSampler` | Adaptive approximation, domain and discontinuity checks, drawing-path separation, bounded terminal refinement for steep continuous curves, and evaluation limits. |
| `CurveSamplingJob` | Incremental visible-geometry builds, bounded work per advance, retained probes, and completion reporting. |
| `CurveEvaluationBudget` | Qt-independent callback accounting shared across curve-sampling operations, with explicit exhaustion reporting. |
| `GraphRepaintScheduler` | Qt-independent navigation coalescing, generation tracking, and cancellation of obsolete deferred repaint requests. |
| `GraphGridLayout` | Qt-independent validation of grid steps and pixel-space grid positions at viewport boundaries and extreme finite centers. |
| `CurveGeometryCache` | Qt-independent, expression-identity-based cache management, lazy geometry construction, reuse, and invalidation. |

The Qt-specific `GraphWorkspacePersistence` codec validates and preserves workspace definitions and slider settings before MainWindow commits a load. The Qt-specific `MathExpressionEdit` wraps `QLineEdit` with selection-aware math insertion, documented editor-local keyboard shortcuts, graph-row navigation signals, and syntax-state properties; `MainWindow` performs immediate expression feedback without forcing curve redraw on each keystroke. The Qt-specific `VariableSliderPanel` provides interactive session-local scalar controls backed by `VariableDefinitions`; it notifies `MainWindow` of value and scope edits, and `GraphCanvas` selectively invalidates only affected curves. Slider configurations persist in version-2 graph workspaces (Issue #35). The Qt-specific `GraphCanvas` handles mouse and wheel events, grid and axis painting, coordinate tracing, and rendering the sampler's screen-space geometry. It stores generated `QPainterPath` values in `CurveGeometryCache` so ordinary repaints don't reevaluate unchanged curves. Geometry cache rebuilds advance resumable sampling jobs under per-curve-per-paint and shared callback limits. Interactive navigation coalesces refreshes and temporarily transforms completed paths; replacement geometry is published after completion. Per-curve raster caches reuse unchanged rendering, account for device scale, and preserve previews while builds are deferred. Resizes cancel stale refresh work, and bounded pixel-space grid positions prevent overflowed labels at extreme coordinates. `MainWindow` constructs the scientific, graphing, and exact-fraction interfaces programmatically; no Qt Designer `.ui` files are required.

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
│   │   ├── MathExpressionInput.hpp
│   │   ├── GraphVariableDependencies.hpp
│   │   ├── GraphViewport.hpp
│   │   ├── GraphViewport.cpp
│   │   ├── CurveSampler.hpp
│   │   ├── CurveSampler.cpp
│   │   ├── PolarExpression.hpp
│   │   ├── ImplicitContour.hpp
│   │   ├── ImplicitContour.cpp
│   │   ├── IntersectionDetection.hpp
│   │   ├── IntersectionDetection.cpp
│   │   ├── GraphFeatureDetection.hpp
│   │   ├── GraphFeatureDetection.cpp
│   │   ├── InequalityRegion.hpp
│   │   ├── InequalityRegion.cpp
│   │   ├── GraphTrace.hpp
│   │   ├── GraphTrace.cpp
│   │   ├── ParametricSampler.hpp
│   │   ├── ParametricSampler.cpp
│   │   ├── CurveGeometryCache.hpp
│   │   ├── GraphRepaintScheduler.hpp
│   │   └── GraphGridLayout.hpp
│   └── gui/
│       ├── MainWindow.hpp
│       ├── MainWindow.cpp
│       ├── GraphWorkspacePersistence.hpp
│       ├── MathExpressionEdit.hpp
│       ├── MathExpressionEdit.cpp
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
    ├── MathExpressionInputTests.cpp
    ├── MathExpressionEditorTests.cpp
    ├── ExpressionDiagnosticTests.cpp
    ├── ExpressionInlineFeedbackTests.cpp
    ├── MathKeyboardShortcutTests.cpp
    ├── GraphWorkspacePersistenceTests.cpp
    ├── VariableFunctionInteractionTests.cpp
    ├── DynamicEvaluationMatrixTests.cpp
    ├── ParametricSamplerTests.cpp
    ├── ParametricGraphTests.cpp
    ├── PolarExpressionTests.cpp
    ├── PolarGraphTests.cpp
    ├── ImplicitContourTests.cpp
    ├── ImplicitGraphTests.cpp
    ├── IntersectionDetectionTests.cpp
    ├── IntersectionGraphTests.cpp
    ├── GraphFeatureDetectionTests.cpp
    ├── GraphFeatureGraphTests.cpp
    ├── InequalityRegionTests.cpp
    ├── InequalityGraphTests.cpp
    ├── GraphRepresentationControlsTests.cpp
    ├── GraphTraceTests.cpp
    ├── GraphTracingTests.cpp
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

With GUI and testing enabled, CTest registers **52 tests from 51 executables**: thirty-one Qt-independent tests and twenty-one offscreen Qt invocations from twenty widget test executables. The coverage executable also runs separately at 2× display scale. Core-only builds register thirty-one tests:

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
| `RationalMathExpressionInputTests` | Qt-free Unicode mathematical symbol translation, parser compatibility, numerical results, and incomplete-edit heuristics. |
| `RationalMathExpressionEditorTests` | Offscreen Qt editor selection/caret semantics, bracket insertion, graph symbol toolbar, live syntax feedback, and scientific/graph integration. |
| `RationalExpressionDiagnosticTests` | Qt-independent parser error kinds, source offsets, unknown names, function failures, Unicode source-column translation, and deterministic invalid-expression sweeps. |
| `RationalExpressionInlineFeedbackTests` | Offscreen Qt real MainWindow feedback, error tooltips, Unicode positions, invalid-row isolation, and recovery without corrupting other graph rows. |
| `RationalMathKeyboardShortcutTests` | Offscreen Qt keyboard symbol/function insertion, cursor selection and native edit controls, real MainWindow graph-row navigation, row creation, and immediate parser feedback. |
| `RationalGraphWorkspacePersistenceTests` | Version-1 and version-2 JSON round trips, slider values/ranges, scalar and function dependencies, malformed-input rejection, MainWindow Save/Open and rollback protections. |
| `RationalVariableFunctionInteractionTests` | Cross-owner scalar/function dependency resolution, nested captures, immutable snapshots, rejected cycles/invalid updates, domain recovery, scope rollback and selective graph invalidation. |
| `RationalDynamicEvaluationMatrixTests` | Fixed deterministic scalar-update and evaluation matrix with independent numeric oracles, exact dependency/change reporting, unrelated-curve preservation and angle-mode transition regressions. |
| `RationalParametricSamplerTests` | Qt-free circle reference geometry, adaptive incremental sampling, shared budgets, invalid-interval and invalid-domain cases. |
| `RationalParametricGraphTests` | Qt offscreen mixed Cartesian/parametric graph rendering, editor feedback, version-3 workspace round trips, and malformed-workspace rejection. |
| `RationalPolarExpressionTests` | Qt-free circle and rose analytical cases, degree/radian coordinate conversion, negative radii, bound-variable evaluation, and bounded shared sampling. |
| `RationalPolarGraphTests` | Offscreen Qt polar row integration, cache identity/invalidation, mixed version-3 JSON persistence, rejected loads, and MainWindow Save/Open. |
| `RationalImplicitContourTests` | Qt-independent implicit equation normalization, numerical circle reference, disconnected contours, invalid cells, per-paint evaluation budgets, and deterministic incremental sampling. |
| `RationalImplicitGraphTests` | Offscreen Qt implicit row controls, shared renderer/cache isolation, version-3 mixed workspace round trips, rejected malformed equations, and real MainWindow recovery. |
| `RationalIntersectionDetectionTests` | Qt-free bounded Cartesian pair intersections, analytical crossings/tangencies, domain validation, coincidence/discontinuity handling, explicit residuals, evaluation/result limits, and dynamic variable bindings. |
| `RationalIntersectionGraphTests` | Offscreen Qt MainWindow toolbar search, analytical results, tolerance display, unsupported representation scope, invalid-row isolation, report invalidation and recovery. |
| `RationalGraphFeatureDetectionTests` | Qt-free analytical Cartesian roots, local minima/maxima, stationary inflections, cusps, discontinuities, budgets, binding snapshots and invalid numerical inputs. |
| `RationalGraphFeatureGraphTests` | Offscreen MainWindow Features action, numerical results, GUI report invalidation and recovery, and explicit representation limitations. |
| `RationalInequalityRegionTests` | Qt-independent inequalities, reversed/Unicode comparators, strict/inclusive boundaries, clipped spans and non-finite/extreme-range protection. |
| `RationalInequalityGraphTests` | Offscreen Qt inequality rows, shaded pixels, cache reuse, viewport changes, validation and version-3 workspace persistence. |
| `RationalGraphRepresentationControlsTests` | Offscreen Qt creation controls for five graph types, accessible metadata, collapsible help, narrow-sidebar usability, focus, and mixed workspace Save/Open. |
| `RationalGraphTraceTests` | Qt-free nearest-segment tracing, discontinuity separation, finite limits, interpolation, and retained parametric metadata. |
| `RationalGraphTracingTests` | Offscreen Qt inspection across Cartesian, parametric, polar (including signed radius), implicit, inequality, and feature-marker cases. |
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
- **Graphing:** Explicit Cartesian `y = f(x)` and finite-interval parametric `x(t), y(t)` are supported. Implicit and inequality graphs, parameter-aware polar/parametric hover tracing, and advanced coordinate inspection remain future work. Adaptive sampling includes bounded secondary refinement for Cartesian curves; neither representation can guarantee detection of every narrow mathematical discontinuity or high-frequency feature.
- **Viewport precision:** Viewport navigation uses double-precision coordinates and precision-aware pan/zoom calculations. Operations that cannot be represented within the configured screen-space error tolerance are rejected without modifying viewport state. Arbitrary-precision navigation is not supported.
- **Rendering:** Valid paths are cached, and rapid navigation coalesces geometry refreshes with temporary affine previews. Resize changes invalidate stale paths and obsolete timers. Grid lines/labels use bounded pixel-space positions; extremely large finite viewport centers may suppress unreliable grid markers. Invalidated paths rebuild incrementally under per-curve-per-paint and shared callback budgets; complete replacements may take multiple paints. Retained previews include finite surrounding coverage and can omit newly exposed regions after sufficiently large navigation changes. The single-call sampler can still return explicitly truncated geometry. Sampling and other mathematical callbacks remain on the GUI thread. Coalescing and callback ceilings do not guarantee frame time, input latency, or increased FPS.
- **Not yet supported:** Implicit graphs; complex arithmetic; matrices; GUI authoring of reusable functions and general scalar assignments; symbolic algebra; numerical integration and differentiation; and inequality shading.
- **Platform verification:** Linux verified; native Windows and Qt 5 builds not yet verified.

## Development and contributions

Development work, defects, enhancements, and technical improvements are tracked in the repository's [GitHub Issues](https://github.com/SpilledMilkFactory/RationalStudio/issues). Major goals are grouped through GitHub Milestones and the [public project board](https://github.com/users/SpilledMilkFactory/projects/1), rather than duplicated in this README.

Contributions and technical discussion are welcome. Review existing issues before beginning substantial work, keep mathematical algorithms separate from presentation logic where practical, and add regression tests for changes to numerical behavior.

## License

Rational Studio is licensed under the **MIT License**. See [LICENSE.MD](LICENSE.MD).

Qt, Boost, and other dependencies remain subject to their own licenses; the project's MIT License does not supersede third-party terms.
