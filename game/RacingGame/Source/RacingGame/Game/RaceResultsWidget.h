// Production results screen for the vertical slice (PR5).
// Pure C++ UMG: no editor assets. Display-only; restart flow belongs to
// PR6, so this screen has no buttons. Rows are built from the manager's
// finish snapshot at show time, which is immutable by then.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "RaceResultsWidget.generated.h"

class ARaceManager;
class UTextBlock;
class UVerticalBox;
class SWidget;

UCLASS()
class RACINGGAME_API URaceResultsWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// Builds the static tree on first slate build (same engine constraint
	// as the other production widgets: the root must exist before the
	// first build resolves it).
	virtual TSharedRef<SWidget> RebuildWidget() override;

	// Fills the rows from the manager snapshot. Safe to call once the
	// manager reports HasResults; entries never change afterward.
	void ShowResults(ARaceManager* Manager);

private:
	void BuildResultsTree();

	UPROPERTY()
	TObjectPtr<UVerticalBox> RowsBox = nullptr;
};
