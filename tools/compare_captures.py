"""Compare original-viewport framebuffer captures; report state differences.

Exit 1 on any framebuffer mismatch; --strict-state also requires state equality.
"""
import argparse
import hashlib
import json
from pathlib import Path

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('baseline',type=Path)
    p.add_argument('candidate',type=Path)
    p.add_argument('--strict-state',action='store_true')
    args=p.parse_args()
    failed=False
    captures=sorted(args.baseline.glob('*.fb.bgrx'))
    if not captures: raise SystemExit('No baseline framebuffer captures')
    for fb in captures:
        tag=fb.name.removesuffix('.fb.bgrx')
        for ext in ['fb.bgrx','wram.bin','sram.bin']:
            a=args.baseline/(tag+'.'+ext); b=args.candidate/a.name
            if not a.exists() or not b.exists():
                print(json.dumps({'tag':tag,'kind':ext,'missing':True}));failed=True;continue
            x,y=a.read_bytes(),b.read_bytes()
            same=x==y
            print(json.dumps({'tag':tag,'kind':ext,'equal':same,
                'different_bytes':sum(i!=j for i,j in zip(x,y))+abs(len(x)-len(y)),
                'baseline_sha256':hashlib.sha256(x).hexdigest(),
                'candidate_sha256':hashlib.sha256(y).hexdigest()}))
            if not same and (ext=='fb.bgrx' or args.strict_state): failed=True
    return int(failed)

if __name__=='__main__': raise SystemExit(main())
