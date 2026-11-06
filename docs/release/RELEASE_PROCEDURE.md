# Rational Studio — versioning and reproducible release procedure (Issue #66)

**Status:** Procedure and candidate-assembly tooling. **This document does not authorize a public release.** The release owner must review the exact packages, evidence and licensing/source-availability gates before publishing. This issue creates repeatable **source-revision** builds; identical bytes from independently rebuilt `.deb`/ZIP packages are *not* promised.

## Source/version contract

- `CMakeLists.txt` `project(RationalStudio VERSION X.Y.Z LANGUAGES CXX)` is the **only canonical application version**. It supplies the CMake package version, the Windows portable ZIP naming, `RationalCLI --version`, and the GUI application/About version.
- First public release tag is **`v0.1.0`**, an **annotated** Git tag on the reviewed, merged source commit. Do **not** create a tag before Issue #66 is merged and all subsequent release fixes have landed. Never move or reuse a published tag.
- Debian package version is `<project_version>-1` (`rationalstudio_0.1.0-1_ubuntu24.04_amd64.deb`). CPack initially produces `rationalstudio_0.1.0-1_amd64.deb`; the assembler names each release copy with `_ubuntu24.04_amd64.deb` or `_ubuntu26.04_amd64.deb`, retaining bytes and internal metadata. The suffix `-1` is the packaging revision, not a new application feature version.
- Windows application ZIP is `RationalStudio-0.1.0-Windows-x64.zip`. Its **inner directory is also** `RationalStudio-0.1.0-Windows-x64/`. Do not upload the GitHub Actions transport ZIP as the user-facing application ZIP.
- A release always identifies the exact commit SHA-1, source-archive hash, binary archive hashes, Qt-source archive hashes and qualification-run identifiers. Do not upload binaries built from a different commit or reuse previous issue artifacts.
- Historical build/test details remain in `VERIFICATION.md`; historical feature engineering notes remain in `docs/ENGINEERING_HISTORY.md`; neither is a mutable, version-specific release manifest. Public release notes reside in `docs/release/RELEASE_NOTES_v0.1.0.md` and are copied to the assembled release assets.

## 1. Merge and tag exact source revision

After merging this issue to `main` and ensuring the usual GitHub Actions checks pass, make a **fresh local checkout** with no modified or untracked sources:

```bash
git fetch origin --tags
git switch main
git pull --ff-only origin main
git status --short
# Review and record: git rev-parse HEAD
# Only after the final source is accepted:
git tag -a v0.1.0 -m 'Rational Studio v0.1.0 — Initial Public Release'
python3 scripts/release/prepare_release.py --tag v0.1.0 check-tag
# Compare tag commit to main, then push the tag (manual action):
git rev-parse v0.1.0^{}
git push origin v0.1.0
```

Do **not** run the tag-check on the ZIP handed to ChatGPT; that is a source export without a `.git` directory. It must run in the maintainer's real Git checkout. If the tag has already been published and a fix is needed, cut a new release/tag; never silently retag. CI users may clone a detached tag checkout and run the verifier there (no untracked source files allowed).

## 2. Rerun qualification from exactly the tagged source

The existing five GitHub Actions workflows support `workflow_dispatch`. Explicitly dispatch **each** against `v0.1.0` using GitHub's workflow UI (select the tag), or GitHub CLI:

```bash
gh workflow run linux-cmake-ctest.yml --ref v0.1.0
gh workflow run linux-debian-package.yml --ref v0.1.0
gh workflow run linux-license-compliance.yml --ref v0.1.0
gh workflow run linux-gui-smoke.yml --ref v0.1.0
gh workflow run windows-portable-package.yml --ref v0.1.0
```

