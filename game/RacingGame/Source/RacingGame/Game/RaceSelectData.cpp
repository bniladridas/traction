// See header.

#include "RaceSelectData.h"

TArray<FRaceCarPreset> GetRaceCarPresets()
{
	TArray<FRaceCarPreset> Out;

	FRaceCarPreset Balanced;
	Balanced.Name = TEXT("Balanced");
	Out.Add(Balanced);

	FRaceCarPreset Sprint;
	Sprint.Name = TEXT("Sprint");
	Sprint.Config.MassKg = 950.0f;
	Sprint.Config.FinalDriveRatio = 4.1f;
	Out.Add(Sprint);

	FRaceCarPreset Tourer;
	Tourer.Name = TEXT("Tourer");
	Tourer.Config.MassKg = 1450.0f;
	Tourer.Config.FinalDriveRatio = 3.0f;
	Out.Add(Tourer);

	return Out;
}

TArray<FRaceTrackEntry> GetRaceTrackEntries()
{
	TArray<FRaceTrackEntry> Out;

	FRaceTrackEntry Circuit;
	Circuit.Name = TEXT("Test Circuit");
	Circuit.MapPath = TEXT("/Game/Track/Track1_TestCircuit");
	Out.Add(Circuit);

	return Out;
}
