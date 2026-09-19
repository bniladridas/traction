// Test-only GameMode for Task 16 full-race verification (Task 16).
// Spawns the track and race manager when absent, sets this run's lap
// count to 3 (the struct default stays 2 for all frozen tasks), snaps
// the player to grid slot 0, spawns five AI rivals on slots 1-5 with
// tiered pace, registers all six, then spawns the Task 16 probe.
// Selected through the `?game=` URL option on the circuit map; earlier
// flows untouched.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "RaceFullRaceTestGameMode.generated.h"

UCLASS()
class RACINGGAME_API ARaceFullRaceTestGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ARaceFullRaceTestGameMode();

	virtual void BeginPlay() override;
};
