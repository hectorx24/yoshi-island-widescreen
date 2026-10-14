#!/bin/sh
set -eu
PROJECT_ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
cd "$PROJECT_ROOT"
ROM_PATH=${1:?Usage: tools/generate.sh /absolute/path/to/owner-rom.smc}
export PATH="$PROJECT_ROOT/local/venv/bin:$HOME/.cargo/bin:$PATH"
PROFILE_PATH=${2:-}
set --
if [ -n "$PROFILE_PATH" ]; then set -- --profile-manifest "$PROFILE_PATH"; fi
python3 upstream/snesrecomp/snesrecomp_cli.py generate --rom "$ROM_PATH" \
  --cfg-dir config --out-dir generated/usa11/generated --funcs-h config/funcs.h \
  --project-root "$PROJECT_ROOT" --cfg-roots --no-host-root-scan \
  --expected-sha256 bd763c1a56365c244be92e6cffefd318780a2a19eda7d5baf1c6d5bd6c1b3e06 "$@"
