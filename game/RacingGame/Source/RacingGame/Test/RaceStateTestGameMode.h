// Test-only GameMode for Task 8 race-state verification (Task 8).
// Spawns the track and the race manager when absent, snaps the vehicle
// to the track-owned start, then spawns the Task 8 probe. Selected
// through the `?game=` URL option on the circuit map; the Task 7 flow
// and all earlier flows stay untouched.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "RaceStateTestGameMode.generated.h"

UCLASS()
class RACINGGAME_API ARaceStateTestGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ARaceStateTestGameMode();

	virtual void BeginPlay() override;
};
