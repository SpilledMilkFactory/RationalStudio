# Rational Studio — Third-party dependency and distribution review

**Issue:** #64 / Milestone 5 · **Scope:** `0.1.0-1` Linux `.deb` using external system Qt6 on Ubuntu 24.04.
**Status:** Technical audit implemented; **legal/release-source review remains required**. This document is not a legal opinion.

## Direct dependency inventory

| Component | How used | License / origin | Distribution in our `.deb` | Required action |
|---|---|---|---|---|
| Rational Studio | First-party executable and CLI | MIT, `LICENSE.MD` | Both compiled binaries; complete MIT text in `usr/share/doc/rationalstudio/copyright` | Keep original copyright and MIT text; source releases must preserve it. |
| Qt6 Core, GUI, Widgets | Qt Widgets UI, platform interfaces | Open-source Qt6 has LGPLv3/GPLv3 choices; exact distro package notices govern | ELF dynamic `DT_NEEDED` libraries, supplied by the OS, not copied | Prominent Qt notice, LGPLv3 + GPLv3 texts, appropriate corresponding-source and library replacement rights; verify deployed Qt version. |
| Qt6 platform plugins (`qt6-qpa-plugins`) | Dynamically loaded Qt platform integration | Upstream Qt and distribution component-specific licenses | `Depends` metadata; installed separately by OS | Verify dependency, plugin origin and distro copyright notices; do not silently bundle plugins. |
| Boost.Multiprecision (`cpp_int.hpp`) | Arbitrary-precision integer arithmetic compiled into core | Boost Software License 1.0 | Headers used at build time; no Boost shared library bundled | Preserve applicable source license notices; include BSL-1.0 text for transparency. |
| GNU C/C++ runtime and glibc | OS shared runtime | Runtime licenses and GCC Runtime Library Exception where applicable | Dynamic system libraries (`libc6`, `libstdc++6`, `libgcc-s1`) | Review distribution-specific notices if redistributing binaries; do not vendor without re-audit. |
| CMake, Ninja, compiler, `dpkg-deb` | Build/package tools | Their respective upstream licenses | Not shipped as runtime payload | Do not mislabel tools as redistributed application dependencies. |
| `accessories-calculator` | Desktop icon *name* | System icon theme | No icon binary in package | Re-audit if a custom icon or font is ever bundled. |

### Sources and notices

- Qt licensing and obligations: https://doc.qt.io/qt-6/licensing.html and https://www.qt.io/faq/qt-open-source-licensing
- Boost BSL-1.0: https://www.boost.org/LICENSE_1_0.txt
- GNU LGPLv3: https://www.gnu.org/licenses/lgpl-3.0.html
- GNU GPLv3: https://www.gnu.org/licenses/gpl-3.0.html
- GCC Runtime Library Exception: https://www.gnu.org/licenses/gcc-exception-3.1.html
- Debian machine-readable copyright guidance: https://www.debian.org/doc/packaging-manuals/copyright-format/1.0/

The license files in `packaging/licenses/` are copies of the unmodified license terms. The project also installs a separate `THIRD_PARTY_NOTICES.txt`; it deliberately **does not replace** `LICENSE.MD` with a claim that the whole program is LGPL or Boost licensed.

## Binary and source boundaries

- `RationalStudio` links to **system** `libQt6Core`, `libQt6Gui`, and `libQt6Widgets`, not static Qt archives. Check this from actual `readelf -d` NEEDED metadata, not inferred CMake settings alone.
- Qt libraries and platform plugins are installed by the target OS as separate packages. The application `.deb` does **not** contain `.so`, `.a`, or plugin binaries. The audit rejects newly bundled libraries and fonts/icons pending review.
- `RationalCLI` is a separate first-party binary using the Qt-free `RationalCore`; it still uses Boost header templates.
- `dpkg-shlibdeps` discovers linked runtime dependencies; an explicit `qt6-qpa-plugins` dependency covers dynamically loaded Qt platform plugins. Package-manager dependency metadata does **not** itself prove licensing compliance.
- The audit inventories the available source files and direct links. Full transitive licenses of an installed distribution's Qt packages, generated resources, system icon themes, and future dependencies require reinspection of the *actual* target image and package versions.

## Required LGPL compliance review before public release

These are **release obligations to substantiate**, not automatically satisfied just because this repository is MIT-licensed or a test passes.

