# Rational Studio v0.1.0

**Initial public release — scientific and graphing calculator (C++20 / Qt Widgets)**

Rational Studio combines a scientific calculator, interactive mathematical graphs, exact arbitrary-precision fractions, and advanced numerical tools in a desktop application. A standalone `RationalCLI` is included with both supported distributions.

## Included in v0.1.0

- **Scientific:** expression entry, constants, trigonometric functions, RAD/DEG operation, history, and live feedback.
- **Graphing:** Cartesian, parametric, polar and implicit equations; vertical inequalities; variables and sliders; numerical tracing, intersections and features; JSON graph workspace save/open; PNG export.
- **Exact Fractions:** reduced arbitrary-precision rational arithmetic.
- **Advanced Math:** complex numbers, differentiation, integration, numerical roots, matrices, vectors, descriptive statistics, and regression.
- **Desktop usability:** keyboard navigation, accessibility metadata, persistent preferences, built-in offline help and installation/user documentation.

## Release assets

| Platform | Distribution | Requirements |
|---|---|---|
| Ubuntu 24.04 amd64 | `rationalstudio_0.1.0-1_amd64.deb` | Linux x86-64 and distro-supplied Qt6 packages (installed by the package manager). |
| Windows 10/11 x64 | `RationalStudio-0.1.0-Windows-x64.zip` | Extract the inner `RationalStudio-0.1.0-Windows-x64/` directory; launch `RationalStudio.exe`. Bundles Qt and supported MSVC runtime DLLs. |

The release also provides a **versioned first-party source archive**, **Qt Base and Qt SVG 6.10.3 corresponding-source archives** for the bundled Windows libraries, `SHA256SUMS.txt`, a source/asset manifest, and a `RELEASE_QUALIFICATION.md` with platform build/test evidence and explicit review boundaries.

See [the installation and user guide](https://github.com/SpilledMilkFactory/RationalStudio/blob/v0.1.0/docs/USER_GUIDE.md) for installation, operation, verification and removal. Use the checksums provided with **this release** to verify downloads; do not reuse checksums from a previous CI build.

## Known limitations and scope

- Windows binaries are unsigned and may trigger SmartScreen warnings; no signed installer is supplied.
- On Windows 10 at **125% display scaling**, the Type Guide and adjacent math-symbol controls may clip/overlap. This is a known, documented limitation; 100% scaling was verified previously.
- Numerical calculations and graph sampling are approximate where noted. Exact fraction arithmetic is separate from floating-point numerical analysis.
- Windows 11 and Qt 5 are not separately qualified native test environments; native Linux distribution targets **Ubuntu 24.04**, not arbitrary Linux distributions.
- Tagged, rebuilt source and auditable release records support repeatable procedures; **identical binary bytes across independent builds are not guaranteed**.

## Licensing and redistribution

First-party source is MIT-licensed. Qt, Boost, ICU (where bundled) and Microsoft runtime libraries retain their own licenses. The bundled Qt source files and relinking/replacement instructions are supplied in accordance with the release owner's recorded source-availability plan; refer to the accompanying package notices and `RELEASE_QUALIFICATION.md` for the exact publication review and evidence. Automated CI results are not a legal certification.

## Technical verification

See `RELEASE_QUALIFICATION.md` and `RELEASE_MANIFEST.json` **attached to this exact GitHub Release** for the source commit, hosted build/test results, package hashes, and named review attestation. `VERIFICATION.md` contains the historical issue-by-issue evidence and is not a substitute for a tag-matched final release audit.
