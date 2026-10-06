// See header.

#include "RaceGameMode.h"
#include "RaceMenuWidget.h"
#include "RaceTrack.h"
#include "RaceManager.h"
#include "RaceVehicle.h"
#include "RaceAIDriver.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "TimerManager.h"

namespace
{
	const TCHAR* CircuitMapPath = TEXT("/Game/Track/Track1_TestCircuit");
}

ARaceGameMode::ARaceGameMode()
{
	DefaultPawnClass = ARaceVehicle::StaticClass();
}

void ARaceGameMode::BeginPlay()
{
	Super::BeginPlay();

	UE_LOG(LogTemp, Display, TEXT("RACEGAME: production mode boot on %s"), *GetWorld()->GetMapName());
	if (IsRaceMap())
	{
		EnterDefaultRaceState();
	}
	else
	{
		EnterMenuState();
	}
}

void ARaceGameMode::StartDefaultRace()
{
	UE_LOG(LogTemp, Display, TEXT("RACEGAME: loading circuit under production mode"));
	UGameplayStatics::OpenLevel(GetWorld(), CircuitMapPath, true, TEXT("game=/Script/RacingGame.RaceGameMode"));
}

void ARaceGameMode::QuitGame()
{
	UKismetSystemLibrary::QuitGame(GetWorld(), nullptr, EQuitPreference::Quit, false);
}

bool ARaceGameMode::IsRaceMap() const
{
	return GetWorld()->GetMapName().Contains(TEXT("Track1_TestCircuit"));
}

void ARaceGameMode::EnterMenuState()
{
	URaceMenuWidget* Menu = CreateWidget<URaceMenuWidget>(GetWorld(), URaceMenuWidget::StaticClass());
	if (Menu)
	{
		Menu->AddToViewport(100);
		if (APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0))
		{
			FInputModeUIOnly InputMode;
			if (TSharedPtr<SWidget> Focus = Menu->GetFocusTarget())
			{
				InputMode.SetWidgetToFocus(Focus.ToSharedRef());
			}
			PC->SetInputMode(InputMode);
			PC->bShowMouseCursor = true;
		}
		UE_LOG(LogTemp, Display, TEXT("RACEGAME: menu shown"));
	}

	// Headless verification path (mirrors the probe -Task10Shots pattern):
	// -RaceAutoStart invokes the widget's Start handler without a click,
	// so the button and the headless path exercise one function.
	if (FParse::Param(FCommandLine::Get(), TEXT("RaceAutoStart")))
	{
		TWeakObjectPtr<URaceMenuWidget> WeakMenu(Menu);
		FTimerHandle H;
		GetWorldTimerManager().SetTimer(H, [WeakMenu]()
		{
			if (WeakMenu.IsValid())
			{
				WeakMenu->PressStart();
			}
		}, 2.0f, false);
		UE_LOG(LogTemp, Display, TEXT("RACEGAME: autostart armed"));
	}
}

void ARaceGameMode::EnterDefaultRaceState()
{
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
			Pawn->SetActorLocationAndRotation(Track->GetStartPosition(),
				FRotator(0.0f, Track->GetStartYawDeg(), 0.0f), false, nullptr, ETeleportType::TeleportPhysics);
		}
	}

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
			UE_LOG(LogTemp, Display, TEXT("RACEGAME: default AI spawned"));
		}
		Manager->StartRace();
		UE_LOG(LogTemp, Display, TEXT("RACEGAME: default race started"));
	}

	if (APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0))
	{
		PC->SetInputMode(FInputModeGameOnly());
		PC->bShowMouseCursor = false;
	}
}
