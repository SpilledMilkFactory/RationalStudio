# Rational Studio v0.1.1 — Release Tooling Update

This release incorporates the merged cross-platform Windows ZIP directory validation fix and updates the README. Directory entries using Windows separators are classified consistently across hosts, while unsafe paths, directory entries carrying file data, altered notices and incorrect payload hashes remain rejected.

**Release version: v0.1.1. Application/package version: 0.1.0.** Per the owner's request, this draft uses the newly built packages from PR #124's completed qualification batch, without new GitHub Actions runs. The binaries have not been relabeled as application version 0.1.1. The release source archive includes the merged tooling fix and README update; the manifest identifies the exact package-build and release-source revisions separately.

## Downloads

| Target | Package |
|---|---|
| Ubuntu 24.04 LTS AMD64 | `rationalstudio_0.1.0-1_ubuntu24.04_amd64.deb` |
| Ubuntu 26.04 LTS AMD64 | `rationalstudio_0.1.0-1_ubuntu26.04_amd64.deb` |
| Windows 10/11 x64 | `RationalStudio-0.1.0-Windows-x64.zip` |

On Ubuntu, install the matching local package with `sudo apt install ./PACKAGE.deb`, then launch `RationalStudio` or `RationalCLI`. Ubuntu packages use separately resolved system Qt6 dependencies. On Windows, extract the ZIP and run `RationalStudio.exe`; keep its DLLs and plugin directories together. `RationalCLI.exe` is included. Windows 11 is an intended target, not an independently qualified native workstation.

## Source, verification and limitations

The first-party source uses the MIT license. The two Qt 6.10.3 module source archives accompany the Windows distribution. See [source/replacement instructions](https://github.com/SpilledMilkFactory/RationalStudio/blob/v0.1.1/packaging/windows/QT_SOURCE_AND_RELINKING.md) and [third-party review](https://github.com/SpilledMilkFactory/RationalStudio/blob/v0.1.1/THIRD_PARTY_DEPENDENCIES.md). Source hashes and technical audits do not constitute legal certification; owner distribution reviews remain separate.

Use `SHA256SUMS.txt` to verify downloads. `RELEASE_MANIFEST.json` and `RELEASE_QUALIFICATION.md` identify package provenance, the reused successful checks, native Ubuntu 26.04 evidence and pending acceptance/review limitations. Windows remains unsigned and the known 125% scaling issue remains open. Numerical methods and graph sampling can be approximate. No byte-for-byte reproducibility across independent builds is claimed.

Documentation: [README](https://github.com/SpilledMilkFactory/RationalStudio/blob/v0.1.1/README.md), [user guide](https://github.com/SpilledMilkFactory/RationalStudio/blob/v0.1.1/docs/USER_GUIDE.md), [verification history](https://github.com/SpilledMilkFactory/RationalStudio/blob/v0.1.1/VERIFICATION.md), [engineering history](https://github.com/SpilledMilkFactory/RationalStudio/blob/v0.1.1/docs/ENGINEERING_HISTORY.md).
