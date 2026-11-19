# Milestone 9 staged camera and visibility pass

Starting checkpoint: `bda7ae5` on `tekken-force-m9`, already pushed and recoverable.
The owner's expanded request is implemented in stages: camera/visibility first,
then backward animation, targeting/crowds, and presentation. This pass stops for
the first handoff. It does not claim the whole milestone is complete.

## Reference and investigation

The pasted request omitted a video filename. The latest local Tekken capture,
`tekken_recomp_ntqhc1BHr0.mp4` (84.16 s, 3522x1982, 30 fps), matches its reset near
73.5 s and is used as the reference. Launcher confirmation was requested from the
owner; that is distinct from the directly verified file and frame contents.

Inspected consecutive source frames 68–76 (2.40 s), 1115–1123 (37.30 s),
1577–1585 (52.70 s), and 2201–2209 (73.50 s reset, separately). The later frames
show the fight receding far into the flower field and leaving extensive foreground.
The three gameplay windows contain lateral scenery steps; the reset fades to black.

Before changing framing, repeated the constant-view/projection diagnosis. Across
360 consecutive captured frames the bridge-background ROI showed zero translation.
5,764 sampled native publications had exactly one view, projection and combined VP.
The starting checkpoint's follow replay retained a constant projection and bounded
24 mm camera steps, but its background ROI moved by up to 5 pixels per rendered
frame. The full size of the marked source-video shifts was not reproduced, so no
new renderer override is justified or claimed to fix an established alternate path.
Earlier raw-root feedback findings remain in `tekken_force_camera_notes.md`.

Visibility has a directly observed cause: unlimited walkable depth with a camera
that only follows horizontally. The camera intentionally does not chase the fight
into the background. The old free/target-relative movement can move it many metres
away, reducing projected character size. The old wide/low framing compounds the
flower obstruction. This pass addresses those geometry/framing issues directly.

## Changes

- Constrain host-controlled floor travel to stage-relative depth -900 to +900 mm
  around the native Tulip starting area, without changing along-stage coordinates.
  Apply the same floor constraint to player/enemy steering and pose reconciliation.
  Existing attack/reaction move IDs, frames, height and along-stage knockback remain.
- Use a fixed 4.8 m base distance, 1.8 m rise and 0.7 m aim height, scaled by the
  existing pullback setting. Default 50 means 7.2 m distance, 2.7 m rise and 1.05 m
  aim height. The viewing angle is a constant approximately 20.6 degrees; there is
  no combat-driven zoom. Reset reloads framing settings as before.
- Narrow the horizontal dead zone to 700 mm half-width. Ease acceleration/deceleration
  at 1.5 mm/tick per tick with a 48 mm/tick speed cap. Discrete braking
  prevents a sharp stop at the dead-zone edge; reversal is covered by the host test.
- A longer combat check exposed another visibility cause: the walking-only camera
  reference became stale during attack displacement, leaving P1 outside the viewport
  in 3,409 of 16,945 sampled publications. Track settled combat/reaction displacement
  as well, including the short return-to-idle period. Require two distinct battle
  ticks to confirm a large root change; repeated callbacks cannot confirm a spike.
  Target selection alone still supplies no tracking input. The higher speed ceiling
  lets the eased camera catch up with combat lunges without an abrupt velocity change.
- Add a `position-reset` trace marker to distinguish explicit resets from ordinary
  camera continuity. Keep the existing stationary/legacy diagnostic modes.

## Checks completed before handoff

- Windows build; gesture test; camera test including acceleration, reversal and
  rotated stage-axis depth constraints. An initial easing test caught a sharp stop
  near the zone edge; discrete braking was corrected before the gameplay build.
- Native controller-input replay: walking, double taps and second holds, attacks,
  RB cycling, hold-to-release, reacquisition and reset. This is scripted native
  Xbox input, not a claimed physical DualSense playtest.
- At default pullback 50, projecting a 1.8 m standing reference gives 271.9–327.6 px
  height across the walking band in a 1280x720 frame; foot Y is 462.9–607.1 px.
  Minimum pullback 0 also keeps the feet within the viewport (maximum Y 654.4 px).
  Configuration was restored to 50.
- Sampled free-depth travel and combat kept the tested roots near the walking band
  and P1 at ground Y=0. Native committed animation/pose updates can briefly publish
  positions beyond the steering band; this is not an absolute collider wall or a
  claim that every native actor-reposition path is fixed. Those excursions are
  retained as a limitation for subsequent targeting/animation work.
- Rendered front/center/back screenshots show larger fighters and a more readable
  ground plane. Flowers and crowd occlusion remain; no foliage assets were removed.
- Final 360-frame controller replay: maximum measured bridge-background translation
  3 px in adjacent native frames, compared with 5 px in the starting checkpoint
  replay. This is a continuity observation, not proof about every source-video event.
