// Test-only GameMode for Task 9 AI verification (Task 9).
// Spawns the track and race manager when absent, snaps the player to the
// track-owned start, spawns one AI-driven vehicle on the grid with its
// driver component, registers it, then spawns the Task 9 probe. Selected
// through the `?game=` URL option on the circuit map; earlier flows stay
// untouched.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "RaceAITestGameMode.generated.h"

UCLASS()
class RACINGGAME_API ARaceAITestGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ARaceAITestGameMode();

	virtual void BeginPlay() override;
};
