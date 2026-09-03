// RacingGame drivetrain (Task 6). Engine torque, RPM, automatic
// transmission, and open differential in one small deterministic layer.
// It produces longitudinal force requests for the driven wheels and never
// applies forces to the vehicle body; the tire path stays authoritative.
// Coupling is rigid (no clutch slip, no torque converter): RPM follows the
// wheels above idle, floored at idle, capped at the configured maximum.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "RaceVehicleConfig.h"
#include "RaceDrivetrain.generated.h"

UCLASS(ClassGroup = Vehicle, meta = (BlueprintSpawnableComponent))
class RACINGGAME_API URaceDrivetrain : public UActorComponent
{
	GENERATED_BODY()

public:
	void ApplyConfig(const FRaceVehicleConfig& Config);

	// Advances gear and RPM state, then returns the total longitudinal
	// request in newtons for the driven axle. Positive drives forward
	// (or holds reverse context per state); the movement splits it over
	// contacting driven wheels into the tire path.
	// Throttle and Brake are normalized 0 to 1. Speed in cm/s, signed.
	float UpdateDrive(float Throttle, float Brake, float SpeedCmS);

	float GetEngineRPM() const { return EngineRPM; }
	float GetEngineTorque() const { return EngineTorque; }
	float GetLastShaftTorque() const { return LastShaftTorque; }
	// -1 is reverse, 0 to N-1 are forward gears.
	int32 GetGearIndex() const { return GearIndex; }
	int32 GetUpshiftCount() const { return Upshifts; }
	int32 GetDownshiftCount() const { return Downshifts; }
	bool SawReverse() const { return bSawReverse; }

private:
	float EvalTorque(float RPM) const;
	float CoupledRPM(float SpeedCmS, float Ratio) const;

	FRaceVehicleConfig ActiveConfig;
	float EngineRPM = 1000.0f;
	float EngineTorque = 0.0f;
	float LastShaftTorque = 0.0f;
	int32 GearIndex = 0;
	int32 Upshifts = 0;
	int32 Downshifts = 0;
	bool bSawReverse = false;
};