**Required results:** Linux Debug 73/73, Linux Release 73/73, Linux core ASan/UBSan 43/43, Linux Debian package 73/73 plus CPack/audit/extracted smoke, Linux licensing/dependency audit 73/73 plus notices/ELF/source checks, Ubuntu offscreen/Xvfb smoke 2 registered CTests plus real Xvfb execution, and Windows MSVC 2022/Qt 6.10.3 Release package 73/73 plus ZIP validation/extracted GUI/CLI smoke. Also verify the separate native Windows clean-machine test and native Ubuntu package installation/removal as release-owner gates.

For **every job**, inspect the *actual* successful GitHub Actions run, job conclusion, **run `head_sha`**, logs, environment, CTest count, warning checks, and archived evidence. Each `head_sha` must equal `git rev-parse v0.1.0^{}`. A successful run on `main` at a *different* SHA is not sufficient. The issue #65 merge's previous green checks remain historical, not a substitute for fresh final-tag runs. CI may upload additional archive layers; extract the nested original package and its original package hash, not the outer transport archive. Keep archived run evidence and URLs in your release sign-off record.

Retrieve the actual `.deb` from the tagged **Linux Debian Package** run, the actual portable inner ZIP from the tagged **Windows Portable Package** run, and inspect and retain `SHA256` values before packaging the final bundle. The `prepare_release.py` assembler *re-runs* the Debian and Windows package validators and **requires their original tagged-run SHA-256 values in the evidence record to match the supplied binary assets**. The release owner must independently verify that those recorded digests really came from the specified successful run; the assembler cannot independently query GitHub. Do not fabricate run results to satisfy the template.

## 2a. Build Ubuntu 26.04 natively from the same tagged commit

Use a native Ubuntu 26.04 AMD64 host. Verify `/etc/os-release`, architecture,
compiler, CMake and Qt versions; record them with the exact tag SHA. Perform a
fresh Release configure with `RATIONALSTUDIO_ENABLE_LINUX_PACKAGING=ON`,
`CMAKE_INSTALL_PREFIX=/usr`, GUI/CLI and testing enabled. Build, run all 73
CTests, generate the DEB with CPack and run both `check_deb.py` and
`audit_licenses.py --root . --deb PACKAGE --json REPORT`. Retain configure,
build, CTest/JUnit, package metadata/contents, ELF dependencies and hashes.
`scripts/verify_issue61.sh` provides the existing fresh-build sequence and
extracted GUI startup check; run the separate licensing audit as well.

Verify the extracted RationalCLI arithmetic and GUI. Independently test APT
installation/dependency resolution, native desktop/graph/save/load/export,
and removal on a clean Ubuntu 26.04 machine where available. Mark unavailable
clean-machine acceptance **PENDING**. Offscreen/Xvfb or extracted execution on
the build host must be labeled as such. Never install/remove packages on the
developer's host merely to simulate a clean machine.

Populate `ubuntu_26_04` separately from the seven hosted checks: exact commit,
native Release status, host/OS/toolchain, CTests, original package digest,
actual `Depends`, validation/audit results, runtime status, public-safe evidence
references and acceptance limitations. The Ubuntu 24.04 Actions run cannot
qualify this package. The older archived Ubuntu 26.04 DEB is a reference only:
it lacks the required guide/notices and must never be uploaded.

## 3. Prepare matching corresponding Qt source assets

This Windows distribution bundles Qt 6.10.3 libraries. Complete the Qt Base/Qt SVG provenance and third-party notice review specified in `THIRD_PARTY_DEPENDENCIES.md` and `packaging/windows/QT_SOURCE_AND_RELINKING.md`, including ICU 64.2, individual platform plugins, corresponding source and any modifications, and LGPL library replacement. Qt-source assets are **not** committed to Git:

```bash
python3 scripts/packaging/prepare_qt_source_assets.py --output /path/to/qt-source-assets
python3 scripts/packaging/prepare_qt_source_assets.py --output /path/to/qt-source-assets --offline
```

