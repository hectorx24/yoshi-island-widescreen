"""Read-only ROM intake; normalized hashes exclude a detected copier header."""
import argparse
import hashlib
import json
from pathlib import Path

USA10_SHA256 = '9b4957466798bbdb5b43a450bbb60b2591ae81d95b891430f62d53ca62e8bc7b'
USA11_SHA256 = 'bd763c1a56365c244be92e6cffefd318780a2a19eda7d5baf1c6d5bd6c1b3e06'
USA11_REFERENCE_MD5 = 'ce1e3e33b6e39d37b43d7de599f9e785'

def inspect(raw):
    header_size = 512 if len(raw) % 32768 == 512 else 0
    data = raw[header_size:]
    if len(data) != 2097152:
        raise ValueError('Expected a 2 MiB Yoshi Island image, optionally with a 512-byte copier header')
    h = data[0x7fc0:0x8000]
    checksum = int.from_bytes(h[30:32], 'little')
    complement = int.from_bytes(h[28:30], 'little')
    if h[:21].decode('ascii', errors='replace').strip() != 'YOSHI\'S ISLAND':
        raise ValueError('Internal title is not YOSHI\'S ISLAND')
    if checksum ^ complement != 65535:
        raise ValueError('Invalid internal checksum/complement pair')
    digest = hashlib.sha256(data).hexdigest()
    return dict(file_sha256=hashlib.sha256(raw).hexdigest(), normalized_sha256=digest,
                normalized_md5=hashlib.md5(data).hexdigest(),
                matches_secondary_usa11=hashlib.md5(data).hexdigest()==USA11_REFERENCE_MD5,
                copier_header_bytes=header_size, size=len(data), title=h[:21].decode().strip(),
                map_mode=h[21], cartridge_type=h[22], region=h[25], revision_byte=h[27],
                checksum=checksum, computed_checksum=sum(data) & 65535,
                matches_requested_usa10=digest == USA10_SHA256,
                reset_vector=int.from_bytes(h[60:62], 'little'))

def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('rom', type=Path)
    p.add_argument('--require-usa10', action='store_true')
    args = p.parse_args()
    result = inspect(args.rom.read_bytes())
    print(json.dumps(result, indent=2))
    return 3 if args.require_usa10 and not result['matches_requested_usa10'] else 0

if __name__ == '__main__':
    raise SystemExit(main())
