# Rational Studio 0.1.0 — User Guide

This guide describes the **current** C++20/Qt desktop application, not the historical milestone-by-milestone development states. Rational Studio has four workspaces: Scientific, Graphing, Exact Fractions, and Advanced Math.

## Getting started

### Linux (Ubuntu 24.04, x86-64)

Once a public release is available, download `rationalstudio_0.1.0-1_amd64.deb` from [Rational Studio Releases](https://github.com/SpilledMilkFactory/RationalStudio/releases). In the directory containing the downloaded file:

```bash
sudo apt install ./rationalstudio_0.1.0-1_amd64.deb
RationalStudio
```

The package uses **system Qt6 libraries**; APT resolves its declared package dependencies. The installed app is also available in the desktop application menu. The Qt-independent terminal program runs as `RationalCLI`. To uninstall, use `sudo apt remove rationalstudio`. This `.deb` is qualified for Ubuntu 24.04; Debian derivatives and other releases may have different dependency availability.

### Windows (10/11 x64)

After the official release is published, download `RationalStudio-0.1.0-Windows-x64.zip` from [Releases](https://github.com/SpilledMilkFactory/RationalStudio/releases), not the outer GitHub Actions artifact ZIP. Extract it into a writable directory and open `RationalStudio.exe`. Keep the included DLLs, `platforms/` folder, `qt.conf`, and `share/doc/` beside the executable in their original relative positions. No separate Qt SDK, compiler or CMake installation is needed for the portable build. To use the terminal calculator, run `RationalCLI.exe` from a PowerShell or Command Prompt window opened in that folder.

This is a **portable, unsigned ZIP**—not a Windows installer. Verify the downloaded checksum against the checksum published with the official release. If SmartScreen presents an unknown-publisher warning, verify the file's origin and decide whether to proceed; do not bypass warnings for files from unknown sources. To remove Rational Studio, close the program and delete the extracted folder. Application preferences may remain in Windows user settings; they are distinct from saved graph workspaces. See [Known limitations](#known-limitations).

### Verify a release archive

Compare the SHA-256 digest of your downloaded file with the value published for that **same asset** on the release page:

```bash
# Linux
sha256sum rationalstudio_0.1.0-1_amd64.deb
# Windows PowerShell
Get-FileHash .\RationalStudio-0.1.0-Windows-x64.zip -Algorithm SHA256
```

A matching hash confirms file integrity relative to the published checksum; it is not a code-signature substitute.

## Scientific calculator

Select **Scientific** and type a mathematical expression. Press **Enter** or select the calculate button to evaluate. The keypad, expression preview, previous-answer variable `ans`, and history assist repeated calculations. Choose **RAD** or **DEG** before evaluating trigonometric expressions. Examples:

```text
2 + 3 * 4
sin(pi / 2)
sqrt(144)
2sin(pi / 4)
2^3^2
```

Scientific results use **IEEE-754 double precision**. `2^3^2` is right-associative and evaluates to 512. Domain errors and malformed expressions display diagnostic feedback. The input editor accepts supported visual mathematical symbols (see [Syntax](#expression-syntax)).

## Graphing calculator

Select **Graphing**, then use the representation buttons to create an expression. The sidebar includes a collapsible **Type guide** for brief examples.

| Representation | What to enter | Notes |
|---|---|---|
| Cartesian | `y = sin(x)` or `y = x^2` | Explicit functions of `x`. |
| Parametric | `x(t) = cos(t)` and `y(t) = sin(t)` | Supply a finite `t` minimum and maximum. |
| Polar | `r = 2cos(theta)` or `r = 2cos(θ)` | Supply finite angular bounds matching RAD/DEG mode. |
| Implicit | `x^2 + y^2 = 25` | Bounded sampled zero contour; may miss fine detail. |
| Inequality | `y > x^2` or `y <= x^2` | Vertical regions only; strict boundaries are dashed and inclusive boundaries solid. |

Drag the graph to pan and use the mouse wheel to zoom around the cursor. Toolbar buttons provide zoom, **Home** (reset viewport), **Intersections** and **Features**. Hover near a completed visible curve to inspect approximate sampled coordinates; parametric/polar readouts include parameter information, and implicit readouts include a residual when available.

**Intersections** inspects pairs of visible Cartesian functions over the current x-range. **Features** inspects roots, local extrema and stationary-inflection candidates for visible Cartesian functions. Both are numerical, bounded, and heuristic; results may be incomplete near discontinuities, tangencies or very fine features. Other curve representations are not accepted by those toolbar actions.

### Variables, sliders and reusable functions

Use the slider controls to add a named scalar (for example `a`) and plot `y = a * sin(x)`. Adjust a slider to update dependent curves. Built-in reserved names (`x`, `ans`, constants and function names) cannot be used as user scalar variables. Workspace files can retain scalar assignments and reusable function definitions; the GUI does not provide a full authoring panel for arbitrary named functions and scalar assignment sources. Do not assume unsupported identifiers will create variables automatically.

### Save, open and export

**Save** writes a versioned JSON graph workspace (expressions, appearance, viewport, angle mode and supported definitions/sliders). **Open** validates the file before replacing the current graph. **PNG** exports an image of the current graph; it is not an editable workspace. Keep JSON workspaces separately if you want to continue editing or reproducing analysis. Older compatible workspace formats are supported; invalid or corrupted files are rejected.

## Exact Fractions

Select **Exact Fractions**, enter a numerator/denominator pair in each operand field (for example `5/2` and `7/4`), choose **Add**, **Subtract**, **Multiply**, or **Divide**, and select **Calculate**. Results are reduced to lowest terms (example: `5/2 + 7/4 = 17/4`). Numerators and denominators are arbitrary-precision integers, limited in practice by memory and time. The decimal representation is **approximate**, not an additional exact decimal number. Zero denominators and division by zero are invalid.

`RationalCLI` provides a separate interactive terminal calculator. Enter two fractions separated by a space, then the arithmetic operator when prompted; for instance input `1/2 1/3` followed by `+` produces `1/2 + 1/3 = 5/6`.

## Advanced Math

The **Advanced Math** tab contains **six** panels: **Complex**, **Calculus**, **Matrices**, **Vectors**, **Statistics**, and **Regression**. Select the relevant operation, fill in its labeled fields and click **Calculate**. Calculus performs approximate numerical differentiation, integration and root finding; statistics/regression operate on finite numerical datasets. Invalid input, domain problems and numerical limitations are reported in the output panel. These operations do **not** add new expression-language commands to the Scientific tab, and their results are not arbitrary-precision or symbolically exact.

## Expression syntax

| Feature | Example / meaning |
|---|---|
| Arithmetic | `+`, `-`, `*`, `/`, `%` (floating remainder) |
| Exponentiation | `2^3`, right-associative as in `2^3^2` |
| Factorial | `5!` (nonnegative integer domain) |
| Grouping | `(2+3)*4` |
| Constants | `pi`, `e`, `tau` |
| State | `ans` (previous scientific answer), `x` (Cartesian graph variable) |
| Trigonometry | `sin`, `cos`, `tan`, `asin`, `acos`, `atan`, `sec`, `csc`, `cot` |
| Hyperbolic | `sinh`, `cosh`, `tanh` |
| Roots/logarithms | `sqrt`, `cbrt`, `ln`, `log`, `log10`, `log2`, `exp` |
| Other | `abs`, `floor`, `ceil`, `round`, `sign`, `deg`, `rad` |
| Multi-argument | `min`, `max`, `pow`, `atan2`, `hypot`, `mod` |

Function arguments use parentheses, such as `hypot(3,4)`; commas separate multiple arguments. Supported visual-input symbols include `π`, `√(…)`, `²`, `³`, `×`, `÷` and `−`. Not all conventional handwritten notation or implicit multiplication patterns are supported. The graph representation editor interprets `y = ...` etc. separately from the general Scientific expression parser. Use the application's inline feedback when an input is incomplete or invalid.

## Keyboard and accessibility

Open **Help → Keyboard shortcuts…** or press **F1** for the full in-app shortcut list. Open **Help → User guide…** with **Ctrl+F1** for a quick offline guide.

| Shortcut | Action |
|---|---|
| `Ctrl+1` / `2` / `3` / `4` | Scientific / Graphing / Exact Fractions / Advanced Math tabs |
| `Ctrl+Tab` / `Ctrl+Shift+Tab` | Next / previous tab |
| `Ctrl+L` / `Ctrl+Shift+L` | Focus current input / graph expression |
| `Ctrl+N` | Add Cartesian graph row (Graphing) |
| `Ctrl+S` / `Ctrl+O` | Save / open graph workspace (Graphing) |
| `Ctrl+Shift+E` | Export graph PNG (Graphing) |
| `Ctrl+0` | Reset graph viewport (Graphing) |
| `Ctrl+Shift+I` / `Ctrl+Shift+F` | Intersections / Features (Graphing) |
| `Alt+P`, `Alt+R`, `Alt+2/3` | Insert π / root / exponent symbols in an expression editor |
| `Alt+S/C/T` | Insert sin/cos/tan templates in an expression editor |
| `Alt+Up/Down`, `Ctrl+Enter` | Navigate graph rows / append a graph row |

Tab/Shift+Tab change control focus. Graph analysis also has text reports. Qt accessibility names, focus indicators and keyboard access are implemented, but compatibility with every screen reader and scaling configuration is not certified; some desktop/window-manager shortcuts may take precedence.

## Preferences

Use the **Preferences** menu to select light/dark appearance, graph grid/axes visibility and whether the last active tab is restored. The app remembers supported preferences across sessions. Graph JSON workspaces are separate from application preferences; opening a workspace may update the current angle mode.

## Known limitations

- Scientific and most advanced numerical results are floating-point approximations; they are not symbolic expressions or unlimited-precision decimals.
- Graph drawing, tracing, feature detection, inequalities and implicit contours use finite, bounded sampling. Very narrow gaps, steep or high-frequency features, discontinuities and certain roots or tangencies can be missed. Analysis reports are numerical estimates, **not proofs**.
- Inequality shading handles **vertical comparisons** only; general Boolean and implicit regions are not provided.
- Built-in advanced math panels do not expose their operations as general expression-parser syntax. Full GUI authoring of reusable functions/assignment definitions is not present.
- On a tested Windows 10 1920×1080 system with **125% scaling**, the Type Guide and some mathematical symbol buttons can clip or overlap. At **100% scaling** the tested controls fit. This limitation is documented, not claimed fixed.
- Windows ZIPs are unsigned and lack an installer; Windows 11, Qt 5 and every Linux distribution have **not** been independently qualified.
- A saved PNG cannot be reopened as an editable graph workspace; save JSON for continued work.

## Troubleshooting

**Windows cannot find Qt DLLs or the platform plugin:** Extract the complete release ZIP; don't run the GUI from the archive preview or move the `.exe` out of its directory. Ensure `platforms/qwindows.dll` remains next to the executable under `platforms/`. Avoid mixing DLLs from other Qt versions.

**Linux does not start because a Qt dependency is missing:** Install the `.deb` using `apt install ./file.deb` rather than unpacking it manually. Check package-manager errors. The released `.deb` targets Ubuntu 24.04 with Qt6.

**Expression shows an error or no curve:** Check parentheses, function spelling, supported operators and variable declarations. Some graph values are undefined or outside the visible viewport. Use **Home** to reset the view.

**Graph output differs from the expected mathematics:** Check RAD/DEG mode and finite parameter bounds, and remember that curves and numerical features are sampled approximations.

**UI controls overlap at 125% scaling on Windows:** The known layout issue remains; 100% scaling was the verified workaround on the reported laptop.

**Report a reproducible defect:** Include Rational Studio version, operating system, scaling percentage, the exact input/workspace, steps to reproduce and expected/actual behavior in a [GitHub issue](https://github.com/SpilledMilkFactory/RationalStudio/issues). Avoid publishing private datasets or credentials in reports.

## Building on Windows

For developers compiling instead of using the portable distribution, install Visual Studio 2022 C++ Build Tools, CMake, the matching Qt 6.10.3 MSVC2022 x64 desktop kit, and Boost.Multiprecision headers. From a Visual Studio developer terminal (adjust paths):

```powershell
cmake -S . -B build-win -G "Visual Studio 17 2022" -A x64 `
  -DCMAKE_PREFIX_PATH="C:\Qt\6.10.3\msvc2022_64" `
  -DRATIONALSTUDIO_BOOST_INCLUDE_DIR="C:\dev\vcpkg\installed\x64-windows\include"
cmake --build build-win --config Release --parallel 4
ctest --test-dir build-win -C Release --output-on-failure
```

A development build may need Qt's `bin` directory on `PATH`; the official portable ZIP is separately staged with its runtime dependencies. See [VERIFICATION.md](../VERIFICATION.md) for the exact Windows 10 x64 qualification and its display limitation.

## Licensing and support

Rational Studio's own code is [MIT licensed](../LICENSE.MD). Distributed Qt, Boost, ICU and Microsoft runtime components have separate terms. Relevant notices are included with release packages; consult [THIRD_PARTY_DEPENDENCIES.md](../THIRD_PARTY_DEPENDENCIES.md) for the detailed review and release gates. Source downloads and releases are linked from the [project's GitHub page](https://github.com/SpilledMilkFactory/RationalStudio).
