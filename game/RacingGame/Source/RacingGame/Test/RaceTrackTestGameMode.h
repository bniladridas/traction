// Test-only GameMode for the Task 7 circuit (Task 7).
// Spawns the track actor when the map does not contain one, snaps the
// vehicle to the track-owned start pose, then spawns the Task 7 probe.
// Selected through the `?game=` URL option; the project default GameMode
// and the Task 2 map flow stay untouched.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "RaceTrackTestGameMode.generated.h"

UCLASS()
class RACINGGAME_API ARaceTrackTestGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ARaceTrackTestGameMode();

	virtual void BeginPlay() override;
};
