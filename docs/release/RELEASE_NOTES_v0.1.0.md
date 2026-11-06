# Rational Studio v0.1.0 — Initial Public Release

## Overview

Rational Studio is a free and open-source scientific and graphing calculator built with C++20 and Qt Widgets. Four desktop workspaces combine expression evaluation, interactive graphs, exact fractions and numerical analysis. Each application package includes the separate `RationalCLI` calculator.

## Major features

- **Scientific:** expression evaluation, scientific functions, mathematical constants, RAD/DEG angle modes, history and input diagnostics.
- **Graphing:** Cartesian, parametric, polar and implicit graphs; supported vertical inequalities; variables and sliders; tracing; numerical intersections and significant points; JSON workspace save/load; PNG export.
- **Exact Fractions:** arbitrary-precision rational arithmetic, exact reduced fractions and terminal access through RationalCLI.
- **Advanced Math:** complex-number calculations, numerical differentiation and integration, root finding, matrix and vector operations, descriptive statistics and regression.

## Downloads and installation

| Platform | Package |
|---|---|
| Ubuntu 24.04 LTS x64 | `rationalstudio_0.1.0-1_ubuntu24.04_amd64.deb` |
| Ubuntu 26.04 LTS x64 | `rationalstudio_0.1.0-1_ubuntu26.04_amd64.deb` |
| Windows 10/11 x64 | `RationalStudio-0.1.0-Windows-x64.zip` |

Choose the Debian package matching your Ubuntu release. Install the downloaded local file with APT, which resolves its system runtime dependencies:

```bash
# Ubuntu 24.04
sudo apt install ./rationalstudio_0.1.0-1_ubuntu24.04_amd64.deb
# Ubuntu 26.04
sudo apt install ./rationalstudio_0.1.0-1_ubuntu26.04_amd64.deb
```

Launch **Rational Studio** from the application menu or run `RationalStudio`. Run `RationalCLI` for terminal arithmetic. For example, enter `1/2 1/3`, then `+` at the operation prompt to obtain `5/6`. Remove the package with `sudo apt remove rationalstudio`.

On Windows, extract the application ZIP into a writable folder and run `RationalStudio.exe` inside `RationalStudio-0.1.0-Windows-x64/`. Keep its DLLs, plugin directories and documentation together. Run `RationalCLI.exe` from a terminal in that folder. The release asset is the application ZIP itself; GitHub Actions transport archives are development downloads. To remove the portable application, close it and delete its extracted folder; saved workspaces and user preferences are separate.

## System requirements and qualification scope

Application packages target x86-64/AMD64. The Ubuntu packages use external, distribution-supplied Qt6 Core, GUI, Widgets and platform plugins, together with the GNU C/C++ runtime and glibc. Their actual `Depends` metadata is generated separately on each target OS; a package for one Ubuntu release is not a substitute for the other.

The portable Windows package targets Windows 10/11 x64 and includes Qt 6.10.3 and app-local MSVC runtime libraries. No Qt SDK or compiler is required to run it. Windows Server 2022 hosted build/testing and historical Windows 10 workstation acceptance are distinct evidence. Windows 11 has not been independently qualified on a native workstation.

Consult the attached `RELEASE_QUALIFICATION.md` for completed checks on the exact tagged packages and for pending native acceptance or owner reviews. Earlier development-package acceptance does not qualify newly generated packages. Qt 5 and other Linux distributions are outside this release's qualification scope.

## Source code and licensing

Rational Studio's first-party source is [MIT licensed](https://github.com/SpilledMilkFactory/RationalStudio/blob/v0.1.0/LICENSE.MD). `RationalStudio-0.1.0-source.tar.gz` is exported from the exact tagged Git commit.

The release also supplies `qtbase-everywhere-src-6.10.3.tar.xz` and `qtsvg-everywhere-src-6.10.3.tar.xz` for the bundled Windows Qt components. See [Qt source and replacement instructions](https://github.com/SpilledMilkFactory/RationalStudio/blob/v0.1.0/packaging/windows/QT_SOURCE_AND_RELINKING.md) and [third-party dependency review](https://github.com/SpilledMilkFactory/RationalStudio/blob/v0.1.0/THIRD_PARTY_DEPENDENCIES.md). Qt, Boost, ICU where bundled, and Microsoft runtime libraries retain their own terms. Archive availability and automated audits do not constitute legal certification or owner approval of public distribution. Exact binary/source correspondence, third-party notices and redistribution terms remain explicit owner review obligations.

## Known limitations

- On Windows at **125% display scaling**, the Type Guide and nearby mathematical controls may clip or overlap. This issue remains open; historical acceptance at 100% scaling does not establish a fix.
- Numerical methods and graphical sampling are approximate and can miss features, particularly near discontinuities or narrow structures. Exact fractions are a separate arithmetic mode.
- Windows binaries are unsigned and may produce a SmartScreen warning. Verify the source and checksum before deciding whether to run a download; keep Windows security protections enabled.
- Release procedures preserve source provenance. Byte-for-byte equality of independently rebuilt binaries has not been demonstrated.

## Verification and checksums

Download `SHA256SUMS.txt` alongside the assets. Compare each downloaded file's hash with its matching entry:

```bash
sha256sum rationalstudio_0.1.0-1_ubuntu26.04_amd64.deb
# With all assets downloaded into one directory:
sha256sum --check SHA256SUMS.txt
```

On Windows PowerShell:

```powershell
Get-FileHash .\RationalStudio-0.1.0-Windows-x64.zip -Algorithm SHA256
```

`RELEASE_MANIFEST.json` records source revision, artifact sizes/digests, build provenance and review limitations. `RELEASE_QUALIFICATION.md` records exact-release evidence and review status. Only those completed, revision-matched results apply to these binaries; historical green PR checks alone are insufficient. The checksum inventory covers both Qt modules and the manifest; the inventory does not checksum itself.

## Documentation

- [README](https://github.com/SpilledMilkFactory/RationalStudio/blob/v0.1.0/README.md)
- [User guide](https://github.com/SpilledMilkFactory/RationalStudio/blob/v0.1.0/docs/USER_GUIDE.md)
- [Historical verification report](https://github.com/SpilledMilkFactory/RationalStudio/blob/v0.1.0/VERIFICATION.md)
- [Engineering history](https://github.com/SpilledMilkFactory/RationalStudio/blob/v0.1.0/docs/ENGINEERING_HISTORY.md)
- [Third-party dependency review](https://github.com/SpilledMilkFactory/RationalStudio/blob/v0.1.0/THIRD_PARTY_DEPENDENCIES.md)