- After the settled-combat-reference correction, a fresh native 1v4 clear passed
  in 58.1 seconds with P1 at 141 HP. P1 stayed inside the horizontal viewport in
  all 14,008 publications in the final sampled session (screen X approximately
  345–1051 in 1280x720). The actual KO/removal controller clip contains 360
  consecutive frames, maximum background translation 2 px across adjacent pairs.
  Reset restored all five actors at independent full health. Settled target cycling
  and hold/free/reacquire transitions produced zero measured camera drift.
- The final session had one projection value. Ordinary eye steps reached at most
  40.50002 mm with acceleration changes at most 1.50022 mm/tick per tick; explicit
  reset markers are excluded. Projected pose-root foot Y remained about 435–653 px.
- Normal offline Arcade applied native damage (opponent 157/170, P1 230/230) and
  retained its stock camera and HUD. No Force framing/band behavior is active there.

The capture/trace evidence is local under `port/build-force/playtest/m9-visibility`.
`camera-visibility-playtest.mp4` is the current native rendered-frame recording
of combat and KO/removal after the settled-reference fix. Earlier `final-*`/baseline
captures are intermediate evidence, not the current build's acceptance clip.
Readback can slow input
timing, so it is not a performance benchmark or exact controller-log reconstruction.

## Remaining stages

The owner subsequently replaced the proposed fixed-axis targeted comparison with native
full-focus duel movement; see the refinement checkpoint below. Enemy depth alignment,
crowd spacing, selected gauge highlight, character-name HUD and Practice readout/reset-hint
suppression remain later work. Preserve Tulip, four independent
Combots, native health bars, exclusive attack turns and the current encounter loop.


## Native duel and HUD refinement (19 November 2026)

The owner's latest instruction supersedes the fixed-stage-axis targeting comparison:
locked forward/backward movement follows the line to the selected enemy like a normal
match. Free movement and double-tap foreground/background gestures remain available.

Observed cause of the backward slide: the stock Kazuya backward entry started, but
`sub_82314EA0` subsequently replaced native walk choices with `neutral + 1`, restarting
a forward-walk pose at frame 1. Host arena translation still moved the fighter away.
The custom walk reconciliation also discarded native locomotion root displacement.

Locked horizontal locomotion now retains native input execution, walk/backstep choices,
entry/loop transitions and native root movement. Arena translation/reconciliation applies
only to free mode and explicit depth gestures. The existing settled-root filter supplies
native walking travel to the horizontal camera. Target selection still changes facing,
not the player's floor position or camera basis. Committed moves/reactions retain their
native motion and duration.

The native timer draw is suppressed only in Force Practice. A scoped sprite-call check
removes the CPU-side under-gauge light, preserving the player light and small native
health bars. Costume requests use the two native Combot entries, with default equipment
scheme indices 0/1 assigned by slots 1,3,2,4 as 0,1,0,1. Equipment absent from the selected
scheme is cleared. Combot's native fighting-profile request remains enabled: disabling
it loaded an untuned Combot move table and was rejected during testing. The boot-time
practice dummy is deliberately not treated as a battle Combot.

Observed checks on the source-built Windows executable:
- Kazuya and Jun native backward entries/loops advance with both D-pad and stick.
  All four selected opponents: backward entry and loop advance normally with D-pad/stick;
  both native left/right facings exercised. Sampled backward-walk facing dot product
  against the actual selected enemy was at least 0.999 in the final focused replay.
- Native directional attacks included Kazuya shoryu and low attacks. All four independent
  enemy health values decreased during normal input-only combat. No HP/pose memory writes
  were used to manufacture hits or movement.
- Double-tap depth/second hold, RB cycle, hold-to-free and nearest reacquisition replayed.
  Reset restored P1 and all four enemies to their independent full health values.
- Ordinary offline Arcade retained the stock clock/two-side HUD and applied native
  damage (opponent 168/180 while P1 remained 180/180 in the sampled check).
- Rendered Force frames show no infinity symbol or CPU-side electric light. Player light,
  top player bar, four bottom gauges and target arrow remain.
- Read-only wardrobe inspection confirms native scheme-0 items on slots 1/2 and scheme-1
  items on slots 3/4. The extra Practice model still shows a pre-existing protruding mesh
  artifact; these item-table checks do not establish that its entire rendered costume is
  correct. Do not describe this as a completed fifth-model rendering repair.

- Final session: 14,665 parsed camera publications had one projection (one incomplete/
  interleaved trace line was excluded). Final 360-frame native replay: background translation at most
  5 px between adjacent frames in the tracked bridge ROI. This is a continuity check,
  not proof that all events in earlier owner recordings are resolved.

Native captures and validation samples are local in `port/build-force/playtest/m9-visibility`.
These are scripted native controller inputs, not a physical DualSense acceptance test.
The wider milestone remains open for owner playtesting and remaining refinements.
