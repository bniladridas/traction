# Audio

V1: engine (RPM x throttle x load -> pitch + layer crossfade), gear shift blip,
tire rolling/skid (slip driven), wind (speed), impacts/suspension thumps, UI clicks.

Structure: `UAudio` params updated per tick from vehicle. No static loop pitch.
FMOD only if UE audio proves insufficient. Defer that decision to M6.
