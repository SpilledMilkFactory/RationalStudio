# Rational Studio — Issue #62 Windows runtime redistribution review

**Prepared:** 2026-11-02  
**Scope:** RationalStudio 0.1.0 portable Windows x64 ZIP, MSVC 2022 / Qt 6.10.3.  
**Status:** **CRT binary provenance PASS**; **public redistribution sign-off PENDING**.  
**Basis:** Maintainer-supplied `Windows_Licensing_Output(1).txt` and `msvc-crt-package-audit.csv`; an earlier Windows package inventory. This file is a technical evidence record, not a legal opinion.

## A. Verified Microsoft Visual C++ runtime binaries

The maintainer extracted `issue62-evidence/packages/RationalStudio-0.1.0-Windows-x64.zip` on the Windows build host, located the actual directory containing `RationalStudio.exe`, and compared each packaged DLL's SHA-256 to the identically named file in the ordinary desktop (not `onecore` and not `debug_nonredist`) Visual Studio directory:

`VC/Redist/MSVC/14.44.35112/x64/Microsoft.VC143.CRT/`

The comparison **passed for all ten files**:

| DLL | Package-to-official-directory SHA-256 result |
| --- | --- |
| `concrt140.dll` | PASS |
| `msvcp140.dll` | PASS |
| `msvcp140_1.dll` | PASS |
| `msvcp140_2.dll` | PASS |
| `msvcp140_atomic_wait.dll` | PASS |
| `msvcp140_codecvt_ids.dll` | PASS |
| `vccorlib140.dll` | PASS |
| `vcruntime140.dll` | PASS |
| `vcruntime140_1.dll` | PASS |
| `vcruntime140_threads.dll` | PASS |

**What this verifies:** At comparison time, the ten DLLs inside that local ZIP were byte-for-byte identical to their counterparts in the designated Visual Studio x64 redistribution folder. It resolves the earlier incorrect `onecore` folder selection and unpopulated extraction path. The supplied CSV records PASS/FAIL results rather than the two actual hashes; preserve the matching ZIP, its checksum, and the full build-specific `issue62-evidence/msvc-redist-inventory.txt` for independent reproduction.

**What remains for the release owner:** Record acceptance of the applicable Microsoft Visual Studio Build Tools 2022 license terms and REDIST grant. Microsoft's 2022 list says unmodified files from `VC/Redist` may be distributed by appropriately licensed parties; `debug_nonredist` is prohibited. Confirm that **Build Tools edition/use rights**, rather than rights assumed for a different VS edition, cover the proposed release. Recheck on the *final GitHub-produced ZIP*, which may contain a different toolchain servicing version.

References:
- https://learn.microsoft.com/en-us/visualstudio/releases/2022/redistribution
- https://learn.microsoft.com/en-us/cpp/windows/redistributing-visual-cpp-files
- https://www.microsoft.com/licensing/guidance/Visual-Studio
- https://visualstudio.microsoft.com/license-terms/

## B. Qt 6.10.3 version and module evidence

The installed Qt kit reports `6.10.3` using `qmake -query QT_VERSION`. The following native DLLs each reported file/product version `6.10.3.0`:

- `Qt6Core.dll`
- `Qt6Gui.dll`
- `Qt6Widgets.dll`
- `Qt6Network.dll`
- `Qt6Svg.dll`

An **earlier** supplied Windows package archive contained the following eleven Qt plugins (confirm unchanged in the final hardened release ZIP):

| Plugin location | File |
| --- | --- |
| `generic/` | `qtuiotouchplugin.dll` |
| `iconengines/` | `qsvgicon.dll` |
| `imageformats/` | `qico.dll`, `qsvg.dll`, `qjpeg.dll`, `qgif.dll` |
| `networkinformation/` | `qnetworklistmanager.dll` |
| `platforms/` | `qwindows.dll` |
| `styles/` | `qmodernwindowsstyle.dll` |
| `tls/` | `qcertonlybackend.dll`, `qschannelbackend.dll` |

Qt module license selection and any corresponding third-party notices **must be checked against the actual deployed Qt 6.10.3 module/plugin build**, using the Qt SBOM and source. Qt's third-party inventory includes (among other components) zlib and PCRE2 in Qt Core, FreeType/HarfBuzz/JPEG/PNG in Qt GUI, public-suffix components in Qt Network, and XSVG in Qt SVG. Do not assume every upstream component was compiled into this binary, and do not assume the presently packaged LGPL/GPL license texts cover all third-party attribution obligations.

Official references:
- https://doc.qt.io/qt-6.10/licenses-used-in-qt.html
- https://doc.qt.io/qt-6.10/licensing.html
- https://www.qt.io/development/open-source-lgpl-obligations

