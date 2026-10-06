// See header.

#include "RaceGameMode.h"
#include "RaceMenuWidget.h"
#include "RaceSelectWidget.h"
#include "RaceSelectData.h"
#include "RaceResultsWidget.h"
#include "RaceHudModel.h"
#include "RaceHudWidget.h"
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

ARaceGameMode::ARaceGameMode()
{
	DefaultPawnClass = ARaceVehicle::StaticClass();
}

void ARaceGameMode::InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage)
{
	Super::InitGame(MapName, Options, ErrorMessage);
	FParse::Value(*Options, TEXT("car="), SelectedCar);
	FParse::Value(*Options, TEXT("track="), SelectedTrack);
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

void ARaceGameMode::OpenMenu()
{
	if (SelectWidget)
	{
		SelectWidget->RemoveFromParent();
		SelectWidget = nullptr;
	}
	EnterMenuState();
}

void ARaceGameMode::OpenSelection()
{
	UE_LOG(LogTemp, Display, TEXT("RACEGAME: selection opened"));
	if (MenuWidget)
	{
		MenuWidget->RemoveFromParent();
		MenuWidget = nullptr;
	}
	if (URaceSelectWidget* Select = CreateWidget<URaceSelectWidget>(GetWorld(), URaceSelectWidget::StaticClass()))
	{
		SelectWidget = Select;
		SelectWidget->AddToViewport(100);
		if (APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0))
		{
			FInputModeUIOnly InputMode;
			if (TSharedPtr<SWidget> Focus = SelectWidget->GetFocusTarget())
			{
				InputMode.SetWidgetToFocus(Focus.ToSharedRef());
			}
			PC->SetInputMode(InputMode);
			PC->bShowMouseCursor = true;
		}
		UE_LOG(LogTemp, Display, TEXT("RACEGAME: selection shown"));
	}
}

void ARaceGameMode::StartSelectedRace(int32 CarIdx, int32 TrackIdx)
{
	const TArray<FRaceTrackEntry> Tracks = GetRaceTrackEntries();
	const int32 SafeTrack = Tracks.Num() > 0 ? FMath::Clamp(TrackIdx, 0, Tracks.Num() - 1) : 0;
	const FString MapPath = Tracks.Num() > 0 ? Tracks[SafeTrack].MapPath : TEXT("/Game/Track/Track1_TestCircuit");
	UE_LOG(LogTemp, Display, TEXT("RACEGAME: loading %s under production mode (car=%d track=%d)"),
		*MapPath, CarIdx, SafeTrack);
	const FString URLOptions = FString::Printf(TEXT("game=/Script/RacingGame.RaceGameMode?car=%d?track=%d"),
		CarIdx, SafeTrack);
	UGameplayStatics::OpenLevel(GetWorld(), FName(*MapPath), true, URLOptions);
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
	MenuWidget = CreateWidget<URaceMenuWidget>(GetWorld(), URaceMenuWidget::StaticClass());
	if (MenuWidget)
	{
		MenuWidget->AddToViewport(100);
		if (APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0))
		{
			FInputModeUIOnly InputMode;
			if (TSharedPtr<SWidget> Focus = MenuWidget->GetFocusTarget())
			{
				InputMode.SetWidgetToFocus(Focus.ToSharedRef());
			}
			PC->SetInputMode(InputMode);
			PC->bShowMouseCursor = true;
		}
		UE_LOG(LogTemp, Display, TEXT("RACEGAME: menu shown"));
	}

	// Headless verification path (mirrors the probe -Task10Shots pattern):
	// -RaceAutoStart starts the selected race without clicks, through the
	// same StartSelectedRace the Confirm button uses. Optional
	// -RaceAutoCar=N and -RaceAutoTrack=M choose the entries.
	if (FParse::Param(FCommandLine::Get(), TEXT("RaceAutoStart")))
	{
		int32 AutoCar = 0;
		int32 AutoTrack = 0;
		FParse::Value(FCommandLine::Get(), TEXT("RaceAutoCar="), AutoCar);
		FParse::Value(FCommandLine::Get(), TEXT("RaceAutoTrack="), AutoTrack);
		FTimerHandle H;
		GetWorldTimerManager().SetTimer(H, [this, AutoCar, AutoTrack]()
		{
			StartSelectedRace(AutoCar, AutoTrack);
		}, 2.0f, false);
		UE_LOG(LogTemp, Display, TEXT("RACEGAME: autostart armed car=%d track=%d"), AutoCar, AutoTrack);
	}

	// -RaceAutoSelect opens the selection screen without starting a race,
	// evidencing the menu-to-selection transition headlessly.
	if (FParse::Param(FCommandLine::Get(), TEXT("RaceAutoSelect")))
	{
		FTimerHandle SelH;
		GetWorldTimerManager().SetTimer(SelH, [this]()
		{
			OpenSelection();
		}, 2.0f, false);
		UE_LOG(LogTemp, Display, TEXT("RACEGAME: autoselect armed"));
	}
}

