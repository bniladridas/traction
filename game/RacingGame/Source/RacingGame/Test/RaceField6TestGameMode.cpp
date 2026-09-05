// See header.

#include "RaceField6TestGameMode.h"
#include "Task14Probe.h"
#include "RaceTrack.h"
#include "RaceManager.h"
#include "RaceVehicle.h"
#include "RaceAIDriver.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"

ARaceField6TestGameMode::ARaceField6TestGameMode()
{
	DefaultPawnClass = ARaceVehicle::StaticClass();
}

void ARaceField6TestGameMode::BeginPlay()
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

	// Five AI rivals, frozen tiers and lines matching Task14Probe.h.
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
				UE_LOG(LogTemp, Display, TEXT("RACEFIELD14E2E: AI slot %d pace %.2f line %.0f"), Slot, Tiers[Slot - 1], Lines[Slot - 1]);
			}
		}
	}

	FTimerHandle H;
	GetWorldTimerManager().SetTimer(H, [this]()
	{
		FActorSpawnParameters P;
		P.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		GetWorld()->SpawnActor<ATask14Probe>(ATask14Probe::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator, P);
		UE_LOG(LogTemp, Display, TEXT("RACEFIELD14E2E: probe spawned"));
	}, 1.0f, false);
}
