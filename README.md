# Rational Studio

[![Linux CI](https://github.com/SpilledMilkFactory/RationalStudio/actions/workflows/linux-cmake-ctest.yml/badge.svg)](https://github.com/SpilledMilkFactory/RationalStudio/actions/workflows/linux-cmake-ctest.yml)
[![Windows portable package](https://github.com/SpilledMilkFactory/RationalStudio/actions/workflows/windows-portable-package.yml/badge.svg)](https://github.com/SpilledMilkFactory/RationalStudio/actions/workflows/windows-portable-package.yml)

**Rational Studio** is a free, open-source C++20/Qt Widgets scientific and graphing calculator with exact fraction arithmetic and numerical analysis. It has four desktop workspaces: **Scientific**, **Graphing**, **Exact Fractions**, and **Advanced Math**. A separate `RationalCLI` program offers exact fractions in a terminal.

**Version:** 0.1.0 (release preparation) · **Platforms:** Linux and Windows x64 · **License:** [MIT](LICENSE.MD).

> **Release status:** The GitHub Actions build/package jobs and regression suites are qualified. Issue #66 establishes the tag-matched release qualification and publication procedure; a public v0.1.0 GitHub Release must not be assumed until it is visibly published. Until that release exists, workflow artifacts are development artifacts, not official release downloads. Consult [VERIFICATION.md](VERIFICATION.md) for platform-specific evidence and [the release procedure](docs/release/RELEASE_PROCEDURE.md) for tag, checksum, review, and publication requirements.

## Download and run

When v0.1.0 is published, use the assets under [GitHub Releases](https://github.com/SpilledMilkFactory/RationalStudio/releases). Do not assume a Release exists until an asset is visible there.

| Platform | Intended release asset | How to run |
|---|---|---|
| Ubuntu 24.04 x86-64 (Qt6) | `rationalstudio_0.1.0-1_ubuntu24.04_amd64.deb` | `sudo apt install ./rationalstudio_0.1.0-1_ubuntu24.04_amd64.deb`; launch **Rational Studio** from the app menu or run `RationalStudio`. |
| Ubuntu 26.04 x86-64 (Qt6) | `rationalstudio_0.1.0-1_ubuntu26.04_amd64.deb` | `sudo apt install ./rationalstudio_0.1.0-1_ubuntu26.04_amd64.deb`; launch **Rational Studio** from the app menu or run `RationalStudio`. |
| Windows 10/11 x64 | `RationalStudio-0.1.0-Windows-x64.zip` | Extract the **application ZIP**, then double-click `RationalStudio.exe` at the extracted folder root. Qt and the supported MSVC runtime DLLs are already included. |

Windows builds are **unsigned**; SmartScreen may display a publisher warning. Verify that you downloaded the exact release asset and checksum from this project's Releases page before deciding whether to run it. On Windows, `RationalCLI.exe` can be run from a terminal. For installation, package checksum verification, troubleshooting, and removal, see the [user guide](docs/USER_GUIDE.md).

## Use Rational Studio

| Workspace | What it does | Example |
|---|---|---|
| **Scientific** | Functions, constants, angle mode, history and live validation | `sin(pi/2) + sqrt(9)` |
| **Graphing** | Cartesian, parametric, polar, implicit and vertical inequality plots; sliders; tracing; numerical features and intersections; JSON workspace save/load and PNG export | `y = x^2`, `x^2+y^2=25`, `y >= x^2` |
| **Exact Fractions** | Reduced arbitrary-precision rational arithmetic, plus an approximate decimal display | `5/2 + 7/4 = 17/4` |
| **Advanced Math** | Complex numbers, numerical calculus, matrices, vectors, descriptive statistics and regression | Choose an operation within its panel |

Open **Help → User guide…** (`Ctrl+F1`) for built-in offline help, or **Help → Keyboard shortcuts…** (`F1`) for the shortcut reference. The complete [user guide](docs/USER_GUIDE.md) contains examples, mathematical syntax, accessibility/navigation information, limitations and troubleshooting.

## Build from source

Requires a C++20 compiler, CMake ≥ 3.16, Ninja or another generator, Boost.Multiprecision headers, and Qt Widgets development libraries for the GUI. Qt 6 is the qualified desktop configuration; Qt 5 has not been independently qualified.

**Ubuntu/Debian:**

```bash
sudo apt update
sudo apt install build-essential cmake ninja-build libboost-dev qt6-base-dev

git clone https://github.com/SpilledMilkFactory/RationalStudio.git
cd RationalStudio
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
ctest --test-dir build --output-on-failure
./build/RationalStudio
```

The same build also produces `./build/RationalCLI` (if enabled). To build/test the Qt-independent core, add `-DRATIONALSTUDIO_BUILD_GUI=OFF`. On Windows use Visual Studio 2022 x64, the Qt 6.10.3 MSVC2022 x64 kit, and Boost headers; full native build instructions are in [the user guide](docs/USER_GUIDE.md#building-on-windows) and [engineering history](docs/ENGINEERING_HISTORY.md).

## Design and verification

The Qt-independent `RationalCore` implements rational arithmetic, expression parsing, curve sampling and numerical algorithms. The Qt Widgets front end uses bounded sampling, cached graph paths and explicit input diagnostics. Mathematical outputs are approximations where indicated; graphical sampling is not a proof of continuity, and the scientific workspace uses IEEE-754 double precision.

The current Windows and Linux GUI regression suites have **73 CTests** each; core-only sanitizer tests and packaging audits have separate evidence. See [VERIFICATION.md](VERIFICATION.md) for exact historical runs and [engineering history](docs/ENGINEERING_HISTORY.md) for the implementation milestones. The Windows 125%-scaling Type Guide/math-button clipping issue remains documented and is not claimed fixed.

## Security, licensing and contributions

Rational Studio's first-party source is [MIT licensed](LICENSE.MD). Qt, Boost and other redistributed components retain their own licenses. Consult [third-party dependency review](THIRD_PARTY_DEPENDENCIES.md) and the notices provided with each package. Final source-availability and licensing review remains part of the public release gate for Issue #66; a successful CI run is not legal certification.

Report problems and discuss improvements via [GitHub Issues](https://github.com/SpilledMilkFactory/RationalStudio/issues). The project tracks work through its public [milestones](https://github.com/SpilledMilkFactory/RationalStudio/milestones) and [development board](https://github.com/users/SpilledMilkFactory/projects/1). Existing test conventions and separation of mathematical core from UI should be preserved in contributions.
