# Building from source

Most players just want the installer from [Releases](https://github.com/shihabal3amri/tekken-tag-tournament-2-pc/releases/latest).
This page is for developers.

This is a static recompilation, so building needs your own copy of the game too: the recompiled
C++ is generated on your machine from your `Default.xex` + Title Update 3, and is never stored in
this repository (neither is anything else from the disc or the updates).

## Requirements

- Visual Studio 2022 Build Tools (MSVC x64)
- LLVM/Clang (clang-cl)
- CMake + Ninja (the copies bundled with Visual Studio are fine)
- Python 3.10+ (Pillow for the image tools)

The scripts expect the checkout at `C:\ttt2` (a junction is fine: `mklink /J C:\ttt2 <checkout>`),
because some tools choke on spaces and parentheses in paths.

## Steps

1. **Your game files.** Put your disc's `Default.xex` at `re\Default.xex`, put your title updates in
   `Updates\`, and extract them to `re\extracted_tu\` with `python port\tools\stfs.py`.
2. **The recompiler.** Clone [XenonRecomp](https://github.com/hedge-dev/XenonRecomp) (`--recursive`)
   into `tools\XenonRecomp`, check out `ddd128b`, apply `tools\patches\XenonRecomp.patch`
   (`git apply`), and run `tools\build_xenonrecomp.cmd`.
3. **Generate the C++** (into `re\ppc\`):
   1. XenonAnalyse on the TU3-patched image, then `python port\tools\gen_missing_switches.py`
   2. `XenonRecomp re\recomp\tekken.toml tools\XenonRecomp\XenonUtils\ppc_context.h`
   3. `python port\tools\gen_missing_functions.py`, and re-run XenonRecomp until no `// ERROR`
      sites remain
   4. `python port\tools\debug_probes.py` (applies the runtime's generated-code patches)
   5. `python port\tools\check_recomp_acceptance.py`
   6. `python port\tools\gen_kernel_stubs.py`
4. **FFmpeg** n7.1 source in `tools\ffmpeg_dl\FFmpeg-n7.1`, configured as described at the top of
   `port\third_party\ffmpeg_xma\CMakeLists.txt` (XMA decoding only).
5. **Steamworks SDK** 1.65 in `tools\steamworks_sdk_165\sdk` (optional; without it the build has
   no Steam online).
6. **Build**: `tools\build_port.cmd build-o1 1 8 ON` (the game, `port\build-o1\tekken_recomp.exe`),
   then `tools\build_setup.cmd build-o1 8` (the installer, `port\build-o1\setup\ttt2_setup.exe`).
   How the installer works: [port/INSTALLER.md](port/INSTALLER.md).

## Running a development build

To run a build without regenerating the game files, install a release once and point the build
at it:

```
set TTT2_VFS_ROOT=<install folder>\game
port\build-o1\tekken_recomp.exe "<install folder>\game\tekken.img"
```

The development exe is a console program and logs to stderr; redirect it to a file
(`... > run.txt 2>&1`) for long runs. `TTT2_NET=loopback` keeps online play on this PC (two
instances, no Steam).

### Tekken Force test encounter

Add `--tekken-force` before the image path to put **Tekken Force Test** in the main menu's
top slot for that session. Select one fighter to control. The game loads Tulip Festival and
four Combot opponents automatically.
Enemies are restricted to the two installed Combot default costume schemes. Their native
equipment tables select Costume 1/2/1/2 in enemy gauge order; unlisted equipment is cleared.
The fifth Practice-based model can still show a protruding mesh artifact, so its complete
rendered outfit remains a known limitation.

The D-pad, left stick, and default keyboard arrow keys use the same controls. While locked
onto an enemy, left/right use native Tekken forward/backward walking and move commands
along the line between the two fighters. Backward movement keeps the fighter facing the
selected enemy, including when that line is diagonal to the stage. Native walk entry and
loop animations own their root motion; target selection does not translate the player.
Double-tap up/down to step into the background/foreground around that enemy; hold the
second tap to keep walking. A single held up/down retains native jump/crouch after the
double-tap window. Fighters use stock walk animations. The Force HUD omits the round timer/infinity symbol
and the unused opponent-side electric decoration. Along-stage travel remains available;
foreground/background walking uses a 1.8 m band around the native Tulip starting floor.
The closer, steeper fixed view keeps depth travel from shrinking combat into the background.
The side camera follows
the player horizontally with a central dead zone. Arena walking and settled combat
movement drive tracking; one-frame native root spikes and depth travel do not.
Horizontal follow commits after
the native pose update, with a 700 mm half-width dead zone, acceleration easing and a
48 mm per battle tick speed limit. The arena view is kept consistent across native
camera publications.

Enemies enter from different off-screen positions using Combot's native run animation, then
focus on the player. One enemy at a time approaches to attack; the others wait around the
player in separate quadrants, with separation between enemies. Attack turns rotate after
the strike and recovery, or pass on if the attacker is
interrupted or defeated. Step across the floor during the windup to evade.
Tap **RB** to cycle the living enemy targets. A gold arrow above the selected enemy marks
the target; a new living target is selected after its defeat. Hold **RB** for 300 ms to release
targeting and move freely in eight directions. Free movement remains active after releasing
RB; tap it again to lock the nearest living enemy. Attacks face the selected target.
The left stick and D-pad retain native directional command
history. Directional attacks and command sequences use the fighter's left/right facing,
including when the fighter faces diagonally. Directions with attack buttons retain their
native move commands immediately. Melee checks keep native hit timing,
height and reach, with a side-view query extending coverage around the target.
With Jun, try
**X** for a jab and **A + B** for a knockdown on an Xbox controller. The default keyboard
equivalents are **Z** and **A + S**. **A** also performs a wake-up kick when your fighter has
landed after a knockdown. The player's life bar is at the top. Each Combot has its own smaller
native Tekken-style bar at the bottom, using the same frame and gradient artwork as the player.
The green CPU label is hidden. An enemy's empty bar clears when it leaves the floor after a knockout.
Reset the encounter to restore all fighters and their life bars.

The mode creates **tekken-force.ini beside the executable** on first use:

~~~ini
[Camera]
PullbackPercent=50
~~~

This adds 50% to the mode's base camera eye-to-target distance. Use 0 for that distance,
100 for twice the distance, or any percentage from 0 through 200. Invalid values use 50;
numeric values outside the range are clamped. Edit the file and reset the encounter to
apply it without rebuilding. The camera follows the player at every setting. A development
launch can select a different file with TTT2_TF_CONFIG. Regular TTT2 does not read this config.

The encounter starts with **Position Settings → Battle Position → Default Position** enabled.
Hold **Back/View** and tap **A** on an Xbox controller to reset all fighters. With the stock
keyboard bindings, hold **Backspace** and tap the letter **A**. The game shows **Back + A Position**
at the lower left when reset is available. Launch without `--tekken-force` for the regular main menu.

The host gesture state can be tested without game assets by building the
`tekken_force_controls_test` target and running `ctest -R tekken_force_controls --output-on-failure`
from the build directory.

### Camera diagnosis

Launch a development session with `TTT2_TF_CAMERA=stationary` to disable follow and
freeze the native view, projection and combined matrix at the initial arena view.
This diagnostic keeps the camera fixed through movement, combat, targeting and reset.
Restart the process to leave the diagnostic or change its initial framing.
`TTT2_TF_CAMERA=legacy` retains the previous two-axis raw-root follow for controlled
comparison. Leave this variable unset for the corrected horizontal follow.

Optional `TTT2_TF_CAMERA_TRACE=<absolute JSONL path>` records raw root/pose positions
for follow-timing correlation, final native camera publications, and the actual c8-c11 vertex
constants consumed by each distinct draw matrix/shader per rendered frame. It does
not replace renderer constants. Trace files and captures are local diagnostics and
must not be committed. Ordinary launches leave both environment variables unset.

The Milestone 9 camera isolation and acceptance evidence is recorded in
[the camera diagnosis notes](port/tools/tekken_force_camera_notes.md). Targeting/movement
comparison changes are a separate step after the owner's camera playtest.
The subsequent bounded-depth visibility pass is documented in
[the visibility notes](port/tools/tekken_force_visibility_notes.md).
