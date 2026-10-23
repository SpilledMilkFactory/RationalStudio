#!/usr/bin/env python3
"""Produce an auditable Markdown benchmark baseline from Issue #25 JSON outputs."""
import argparse
import datetime as dt
import json
import os
import platform
from pathlib import Path
import subprocess


def command(*args, cwd=None):
    try:
        result = subprocess.run(args, cwd=cwd, capture_output=True, text=True,
                                timeout=5, check=False)
        lines = (result.stdout.strip() or result.stderr.strip()).splitlines()
        return lines[0][:250] if lines else ''
    except (OSError, subprocess.TimeoutExpired, IndexError):
        return "not available"


def cpu_model():
    proc = Path('/proc/cpuinfo')
    if proc.exists():
        for line in proc.read_text(errors='replace').splitlines():
            if line.startswith('model name'):
                return line.split(':', 1)[-1].strip()
    return platform.processor() or 'not available'


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--input', type=Path, default=Path('benchmark-results/issue25'))
    parser.add_argument('--output', type=Path)
    parser.add_argument('--repo-root', type=Path, default=Path('.'))
    parser.add_argument('--repetitions', default='not specified')
    parser.add_argument('--navigation-events', default='not specified')
    args = parser.parse_args()
    out = args.output or (args.input / 'RESULTS.md')
    input_paths = [args.input / 'graph_sampling.json', args.input / 'graph_canvas.json']
    try:
        records = [json.loads(p.read_text(encoding='utf8')) for p in input_paths]
    except (OSError, json.JSONDecodeError) as e:
        parser.error(f'Cannot parse benchmarks: {e}')
    for rec in records:
        if not rec.get('groups'):
            parser.error('Benchmark JSON contains no groups')
    metadata = [
        ('Date (UTC)', dt.datetime.now(dt.timezone.utc).isoformat(timespec='seconds')),
        ('Git commit', command('git', 'rev-parse', 'HEAD', cwd=args.repo_root)),
        ('Git worktree', command('git', 'status', '--porcelain', cwd=args.repo_root) or 'clean'),
        ('OS', platform.platform()),
        ('CPU', cpu_model()),
        ('Available hardware threads', str(os.cpu_count())),
        ('Governor', Path('/sys/devices/system/cpu/cpu0/cpufreq/scaling_governor').read_text().strip()
         if Path('/sys/devices/system/cpu/cpu0/cpufreq/scaling_governor').exists() else 'not available'),
        ('Display session', os.environ.get('XDG_SESSION_TYPE', 'not available')),
        ('Sampling compiler', records[0].get('compiler', 'not available')),
        ('Sampling build type', records[0].get('configuration', 'not available')),
        ('Qt benchmark build type', records[1].get('configuration', 'not available')),
        ('Trials per group', str(args.repetitions)),
        ('Events per navigation burst', str(args.navigation_events)),
        ('Qt offscreen mode', 'QT_QPA_PLATFORM=offscreen'),
    ]
    contents = ['# Rational Studio — Issue #25 Benchmark Results', '',
                '**Status: local measurements; not performance requirements.**', '',
                'These figures are generated from locally recorded CSV/JSON files and are not',
                'compared to an earlier instrumented release. Times are host- and load-dependent.',
                '', '## Environment', '', '| Attribute | Value |', '|---|---|']
    for label, value in metadata:
        contents.append(f'| {label} | {str(value).replace("|", "/").replace(chr(10), " ")} |')
    for rec in records:
        contents += ['', f'## {rec["benchmark"]}', '',
                     rec.get('notes', ''), '',
                     '| Measurement | Scenario | Curves | N | Median (ms) | p95 (ms) | Mean (ms) | Eval total | Builds total | Trunc. runs |',
                     '|---|---|---:|---:|---:|---:|---:|---:|---:|---:|']
        for row in rec['groups']:
            contents.append('| ' + ' | '.join([
                str(row['category']), str(row['scenario']), str(row['curves']),
                str(row['samples']), f'{row["median_ms"]:.4f}',
                f'{row["p95_ms"]:.4f}', f'{row["mean_ms"]:.4f}',
                str(row['total_evaluations']), str(row['total_path_builds']),
                str(row['truncated_samples'])]) + ' |')
    contents += ['', '## Interpretation and limitations', '',
                 '- `QWidget::render()` benchmarks a preallocated **offscreen Qt raster target**,',
                 '  not compositor presentation or physical mouse-to-photon latency.',
                 '- Cold samples include invalidated geometry generation plus rasterization;',
                 '  warm samples should trigger no mathematical callback work.',
                 '- Sampling totals count mathematical callbacks, not total instruction cycles.',
                 '- Navigation settlement includes Qt timer pacing and event-loop waiting;',
                 '  it is not the same as steady-state repaint cost.',
                 '- Times represent measured work on this host only; do not assert 60 FPS',
                 '  or a guaranteed input latency from these results.',
                 '- Review whether budget truncation or deferred work affects completeness.',
                 '- Repeat at least once under similar conditions before drawing conclusions.',
                 '- Manual GUI appearance, flicker, persistence and PNG export remain separate checks.',
                 '']
    out.parent.mkdir(parents=True, exist_ok=True)
    out.write_text('\n'.join(contents), encoding='utf8')
    print(f'Wrote {out}')


if __name__ == '__main__':
    main()
