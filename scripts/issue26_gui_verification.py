#!/usr/bin/env python3
"""Issue #26 interactive GraphCanvas acceptance recorder (Python stdlib only).

This script does not claim to automate GUI verification. A human enters P/F/S
for each explicit scenario; the script preserves attestations and evidence.
"""
from __future__ import annotations

import argparse
import datetime as dt
import json
import os
import platform
import re
import subprocess
import sys
import tempfile
from pathlib import Path
from typing import Any

SCHEMA_VERSION = 1
ISSUE_NUMBER = 26
CHECKS: tuple[dict[str, Any], ...] = (
    {"id":"GUI-01","area":"Launch and workspace","required":True,"title":"Application starts and graph workspace opens","steps":"Launch RationalStudio. Open the graphing workspace and switch between calculator tabs.","expected":"No crash, freeze, unexpected dialog, corrupted plot, or lost controls."},
    {"id":"GRA-01","area":"Graphing","required":True,"title":"Multiple ordinary curves","steps":"Plot y = sin(x), y = x^2, and y = 2*x+3 together; vary the viewport.","expected":"Shapes, scales, axes, and distinct colors remain visually correct."},
    {"id":"GRA-02","area":"Domains","required":True,"title":"Undefined and restricted domains","steps":"Plot y = sqrt(x), y = ln(x), and y = 1/x; examine the origin and excluded x values.","expected":"Undefined intervals are not drawn; reciprocal branches do not join through x=0."},
    {"id":"GRA-03","area":"Discontinuities","required":True,"title":"Shifted poles and finite steps","steps":"Inspect y = 1/(x-0.133), y = tan(x) (radians), and y = floor(x) at multiple scales.","expected":"No false connecting lines across observed poles or finite step jumps; normal continuous parts remain."},
    {"id":"GRA-04","area":"Steep curves","required":True,"title":"Preserve steep continuous transitions","steps":"Inspect y = 500*x, y = tanh(1000*x), and y = atan(1000*x); zoom around the origin.","expected":"Steep but continuous portions remain connected where accurately representable."},
    {"id":"TRA-01","area":"Tracing","required":True,"title":"Coordinate trace on known functions","steps":"Enable or use the existing point-inspection/tracing interaction. Trace near x=0 for y=x^2 and y=sin(x), then near x=1 for y=x^2.","expected":"Highlighted point and reported coordinates track the correct visible curve within display precision."},
    {"id":"TRA-02","area":"Tracing","required":True,"title":"No misleading tracing over gaps","steps":"Move the cursor across the undefined side of sqrt(x) and across reciprocal/discontinuous gaps; then leave the graph.","expected":"No phantom connected trace path or stuck hover indicator remains."},
    {"id":"PAN-01","area":"Panning","required":True,"title":"Pan in both axes and reverse direction","steps":"Drag horizontally, vertically, diagonally and reverse direction, with at least four curves visible.","expected":"Curves follow navigation, final geometry matches the latest viewport, and axes align."},
    {"id":"PAN-02","area":"Panning","required":True,"title":"Prior flicker regression","steps":"Continuously pan with several curves; drag slowly and rapidly, pausing and reversing repeatedly.","expected":"No repeated flashing, blank frames, persistent stale geometry, or Issue #22 line-flicker recurrence."},
    {"id":"ZOO-01","area":"Zooming","required":True,"title":"Off-center cursor-anchored zoom","steps":"Place cursor well away from center over an identifiable curve feature; wheel zoom in/out repeatedly.","expected":"The world point beneath the cursor remains anchored within numerical/display precision."},
    {"id":"ZOO-02","area":"Zooming","required":True,"title":"Zoom extremes and restored views","steps":"Zoom toward both supported limits, attempt more wheel input at each limit, then zoom back.","expected":"No crash, NaN labels, unexpected reverse zoom, numerical drift, or corrupted rendering."},
    {"id":"NAV-01","area":"Navigation","required":True,"title":"Rapid mixed pan/zoom scheduling","steps":"Alternate drag, wheel zoom, and immediate input reversal using multiple active curves; then stop input.","expected":"Temporary previews remain usable; final curves settle correctly with no permanently missing paths."},
    {"id":"RES-01","area":"Resizing","required":True,"title":"Repeated window size changes","steps":"Resize narrow/wide/tall, maximize and restore, and rapidly drag a window border while curves are present.","expected":"Grid and geometry match current dimensions; no stale clipping, invalid labels, or crashes."},
    {"id":"RES-02","area":"Resizing","required":True,"title":"Resize after pending navigation","steps":"Zoom or pan, then resize immediately and inspect when drawing settles.","expected":"No obsolete scheduled redraw replaces geometry for the new dimensions."},
    {"id":"EDT-01","area":"Curve editing","required":True,"title":"Edit, reorder, visibility and color","steps":"Change a formula; toggle curve visibility; change colors and row order where supported.","expected":"Affected geometry updates, hidden curves stay hidden, and unrelated curves remain correct."},
    {"id":"EDT-02","area":"Curve editing","required":True,"title":"Angle mode and previous answer","steps":"Change angle mode for a trig expression and change ans where the graphing UI supports it.","expected":"Dependent curves reevaluate correctly; otherwise explain any unsupported operation in notes."},
    {"id":"BUD-01","area":"Many curves","required":True,"title":"Eight demanding active curves","steps":"Plot up to eight mixed smooth/steep/discontinuous curves and navigate repeatedly.","expected":"UI remains functional; deferred/truncated sampling is acceptable only within documented limits, not corrupted geometry or permanent blanks."},
    {"id":"SAV-01","area":"Persistence","required":True,"title":"Save a graph workspace","steps":"Set several expressions, colors, visibility states and a non-default viewport; save the workspace as JSON.","expected":"Save succeeds and a real file is created at the selected path."},
    {"id":"LOD-01","area":"Persistence","required":True,"title":"Reload saved workspace","steps":"Change the visible scene or reopen; load the JSON saved in SAV-01.","expected":"Supported expressions, visibility, colors and viewport state restore correctly without stale geometry."},
    {"id":"EXP-01","area":"Export","required":True,"title":"Export current graph as PNG","steps":"With multiple curves and a nondefault viewport, export a PNG and inspect the resulting image.","expected":"Export is readable and represents the current settled graph, including intended gaps."},
    {"id":"REG-01","area":"Regressions","required":True,"title":"Other calculator workspaces","steps":"Use a few scientific expressions and one exact fraction operation; return to the graph.","expected":"Core calculator workflows still work and graph content is preserved where expected."},
    {"id":"REG-02","area":"Regressions","required":True,"title":"Final settled view after many interactions","steps":"After all earlier checks, inspect the final settled graph, trace a point, and pan once more.","expected":"No unexpected stuck previews, unresponsive navigation, missing lines, or new flicker."},
    {"id":"OPT-01","area":"Optional","required":False,"title":"Malformed workspace file","steps":"If supported and safe, attempt loading an intentionally malformed JSON workspace (keep a good saved copy).","expected":"Application handles the error gracefully without crashing or modifying the last valid graph."},
    {"id":"OPT-02","area":"Optional","required":False,"title":"Interactive Release build","steps":"Repeat representative trace, pan, zoom, resize and load checks in the Release GUI.","expected":"Release desktop behavior matches Debug; record whether run."},
    {"id":"OPT-03","area":"Optional","required":False,"title":"Additional display scale/monitor","steps":"If available, repeat tracing, panning and resizing at another display scaling or monitor.","expected":"No scaling-induced coordinate or rendering error; record environment."},
)
IDS = {c["id"] for c in CHECKS}
STATES = {"not_run", "pass", "fail", "skip"}


