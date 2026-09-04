// See header.

#include "RaceAITestGameMode.h"
#include "Task9Probe.h"
#include "RaceTrack.h"
#include "RaceManager.h"
#include "RaceVehicle.h"
#include "RaceAIDriver.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"

ARaceAITestGameMode::ARaceAITestGameMode()
{
	DefaultPawnClass = ARaceVehicle::StaticClass();
}

void ARaceAITestGameMode::BeginPlay()
{
	Super::BeginPlay();

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
		}
	}

	ARaceManager* Manager = nullptr;
	for (TActorIterator<ARaceManager> It(GetWorld()); It; ++It)
	{
		Manager = *It;
	}
	if (!Manager)
	{
		FActorSpawnParameters P;
		P.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		Manager = GetWorld()->SpawnActor<ARaceManager>(ARaceManager::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator, P);
	}

	// One AI rival on the grid: behind the start, offset inside the lane.
	if (Track && Manager)
	{
		const FRaceTrackCenterPoint Grid = Track->SampleAtDistance(200.0f);
		const FVector Right(-Grid.Forward.Y, Grid.Forward.X, 0.0f);
		const FVector GridPos = Grid.Position - Right * 200.0f + FVector(0.0f, 0.0f, 60.0f);
		const float GridYaw = FMath::RadiansToDegrees(FMath::Atan2(Grid.Forward.Y, Grid.Forward.X));
		FActorSpawnParameters P;
		P.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		if (ARaceVehicle* AI = GetWorld()->SpawnActor<ARaceVehicle>(ARaceVehicle::StaticClass(), GridPos,
			FRotator(0.0f, GridYaw, 0.0f), P))
		{
			URaceAIDriver* Driver = NewObject<URaceAIDriver>(AI, TEXT("AIDriver"));
			Driver->RegisterComponent();
			Manager->RegisterParticipant(AI);
			UE_LOG(LogTemp, Display, TEXT("RACEAI9E2E: AI spawned at %s"), *GridPos.ToString());
		}
	}

	FTimerHandle H;
	GetWorldTimerManager().SetTimer(H, [this]()
	{
		FActorSpawnParameters P;
		P.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		GetWorld()->SpawnActor<ATask9Probe>(ATask9Probe::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator, P);
		UE_LOG(LogTemp, Display, TEXT("RACEAI9E2E: probe spawned"));
	}, 1.0f, false);
}
