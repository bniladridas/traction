// Test-only GameMode for Task 13 pace verification (Task 13).
// Spawns the track and race manager when absent, snaps the player to
// grid slot 0, spawns two AI rivals on slots 1-2 with frozen pace tiers
// (slot 1 slower, slot 2 faster), registers all three, then spawns the
// Task 13 probe. Selected through the `?game=` URL option on the
// circuit map; earlier flows untouched.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "RacePaceTestGameMode.generated.h"

UCLASS()
class RACINGGAME_API ARacePaceTestGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ARacePaceTestGameMode();

	virtual void BeginPlay() override;
};
