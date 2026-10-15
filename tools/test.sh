#!/bin/sh
set -eu
PROJECT_ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
cd "$PROJECT_ROOT"
mkdir -p local
python3 -m unittest discover -s tests
clang -std=c11 -Wall -Wextra -Werror -Isrc src/display_geometry.c tests/test_geometry.c -o local/test_geometry
local/test_geometry
clang -std=c11 -Wall -Wextra -Werror -Isrc src/achievement_memory.c tests/test_achievement_memory.c -o local/test_achievement_memory
local/test_achievement_memory
python3 tools/checkpoint.py --event 'ROM intake and display geometry tests passed'
