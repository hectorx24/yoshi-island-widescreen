"""Atomic, compact handoff checkpoint. Quota input must come from the app API.

No network, credentials, conversation scraping, model calls or polling.
"""
import argparse
from datetime import datetime, timezone
import json
import math
import os
from pathlib import Path
import subprocess
import sys
import tempfile

START = '<!-- checkpoint:begin -->'
END = '<!-- checkpoint:end -->'

def quota_windows(payload):
    limits = payload.get('rateLimitsByLimitId') or {}
    bucket = limits.get('codex') or payload.get('rateLimits') or {}
    result = {}
    for key in ('primary', 'secondary'):
        window = bucket.get(key) or {}
        duration, used = window.get('windowDurationMins'), window.get('usedPercent')
        label = {300: 'five_hour', 10080: 'weekly'}.get(duration)
        if label and isinstance(used, (int, float)) and not isinstance(used, bool) and math.isfinite(used):
            result[label] = {'remaining': max(0, min(100, 100-used)), 'resets_at': window.get('resetsAt')}
    return result

def atomic_write(path, text):
    with tempfile.NamedTemporaryFile('w', dir=path.parent, delete=False, encoding='utf-8') as f:
        temp = Path(f.name)
        f.write(text)
        f.flush()
        os.fsync(f.fileno())
    os.replace(temp, path)

def checkpoint(root, event, quota=None):
    folder = root/'local/handoff'
    folder.mkdir(parents=True, exist_ok=True)
    state_path = folder/'checkpoint.json'
    state = json.loads(state_path.read_text()) if state_path.exists() else {}
    now = datetime.now(timezone.utc).isoformat(timespec='seconds')
    if quota is not None:
        state['quota'] = quota_windows(quota)
        state['quota_checked_at'] = now
    state['event'] = event
    state['checked_at'] = now
    state['git_base'] = subprocess.check_output(['git', 'rev-parse', '--short', 'HEAD'], cwd=root, text=True).strip()
    changes = subprocess.check_output(['git', 'status', '--short'], cwd=root, text=True).splitlines()
    low = any(w['remaining'] <= 5 for w in state.get('quota', {}).values())
    lines = [START, '## Automatic checkpoint',
             f"- Checked {now}; {event}",
             f"- Git base at checkpoint: {state['git_base']}; {len(changes)} changed/untracked paths (run git status)."]
    if state.get('quota'):
        values = '; '.join(f"{k}: {v['remaining']:g}% remaining" for k,v in state['quota'].items())
        lines.append(f"- Last supported quota observation ({state['quota_checked_at']}): {values}.")
    else:
        lines.append('- Reliable quota observation unavailable; continue milestone checkpoints.')
    if low:
        lines.append('- LOW QUOTA: handoff saved; preserve WIP and precise next action before further expensive work.')
        patch = subprocess.check_output(['git', 'diff', 'HEAD', '--', '.', ':!*.smc', ':!*.sfc'], cwd=root)
        (folder/'tracked-wip.patch').write_bytes(patch)
    lines.append(END)
    status_path = root/'PROJECT_STATUS.md'
    body = status_path.read_text()
    if START in body:
        before, rest = body.split(START, 1)
        _, after = rest.split(END, 1)
        body = before.rstrip()+'\n\n'+'\n'.join(lines)+after
    else:
        body = body.rstrip()+'\n\n'+'\n'.join(lines)+'\n'
    atomic_write(status_path, body)
    atomic_write(state_path, json.dumps(state, indent=2)+'\n')
    return low

def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--event', required=True)
    p.add_argument('--quota-json', help='App get_usage_limits payload, or - for stdin')
    args = p.parse_args()
    quota = None
    if args.quota_json:
        quota = json.loads(sys.stdin.read() if args.quota_json=='-' else Path(args.quota_json).read_text())
    checkpoint(Path(__file__).resolve().parents[1], args.event, quota)

if __name__=='__main__': main()