1. **Exact licensed parts.** Record the real Ubuntu 24.04 Qt module/plugin binary package names and versions shipped on the installation, and inspect their copyright documents. Verify that the modules used are eligible for the intended LGPL option and that no GPL-only Qt addons have been introduced.
2. **Notice and license texts.** Confirm that the package prominently states its Qt use and includes intact LGPLv3 and GPLv3 terms. Both are installed under `/usr/share/doc/rationalstudio`.
3. **User modification/replacement rights.** Confirm users can install interface-compatible modified LGPL Qt shared libraries and run Rational Studio using them. Do not apply DRM, signed-library restrictions, additional EULA prohibitions, or relinking restrictions. Do not assume an offscreen Qt test establishes this right.
4. **Corresponding Qt source availability.** Determine and document a legally sufficient source/offer arrangement under the exact relevant LGPL and Qt licensing terms. Qt's own compliance guidance cautions that pointing only to the upstream Qt Project website does not fulfill every source-availability obligation. A maintainer-controlled archival copy or adequate written offer may be appropriate; the planned distribution arrangement must be reviewed and recorded before release.
5. **Changes to Qt.** If Qt libraries are patched or redistributed later, provide their complete corresponding source and modification notices as required; inspect any independently licensed third-party Qt subcomponents.
6. **Source and notices continuity.** Preserve first-party MIT copyright, Boost notices, source release license files, and the exact package notice/terms at every versioned distribution. Do not claim a complete software bill of materials from the first-party direct-dependency audit alone.
7. **Separate platform review.** Windows and other bundled/static Qt distributions require a new license and source-delivery assessment; the Linux system-Qt decision must not be copied as proof of compliance for those packages.

**Release sign-off requires a recorded owner, a reviewed Qt-source/relinking delivery plan, and a package-specific dependency inventory.** Do not label this milestone *legally certified* based on automated inspection. If legal risk is material, obtain qualified open-source licensing counsel.

## Qualification and evidence

Run source and package audits with:

```bash
python3 scripts/compliance/test_audit_licenses.py -v
python3 scripts/compliance/audit_licenses.py --root .
python3 scripts/compliance/audit_licenses.py --root . --deb path/to/rationalstudio_0.1.0-1_amd64.deb \
    --json audit.json
```

Use `RATIONALSTUDIO_JOBS=4 bash scripts/verify_issue64.sh` for a fresh Qt6 Release build, all **71** established CTests, real CPack Debian package, extracted-GUI launch, source provenance, notice/term matching, and ELF dependency checks.

The source-only mode verifies *declared files*, not an installed package. The package mode uses the actual `.deb` and rejects a package missing notices, modified license texts, unreviewed bundled shared libraries, or unreviewed binary assets. These checks are technical controls; they do **not** determine whether the entire license obligation has been legally satisfied.

## Issue #62: Windows portable distribution (separate bundled-library review)

**Scope:** Windows 10 x64 / MSVC 2022 / Qt 6.10.3 portable ZIP. Unlike the
Linux `.deb`, the Windows ZIP **bundles** Qt6 DLLs, the `platforms/qwindows.dll`
plugin, and app-local Microsoft Visual C++ runtime libraries. The package is
produced using `scripts/verify_issue62.ps1`, with Qt deployment by the Qt SDK's
`windeployqt.exe --release --no-compiler-runtime`; MSVC CRT files are copied
separately from the official `VC/Redist/MSVC/.../x64/Microsoft.VC143.CRT`
redistributable directory. Boost.Multiprecision remains a header-based build
input; it is not packaged as an independent DLL.

The ZIP contains `share/doc/rationalstudio/THIRD_PARTY_NOTICES.txt`, the exact
first-party `LICENSE.MD`, and the LGPLv3, GPLv3, and Boost BSL-1.0 license texts.
The source archive ZIP inspector checks notices byte-for-byte against their
source counterparts, Qt and CRT DLL names, platform plugin placement, safe ZIP
paths, and per-file SHA-256 inventory. It emits a complete non-Qt DLL list for
manual legal review. It cannot establish full SBOM/license compliance from the
filenames alone.

**Separate release gate:** Before distributing the Windows ZIP, identify every
bundled Qt module/plugin and any transitive library or asset using the *exact*
packaged binary file inventory; confirm eligibility for the intended license
option; make the legally sufficient corresponding source available or provide
an adequate offer, with installation information/relinking and notices as
required; and check Microsoft's license terms for app-local CRT redistribution.
Do not distribute debug/non-redistributable compiler files. Prefer a servicing
strategy for app-local CRT updates. Record a named reviewer and evidence before
publishing any GitHub Release. The source-only and package audits are technical
checks, **not legal certification**.

The standalone app still requires an independent test on a Windows host with
**no Qt SDK and no MSVC Build Tools**, including GUI startup, CLI arithmetic,
graph/save/load, and Windows scaling. The known Windows 125% scaling layout
issue remains open and is not claimed fixed by the packaging change.

References:
- Qt Windows deployment: https://doc.qt.io/qt-6.10/windows-deployment.html
- Qt LGPL requirements: https://www.qt.io/development/open-source-lgpl-obligations
- MSVC runtime local deployment: https://learn.microsoft.com/en-us/cpp/windows/choosing-a-deployment-method
- MSVC redistributable file licensing: https://learn.microsoft.com/en-us/cpp/windows/redistributing-visual-cpp-files
