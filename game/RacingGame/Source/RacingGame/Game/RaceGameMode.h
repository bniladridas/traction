// Production game mode for the vertical slice (PR1 menu shell, PR2 selection).
// Owns the production boot path: menu state on the menu map, car/track
// selection, then default or selected race setup on the circuit map.
// Selection crosses the map boundary as URL options (car=/track=): the mode
// is recreated per map load, so selection state cannot live on the instance
// across OpenLevel.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "RaceGameMode.generated.h"

class URaceMenuWidget;
class URaceSelectWidget;
class ARaceTrack;
class ARaceManager;
class ARaceVehicle;
class URaceAIDriver;

UCLASS()
class RACINGGAME_API ARaceGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ARaceGameMode();

	virtual void InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage) override;
	virtual void BeginPlay() override;

	// Selection flow (PR2): menu <-> selection, then race with the chosen
	// car/track indices. Autostart calls StartSelectedRace directly with
	// headless-provided indices, exercising the same function Confirm uses.
	void OpenMenu();
	void OpenSelection();
	void StartSelectedRace(int32 CarIdx, int32 TrackIdx);

	// Restart flow (PR6): clean new race through the production flow.
	// Resets manager, cars, AI, widgets, and mode flags, then re-arms the
	// same start sequence. Called by the results Restart button and by
	// -RaceAutoRestart headlessly.
	void RestartRace();

	// Menu action: quit the game.
	void QuitGame();

private:
	// True when the current map is the race circuit.
	bool IsRaceMap() const;

	// Menu-map behavior: show the menu widget, route input to UI.
	void EnterMenuState();

	// Circuit-map behavior: apply the selected car preset to the player,
	// seat the player, register one AI rival, start the race.
	void EnterDefaultRaceState();

	// Selection parsed from URL options; defaults select the first entries.
	int32 SelectedCar = 0;
	int32 SelectedTrack = 0;

	UPROPERTY()
	TObjectPtr<URaceMenuWidget> MenuWidget = nullptr;

	UPROPERTY()
	TObjectPtr<URaceSelectWidget> SelectWidget = nullptr;

	// Production race HUD (PR3): model bound to the manager plus the thin
	// display shell. Created at race setup; the shell shows the countdown.
	UPROPERTY()
	TObjectPtr<class URaceHudModel> HudModel = nullptr;

	UPROPERTY()
	TObjectPtr<class URaceHudWidget> HudWidget = nullptr;

	FTimerHandle GreenPollHandle;
	bool bInputReleased = false;
	bool bLoggedCountdown = false;

	// Results screen (PR5): shown once when the manager finalizes results.
	// Display-only; restart flow belongs to PR6.
	UPROPERTY()
	TObjectPtr<class URaceResultsWidget> ResultsWidget = nullptr;

	FTimerHandle ResultsPollHandle;
	bool bResultsShown = false;

	// Production race actors, owned across restarts within one map load.
	UPROPERTY()
	TObjectPtr<ARaceTrack> RaceTrack = nullptr;

	UPROPERTY()
	TObjectPtr<ARaceManager> RaceManager = nullptr;

	UPROPERTY()
	TObjectPtr<ARaceVehicle> AIVehicle = nullptr;

	UPROPERTY()
	TObjectPtr<URaceAIDriver> AIDriver = nullptr;

	// Race numbering and first-race summary for the independence check:
	// the second snapshot must stand on its own, not continue the first.
	int32 RaceNumber = 0;
	float FirstBestTime = -1.0f;
	int32 FirstEntries = 0;

	// Seats player and AI on grid slots (shared by initial setup and
	// restart). Re-seating after OnVehicleReset is what keeps restarted
	// races on the proven-clear grid instead of drifting.
	void SeatCars();

	// Starts one race: manager StartRace, input hold, transition and
	// results polls, per-race flag resets. Shared by initial setup and
	// restart so both enter through the same sequence.
	void ArmRaceStart();
};
