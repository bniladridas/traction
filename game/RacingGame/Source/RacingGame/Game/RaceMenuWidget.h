// Minimal menu shell for the vertical slice (PR1).
// Pure C++ UMG: no editor assets. Title plus Start/Quit buttons that call
// back into the production game mode. Selection UI arrives in PR2.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "RaceMenuWidget.generated.h"

class UButton;
class SWidget;

UCLASS()
class RACINGGAME_API URaceMenuWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// Builds the widget tree on first slate build. Tree construction lives
	// here rather than NativeConstruct: the first RebuildWidget resolves the
	// root before NativeConstruct runs, so a tree built there would never
	// be drawn (cached empty spacer, no error).
	virtual TSharedRef<SWidget> RebuildWidget() override;

	// Invokes the Start action. Called by the Start button; the headless
	// autostart path calls this same function so both exercise one handler.
	void PressStart();

	// Slate widget to focus when the menu is shown (the Start button,
	// so gamepad/keyboard input lands somewhere focusable).
	TSharedPtr<SWidget> GetFocusTarget() const;

protected:
	UFUNCTION()
	void OnStartClicked();

	UFUNCTION()
	void OnQuitClicked();

private:
	void BuildMenuTree();

	UPROPERTY()
	TObjectPtr<UButton> StartButton = nullptr;

	UPROPERTY()
	TObjectPtr<UButton> QuitButton = nullptr;
};
