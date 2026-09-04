// Test-only GameMode for Task 12 field verification (Task 12).
// Spawns the track and race manager when absent, snaps the player to
// grid slot 0, spawns two AI rivals on grid slots 1-2 with drivers,
// registers all three, then spawns the Task 12 probe. Selected through
// the `?game=` URL option on the circuit map; earlier flows untouched.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "RaceFieldTestGameMode.generated.h"

UCLASS()
class RACINGGAME_API ARaceFieldTestGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ARaceFieldTestGameMode();

	virtual void BeginPlay() override;
};
