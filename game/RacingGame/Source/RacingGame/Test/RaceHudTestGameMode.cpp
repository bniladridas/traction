// See header.

#include "RaceHudTestGameMode.h"
#include "Task17Probe.h"
#include "RaceTrack.h"
#include "RaceManager.h"
#include "RaceVehicle.h"
#include "RaceAIDriver.h"
#include "RaceHudModel.h"
#include "RaceHudWidget.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"

ARaceHudTestGameMode::ARaceHudTestGameMode()
{
	DefaultPawnClass = ARaceVehicle::StaticClass();
}

void ARaceHudTestGameMode::BeginPlay()
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
	if (Manager)
	{
		Manager->RaceConfig.LapCount = Task17Limits::RaceLaps;
		UE_LOG(LogTemp, Display, TEXT("RACEHUD17E2E: lap count %d for this run only"), Manager->RaceConfig.LapCount);
	}

	// Player on grid slot 0. The probe drives it with pursuit (Task 15
	// pattern): no AI driver of its own, so participant 0 stays on a
	// proven line and finishes ahead of the pack.
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
				UE_LOG(LogTemp, Display, TEXT("RACEHUD17E2E: AI slot %d spawned"), Slot);
			}
		}
	}

	// HUD presentation: bound model from the manager (sole authority),
	// then the thin UMG shell.
	if (Manager)
	{
		HudModel = NewObject<URaceHudModel>(this);
		HudModel->Init(Manager, HudConfig);
		HudWidget = CreateWidget<URaceHudWidget>(GetWorld(), URaceHudWidget::StaticClass());
		if (HudWidget)
		{
			HudWidget->BindModel(HudModel);
			HudWidget->AddToViewport();
		}
		UE_LOG(LogTemp, Display, TEXT("RACEHUD17E2E: hud model bound=%d widget=%d"),
			HudModel->IsBound(), HudWidget != nullptr);
	}

	FTimerHandle H;
	GetWorldTimerManager().SetTimer(H, [this]()
	{
		FActorSpawnParameters P;
		P.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		GetWorld()->SpawnActor<ATask17Probe>(ATask17Probe::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator, P);
		UE_LOG(LogTemp, Display, TEXT("RACEHUD17E2E: probe spawned"));
	}, 1.0f, false);
}