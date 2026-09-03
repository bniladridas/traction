// See header. Prototype only; no Task 3 systems (torque, gears, tires).

#include "RaceVehicle.h"
#include "RaceVehicleMovement.h"
#include "RaceDrivetrain.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "Engine/World.h"
#include "UObject/ConstructorHelpers.h"

ARaceVehicle::ARaceVehicle()
{
	CollisionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("CollisionBox"));
	CollisionBox->SetBoxExtent(FVector(110.0f, 55.0f, 40.0f));
	CollisionBox->SetCollisionProfileName(TEXT("Pawn"));
	RootComponent = CollisionBox;

	CubeMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CubeMesh"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeAsset(TEXT("/Engine/BasicShapes/Cube"));
	if (CubeAsset.Succeeded())
	{
		CubeMesh->SetStaticMesh(CubeAsset.Object);
	}
	CubeMesh->SetRelativeScale3D(FVector(2.2f, 1.1f, 0.8f));
	CubeMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	CubeMesh->SetupAttachment(RootComponent);

	CameraArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraArm"));
	CameraArm->TargetArmLength = 500.0f;
	CameraArm->SetRelativeLocation(FVector(0.0f, 0.0f, 140.0f));
	CameraArm->SetRelativeRotation(FRotator(-14.0f, 0.0f, 0.0f));
	CameraArm->bInheritPitch = false;
	CameraArm->bInheritRoll = false;
	CameraArm->SetupAttachment(RootComponent);

	ChaseCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("ChaseCamera"));
	ChaseCamera->SetupAttachment(CameraArm);

	VehicleMovement = CreateDefaultSubobject<URaceVehicleMovement>(TEXT("VehicleMovement"));

	Drivetrain = CreateDefaultSubobject<URaceDrivetrain>(TEXT("Drivetrain"));
}

void ARaceVehicle::BeginPlay()
{
	Super::BeginPlay();
	VehicleMovement->SetUpdatedComponent(CollisionBox);
	VehicleMovement->ApplyConfig(VehicleConfig);
	VehicleMovement->SetDrivetrain(Drivetrain);
	Drivetrain->ApplyConfig(VehicleConfig);
	SettleToGround();
	InitialTransform = GetActorTransform();
}

void ARaceVehicle::SettleToGround()
{
	UWorld* World = GetWorld();
	if (!World || !CollisionBox)
	{
		return;
	}
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(this);
	FHitResult Hit;
	const FVector Loc = GetActorLocation();
	if (World->LineTraceSingleByChannel(Hit, Loc, Loc - FVector(0.0f, 0.0f, 500.0f), ECC_WorldStatic, Params))
	{
		FVector Settled = Loc;
		Settled.Z = Hit.Location.Z + CollisionBox->GetScaledBoxExtent().Z;
		SetActorLocation(Settled, false, nullptr, ETeleportType::TeleportPhysics);
	}
}

void ARaceVehicle::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	// Legacy bindings (config in DefaultInput.ini). A future pass migrates
	// these to Enhanced Input actions; the Apply* API stays unchanged.
	PlayerInputComponent->BindAxis(TEXT("RaceThrottle"), this, &ARaceVehicle::OnThrottleAxis);
	PlayerInputComponent->BindAxis(TEXT("RaceSteer"), this, &ARaceVehicle::OnSteerAxis);
	PlayerInputComponent->BindAction(TEXT("RaceReset"), IE_Pressed, this, &ARaceVehicle::ResetVehicle);
}

void ARaceVehicle::OnThrottleAxis(float Value)
{
	// Single axis carries both pedals: forward is throttle, back is brake.
	if (Value >= 0.0f)
	{
		ApplyThrottle(Value);
		ApplyBrake(0.0f);
	}
	else
	{
		ApplyThrottle(0.0f);
		ApplyBrake(-Value);
	}
}

void ARaceVehicle::OnSteerAxis(float Value)
{
	ApplySteering(Value);
}

void ARaceVehicle::ApplyThrottle(float Value)
{
	PendingCommand.Throttle = FMath::Clamp(Value, 0.0f, 1.0f);
	VehicleMovement->SetDriveCommand(PendingCommand);
}

