# Qt 6.10.3 source and replacement information — Windows x64

Rational Studio is distributed under the MIT license. The bundled Qt shared
libraries/plugins are separately licensed; qualifying Qt components are
available under LGPLv3. The LGPLv3 and GPLv3 texts accompany this ZIP.

## Replace the Qt libraries

This is a dynamically linked Windows application. Qt DLLs reside alongside
`RationalStudio.exe`, and plugins are under `platforms/` and other named
plugin directories. A user can replace the bundled compatible Qt DLLs/plugins
with their own compatible builds. The application does not enforce signed Qt
DLLs, restrict reverse engineering for debugging modified Qt, or prohibit
running a modified Qt build. Compatibility and ABI matching remain necessary.

To test substitution: extract the ZIP into a writable directory; back up the
Qt DLLs; replace the library/plugin DLLs with compatible x64 MSVC builds;
run `RationalStudio.exe` and its user-interface checks. If the modified Qt
needs additional runtime libraries, supply them in the local application
folder. Do not treat this descriptive process as proof of legal compliance.

## Corresponding source: publisher must finalize before distributing

Version/build used: **Qt 6.10.3 / MSVC 2022 x64** (subject to verification of
actual files in each release artifact).

**RELEASE BLOCKER:** Prior to publishing, the maintainer must publish or
otherwise legally supply the complete corresponding source of the exact Qt
libraries/plugins actually bundled, including applicable modifications,
build scripts and third-party components, or a legally sufficient written
offer with exact instructions. Retain the source archive, checksum,
retrieval procedure, availability duration and responsible contact in
release evidence. A generic link to a Qt download page is not sufficient
release evidence. Do not mark this section fulfilled until that happens.

The publisher's final versioned GitHub Release notes should identify:
- The Qt source archive(s) or legally sufficient written-offer text/endpoint.
- SHA-256 checksums and exact Qt build/provenance information.
- Instructions needed to build and substitute the shipped LGPL components.
- Applicable third-party license notices and any Qt modifications.
- Contact/owner for source requests, if using an offer.

Reference: https://www.qt.io/development/open-source-lgpl-obligations