def now() -> str:
    return dt.datetime.now(dt.timezone.utc).isoformat(timespec="seconds")


def shell_output(cmd: list[str], cwd: Path) -> str:
    try:
        x = subprocess.run(cmd, cwd=cwd, capture_output=True, text=True, timeout=8, check=False)
        return x.stdout.strip() if x.returncode == 0 else "unavailable"
    except (OSError, subprocess.TimeoutExpired):
        return "unavailable"


def metadata(root: Path, app: str) -> dict[str, str]:
    return {
        "utc_started": now(), "git_commit": shell_output(["git", "rev-parse", "HEAD"], root),
        "git_status_porcelain": shell_output(["git", "status", "--porcelain"], root),
        "os": platform.platform(), "python": platform.python_version(),
        "session_type": os.environ.get("XDG_SESSION_TYPE", "unknown"),
        "desktop": os.environ.get("XDG_CURRENT_DESKTOP", "unknown"),
        "qt_platform": os.environ.get("QT_QPA_PLATFORM", "default"),
        "app": app,
    }


def fresh_data(root: Path, app: str) -> dict[str, Any]:
    return {"schema_version": SCHEMA_VERSION, "issue": ISSUE_NUMBER,
        "metadata": metadata(root, app), "updated_utc": now(),
        "checks": {c["id"]: {"status": "not_run", "notes": "", "evidence": "", "updated_utc": None} for c in CHECKS}}


