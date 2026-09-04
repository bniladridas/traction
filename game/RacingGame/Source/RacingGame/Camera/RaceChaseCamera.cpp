// See header.

#include "RaceChaseCamera.h"
#include "RaceVehicle.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"

URaceChaseCamera::URaceChaseCamera()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void URaceChaseCamera::Init(USpringArmComponent* InArm, UCameraComponent* InCam)
{
	Arm = InArm;
	Cam = InCam;
	if (Arm)
	{
		Arm->TargetArmLength = CameraConfig.ArmLength;
		Arm->SetRelativeLocation(FVector(0.0f, 0.0f, CameraConfig.HeightOffset));
		Arm->SetRelativeRotation(FRotator(CameraConfig.BasePitchDeg, 0.0f, 0.0f));
		// No collision probe: it snaps the arm near walls, which defeats
		// deterministic measurement. Wall clipping is a visual art-pass
		// concern, not a Task 10 behavior.
		Arm->bDoCollisionTest = false;
	}
	SmoothedRelYaw = 0.0f;
	SmoothedPitch = CameraConfig.BasePitchDeg;
	SmoothedArm = CameraConfig.ArmLength;
	bHasLast = false;
}

void URaceChaseCamera::SnapToTarget()
{
	if (!Arm)
	{
		return;
	}
	Arm->SetRelativeRotation(FRotator(SmoothedPitch, SmoothedRelYaw, 0.0f));
	Arm->TargetArmLength = SmoothedArm;
	bHasLast = false;
}

float URaceChaseCamera::GetArmLength() const
{
	return Arm ? Arm->TargetArmLength : 0.0f;
}

void URaceChaseCamera::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	ARaceVehicle* Pawn = Cast<ARaceVehicle>(GetOwner());
	if (!Pawn || !Arm || DeltaTime <= 0.0f)
	{
		return;
	}
	const FVector PawnLoc = Pawn->GetActorLocation();
	if (bHasLast && FVector::Dist(PawnLoc, LastPawnLoc) > CameraConfig.TeleportSnapDist)
	{
		// Discontinuity: hold current smoothed state, skip easing this
		// tick so no streak forms across the jump.
		LastPawnLoc = PawnLoc;
		Arm->SetRelativeRotation(FRotator(SmoothedPitch, SmoothedRelYaw, 0.0f));
		Arm->TargetArmLength = SmoothedArm;
		return;
	}
	LastPawnLoc = PawnLoc;
	bHasLast = true;

	const float YawRate = Pawn->GetYawRate();
	const float SpeedAbs = FMath::Abs(Pawn->GetForwardSpeed());
	const float TargetRelYaw = FMath::Clamp(YawRate * CameraConfig.LookaheadGain,
		-CameraConfig.LookaheadMaxDeg, CameraConfig.LookaheadMaxDeg);
	const float TargetPitch = CameraConfig.BasePitchDeg + SpeedAbs * CameraConfig.PitchSpeedGain;
	const float TargetArm = FMath::Min(CameraConfig.MaxArmLength,
		CameraConfig.ArmLength + SpeedAbs * CameraConfig.ArmSpeedGain);

	const float K = 1.0f - FMath::Exp(-CameraConfig.SmoothingRate * DeltaTime);
	SmoothedRelYaw += FRotator::NormalizeAxis(TargetRelYaw - SmoothedRelYaw) * K;
	SmoothedPitch += (TargetPitch - SmoothedPitch) * K;
	SmoothedArm += (TargetArm - SmoothedArm) * K;

	Arm->SetRelativeRotation(FRotator(SmoothedPitch, SmoothedRelYaw, 0.0f));
	Arm->TargetArmLength = SmoothedArm;
}
