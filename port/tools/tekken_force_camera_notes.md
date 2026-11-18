# Milestone 9 camera diagnosis and staged acceptance

This records the earlier camera checkpoint. The owner's subsequent 84-second
reference and bounded-depth/framing pass are in
[the visibility notes](tekken_force_visibility_notes.md).

Base: merged main `76e5399` (Milestone 8 PR #4). Branch: `tekken-force-m9`.
This checkpoint changes camera diagnosis/follow only. The fixed-stage-axis targeting
comparison is deferred until the owner playtests the camera correction.

## Acceptance reference

The supplied `tekken_recomp_asIcLvFbJd.mp4` is 99.60 seconds, 2322x1306, 30 fps.
Reviewed nine consecutive source frames around each requested point:

| Time | Source frame center | Inspected frame range |
| --- | --- | --- |
| 00:10.80 | 324 | 320–328 |
| 00:14.60 | 438 | 434–442 |
| 00:33.83 | 1015 | 1011–1019 |
| 01:38.73 | 2962 | 2958–2966 |
| 01:00.50, separate reset/fade | 1815 | 1811–1819 |

The four requested gameplay windows show abrupt scene shifts. The fifth window
fades to black and returns at reset; it is not counted as the same camera fault.
The capture contains no controller event log, so the scripted replay reconstructs
Kazuya movement, attacks, cycling, free/lock transitions and defeats rather than
claiming a frame-exact reproduction of the owner's button sequence.

## Observed isolation results

- `TTT2_TF_CAMERA=stationary` freezes view, projection and combined VP. It preserves
  the existing movement, targeting, turn-taking, stage, enemies and HUD.
- Stationary run: 17,488 camera publications/rendered-frame observations, with
  exactly one view, projection and combined VP value. 695,670 world-draw matrix
  records matched the fixed native VP. Other draw matrices include UI/material
  transforms and are not assumed to be the world camera.
- Across 600 consecutive reconstructed movement/combat frames and a 240-frame
  low-health combat window, a bridge-background ROI had zero integer-pixel translation in
  all 838 adjacent pairs. Consecutive-frame review found no scene jump there.
- The stationary run completed a native 1v4 clear in 104.9 seconds, P1 surviving
  with 115 HP. All four enemies reached zero and left the floor.
- The previous follow mode, using the same scripted movement/attack/cycle sequence,
  reproduced a background step up to 46 pixels across 599 adjacent frame pairs.
  Projection stayed constant. Its final rendered world VP matched the native
  follow publication, rather than an unexplained replacement projection.

## Cause established in the reproduction

At battle tick 921, the root changed during the native update from approximately
(-875.5, -731.0) to (514.9, -1284.6) in floor X/Z, about 1.5 m. Tick 922 sampled
that new root and moved the camera 296.44 mm. The measured eye agrees with the
old `0.20 * (root - follow)` calculation to within 0.001 mm. This publication was
used by world draws in rendered frame 3108.

The previous camera followed a discontinuous native combat/root reference in
both floor axes, before the native frame finished updating/reconciling poses. It
therefore promoted actor repositioning into a visible scene pan on the next tick.
Stationary isolation removes that background jump, while actor repositioning can
still be observed. This identifies the reproduced camera feedback cause; it does
not identify every routine that moved the actor, or prove all four original video
events had an identical internal cause without a trace of that original session.

No additional renderer correction was added to conceal this result. Draw tracing
is read-only. The constant-matrix override is confined to stationary diagnosis.

## Corrected follow

- Only accepted player arena walking updates the tracking reference. Native
  attack lunges, target/opponent repositioning and idle target motion do not.
- Follow is along the fixed stage-right axis, with a 1000 mm half-width central
  dead zone. The camera does not follow floor depth or turn with a selection.
- Follow commits after native pose reconciliation, once per battle tick, using
  20% smoothing and a maximum 24 mm step. Render publications never advance it.
- The existing reset/fade restores starting framing separately.
- Default launch uses corrected follow. `TTT2_TF_CAMERA=legacy` and `stationary`
  are opt-in diagnostic comparisons. `TTT2_TF_CAMERA_TRACE=<JSONL path>` logs
  sampled root/pose, final native view/projection/VP, and actual draw constants.

Initial corrected observations: no camera movement for travel inside the dead
zone; horizontal pan beyond it; zero measured framing drift during settled target
cycling, enemy movement and lock/free transitions; no camera travel for pure depth
walking. A 600-frame reconstructed replay measured at most one pixel of bridge
background translation per adjacent pair, with no pair above three pixels. The
native camera's ordinary step was bounded at 24.00005 mm; projection stayed fixed.

The corrected build completed a native 1v4 clear in 99.6 seconds, P1 surviving at
114 HP, with all enemies removed. A separate actual-KO test confirmed the selected
enemy reached zero, the arrow handed off, and the actor left the floor. Its 240
consecutive post-KO/removal frames had zero measured background translation in all
239 adjacent pairs. Player/enemy root repositioning and depth drift remain visible
with the existing movement/facing implementation; they are separate from the
stable world camera and belong in the deferred comparison, not a claimed fix here.

The final corrected trace contains 27,219 native publications and 27,218 observed
rendered frames. Projection has one value. All ordinary eye steps are at most
24.00005 mm; the three larger movements coincide with explicit resets returning
to starting framing and are treated separately. Build, gesture CTest and camera
CTest passed. Ordinary Arcade retained its stock HUD/camera and applied native
damage (P1 220/230) after explicit combat inputs. An initial passive damage wait
timed out; it was replaced with the explicit combat check, not reported as a pass.

Owner visual acceptance is still pending. The milestone is not complete: hand
off this camera checkpoint before starting the targeting comparison. No milestone
completion PR has been opened at this stage.

## Evidence and limits

Local captures, source-frame sheets, JSONL traces and analysis summaries are under
`port/build-force/playtest/m9-camera`. Raw diagnostic bitmaps are encoded to local
MP4 clips and only those generated raw frames are removed. Readback can slow the
test; these clips are indexed by rendered frame, not a performance benchmark.
Phase correlation uses ROI (800,16)–(1180,160) in 1280x720 native captures. Its
integer-pixel results complement visual review and matrix tracing; they are not
an exhaustive image-quality guarantee.

The camera checkpoint must be handed to the owner before implementing the next
comparison: fixed stage-relative movement axes, initial left/right combat facing,
depth alignment before enemy attacks, and RB/arrow selection that cannot reposition
the player or rotate movement axes. Preserve double-tap/second-hold gestures,
stick/D-pad equivalence, move commands, turn-taking, Tulip, four Combots and gauges.