Use the existing local Qt module archives when available. Copy only the two named module files to a staging directory and verify the original Qt-only inventory before invoking the helper offline. Never recursively copy `qt-everywhere-full` or include a full-source archive. The helper writes `SHA256SUMS.txt` in its output directory; do not point it at a read-only archive store. The assembler copies only the two whitelisted module assets. If publishing a Qt-only inventory separately, name it `QT_SOURCE_SHA256SUMS.txt`; the final top-level inventory covers all assets.

The prepare helper checks the expected upstream Qt 6.10.3 SHA-256 values for *both* source modules. Those hashes do not independently prove correspondence to a *specific* uploaded Qt binary: record that comparison as a separate human release review. **Do not publish** until a valid corresponding-source arrangement and the necessary notices/build/replacement instructions are in place, including relevant transitive source where required.

## 4. Record technical evidence and owner review status

Copy `docs/release/qualification-template.json` to a working location **outside the repo** (such as `/tmp/rationalstudio-release/qualification.json`). Replace every placeholder with evidence for **the exact tag and packages**. Check all workflow run URLs and SHA values in the GitHub UI/API, and capture actual toolchains; the template's expected CTest counts are not themselves proof of a run.

Each hosted job records the successful conclusion, workflow name, exact
revision, runner/toolchain, CTest count, result summary and public-safe evidence
references. Packaging jobs additionally record their numeric Actions artifact
ID and original inner-package SHA-256. Inspect actual run logs/API results;
the assembler validates supplied evidence and does not independently contact
GitHub. Preserve detailed raw provenance privately; avoid private paths or
credentials in public evidence references.

For **final approval mode**, fill out every human review field with a named reviewer, timestamp and an actual **specific evidence reference** for the Linux Qt distribution, exact Windows Qt modules/plugins and third-party terms, ICU, the official CRT redistribution basis, clean Linux and Windows runs, Qt corresponding-source/relinking, and final licensing approval. The tool rejects missing/pending fields and requires this literal human approval statement:

> I have reviewed the exact release assets and approve public distribution.

The assembler validates the record's **structure** and SHA agreement; it does not contact GitHub to attest run success or determine whether the stated distribution rights are legally sufficient. Preserve the signed/approved raw review record and review artifacts privately with the release evidence. Do not present its existence as independent legal certification.

For an **unpublished draft candidate**, leave `reviewer`, `reviewed_utc` and
`approval_statement` empty unless there is genuine reviewer metadata to retain;
keep all unresolved reviews explicitly `PENDING`. Use `--draft-candidate`.
This mode permits pending human review and clean-machine acceptance, but still
requires all seven successful hosted jobs, a native Ubuntu 26.04 Release build,
73 passing native CTests, extracted GUI/CLI smoke, both package validators,
licensing audits, source revision agreement and every artifact hash. It never
creates approval evidence. Final mode (the default) still fails closed for
missing approval, pending reviews or pending Ubuntu 26.04 native acceptance.

## 5. Assemble release assets offline

Run this on an Ubuntu/Debian host with `dpkg-deb`, Python 3, Git and the complete tracked repository checkout at the tag; the source directory **must be clean**. Use **outside-repository paths** and an output directory that does not yet exist:

```bash
python3 scripts/release/prepare_release.py \
  --root . --tag v0.1.0 assemble \
  --draft-candidate \
  --deb /path/to/ubuntu24/rationalstudio_0.1.0-1_amd64.deb \
  --deb-ubuntu26 /path/to/ubuntu26/rationalstudio_0.1.0-1_amd64.deb \
  --windows-zip /path/to/RationalStudio-0.1.0-Windows-x64.zip \
  --qt-source-dir /path/to/qt-source-assets \
  --evidence /path/to/qualification.json \
  --output /path/to/RationalStudio-v0.1.0-release-assets
```

The assembler refuses a dirty checkout, mismatched/missing/lightweight Git tag, untracked required files, conflicting versions, mismatched run commits, missing/pass-count evidence, missing or mismatched tagged-run package SHA-256 values, incomplete reviewer sign-off in final mode, incomplete/invalid Windows or either Ubuntu package (including licensing audits), mismatched Qt source checksums, and an existing output directory. It exports a first-party source `tar.gz` **directly from the immutable Git commit object** and writes these accompanying files:

