// See header. Prototype only; no Task 3 systems (torque, gears, tires).

#include "RaceVehicle.h"
#include "RaceVehicleMovement.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
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
}

void ARaceVehicle::BeginPlay()
{
	Super::BeginPlay();
	InitialTransform = GetActorTransform();
	VehicleMovement->SetUpdatedComponent(CollisionBox);
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
	VehicleMovement->SetThrottle(Value);
}

void ARaceVehicle::ApplyBrake(float Value)
{
	VehicleMovement->SetBrake(Value);
}

void ARaceVehicle::ApplySteering(float Value)
{
	VehicleMovement->SetSteering(Value);
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