void ARaceGameMode::EnterDefaultRaceState()
{
	const TArray<FRaceCarPreset> Cars = GetRaceCarPresets();
	const int32 SafeCar = Cars.Num() > 0 ? FMath::Clamp(SelectedCar, 0, Cars.Num() - 1) : 0;

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

	// Grid from the track's slot table (PR4): the player takes a rear slot
	// and the AI takes pole, so neither starts inside the other's launch
	// corridor. Parking the player at the lone start pose put it in the
	// AI's path and wedged every headless run; table slots carry an
	// offline pairwise-clearance proof.
	if (Track)
	{
		FVector PlayerLoc;
		float PlayerYaw = 0.0f;
		Track->GetGridPose(3, PlayerLoc, PlayerYaw);
		if (APawn* Pawn = UGameplayStatics::GetPlayerPawn(GetWorld(), 0))
		{
			Pawn->SetActorLocationAndRotation(PlayerLoc,
				FRotator(0.0f, PlayerYaw, 0.0f), false, nullptr, ETeleportType::TeleportPhysics);
			if (Cars.Num() > 0)
			{
				if (ARaceVehicle* Vehicle = Cast<ARaceVehicle>(Pawn))
				{
					Vehicle->SetVehicleConfig(Cars[SafeCar].Config);
					UE_LOG(LogTemp, Display, TEXT("RACEGAME: car preset applied %s mass=%.0f finaldrive=%.2f"),
						*Cars[SafeCar].Name, Cars[SafeCar].Config.MassKg, Cars[SafeCar].Config.FinalDriveRatio);
				}
			}
		}
	}

	if (Track && Manager)
	{
		FVector AILoc;
		float AIYaw = 0.0f;
		Track->GetGridPose(0, AILoc, AIYaw);
		FActorSpawnParameters P;
		P.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		if (ARaceVehicle* AI = GetWorld()->SpawnActor<ARaceVehicle>(ARaceVehicle::StaticClass(), AILoc,
			FRotator(0.0f, AIYaw, 0.0f), P))
		{
			URaceAIDriver* Driver = NewObject<URaceAIDriver>(AI, TEXT("AIDriver"));
			Driver->RegisterComponent();
			Manager->RegisterParticipant(AI);
			UE_LOG(LogTemp, Display, TEXT("RACEGAME: default AI spawned"));
		}
		Manager->StartRace();
		UE_LOG(LogTemp, Display, TEXT("RACEGAME: default race started"));
	}

	// Production race HUD: model bound to the manager (sole authority),
	// then the thin display shell. Same composition as the Task 17 test
	// flow, minus the probe.
	if (Manager)
	{
		HudModel = NewObject<URaceHudModel>(this);
		HudModel->Init(Manager, FRaceHudConfig());
		HudWidget = CreateWidget<URaceHudWidget>(GetWorld(), URaceHudWidget::StaticClass());
		if (HudWidget)
		{
			HudWidget->BindModel(HudModel);
			HudWidget->AddToViewport();
		}
		UE_LOG(LogTemp, Display, TEXT("RACEGAME: hud bound=%d widget=%d"),
			HudModel->IsBound() ? 1 : 0, HudWidget != nullptr ? 1 : 0);
	}

	// Human input stays held until the manager reaches Racing. The AI
	// already gates itself on the phase; this closes the same gate for
	// the player without giving the vehicle a manager dependency.
	if (APawn* Pawn = UGameplayStatics::GetPlayerPawn(GetWorld(), 0))
	{
		if (APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0))
		{
			Pawn->DisableInput(PC);
			UE_LOG(LogTemp, Display, TEXT("RACEGAME: input held until green"));
		}
	}
	bInputReleased = false;
	bLoggedCountdown = false;
	bResultsShown = false;
	TWeakObjectPtr<ARaceManager> WeakManager(Manager);
	GetWorldTimerManager().SetTimer(GreenPollHandle, [this, WeakManager]()
	{
		if (!WeakManager.IsValid())
		{
			return;
		}
		const ERacePhase Phase = WeakManager->GetPhase();
		if (!bLoggedCountdown && Phase == ERacePhase::Countdown)
		{
			bLoggedCountdown = true;
			UE_LOG(LogTemp, Display, TEXT("RACEGAME: countdown observed T-%.1f"),
				WeakManager->GetCountdownRemaining());
		}
		if (!bInputReleased && Phase == ERacePhase::Racing)
		{
			bInputReleased = true;
			if (APawn* Pawn = UGameplayStatics::GetPlayerPawn(GetWorld(), 0))
			{
				if (APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0))
				{
					Pawn->EnableInput(PC);
				}
			}
			UE_LOG(LogTemp, Display, TEXT("RACEGAME: input enabled at Racing"));
			GetWorldTimerManager().ClearTimer(GreenPollHandle);
		}
	}, 0.1f, true);

	// Results screen appears once the manager finalizes results, even if
	// parked participants never finish: the table covers finishers only
	// by manager design, so headless runs still exercise this path.
	TWeakObjectPtr<ARaceManager> WeakResultsManager(Manager);
	GetWorldTimerManager().SetTimer(ResultsPollHandle, [this, WeakResultsManager]()
	{
		if (bResultsShown || !WeakResultsManager.IsValid() || !WeakResultsManager->HasResults())
		{
			return;
		}
		bResultsShown = true;
		if (HudWidget)
		{
			HudWidget->RemoveFromParent();
		}
		if (URaceResultsWidget* Results = CreateWidget<URaceResultsWidget>(GetWorld(), URaceResultsWidget::StaticClass()))
		{
			ResultsWidget = Results;
			ResultsWidget->AddToViewport(200);
			ResultsWidget->ShowResults(WeakResultsManager.Get());
		}
		GetWorldTimerManager().ClearTimer(ResultsPollHandle);
	}, 0.5f, true);

	if (APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0))
	{
		PC->SetInputMode(FInputModeGameOnly());
		PC->bShowMouseCursor = false;
	}
}
