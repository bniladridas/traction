// Race HUD presentation configuration (Task 17).
// Owns what the player sees: which elements exist, how fast they poll,
// and the exact display templates. The model and widget never hard-code
// a format string; any text change belongs here. Race truth stays in
// ARaceManager; this struct only describes presentation.

#pragma once

#include "CoreMinimal.h"
#include "RaceHudConfig.generated.h"

USTRUCT(BlueprintType)
struct FRaceHudConfig
{
	GENERATED_BODY()

	// Poll rate for the presentation layer. Presentation-only: it does
	// not add or remove gameplay ticks, and never alters simulation.
	UPROPERTY(EditAnywhere, Category = "Race|HUD")
	float UpdateRateHz = 20.0f;

	// Element visibility.
	UPROPERTY(EditAnywhere, Category = "Race|HUD")
	bool bShowCountdown = true;
	UPROPERTY(EditAnywhere, Category = "Race|HUD")
	bool bShowLap = true;
	UPROPERTY(EditAnywhere, Category = "Race|HUD")
	bool bShowPosition = true;
	UPROPERTY(EditAnywhere, Category = "Race|HUD")
	bool bShowFinish = true;

	// Display templates. {Token} placeholders are filled by the model
	// from verified manager state. Tokens: {Countdown}, {Lap}, {Total},
	// {Position}, {Field}.
	UPROPERTY(EditAnywhere, Category = "Race|HUD")
	FString CountdownTemplate = TEXT("{Countdown}");
	UPROPERTY(EditAnywhere, Category = "Race|HUD")
	FString LapTemplate = TEXT("Lap {Lap}/{Total}");
	UPROPERTY(EditAnywhere, Category = "Race|HUD")
	FString PositionTemplate = TEXT("{Position}/{Field}");
	UPROPERTY(EditAnywhere, Category = "Race|HUD")
	FString FinishTemplate = TEXT("Finished {Position} of {Field}");
};