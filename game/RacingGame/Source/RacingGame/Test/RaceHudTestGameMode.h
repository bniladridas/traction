// Test-only GameMode for Task 17 HUD verification (Task 17).
// Mirrors the Task 16 full-race field (grid slot 0 + 5 tiered-pace AI)
// but attaches an AI driver to the player pawn too, so participant 0
// races and finishes (required for the HUD finish gate), creates the
// presentation model bound to the manager, instantiates the thin UMG
// shell, and sets this run's lap count to 3 (struct default stays 2).
// Selected through the `?game=` URL option on the circuit map; earlier
// flows untouched.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "RaceHudConfig.h"
#include "RaceHudTestGameMode.generated.h"

class URaceHudModel;
class URaceHudWidget;

UCLASS()
class RACINGGAME_API ARaceHudTestGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ARaceHudTestGameMode();

	// Presentation config for this run; EditAnywhere for tuning the
	// templates and element toggles without touching the code.
	UPROPERTY(EditAnywhere, Category = "Race|HUD")
	FRaceHudConfig HudConfig;

	URaceHudModel* GetHudModel() const { return HudModel; }
	URaceHudWidget* GetHudWidget() const { return HudWidget; }

	virtual void BeginPlay() override;

private:
	UPROPERTY()
	TObjectPtr<URaceHudModel> HudModel = nullptr;
	UPROPERTY()
	TObjectPtr<URaceHudWidget> HudWidget = nullptr;
};