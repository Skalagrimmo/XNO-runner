# Vector Hazard Art Direction

**Status:** art/gameplay direction for the next M1 rendering pass  
**Date:** 2026-11-20

## 1. Core visual identity

XNO Runner should feel like a mechanical digital corridor that is alive. Hazards are not static boxes placed on a track: they are dormant mechanisms, broken infrastructure, and hostile data structures that react to the approaching runner.

The visual language combines:
- Wireframe machinery with visible hinges, rails, pivots, and linkages.
- Large, readable silhouettes that can be recognized on a small phone display.
- Motion driven by distance-to-player and fixed simulation time, not frame-count animation.
- A restrained palette: cyan/teal for corridor infrastructure, amber for warning, magenta/orange for active danger, and a very dark void for missing geometry.
- Short, legible warning phases before collision becomes possible.

The TRON-like inspiration is limited to the abstract digital-grid mood and the idea of a runner fragmenting into data on failure. All geometry, names, signage, and effects should be original to XNO Runner.

## 2. Hazard concepts

### A. Unfolding mechanism — FOLD_GATE

**Silhouette:** a dormant narrow frame embedded in the floor/wall. As the player approaches, paired articulated arms rotate outward and lock into a wide barrier across one lane. Secondary braces slide into place to make the mechanism feel mechanical rather than magical.

**Animation:**
1. Dormant: a thin vertical spine and two folded arms, low-intensity cyan.
2. Wake: hinge nodes illuminate amber; the arms begin to rotate.
3. Deployment: arms extend with a quick, readable snap; braces connect.
4. Active: the barrier remains across the lane with a pulsing danger edge.
5. Passed: the arms retract or collapse into a compact frame behind the player.

**Gameplay:** blocks one lane. The player must move to either of the other two lanes before the deployment finishes. This is the first obstacle family to implement.

### B. Bit chasm — BIT_CHASM

**Silhouette:** several rectangular floor plates disappear into a dark gap. The edge is a broken vector outline, with small grid squares detaching and falling into the void. A few isolated scan-lines make the depth readable.

**Animation:**
1. Stable floor: normal corridor grid.
2. Warning: floor seams light amber and plates begin separating.
3. Collapse: tiles detach in deterministic groups and drop below the floor plane.
4. Open gap: a dark interruption with animated falling fragments and bright edge rails.
5. Recovery: distant plates reassemble only after the player has passed.

**Signature gameplay mechanic:** BIT_CHASM spans the full corridor and cannot be bypassed by changing lanes. When the player enters the contextual-action window, the right action button becomes available:

1. First press — ANCHOR. A single plate in the player's current lane locks in mid-void, and the player blinks onto it.
2. The run's forward distance pauses. The plate visibly fractures and its fixed-time collapse timer continues.
3. Second press — BLINK. The player teleports from the plate to solid ground beyond the gap.
4. If the second press does not arrive before the plate collapses, the player falls and the run ends.

Only BIT_CHASM owns this two-stage action. It is not a universal dodge, jump, or ability for other hazard families. The first press is valid only inside the approach window; pressing too early or after the edge has no effect.

### C. Iris / spider gate — IRIS_GATE

**Silhouette:** six or eight long blades pivot around an off-centre hub, opening like a mechanical iris or the legs of a metal insect. The blades do not form a generic flat wall; their rotation and layered depth make the silhouette distinct.

**Animation:** a quiet central hub pulses, blades unfold in sequence, then lock into a dangerous aperture shape. The gaps between blades should remain visually obvious, but the gameplay collision must still correspond to the chosen lane rules.

**Gameplay:** blocks one lane in the first implementation. A later variant can leave one timed safe aperture.

### D. Raster sweeper — RASTER_SWEEPER

**Silhouette:** a suspended axle with two or three long vector bars. It rotates or sweeps through the corridor like a scanning machine. Thin secondary lines trail its movement.

**Gameplay:** later timing hazard. Requires either a safe-phase opening or a lane pattern that can be validated deterministically.

### E. Data mine — DATA_MINE

**Silhouette:** an octahedral or wireframe polyhedron floating just above the floor, with nested internal geometry and a small orbiting ring. On activation, its faces misalign and split into independent vector fragments.

**Gameplay:** initially blocks one lane as a contact hazard; a later collectible/bonus variant may be safely passed or collected from the adjacent lane. Keep hazard and reward variants unmistakably different.

## 3. Player failure effect — BIT_DISINTEGRATION

When collision is confirmed by the simulation:
1. Freeze runner progress and record the exact collision obstacle ID.
2. Preserve the current player silhouette as a wireframe for a brief impact flash.
3. Break the silhouette into small square/vector fragments.
4. Send fragments outward and downward with deterministic directions derived from run seed, obstacle ID, and fragment index.
5. Fade fragments over a short interval; leave a small cluster of pixels before showing the result/restart state.

The effect is presentation only. It must not determine the collision, alter score, or consume random values from the obstacle-generation stream. Its own deterministic seed must be separate.

## 4. Animation and simulation contract

- The runner simulation remains authoritative for obstacle identity, lane, world distance, collision window, and hit result.
- The renderer receives read-only hazard descriptors and derives hinge/blade/tile animation from simulation distance and a fixed animation profile.
- The warning trigger must be far enough ahead to make a lane change possible. Initial tuning target: wake 12–16 world units before the collision plane; validate against lane transition time and current speed.
- The geometry must not visually become collidable before the simulation considers it active, nor remain visibly harmless after collision is enabled.
- Animation must be deterministic for a run seed and obstacle ID.
- Rebuilding a scene at the same simulation tick must produce the same geometry.
- No allocation-heavy particle effects in the render loop if a reusable buffer can be used.
- Avoid excessive thin lines. Silhouette and motion take priority over decorative detail.

## 5. First implementation order

1. FOLD_GATE: first visible hazard; two hinged arms and a crossbar, one lane blocked.
2. BIT_CHASM: second visible hazard; removable floor panels and a broken edge outline, initially lane-blocking only.
3. BIT_DISINTEGRATION: crash presentation using the confirmed collision obstacle ID.
4. IRIS_GATE: alternate mechanical silhouette.
5. RASTER_SWEEPER and DATA_MINE: later variety after timing and readability are tested.

## 6. Acceptance criteria

- Each hazard is recognizable from its silhouette at phone resolution.
- Warning → deployment → active → passed states are visually distinct.
- The same seed and simulation tick reproduce the same hazard geometry.
- FOLD_GATE's blocked lane is readable before its collision window.
- BIT_CHASM visibly spans all three lanes and is not avoidable through lateral movement.
- The first contextual press stabilizes a plate in the current lane and freezes forward distance.
- The second contextual press completes the teleport to solid ground before the fixed timer expires.
- Missing the second press produces a deterministic BIT_CHASM fall.
- Collision rules and visual deployment remain synchronized.
- The bit-disintegration effect runs only after a confirmed crash and never changes the run result.
- Hazards are tested on the existing SM-J710F baseline before increasing line count or animation complexity.

## 7. M1 art milestone definition

A meaningful next build should contain at least two rendered hazard families—FOLD_GATE and BIT_CHASM—plus the BIT_DISINTEGRATION crash effect. A moving corridor with invisible simulation hazards is not sufficient as a player-facing runner.
