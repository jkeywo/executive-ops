# Flight is kinematic, not simulated

The aircraft moves by swept transform through a `UFloatingPawnMovement` subclass
rather than as a simulated rigid body driven by forces.

Simulated physics was considered and rejected. It would give plausible collisions
and crashes for free, but a flight model mediated by a solver is markedly harder
to tune for feel, and M8 is explicitly a feel pass. It would also make the
scripted extraction arrival a control problem rather than a lerp, which the
functional tests depend on being deterministic.

The movement module is the seam that makes the swap possible later: the pawn feeds
it a demand either way.