void ARaceVehicle::ApplyBrake(float Value)
{
	PendingCommand.Brake = FMath::Clamp(Value, 0.0f, 1.0f);
	VehicleMovement->SetDriveCommand(PendingCommand);
}

void ARaceVehicle::ApplySteering(float Value)
{
	PendingCommand.Steering = FMath::Clamp(Value, -1.0f, 1.0f);
	VehicleMovement->SetDriveCommand(PendingCommand);
}

void ARaceVehicle::ResetVehicle()
{
	ApplyThrottle(0.0f);
	ApplyBrake(0.0f);
	ApplySteering(0.0f);
	VehicleMovement->ResetSpeed();
	SetActorTransform(InitialTransform, false, nullptr, ETeleportType::TeleportPhysics);
}

float ARaceVehicle::GetForwardSpeed() const
{
	return VehicleMovement ? VehicleMovement->GetForwardSpeed() : 0.0f;
}

float ARaceVehicle::GetLateralSpeed() const
{
	return VehicleMovement ? VehicleMovement->GetLateralSpeed() : 0.0f;
}

float ARaceVehicle::GetYawRate() const
{
	return VehicleMovement ? VehicleMovement->GetYawRate() : 0.0f;
}

float ARaceVehicle::GetThrottleInput() const
{
	return VehicleMovement ? VehicleMovement->GetDriveCommand().Throttle : 0.0f;
}

float ARaceVehicle::GetBrakeInput() const
{
	return VehicleMovement ? VehicleMovement->GetDriveCommand().Brake : 0.0f;
}

FVector ARaceVehicle::GetTotalTireForce() const
{
	return VehicleMovement ? VehicleMovement->GetTotalTireForce() : FVector::ZeroVector;
}

float ARaceVehicle::GetVerticalSpeed() const
{
	return VehicleMovement ? VehicleMovement->GetVerticalSpeed() : 0.0f;
}

bool ARaceVehicle::IsGrounded() const
{
	return VehicleMovement && VehicleMovement->IsGrounded();
}

bool ARaceVehicle::GetWheelContact(int32 Index) const
{
	return VehicleMovement && VehicleMovement->GetWheelContact(Index);
}

float ARaceVehicle::GetWheelCompression(int32 Index) const
{
	return VehicleMovement ? VehicleMovement->GetWheelCompression(Index) : 0.0f;
}

float ARaceVehicle::GetWheelNormalLoad(int32 Index) const
{
	return VehicleMovement ? VehicleMovement->GetWheelNormalLoad(Index) : 0.0f;
}

float ARaceVehicle::GetWheelLongForce(int32 Index) const
{
	return VehicleMovement ? VehicleMovement->GetWheelLongForce(Index) : 0.0f;
}

float ARaceVehicle::GetWheelLatForce(int32 Index) const
{
	return VehicleMovement ? VehicleMovement->GetWheelLatForce(Index) : 0.0f;
}

FVector ARaceVehicle::GetWheelContactPoint(int32 Index) const
{
	return VehicleMovement ? VehicleMovement->GetWheelContactPoint(Index) : FVector::ZeroVector;
}

FVector ARaceVehicle::GetWheelContactNormal(int32 Index) const
{
	return VehicleMovement ? VehicleMovement->GetWheelContactNormal(Index) : FVector::UpVector;
}

const FRaceVehicleConfig& ARaceVehicle::GetActiveConfig() const
{
	return VehicleMovement->GetActiveConfig();
}

float ARaceVehicle::GetEngineRPM() const
{
	return Drivetrain ? Drivetrain->GetEngineRPM() : 0.0f;
}

float ARaceVehicle::GetEngineTorque() const
{
	return Drivetrain ? Drivetrain->GetEngineTorque() : 0.0f;
}

int32 ARaceVehicle::GetGearIndex() const
{
	return Drivetrain ? Drivetrain->GetGearIndex() : 0;
}

int32 ARaceVehicle::GetUpshiftCount() const
{
	return Drivetrain ? Drivetrain->GetUpshiftCount() : 0;
}

int32 ARaceVehicle::GetDownshiftCount() const
{
	return Drivetrain ? Drivetrain->GetDownshiftCount() : 0;
}

float ARaceVehicle::GetLastShaftTorque() const
{
	return Drivetrain ? Drivetrain->GetLastShaftTorque() : 0.0f;
}
