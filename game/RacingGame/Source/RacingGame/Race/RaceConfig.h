// RacingGame race configuration (Task 8).
// Owns race rules only: lap count and countdown. Checkpoint count,
// start/finish pose, and widths resolve from the track at runtime on
// purpose: duplicating them here would let the two sources diverge.

#pragma once

#include "CoreMinimal.h"
#include "RaceConfig.generated.h"

USTRUCT(BlueprintType)
struct FRaceConfig
{
	GENERATED_BODY()

	// Laps required to finish. Sized at 2 for the E2E program (one dirty
	// lap attempt, then the laps that count); production values later.
	UPROPERTY(EditAnywhere, Category = "Race|Rules")
	int32 LapCount = 2;

	// Countdown duration in seconds before Racing begins.
	UPROPERTY(EditAnywhere, Category = "Race|Rules")
	float CountdownDuration = 3.0f;
};
