// Production game mode for the vertical slice (PR1).
// Owns the production boot path: menu state on the menu map, default race
// setup on the circuit map. No test probes, no test map overrides needed
// once this mode is selected. Race setup here uses defaults only;
// car/track selection arrives in PR2.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "RaceGameMode.generated.h"

UCLASS()
class RACINGGAME_API ARaceGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ARaceGameMode();

	virtual void BeginPlay() override;

	// Menu action: load the circuit map under this production mode,
	// which then performs default race setup on BeginPlay.
	void StartDefaultRace();

	// Menu action: quit the game.
	void QuitGame();

private:
	// True when the current map is the race circuit.
	bool IsRaceMap() const;

	// Menu-map behavior: show the menu widget, route input to UI.
	void EnterMenuState();

	// Circuit-map behavior: spawn production actors with defaults,
	// seat the player, register one AI rival, start the race.
	void EnterDefaultRaceState();
};
