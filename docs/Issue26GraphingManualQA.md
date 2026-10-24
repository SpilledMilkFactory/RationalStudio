# Rational Studio — Issue #26: Interactive Graphing GUI Verification

**Issue:** #26 — Perform interactive GUI verification of graphing behavior  
**Acceptance scope:** Verify graphing, tracing, panning, zooming, resizing, saving, and loading without regressions.  
**Baseline:** Issue #25; GUI-enabled CTest **18** and Qt-disabled CTest **13**, unchanged by this manual QA issue.  
**Evidence:** Human-observed status for each required step, recorded in machine-readable JSON and Markdown.

## Why this issue needs manual verification

The existing core suites and five offscreen Qt widget test suites do not test all real desktop event ordering, visible graphics, mouse-to-display tracing, file dialogs, or GUI persistence workflows. The new recorder captures an explicit human attestation without claiming automated visual proof. It does not change production code.

## Preparation

Start from the latest merged `main` on a new Issue #26 feature branch. Use a fresh Debug build (GUI enabled) and confirm the existing 18 CTests still pass. Optionally repeat the 18 CTests in Release and the existing 13 core-only CTests with ASan/UBSan. **Do not create an additional CTest target for the manual checklist.**

```bash
cmake -S . -B build-issue26 -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build-issue26 --parallel
ctest --test-dir build-issue26 --output-on-failure
python3 -m unittest discover -s tests -p 'test_issue26_gui_verification.py' -v
```

Test with realistic graphing expressions and preserve copies of any workspace files before editing them. Recommended test data include `sin(x)`, `x^2`, `2*x+3`, `sqrt(x)`, `ln(x)`, `1/x`, `1/(x-0.133)`, `tan(x)`, `floor(x)`, `500*x`, `tanh(1000*x)`, and `atan(1000*x)`. Use radian mode for tangent asymptotes. Avoid treating any finite sampler as a proof against all undetected discontinuities.

## Interactive verification session

Use the optional launcher to start the GUI while prompting each scenario:

```bash
python3 scripts/issue26_gui_verification.py \
  --app build-issue26/RationalStudio --launch
```

Alternatively, launch `./build-issue26/RationalStudio` yourself and omit `--launch`. The script asks for **P** (pass), **F** (fail), **S** (skip), or **Q** (quit) for each case. No case is automatically passed. Record observed results and any screenshot/log references. If the run is interrupted or you press Q, all previously entered answers are already saved.

The checklist contains **22 required** and **3 optional** cases. A **PASS** verdict requires every required case to be explicitly marked passed and no optional case marked failed. Required skipped/unanswered cases produce **INCOMPLETE**. Any failed case produces **FAIL**. Optional skipped cases are permitted. Re-run after fixing failures: keep the old JSON/Markdown as historical evidence, then start a fresh `--output` file rather than overwriting past observations.

Outputs are saved by default to:

```text
verification-results/issue26/manual_gui.json
verification-results/issue26/manual_gui.md
```

Resume an interrupted run:

```bash
python3 scripts/issue26_gui_verification.py --resume
```

Check the actual recorded verdict:

```bash
python3 scripts/issue26_gui_verification.py --check
```

Generate the Markdown view again from existing evidence:

```bash
python3 scripts/issue26_gui_verification.py --report
```

Print every detailed step without running the interactive recorder:

```bash
python3 scripts/issue26_gui_verification.py --list
```

## Acceptance evidence and sign-off

Review the JSON and Markdown report together with the CTest log. Verify the Git commit, session information, failed/skipped cases, and any files or screenshots noted. If a required case fails, leave Issue #26 open while investigating: successful core/Qt CTests do **not** override an observed GUI regression. In particular, investigate a recurrence of the Issue #22 panning flicker rather than labeling it acceptable temporary preview behavior.

After interactive testing, share the report and terminal output. The `VERIFICATION.md` updater for Issue #26 should then record the **actual observed result**, keeping Issues #17–#25 historical records and the Issue #25 benchmark baseline intact. This checklist is not a quantitative performance claim, accessibility audit, pixel-difference oracle, or native Windows compatibility test.
