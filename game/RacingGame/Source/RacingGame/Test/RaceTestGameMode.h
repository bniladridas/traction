// Test-only GameMode for the Task 2 flat prototype map (Task 2).
// Spawns the RacingGame cube vehicle. Selected through the `?game=` URL
// option by the harness, so the project default GameMode stays untouched.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "RaceTestGameMode.generated.h"

UCLASS()
class RACINGGAME_API ARaceTestGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ARaceTestGameMode();

	virtual void BeginPlay() override;
};
