// Race HUD widget (Task 17).
// Thin UMG display shell. Owns no race rules and no formatting logic:
// it binds to the presentation model and refreshes it on the widget's
// own tick at the configured presentation rate. A gameplay-facing text
// binding reads the model's display fields; the model update here never
// alters simulation.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "RaceHudModel.h"
#include "RaceHudWidget.generated.h"

UCLASS()
class RACINGGAME_API URaceHudWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// Binds the display shell to a presentation model. Called at
	// widget construction by the owning GameMode.
	void BindModel(URaceHudModel* InModel);

	URaceHudModel* GetModel() const { return Model; }

protected:
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
	UPROPERTY()
	TObjectPtr<URaceHudModel> Model = nullptr;
	float Accumulator = 0.0f;
};