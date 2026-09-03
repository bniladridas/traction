// See header. Small explicit state machine, no hidden dynamics.

#include "RaceDrivetrain.h"

void URaceDrivetrain::ApplyConfig(const FRaceVehicleConfig& Config)
{
	ActiveConfig = Config;
	EngineRPM = ActiveConfig.EngineIdleRPM;
	EngineTorque = 0.0f;
	GearIndex = 0;
	Upshifts = 0;
	Downshifts = 0;
	bSawReverse = false;
	UE_LOG(LogTemp, Display, TEXT("RACEDRIVE: config gears=%d up=%.0f down=%.0f fd=%.2f eff=%.2f r=%.3f rev=%.1f"),
		ActiveConfig.GearRatios.Num(), ActiveConfig.ShiftUpRPM, ActiveConfig.ShiftDownRPM,
		ActiveConfig.FinalDriveRatio, ActiveConfig.DrivetrainEfficiency,
		ActiveConfig.WheelRadiusM, ActiveConfig.ReverseRatio);
}

float URaceDrivetrain::EvalTorque(float RPM) const
{
	const TArray<FVector2D>& Points = ActiveConfig.EngineTorqueCurve;
	if (Points.Num() == 0)
	{
		return 0.0f;
	}
	const float R = FMath::Max(0.0f, RPM);
	if (R <= Points[0].X)
	{
		return Points[0].Y;
	}
	for (int32 i = 1; i < Points.Num(); ++i)
	{
		if (R <= Points[i].X)
		{
			const FVector2D& A = Points[i - 1];
			const FVector2D& B = Points[i];
			const float T = (R - A.X) / FMath::Max(1.0f, B.X - A.X);
			return FMath::Lerp(A.Y, B.Y, T);
		}
	}
	return Points.Last().Y;
}

float URaceDrivetrain::CoupledRPM(float SpeedCmS, float Ratio) const
{
	// Wheel angular speed from rolling radius, through the ratios.
	const float WheelRadS = FMath::Abs(SpeedCmS) / 100.0f / FMath::Max(0.01f, ActiveConfig.WheelRadiusM);
	return WheelRadS * Ratio * ActiveConfig.FinalDriveRatio * 60.0f / (2.0f * 3.14159265f);
}

float URaceDrivetrain::UpdateDrive(float Throttle, float Brake, float SpeedCmS)
{
	const int32 NumGears = ActiveConfig.GearRatios.Num();
	const bool bMoving = FMath::Abs(SpeedCmS) > 50.0f;

	// Reverse state transitions with hysteresis. Brake held near
	// standstill engages reverse; throttle past the forward threshold
	// releases back to first.
	if (GearIndex >= 0 && Brake > 0.05f && SpeedCmS < ActiveConfig.ReverseEngageSpeed && SpeedCmS > -ActiveConfig.ReverseEngageSpeed)
	{
		GearIndex = -1;
		bSawReverse = true;
	}
	else if (GearIndex < 0 && Throttle > 0.05f && SpeedCmS > ActiveConfig.ForwardEngageSpeed)
	{
		GearIndex = 0;
	}

	float Request = 0.0f;
	LastShaftTorque = 0.0f;
	if (GearIndex < 0)
	{
		// Reverse gear: brake drives backward through the ratio at every
		// speed, so the force is continuous across standstill and no
		// threshold hand-off can chatter. Throttle recovers forward,
		// which is also how the state releases back to first gear.
		EngineRPM = FMath::Clamp(CoupledRPM(SpeedCmS, ActiveConfig.ReverseRatio),
			ActiveConfig.EngineIdleRPM, ActiveConfig.EngineMaxRPM);
		EngineTorque = EvalTorque(EngineRPM);
		if (Throttle > 0.0f)
		{
			Request = Throttle * ActiveConfig.BrakeForceN;
		}
		else if (Brake > 0.0f)
		{
			const float Shaft = EngineTorque * ActiveConfig.ReverseRatio
				* ActiveConfig.FinalDriveRatio * ActiveConfig.DrivetrainEfficiency;
			LastShaftTorque = Shaft;
			Request = -Brake * Shaft / FMath::Max(0.01f, ActiveConfig.WheelRadiusM);
		}
	}
	else
	{
		if (NumGears > 0)
		{
			GearIndex = FMath::Clamp(GearIndex, 0, NumGears - 1);
		}
		const float Ratio = (NumGears > 0) ? ActiveConfig.GearRatios[GearIndex] : 1.0f;
		EngineRPM = FMath::Clamp(FMath::Max(ActiveConfig.EngineIdleRPM, CoupledRPM(SpeedCmS, Ratio)),
			ActiveConfig.EngineIdleRPM, ActiveConfig.EngineMaxRPM);
		// Automatic shifts with hysteresis, only while rolling.
		if (bMoving && EngineRPM >= ActiveConfig.ShiftUpRPM && GearIndex < NumGears - 1)
		{
			++GearIndex;
			++Upshifts;
			const float NewRatio = ActiveConfig.GearRatios[GearIndex];
			EngineRPM = FMath::Clamp(FMath::Max(ActiveConfig.EngineIdleRPM, CoupledRPM(SpeedCmS, NewRatio)),
				ActiveConfig.EngineIdleRPM, ActiveConfig.EngineMaxRPM);
		}
		else if (bMoving && EngineRPM <= ActiveConfig.ShiftDownRPM && GearIndex > 0)
		{
			--GearIndex;
			++Downshifts;
			const float NewRatio2 = ActiveConfig.GearRatios[GearIndex];
			EngineRPM = FMath::Clamp(FMath::Max(ActiveConfig.EngineIdleRPM, CoupledRPM(SpeedCmS, NewRatio2)),
				ActiveConfig.EngineIdleRPM, ActiveConfig.EngineMaxRPM);
		}
		EngineTorque = EvalTorque(EngineRPM) * Throttle;
		if (Throttle > 0.0f)
		{
			const float Shaft = EngineTorque * Ratio
				* ActiveConfig.FinalDriveRatio * ActiveConfig.DrivetrainEfficiency;
			LastShaftTorque = Shaft;
			Request = Shaft / FMath::Max(0.01f, ActiveConfig.WheelRadiusM);
		}
		else if (SpeedCmS > ActiveConfig.EngineBrakeMinSpeed)
		{
			// Engine braking: small coupled drag through the tire path.
			Request = -ActiveConfig.EngineBrakeForceN;
		}
	}
	return Request;
}
