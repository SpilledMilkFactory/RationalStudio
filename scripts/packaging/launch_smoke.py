#!/usr/bin/env python3
"""Launch a package-extracted Qt GUI and reject early failures.
This is a same-host/offscreen smoke test, not clean VM runtime qualification.
"""
import argparse
import os
from pathlib import Path
import subprocess
import tempfile
import time


def check(executable: Path, seconds: float) -> str:
    if not executable.is_file():
        raise ValueError(f"Not a file: {executable}")
    with tempfile.TemporaryDirectory(prefix='rationalstudio-smoke-') as home:
        env = os.environ.copy()
        env.update(HOME=home, XDG_CONFIG_HOME=home, XDG_CACHE_HOME=home,
                   QT_QPA_PLATFORM='offscreen')
        for key in ('QT_PLUGIN_PATH', 'QT_QPA_PLATFORM_PLUGIN_PATH', 'LD_LIBRARY_PATH'):
            env.pop(key, None)
        proc = subprocess.Popen([str(executable)], env=env,
                                stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        time.sleep(seconds)
        if proc.poll() is not None:
            stdout, stderr = proc.communicate(timeout=3)
            raise ValueError(f"GUI terminated early (code={proc.returncode}): "
                             f"{(stdout+stderr).decode(errors='replace')[-3000:]}")
        proc.terminate()
        try:
            proc.communicate(timeout=5)
        except subprocess.TimeoutExpired:
            proc.kill()
            proc.communicate()
        return f"PASS: GUI remained running for {seconds:g}s with offscreen Qt"

if __name__ == '__main__':
    ap = argparse.ArgumentParser()
    ap.add_argument('executable', type=Path)
    ap.add_argument('--seconds', type=float, default=2)
    a = ap.parse_args()
    try:
        print(check(a.executable.resolve(), a.seconds))
    except (ValueError, OSError) as error:
        ap.exit(1, f'ERROR: {error}\n')
