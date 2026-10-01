// Test-only GameMode for Task 20 defend-the-line verification (Task 20).
// Spawns the track and race manager when absent, snaps the player to
// grid slot 0, spawns two AI rivals with the frozen pace tiers and lines
// (slot-1 geometry advanced for the defender, slot 2 for the attacker),
// registers all three, then spawns the Task 20 probe. The defender alone
// carries the Task 20 defense layer with the frozen rear-window
// constants; the attacker alone carries the unchanged Task 19 attack
// layer. Selected through the `?game=` URL option on the circuit map;
// earlier flows untouched.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "Task20RacecraftGameMode.generated.h"

UCLASS()
class RACINGGAME_API ATask20RacecraftGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ATask20RacecraftGameMode();

	virtual void BeginPlay() override;
};
