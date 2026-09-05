// See header.

#include "RaceResultsTestGameMode.h"
#include "Task15Probe.h"
#include "RaceTrack.h"
#include "RaceManager.h"
#include "RaceVehicle.h"
#include "RaceAIDriver.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"

ARaceResultsTestGameMode::ARaceResultsTestGameMode()
{
	DefaultPawnClass = ARaceVehicle::StaticClass();
}

void ARaceResultsTestGameMode::BeginPlay()
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

	if (Track)
	{
		if (APawn* Pawn = UGameplayStatics::GetPlayerPawn(GetWorld(), 0))
		{
			FVector Loc;
			float Yaw = 0.0f;
			Track->GetGridPose(0, Loc, Yaw);
			Loc.Z = 40.0f;
			Pawn->SetActorLocationAndRotation(Loc, FRotator(0.0f, Yaw, 0.0f),
				false, nullptr, ETeleportType::TeleportPhysics);
		}
	}

	// Five AI rivals, Task 14 tiers and lines (proven stable field).
	if (Track && Manager)
	{
		const float Tiers[5] = { 0.85f, 1.0f, 0.9f, 0.95f, 1.05f };
		const float Lines[5] = { -120.0f, 120.0f, -120.0f, 120.0f, -120.0f };
		for (int32 Slot = 1; Slot <= 5; ++Slot)
		{
			FVector Loc;
			float Yaw = 0.0f;
			Track->GetGridPose(Slot, Loc, Yaw);
			Loc.Z = 40.0f;
			FActorSpawnParameters P;
			P.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			if (ARaceVehicle* AI = GetWorld()->SpawnActor<ARaceVehicle>(ARaceVehicle::StaticClass(), Loc,
				FRotator(0.0f, Yaw, 0.0f), P))
			{
				URaceAIDriver* Driver = NewObject<URaceAIDriver>(AI, FName(*FString::Printf(TEXT("AIDriver%d"), Slot)));
				Driver->PaceFactor = Tiers[Slot - 1];
				Driver->LineOffset = Lines[Slot - 1];
				Driver->RegisterComponent();
				Manager->RegisterParticipant(AI);
				UE_LOG(LogTemp, Display, TEXT("RACERES15E2E: AI slot %d spawned"), Slot);
			}
		}
	}

	FTimerHandle H;
	GetWorldTimerManager().SetTimer(H, [this]()
	{
		FActorSpawnParameters P;
		P.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		GetWorld()->SpawnActor<ATask15Probe>(ATask15Probe::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator, P);
		UE_LOG(LogTemp, Display, TEXT("RACERES15E2E: probe spawned"));
	}, 1.0f, false);
}