def validate(data: dict[str, Any]) -> None:
    if data.get("schema_version") != SCHEMA_VERSION or data.get("issue") != ISSUE_NUMBER:
        raise ValueError("Unrecognized Issue #26 evidence schema/issue")
    got = data.get("checks")
    if not isinstance(got, dict) or set(got) != IDS:
        raise ValueError("Missing or unexpected GUI checklist IDs (checklist revision?)")
    for cid, item in got.items():
        if not isinstance(item, dict) or item.get("status") not in STATES:
            raise ValueError(f"Invalid state for {cid}")
        for field in ("notes", "evidence"):
            if not isinstance(item.get(field, ""), str):
                raise ValueError(f"Invalid {field} for {cid}")


def assessment(data: dict[str, Any]) -> dict[str, Any]:
    validate(data)
    failed = [c["id"] for c in CHECKS if data["checks"][c["id"]]["status"] == "fail"]
    unfinished = [c["id"] for c in CHECKS if c["required"] and data["checks"][c["id"]]["status"] != "pass"]
    verdict = "FAIL" if failed else ("INCOMPLETE" if unfinished else "PASS")
    counts = {state: sum(x["status"] == state for x in data["checks"].values()) for state in STATES}
    return {"verdict": verdict, "counts": counts, "failed": failed, "unfinished_required": unfinished,
            "required_total": sum(c["required"] for c in CHECKS)}


def md_escape(value: Any) -> str:
    return str(value).replace("|", "\\|").replace("\n", "<br>").replace("\r", "")


def markdown(data: dict[str, Any]) -> str:
    summary = assessment(data)
    m = data.get("metadata", {})
    out = ["# Rational Studio — Issue #26 Interactive GUI Verification", "",
           f"**Human-attested assessment:** **{summary['verdict']}**  ",
           f"**Updated (UTC):** {md_escape(data.get('updated_utc',''))}  ",
           f"**Required checks:** {summary['required_total']}  ",
           f"**Counts:** PASS {summary['counts']['pass']}, FAIL {summary['counts']['fail']}, SKIP {summary['counts']['skip']}, NOT RUN {summary['counts']['not_run']}", "",
           "**Evidence type:** Operator-entered manual observations. This script does not automate GUI behavior or independently verify assertions.", "",
           "## Environment", "",
           "| Field | Value |", "|---|---|" ]
    for k in ("git_commit", "git_status_porcelain", "os", "session_type", "desktop", "qt_platform", "app", "utc_started"):
        out.append(f"| {k} | {md_escape(m.get(k, 'unknown'))} |")
    out += ["", "## Acceptance results", "", "| ID | Area | Required | Scenario | Result | Notes | Evidence |", "|---|---|---|---|---|---|---|"]
    for c in CHECKS:
        row = data["checks"][c["id"]]
        out.append("| " + " | ".join((c["id"], c["area"], "Yes" if c["required"] else "No", md_escape(c["title"]), row["status"].upper(), md_escape(row.get("notes", "")), md_escape(row.get("evidence", "")))) + " |")
    out += ["", "## Outstanding observations", ""]
    if summary["failed"]:
        out.append("**Failed:** " + ", ".join(summary["failed"]))
    if summary["unfinished_required"]:
        out.append("**Unfinished required cases:** " + ", ".join(summary["unfinished_required"]))
    if not summary["failed"] and not summary["unfinished_required"]:
        out.append("No required failures or missing results were recorded.")
    out += ["", "## Interpretation", "", "A PASS attests that the operator marked all mandatory cases passed. It does not establish exhaustive GUI automation, pixel-level correctness, Windows/Qt 5 behavior, or a measured performance gain. Existing CTest and benchmark evidence is separately documented in VERIFICATION.md.", ""]
    return "\n".join(out)


