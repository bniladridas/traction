// Car/track selection screen for the vertical slice (PR2).
// Pure C++ UMG: no editor assets. Lists only the real catalog from
// RaceSelectData; Confirm starts the race with the chosen indices.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "RaceSelectWidget.generated.h"

class UButton;
class UTextBlock;
class SWidget;

UCLASS()
class RACINGGAME_API URaceSelectWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// Same engine constraint as the menu widget: the tree must exist
	// before the first slate build resolves the root.
	virtual TSharedRef<SWidget> RebuildWidget() override;

	TSharedPtr<SWidget> GetFocusTarget() const;

protected:
	UFUNCTION()
	void OnCarPrev();

	UFUNCTION()
	void OnCarNext();

	UFUNCTION()
	void OnTrackPrev();

	UFUNCTION()
	void OnTrackNext();

	UFUNCTION()
	void OnConfirm();

	UFUNCTION()
	void OnBack();

private:
	void BuildSelectTree();
	void RefreshLabels();
	UButton* MakeNavButton(const TCHAR* LabelText);

	int32 CarIdx = 0;
	int32 TrackIdx = 0;

	UPROPERTY()
	TObjectPtr<UTextBlock> CarLabel = nullptr;

	UPROPERTY()
	TObjectPtr<UTextBlock> TrackLabel = nullptr;

	UPROPERTY()
	TObjectPtr<UButton> ConfirmButton = nullptr;
};
