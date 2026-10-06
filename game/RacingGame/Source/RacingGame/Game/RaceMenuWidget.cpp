// See header.

#include "RaceMenuWidget.h"
#include "RaceGameMode.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Kismet/GameplayStatics.h"

TSharedRef<SWidget> URaceMenuWidget::RebuildWidget()
{
	if (WidgetTree->RootWidget == nullptr)
	{
		BuildMenuTree();
	}
	return Super::RebuildWidget();
}

void URaceMenuWidget::BuildMenuTree()
{
	UCanvasPanel* Root = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass());
	WidgetTree->RootWidget = Root;

	// Full-screen dim background so the menu reads over any map.
	UBorder* Background = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
	Background->SetBrushColor(FLinearColor(0.02f, 0.02f, 0.03f, 0.92f));
	if (UCanvasPanelSlot* BGSlot = Root->AddChildToCanvas(Background))
	{
		BGSlot->SetAnchors(FAnchors(0.0f, 0.0f, 1.0f, 1.0f));
		BGSlot->SetOffsets(FMargin(0.0f));
	}

	UVerticalBox* Box = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	if (UCanvasPanelSlot* RootSlot = Root->AddChildToCanvas(Box))
	{
		RootSlot->SetAnchors(FAnchors(0.5f, 0.5f, 0.5f, 0.5f));
		RootSlot->SetAlignment(FVector2D(0.5f, 0.5f));
		RootSlot->SetAutoSize(true);
	}

	UTextBlock* Title = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	Title->SetText(FText::FromString(TEXT("TRACTION")));
	Title->SetJustification(ETextJustify::Center);
	if (UVerticalBoxSlot* TitleSlot = Box->AddChildToVerticalBox(Title))
	{
		TitleSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 24.0f));
	}

	StartButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass());
	UTextBlock* StartLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	StartLabel->SetText(FText::FromString(TEXT("Race")));
	StartLabel->SetJustification(ETextJustify::Center);
	StartButton->AddChild(StartLabel);
	StartButton->OnClicked.AddDynamic(this, &URaceMenuWidget::OnStartClicked);
	Box->AddChildToVerticalBox(StartButton);

	QuitButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass());
	UTextBlock* QuitLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	QuitLabel->SetText(FText::FromString(TEXT("Quit")));
	QuitLabel->SetJustification(ETextJustify::Center);
	QuitButton->AddChild(QuitLabel);
	QuitButton->OnClicked.AddDynamic(this, &URaceMenuWidget::OnQuitClicked);
	Box->AddChildToVerticalBox(QuitButton);

	UE_LOG(LogTemp, Display, TEXT("RACEGAME: menu widget constructed"));
}

void URaceMenuWidget::OnStartClicked()
{
	PressStart();
}

void URaceMenuWidget::PressStart()
{
	UE_LOG(LogTemp, Display, TEXT("RACEGAME: Start pressed"));
	if (ARaceGameMode* Mode = Cast<ARaceGameMode>(UGameplayStatics::GetGameMode(this)))
	{
		Mode->StartDefaultRace();
	}
}

void URaceMenuWidget::OnQuitClicked()
{
	UE_LOG(LogTemp, Display, TEXT("RACEGAME: Quit pressed"));
	if (ARaceGameMode* Mode = Cast<ARaceGameMode>(UGameplayStatics::GetGameMode(this)))
	{
		Mode->QuitGame();
	}
}

TSharedPtr<SWidget> URaceMenuWidget::GetFocusTarget() const
{
	return StartButton ? StartButton->TakeWidget() : TSharedPtr<SWidget>();
}
