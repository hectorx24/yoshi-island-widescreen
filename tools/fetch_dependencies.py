"""Fetch pinned reference repositories, retaining existing clean checkouts."""
import json
import subprocess
from pathlib import Path

root=Path(__file__).resolve().parents[1]
deps=json.loads((root/'dependencies.json').read_text())
for name,dep in deps.items():
    if 'revision' not in dep: continue
    target=root/'upstream'/name
    if target.exists():
        actual=subprocess.check_output(['git','-C',str(target),'rev-parse','HEAD'],text=True).strip()
        if actual!=dep['revision']:
            raise SystemExit(f'{name}: checkout differs from pin; inspect before changing it')
        continue
    target.mkdir(parents=True)
    subprocess.run(['git','init',str(target)],check=True)
    subprocess.run(['git','-C',str(target),'remote','add','origin',dep['url']],check=True)
    subprocess.run(['git','-C',str(target),'fetch','--depth','1','origin',dep['revision']],check=True)
    subprocess.run(['git','-C',str(target),'checkout','--detach','FETCH_HEAD'],check=True)
