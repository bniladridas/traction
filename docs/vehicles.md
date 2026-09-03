# Vehicles: V1 Car

Single simcade car. Chaos or custom `UVehiclePhysicsComponent`.

Body: mass, CoG height, drag, downforce (speed-squared).
Per wheel: steer angle (front), drive torque (rear-biased), brake force, grip/slip curve, suspension travel/stiffness/damping.

Pipeline: input -> engine (torque curve x gear x final drive) -> wheels -> chassis.
All params in DataAsset. Keyboard smoothing + gamepad curves. No hardcoded magic.

Cameras: chase (spring arm, speed FOV, collision) + cockpit (wheel/dash visible).
Audio hooks: RPM, throttle, load, gear, slip ratio.
