# Version-specific findings

References are pinned in dependencies.json; do not repeat their intake.

- Yoshifanatic README explicitly supports USA v1.1 and provides MD5
  ce1e3e33b6e39d37b43d7de599f9e785. It does not supply a SHA-256 there.
  The project's verified local SHA-256 is recorded independently.
- brunovalads README's v1.0 SHA-256 differs. Its address labels cannot be
  treated as v1.1 patch sites.
- v1.1 ROM boot instructions copy $00:C000..FFFF (file offsets $4000..7FFF)
  to $7E:C000..FFFF. The captured WRAM block at frame 1382 matches all
  16384 source bytes exactly. Yoshifanatic's RAM_Map_YI.asm names this block.
- Routine at $00:8150 dispatches game modes by pushing a computed destination
  then RTL. Verified ROM bytes:
  AD18010A6D1801AA8BBD6C814848ABBF6B810048BF6A8100486B.
  Profile promotion incorrectly made it an ordinary AOT function. Symptoms:
  bad return edges, interpreter cap, wrong framebuffer by frame 1200.
  config/bank00.cfg keeps this routine LLE. Corrected profile build has
  70 AOT-eligible variants and exact baseline framebuffers at 600/1200/1382.
  This is a host integration correction, not proof every other AOT routine
  is safe. WRAM and SRAM differences remain to analyze.
- Captured RAM routine snapshots are 512-byte capped and unterminated;
  removing them from the profile leaves the same generated AOT set. They
  do not establish native WRAM execution. Most code still runs in LLE.
- $00:0118 is current game mode. Observed $07 opening, $0A title/file menu,
  $39 new-game narrative, $0E/$0F tutorial loading/gameplay. A mode alone is
  insufficient to classify every boss or scene's widescreen capability.
- SRAM offsets $008C/$0090 are Yoshi X/Y in the version-compatible map;
  tutorial replay changed X from 136 to 270. Tutorial help dialogs await
  button presses; elapsed frames alone do not advance them.
- Hold menu inputs for multiple frames; one-frame taps can miss the game's
  polling interval. tests/tutorial.script is a coverage replay, not a
  passing mechanics assertion for every action it contains.
- Shared PPU supports configurable side margins up to an 800-column buffer.
  This capability does not solve Yoshi's tile streaming, GSU-generated
  sprite content, activation or collision windows. No widescreen claim yet.
- Bosses remain candidates for actual world expansion. No encounter has
  yet demonstrated a limitation that justifies background-only fallback.

# Reference comparison limits

Independent Snes9x core built from the recorded pin; tools/oracle.py drives
the same replay without an emulator UI. Core option defaults must be served:
omitting them leaves the Super FX clock multiplier zero and gives black video.
RGB565 green has Snes9x's redundant bit; normalize to the SNES 5-bit channels.

Before RGB conversion correction, 5-bit-normalized comparison found 2485
different pixels out of 57344 at frames 600 and 1200, and 18528 at 1382.
These differences are unresolved; exact reference rendering/timing is not
qualified. Oracle audio RMS is nonzero; this alone says nothing about the
recomp's audio fidelity.
