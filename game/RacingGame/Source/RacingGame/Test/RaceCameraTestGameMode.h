// Test-only GameMode for Task 10 camera verification (Task 10).
// Same grid as the AI flow (track, manager, player snap, one AI rival
// with driver), but spawns the Task 10 camera probe. Selected through
// the `?game=` URL option on the circuit map; earlier flows untouched.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "RaceCameraTestGameMode.generated.h"

UCLASS()
class RACINGGAME_API ARaceCameraTestGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ARaceCameraTestGameMode();

	virtual void BeginPlay() override;
};