def atomic_text(path: Path, content: str) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    fd, tmp = tempfile.mkstemp(prefix="." + path.name + ".", dir=path.parent, text=True)
    try:
        with os.fdopen(fd, "w", encoding="utf-8") as out:
            out.write(content)
        os.replace(tmp, path)
    finally:
        if os.path.exists(tmp):
            os.unlink(tmp)


def save(data: dict[str, Any], file: Path) -> None:
    validate(data)
    data["updated_utc"] = now()
    atomic_text(file, json.dumps(data, indent=2, ensure_ascii=False) + "\n")
    atomic_text(file.with_suffix(".md"), markdown(data))


def print_checks() -> None:
    for c in CHECKS:
        print(f"\n{c['id']} {'REQUIRED' if c['required'] else 'OPTIONAL'} — {c['title']}\n  DO: {c['steps']}\n  EXPECT: {c['expected']}")


def interactive(data: dict[str, Any], file: Path) -> None:
    print("Issue #26: human-attested GUI checklist. Enter P=pass, F=fail, S=skip, Q=quit.")
    print("Required cases cannot be skipped while still receiving a PASS verdict.")
    save(data, file)
    for c in CHECKS:
        item = data["checks"][c["id"]]
        if item["status"] != "not_run":
            continue
        print(f"\n[{c['id']}] {c['title']} — {'required' if c['required'] else 'optional'}")
        print("DO: " + c["steps"])
        print("EXPECT: " + c["expected"])
        while True:
            answer = input("Result [P/F/S/Q]: ").strip().lower()
            if answer in ("p", "f", "s", "q"):
                break
            print("Enter P, F, S, or Q.")
        if answer == "q":
            print("Progress saved. Resume with --resume.")
            return
        item["status"] = {"p": "pass", "f": "fail", "s": "skip"}[answer]
        item["notes"] = input("Observations/notes (optional): ").strip()
        item["evidence"] = input("Screenshot/log reference (optional): ").strip()
        item["updated_utc"] = now()
        save(data, file)
    print("\nFinal assessment:", assessment(data)["verdict"])
    print("Evidence:", file)
    print("Summary:", file.with_suffix(".md"))


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description="Record Issue #26 manual GUI verification")
    group = parser.add_mutually_exclusive_group()
    group.add_argument("--list", action="store_true", help="print full checklist")
    group.add_argument("--report", action="store_true", help="recreate report from existing JSON")
    group.add_argument("--check", action="store_true", help="return success only if all mandatory cases passed")
    parser.add_argument("--output", type=Path, default=Path("verification-results/issue26/manual_gui.json"))
    parser.add_argument("--resume", action="store_true", help="continue an existing run without changing recorded answers")
    parser.add_argument("--app", default="build-issue26/RationalStudio", help="path to binary for metadata and optional launch")
    parser.add_argument("--launch", action="store_true", help="start native desktop GUI before interactive questions")
    args = parser.parse_args(argv)
    if args.list:
        print_checks()
        return 0
    file = args.output.resolve()
    if args.report or args.check or args.resume:
        if not file.is_file():
            parser.error(f"Evidence JSON does not exist: {file}")
        try:
            data = json.loads(file.read_text(encoding="utf-8"))
            validate(data)
        except (OSError, ValueError, json.JSONDecodeError) as exc:
            parser.error(f"Invalid evidence: {exc}")
    else:
        if file.exists():
            parser.error(f"Refusing to overwrite existing evidence: {file}. Use --resume or another --output.")
        data = fresh_data(Path.cwd(), args.app)
    if args.report:
        atomic_text(file.with_suffix(".md"), markdown(data))
        print("Report generated:", file.with_suffix(".md"))
        return 0
    if args.check:
        result = assessment(data)
        print(json.dumps(result, indent=2))
        return 0 if result["verdict"] == "PASS" else 1
    if args.launch:
        app = Path(args.app).resolve()
        if not app.is_file() or not os.access(app, os.X_OK):
            parser.error(f"Cannot launch GUI binary: {app}")
        try:
            subprocess.Popen([str(app)], cwd=Path.cwd())
            print("Started application:", app)
        except OSError as exc:
            parser.error(f"Could not launch application: {exc}")
    try:
        interactive(data, file)
    except (KeyboardInterrupt, EOFError):
        print("\nInterrupted; all previously answered cases were already saved.")
        return 130
    return 0


if __name__ == "__main__":
    sys.exit(main())
