# v0.1.0 assembly blocker: Windows checkout line endings

The annotated `v0.1.0` tag identifies accepted main commit
`362fd5c08ef0380895de70e6ce76c0bce0b81d01`. All seven jobs in the existing
tagged qualification batch succeeded. No replacement batch was started.

The Windows package came from the successful [tagged Windows run](https://github.com/SpilledMilkFactory/RationalStudio/actions/runs/42156819471).
Its original SHA-256 is
`21573492b7cb7f372cd0631bb2789630fce37cd76cbd31fcd9ea83fb1ca3ba61`.

## Independent assembly failure

The unchanged Windows ZIP validator and draft-candidate assembler both reject
that package with:

```text
ICU license differs from approved source copy
```

The Windows checkout converted the ten source-matched documents and license
files to CRLF. The immutable tagged Git blobs and Linux checkout use LF.
Comparison establishes that every difference is a line-ending conversion;
the license content is present. The hosted validator compared the ZIP against
its own CRLF checkout, so it passed there. Independent assembly compares the
package with the LF source and correctly refuses the byte mismatch.

Affected files are the MIT license, README, user guide, third-party dependency
review, Windows notices and Qt source/relinking guide, and the LGPL, GPL, Boost
and ICU license files. The ZIP and its original workflow checksum were left
unchanged. No validator was relaxed or bypassed.

## Prepared correction and its boundary

The local correction adds `.gitattributes` rules preserving LF for packaged
Markdown and license/notice text, regardless of `core.autocrlf`. A regression
creates a real Git checkout with `core.autocrlf=true` and checks every packaged
source document byte-for-byte. The release tag verifier also requires the
attributes file to be tracked. Existing exact-byte validators remain intact.

This correction is subsequent, unmerged source. It does not change the
existing tag, package or qualification results. It has not been pushed and
does not trigger Actions. Calculator implementation and historical records
are unchanged.

## Owner decision required

Release readiness is **BLOCKED — ARTIFACT VALIDATION FAILED**. No GitHub
Release draft or upload was created. The public release remains unpublished.

The existing pushed `v0.1.0` tag is preserved. The owner must review this fix
and decide the tag/release correction policy before any source acceptance,
tag changes or corrective qualification. Never silently move the existing
tag or claim that old qualification covers changed source. Corrected Windows
packaging must pass the unchanged validator before release assembly resumes.
Any new source revision requires the normal review and qualification process;
no new workflows are authorized by this record.
