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

## Sidestep and body contact (20 November 2026)

The owner recordings `sideSteps.mp4` and `collisions.mp4` show the input history
alongside abrupt lateral movement and fighters occupying the same floor space.
A local replay on `aff6631` reproduced both. In the alternating up/down sequence,
native lateral walk and step animations still contributed root motion after
Force had applied its own displacement. The committed root reached 1,489 mm
of horizontal travel in one tick, and the render publication reached 1,502 mm.
A live pair came within 67 mm in the replay.

Each locked depth gesture now keeps its initial step direction, and Force owns
the displacement of native lateral poses. Body contact sweeps the proposed
floor movement against all other live fighters, stopping at 420 mm of root
separation. This applies to player movement, enemy approaches and spacing, and
native displacement before hit detection and model update. Attack hit volumes
remain separate from body contact.

The final Windows build passed both Force tests and a scripted replay through
the normal Force launcher. The alternating sidestep replay had a largest
published horizontal change of 200 mm during a guard reaction, versus 1,502 mm
on the starting build. Other ordinary step samples reached 24 mm per tick.
The nearest sampled live body gap was 420 mm across sidesteps, dash attempts,
free walking, and combat. RB cycling visited all four enemies. All four could
still take damage, and the position reset restored all five health values.
These results describe the sampled sequence and leave physical controller
feel and pose or limb overlap for visual playtesting.

### Pose-aware contact follow-up

The follow-up recording `collisions_2.mp4` still shows fighters crowding into each
other during close movement. Inspecting adjacent frames around 5.0 s, 32.85 s,
and 40.1 s showed overlapping walk and guard poses. On `2cd0bba`, a Kazuya
replay reproduced overlap between the native pelvis volumes even when the
fighter roots held the 420 mm minimum.

Force now uses the native upper-body and pelvis push spheres relative to their
pose origin. It sweeps the shapes during steering and native displacement and
resolves overlap from an animation's lean even when both roots are stationary.
The separate model displacement also gets a contact check before model update.
An enemy can start its attack if the player's body stops its approach, so a
wider stance does not leave it walking in place outside the old range check.

The original and first pose-based Kazuya runs recorded 1,586 and 22 raw
player/enemy pelvis-pair samples respectively with over 100 mm of penetration.
Those are read-only samples of native buffer contents, which can be read before
collision resolution or after the pose has advanced. The final model check was
run with Miguel on Tulip through the usual launcher. Rendered close-contact
captures showed clearer torso spacing during walking and sidestepping. During
combat all four enemies lost health (230/230/180/230 to 186/195/136/195), and
reset restored the player and all four enemies. Windows build and both Force
tests passed, including an offset-body sweep, a vertical clearance case and
overlap from a pose change without root movement.

Some raw overlap samples and a large native attack-transition pose change
remained in the Miguel run. This check covers collision of the core volumes.
Limb and mesh contact and physical controller feel still need visual playtesting.
The local renders and logs are in `port/build-force/playtest/m9-visibility` as
`pushbox-base-*`, `pushbox-first-*` and `pushbox-model-*`.

### Walk cycles at body contact

In `collisions_3.mp4`, the player advances into the selected Combot and can
appear briefly on the other side of it, for example around 27 s. Miguel's
scripted replay on `98d103d` reproduced the underlying drift on all four
targets. The collision-approved root stayed fixed while the displayed walk
pose kept advancing by about 33 mm per tick. When the animation re-anchored
partway through the loop, the pose snapped back by up to 704 mm in one tick.

The native animation phase advances the pose after the original model check.
Force now samples the per-frame walk displacement at the end of that phase,
clips it against the same live bodies, and publishes the accepted position to
both the gameplay root and the model. Large changes to the native walk anchor
and walk-cycle resets no longer apply the whole offset a second time. Real
forward and backward walk animations are recognized by their native names.
Miguel's moveset stores them separately from his neutral stance, so assuming
that the next move is forward walk could start his alternate stance instead.

A scripted Miguel replay through the normal launcher used both D-pad and stick,
approached and backed away from all four lock targets, cycled lock, used free
movement, and double-tapped both depth directions. The largest adjacent
published player pose change in the approach sequence fell from 704 to
109 mm including reactions, and in the retreat sequence from 659 to 45 mm.
The approach had zero samples over 300 mm of divergence between the published
pose and gameplay root, down from 358. The final replay still damaged all four
Combots and reset all five fighters. A separate replay defeated a Combot,
waited for it to leave the stage, cycled to a surviving target, and held
forward into contact. Native attack poses can still overlap
visually. These measurements are from scripted input rather than a physical
controller. Local samples and camera traces are named `c3-base` and `c3-phase`
in `port/build-force/playtest/m9-visibility`.
