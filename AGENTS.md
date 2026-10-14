Read PROJECT_STATUS.md first, then git status. Continue from recorded evidence.
All Codex models/sessions must continue existing code, not restart research. Maintain PROJECT_STATUS.md as the compact authoritative handoff after meaningful milestones/discoveries and before ending. Record verified behavior, decisions, failures to avoid, commands, Git state and the exact next action; details belong in linked files.
Run python3 tools/checkpoint.py --event '<verified milestone>' after updating the substantive handoff. Build/test scripts checkpoint automatically. No automatic untested commits.
Quota: use the supported Codex app get_usage_limits tool, if available, at session start, substantial milestones and before ending; normally no more than once per 30 minutes. Do not scrape credentials/logs or poll in a background loop. Feed its actual JSON to tools/checkpoint.py --quota-json - --event 'Quota observation'. Match 300/10080-minute windows; missing values mean unknown. At <=5% remaining in either window immediately refresh substantive status and checkpoint, preserve code/WIP, commit tested changes, then continue only within actual limits. At <=10%, recheck at the next substantive milestone. If tool unavailable, use milestone checkpoints. This is observation-based: no supported push notification guarantees detection at the exact crossing.
Never commit ROMs, extracted assets, generated ROM-derived code, or saves.
Keep upstream checkouts separate and record exact revisions.
Never patch a ROM without an exact verified revision. The source ROM is read-only.
Prefer SNESRecomp; any interpreter fallback must be reported explicitly.
Gameplay widescreen requires authentic additional world content; decorative margins do not qualify.
Bosses require individual investigation and game-specific solutions. Fixed cameras or Super FX effects alone do not justify restricting a boss to 4:3. Use background-only extension only for demonstrated encounter limitations; preserve collision boundaries and behavior.
Keep original simulation timing separate from presentation refresh.
Build and test autonomously; do not claim boot, playability, or Android compatibility without evidence.
Do not delegate unless the user explicitly requests it. Keep research and output concise.
