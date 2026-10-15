#!/bin/sh
set -eu
PROJECT_ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
cd "$PROJECT_ROOT"
mkdir -p local
python3 -m unittest discover -s tests
clang -std=c11 -Wall -Wextra -Werror -Isrc src/display_geometry.c tests/test_geometry.c -o local/test_geometry
clang -std=c11 -Wall -Wextra -Werror -Isrc src/yoshi_scene_map.c tests/test_scene_map.c -o local/test_scene_map
local/test_scene_map
local/test_geometry
clang -std=c11 -Wall -Wextra -Werror -Isrc src/achievement_memory.c tests/test_achievement_memory.c -o local/test_achievement_memory
local/test_achievement_memory
if [ -f build/libyoshi_rcheevos.a ]; then
  clang -std=c11 -D_POSIX_C_SOURCE=200809L -Isrc -Iupstream/snesrecomp/runner/src -Iupstream/rcheevos/include tests/test_achievements.c src/achievement_memory.c build/libyoshi_rcheevos.a -lm -lpthread -o local/test_achievements
  local/test_achievements
fi
python3 tools/checkpoint.py --event 'ROM/geometry/quota, achievement memory and available native rc_client fixture tests passed'
