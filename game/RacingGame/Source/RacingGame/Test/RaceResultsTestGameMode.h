// Test-only GameMode for Task 15 results verification (Task 15).
// Spawns the track and race manager when absent, snaps the player to
// grid slot 0, spawns five AI rivals on slots 1-5 with tiered pace,
// registers all six, then spawns the Task 15 probe. Selected through
// the `?game=` URL option on the circuit map; earlier flows untouched.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "RaceResultsTestGameMode.generated.h"

UCLASS()
class RACINGGAME_API ARaceResultsTestGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ARaceResultsTestGameMode();

	virtual void BeginPlay() override;
};
