// See header.

#include "RaceTrackTestGameMode.h"
#include "Task7Probe.h"
#include "RaceTrack.h"
#include "RaceVehicle.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"

ARaceTrackTestGameMode::ARaceTrackTestGameMode()
{
	DefaultPawnClass = ARaceVehicle::StaticClass();
}

void ARaceTrackTestGameMode::BeginPlay()
{
	Super::BeginPlay();

	// Track owns the start: spawn the actor when the map has none, then
	// snap the vehicle to its start pose before the probe runs.
	ARaceTrack* Track = nullptr;
	for (TActorIterator<ARaceTrack> It(GetWorld()); It; ++It)
	{
		Track = *It;
	}
	if (!Track)
	{
		FActorSpawnParameters P;
		P.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		Track = GetWorld()->SpawnActor<ARaceTrack>(ARaceTrack::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator, P);
	}
	if (Track)
	{
		if (APawn* Pawn = UGameplayStatics::GetPlayerPawn(GetWorld(), 0))
		{
			Pawn->SetActorLocationAndRotation(Track->GetStartPosition(),
				FRotator(0.0f, Track->GetStartYawDeg(), 0.0f), false, nullptr, ETeleportType::TeleportPhysics);
			UE_LOG(LogTemp, Display, TEXT("TRACK7E2E: vehicle snapped to start %s yaw=%.2f"),
				*Track->GetStartPosition().ToString(), Track->GetStartYawDeg());
		}
	}

	FTimerHandle H;
	GetWorldTimerManager().SetTimer(H, [this]()
	{
		FActorSpawnParameters P;
		P.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		GetWorld()->SpawnActor<ATask7Probe>(ATask7Probe::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator, P);
		UE_LOG(LogTemp, Display, TEXT("TRACK7E2E: probe spawned"));
	}, 1.0f, false);
}
