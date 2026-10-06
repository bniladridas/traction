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
};
