# Vehicles: V1 Car

Single simcade car. Custom `URaceVehicleMovement` (the template Chaos
vehicle is retained only for project defaults, not driven).

Body: mass 1200 kg, gravity, rolling + quadratic drag. No aero yet
(explicitly deferred).
Per wheel: traced contact, spring-damper suspension
(travel/stiffness/damping), friction-circle tire forces.

Pipeline: input -> `FRaceDriveCommand` -> engine (torque curve x gear x
final drive, `URaceDrivetrain`) -> driven wheels -> tire forces ->
chassis. Service brake stays movement-side.
All params in `FRaceVehicleConfig`. A DataAsset storing the same struct
remains a possible later step, not current architecture.
Keyboard smoothing + gamepad curves. No hardcoded magic.

Cameras: chase driver on every pawn including AI (Task 10: look-ahead,
smoothing, reset snap). Cockpit, speed FOV, and collision remain future.
Audio hooks (RPM, throttle, load, gear, slip) pending.
