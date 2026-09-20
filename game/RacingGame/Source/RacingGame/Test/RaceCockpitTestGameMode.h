// Test-only GameMode for Task 18 cockpit camera verification (Task 18).
// Spawns the player pawn plus one AI rival and registers both with the
// manager, then spawns the probe. The probe drives participant 0 with
// pursuit (Task 15 pattern) and applies a Task 18 camera config via the
// vehicle's SetCameraConfig knob, so the run proves data-driven pitch
// and FOV rather than hardcoded engineering values. Selected through
// the `?game=` URL option on the circuit map; earlier flows untouched.
// Task 10 chase behavior (DefaultView=Chase) is intentionally the
// default and is not restructured here.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "RaceCockpitTestGameMode.generated.h"

UCLASS()
class RACINGGAME_API ARaceCockpitTestGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ARaceCockpitTestGameMode();

	virtual void BeginPlay() override;
};