// Test-only GameMode for Task 14 field verification (Task 14).
// Spawns the track and race manager when absent, snaps the player to
// grid slot 0, spawns five AI rivals on slots 1-5 with frozen pace
// tiers and lines, registers all six, then spawns the Task 14 probe.
// Selected through the `?game=` URL option on the circuit map; earlier
// flows untouched.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "RaceField6TestGameMode.generated.h"

UCLASS()
class RACINGGAME_API ARaceField6TestGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ARaceField6TestGameMode();

	virtual void BeginPlay() override;
};