## C. ICU 64.2 binary finding and notice remediation

The **actual packaged `icuuc.dll`** reports file/product version `64.2.0.0`, company **The ICU Project**. It is from the ICU 64 family, released in April 2019. This corrects the previous ambiguity about its version.

**Required packaging change:** Replace `packaging/licenses/ICU-LICENSE.txt` (copied from upstream `main`) with the unmodified **ICU4C 64.2** license from the official `release-64-2/icu4c/LICENSE` path. The historical file contains its 2019 permission text and additional third-party notices. The final ZIP must contain it at `share/doc/rationalstudio/licenses/ICU-LICENSE.txt`. Verify the installed DLL's provenance and check for applicable security updates or advisories before public distribution.

Official links:
- https://raw.githubusercontent.com/unicode-org/icu/release-64-2/icu4c/LICENSE
- https://github.com/unicode-org/icu/releases/tag/release-64-2
- https://icu.unicode.org/download/64

**Verification:** Inspect exact license text and the final ZIP after rebuilding. Version metadata alone cannot establish whether the ICU DLL was modified, nor that its source was derived directly from the official ICU tag.

## D. Other bundled runtime libraries requiring license/provenance review

The earlier Windows bundle also contained `D3Dcompiler_47.dll` and `opengl32sw.dll`. These are **not** part of the ten CRT-file hash passes above; review their origins and applicable redistribution/third-party terms separately. In particular, software OpenGL may incorporate Mesa/LLVM third-party code. A Qt deployment tool copying a DLL is not proof of the right to redistribute it.

The exact versioned **final** ZIP (not the earlier package) remains authoritative for this list.

## E. Corresponding Qt source availability / replacement rights — NOT YET SATISFIED

A versioned upstream `qt-everywhere-src-6.10.3` archive is a starting point, but matching version strings do not alone prove all bundled binary build inputs, Qt patches, plugins, and necessary third-party source are covered. Before first *public* Windows binary distribution:

1. Retain and verify the complete corresponding Qt source/build materials, including necessary third-party source and modifications.
2. Arrange a source asset or legally sufficient alternative **controlled by the release owner**; do not rely exclusively on a generic upstream link. Keep the archive out of the Git repository; a separate release asset is appropriate.
3. Record a working recipient download URL and SHA-256 in `packaging/windows/QT_SOURCE_AND_RELINKING.md` and the **actual shipped ZIP** or its accompanying release materials.
4. Preserve the ability to install and run compatible modified LGPL-covered Qt DLLs. Keep executable/library deployment instructions and all required notices accessible.
5. Confirm availability/maintenance duration and a named responsible owner.

These publication actions cannot be attested from the uploaded Windows licensing output and CSV alone.

## F. Final sign-off matrix

| Gate | Status |
| --- | --- |
| CRT 10-file source/package byte match | **PASS** for the tested local ZIP |
| Qt installed module version identified | **PASS** (6.10.3) |
| ICU runtime version identified | **PASS** (64.2) |
| ICU 64.2 license installed in newly built ZIP | **PENDING** |
| Remaining bundled DLL/plugin licenses reconciled with actual final ZIP | **PENDING** |
| MSVC Build Tools 2022 applicable redistribution license recorded | **PENDING** |
| Qt corresponding source/third-party material delivery published and verified | **PENDING** |
| Final hardened Windows CI + 73 CTests + portable GUI/CLI smoke | **PENDING final artifact confirmation** |
| Independent clean-machine acceptance of that final ZIP | **PENDING final artifact confirmation** |
| Legal / public-release authorization | **PENDING** |

**Closure distinction:** Issue #62's *technical packaging implementation* may be closed after final hardened tests if remaining public-release obligations are tracked as explicitly blocking Issue #66. Public distribution should not be signed off on the strength of this report alone.


## G. Packaging follow-up after this evidence record

The initial portable ZIP referenced above predates the ICU-license/relinking
packaging hardening; it contains optional `D3Dcompiler_47.dll` and
`opengl32sw.dll`. The next Windows package run excludes them with Qt 6.10.3
`windeployqt` flags, validates their absence, and includes the ICU 64.2
license and Qt source/relinking guide. **Do not treat this planned rerun as a
completed test or the old ZIP as the final public release.**

The two small corresponding-source modules are `qtbase` and `qtsvg`, each
version 6.10.3. See `packaging/windows/QT_SOURCE_AND_RELINKING.md` for their
official 64-character SHA-256 digests, and use a publisher-controlled public
release source download before distributing Windows binaries.
