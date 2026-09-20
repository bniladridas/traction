// Race HUD presentation model (Task 17).
// Transforms verified ARaceManager state into display-ready strings and
// fields using FRaceHudConfig templates. The manager remains the sole
// race-state authority: the model only reads getters, never calculates
// race progress, position, timing, or finish semantics. Presentation
// polling here never drives a gameplay tick or touches simulation.

#pragma once

#include "CoreMinimal.h"
#include "RaceConfig.h"
#include "RaceManager.h"
#include "RaceHudConfig.h"
#include "RaceHudModel.generated.h"

class ARaceManager;

UCLASS()
class RACINGGAME_API URaceHudModel : public UObject
{
	GENERATED_BODY()

public:
	// Binds to the manager instance and stores the presentation config.
	// The manager owns race truth; this object only reads it.
	void Init(ARaceManager* InManager, const FRaceHudConfig& InConfig);

	// Builds display fields from the current verified manager state.
	// Safe to call from a widget's presentation tick or a probe.
	void Refresh();

	bool IsBound() const { return Manager != nullptr; }
	const FRaceHudConfig& GetConfig() const { return Config; }

	// Display fields. During Ready they reflect the base state; counts
	// and texts follow the manager phase exactly.
	int32 GetCountdownSeconds() const { return CountdownSeconds; }
	int32 GetLap() const { return Lap; }
	int32 GetTotalLaps() const { return TotalLaps; }
	int32 GetPosition() const { return Position; }
	int32 GetFieldSize() const { return FieldSize; }
	bool HasFinish() const { return bHasFinish; }

	FString GetCountdownText() const { return CountdownText; }
	FString GetLapText() const { return LapText; }
	FString GetPositionText() const { return PositionText; }
	FString GetFinishText() const { return FinishText; }

private:
	FString ApplyTemplate(const FString& Template, const TMap<FString, FString>& Tokens) const;

	UPROPERTY()
	TObjectPtr<ARaceManager> Manager = nullptr;
	FRaceHudConfig Config;

	int32 CountdownSeconds = 0;
	int32 Lap = 0;
	int32 TotalLaps = 0;
	int32 Position = 1;
	int32 FieldSize = 1;
	bool bHasFinish = false;

	FString CountdownText;
	FString LapText;
	FString PositionText;
	FString FinishText;
};