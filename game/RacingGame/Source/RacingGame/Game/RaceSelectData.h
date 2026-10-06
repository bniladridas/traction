// Selectable content for the vertical slice (PR2).
// Initial catalog only. Vehicle variants are configuration values, not new
// classes, per the FRaceVehicleConfig contract. Every entry must correspond
// to loadable content; no placeholders.

#pragma once

#include "CoreMinimal.h"
#include "RaceVehicleConfig.h"
#include "RaceSelectData.generated.h"

USTRUCT(BlueprintType)
struct FRaceCarPreset
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Race|Select")
	FString Name;

	UPROPERTY(EditAnywhere, Category = "Race|Select")
	FRaceVehicleConfig Config;
};

USTRUCT(BlueprintType)
struct FRaceTrackEntry
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Race|Select")
	FString Name;

	// Hard map path, e.g. /Game/Track/Track1_TestCircuit.
	UPROPERTY(EditAnywhere, Category = "Race|Select")
	FString MapPath;
};

// Initial catalog: stock running gear, a lighter sprint setup, and a
// heavier grand tourer. Deliberately small; tuning is later work.
TArray<FRaceCarPreset> GetRaceCarPresets();

// Initial catalog: the shipped circuit only.
TArray<FRaceTrackEntry> GetRaceTrackEntries();
