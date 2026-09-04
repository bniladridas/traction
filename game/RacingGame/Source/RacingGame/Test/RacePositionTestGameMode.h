// Test-only GameMode for Task 11 position verification (Task 11).
// Spawns the track and race manager when absent, snaps the player to
// the track-owned start, spawns a second participant pawn on the grid,
// registers it, then spawns the Task 11 probe. Selected through the
// `?game=` URL option on the circuit map; earlier flows untouched.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "RacePositionTestGameMode.generated.h"

UCLASS()
class RACINGGAME_API ARacePositionTestGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ARacePositionTestGameMode();

	virtual void BeginPlay() override;
};
