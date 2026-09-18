# The guard's AI is rebuilt on engine systems during M8

The guard's perception, brain and movement move onto `UAIPerceptionComponent`, a
StateTree owned by an `AAIController`, and navmesh pathfinding — replacing a
hand-rolled per-tick world scan, a five-state C++ enum and direct steering.

This deliberately contradicts two documented positions: `EOGuardCharacter.h` says
the current shape is a temporary stand-in to be replaced later, and
`minimal-framework-milestones.md` defines M8 as a feel pass that adds no features.
Doing it now was chosen anyway, on the grounds that every other system is moving
onto engine systems in the same pass and doing the AI separately means a second
disruption.

Recorded because a future reader will otherwise find an AI rewrite inside a
milestone whose own documentation forbids new work, and reasonably wonder why.
The detection ramp and the three-height line-of-sight heuristic are good and
transfer unchanged; they are the part worth not losing.

**Update: done.** Sight is a `UAIPerceptionComponent` on `AEOGuardAIController`,
movement paths over a navmesh, and the five states are a StateTree
(`Content/AI/ST_Guard`) driving C++ tasks. The hand-rolled state machine, the
per-tick scan over every actor in the world, and the direct steering are all
gone.

The pawn still publishes `EEOGuardState`. That is deliberate: the HUD reads it,
the takedown rule turns on it, and the encounter checks assert on it, so the tree
decides behaviour without becoming the only thing that knows what a guard is
doing. It is also what let the 127 ground checks stay meaningful across the
switch rather than being rewritten alongside it.

One consequence worth knowing: resetting a guard now has to restart its tree as
well as its fields. Putting the pawn back is only half of it - the tree otherwise
keeps the state it reached and re-derives the pawn's from it on the next tick.