- `RELEASE_NOTES.md` — complete, versioned public release notes.
- `RELEASE_QUALIFICATION.md` — source commit and reviewer-supplied hosted test/release gate evidence.
- `RELEASE_MANIFEST.json` — version, commit, platform checks and SHA-256/size of every included payload.
- `SHA256SUMS.txt` — SHA-256 for the `.deb`, portable ZIP, Git source tarball, Qt source archives and release-record files, including the manifest itself.

The machine check is structural/cryptographic; it is **not** a blanket audit of transitive licenses or a proof of bitwise-reproducible binaries.

## 6. Create and inspect an unpublished draft

The scripts never invoke GitHub publishing operations. Create a **draft** GitHub Release from the pushed, verified annotated tag, upload *individual files* from the release-assets directory, and compare GitHub uploaded asset SHA-256 values with the bundled `SHA256SUMS.txt`:

```bash
cd /path/to/RationalStudio-v0.1.0-release-assets
# Ensure "gh auth status" and that the chosen repository/tag are correct.
gh release create v0.1.0 \
  --repo SpilledMilkFactory/RationalStudio \
  --verify-tag --draft --title 'Rational Studio v0.1.0 — Initial Public Release' \
  --notes-file RELEASE_NOTES.md \
  rationalstudio_0.1.0-1_ubuntu24.04_amd64.deb \
  rationalstudio_0.1.0-1_ubuntu26.04_amd64.deb \
  RationalStudio-0.1.0-Windows-x64.zip \
  RationalStudio-0.1.0-source.tar.gz \
  qtbase-everywhere-src-6.10.3.tar.xz \
  qtsvg-everywhere-src-6.10.3.tar.xz \
  RELEASE_NOTES.md RELEASE_QUALIFICATION.md RELEASE_MANIFEST.json SHA256SUMS.txt
```

Verify the draft displays the correct tag/commit/title, **Draft** status and
all **10** expected assets. Compare asset sizes and server digests with the
manifest and independently download to a fresh directory to verify
`SHA256SUMS.txt`. Validate extraction of all three application packages and
confirm the two Qt module archives are the only Qt source downloads. The
manifest inventories payloads and reports (excluding itself to avoid a hash
cycle); the top-level checksum file additionally covers the manifest and
excludes only itself. No duplicate names or transport wrappers are allowed.

If a draft already exists, inspect it before updating. Never silently replace
an asset or use `--clobber`. If hashes differ, stop and report the discrepancy.
After upload, query the release again and confirm `isDraft: true` and no
publication timestamp. A successful upload alone does not establish integrity.

**Stop with the release unpublished for owner inspection.** This task does not
authorize publication. Before a future owner publication decision, complete
pending native acceptance/distribution reviews, rerun default final-approval
assembly and reconcile its changed manifest/report/checksums with the draft.
Changing candidate metadata requires explicit, reviewed asset replacement;
never silently overwrite accepted hashes. Do not close Issue #66 merely
because a draft exists. Only the owner handles publication and issue closure.

## 7. Automated regression and provenance checks

```bash
python3 -m unittest discover -s scripts/release -p 'test_*.py' -v
python3 -m unittest discover -s scripts/packaging -p 'test_*.py' -v
python3 scripts/compliance/test_audit_licenses.py -v
```

The new release protocol tests create disposable Git repositories and verify tag/version/cleanliness rejection, evidence contract failure cases, a Git-object source archive, release manifest and checksum integrity. Existing 73/43 CTest suite counts intentionally remain unchanged. Run normal fresh Debug/Release/sanitizer, Debian package/Qt license, Windows portable and GUI smoke workflows for final qualification. No source-control or release actions occur during the Python tests.
