// Test-only GameMode for Task 19 racecraft verification (Task 19).
// Spawns the track and race manager when absent, snaps the player to
// grid slot 0, spawns two AI rivals on slots 1-2 with the frozen pace
// tiers (slot 1 defender slower, slot 2 attacker faster) and frozen
// lines (slot 1 left -120, slot 2 right +120), registers all three,
// then spawns the Task 19 probe. The attacker driver alone has the
// racecraft layer enabled with the frozen window constants; the
// defender stays a plain pursuit driver, so no defender behavior is
// introduced. Selected through the `?game=` URL option on the circuit
// map; earlier flows untouched.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "Task19RacecraftGameMode.generated.h"

UCLASS()
class RACINGGAME_API ATask19RacecraftGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ATask19RacecraftGameMode();

	virtual void BeginPlay() override;
};
