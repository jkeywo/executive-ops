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
