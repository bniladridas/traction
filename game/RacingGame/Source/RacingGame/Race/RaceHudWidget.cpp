// See header.

#include "RaceHudWidget.h"
#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/TextBlock.h"

TSharedRef<SWidget> URaceHudWidget::RebuildWidget()
{
	if (WidgetTree->RootWidget == nullptr)
	{
		BuildDisplayTree();
	}
	return Super::RebuildWidget();
}

void URaceHudWidget::BuildDisplayTree()
{
	UCanvasPanel* Root = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass());
	WidgetTree->RootWidget = Root;

	CountdownText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	CountdownText->SetJustification(ETextJustify::Center);
	if (UCanvasPanelSlot* Slot = Root->AddChildToCanvas(CountdownText))
	{
		Slot->SetAnchors(FAnchors(0.5f, 0.0f, 0.5f, 0.0f));
		Slot->SetAlignment(FVector2D(0.5f, 0.0f));
		Slot->SetAutoSize(true);
	}
	CountdownText->SetVisibility(ESlateVisibility::Collapsed);
}

void URaceHudWidget::BindModel(URaceHudModel* InModel)
{
	Model = InModel;
	if (Model)
	{
		Model->Refresh();
	}
}

void URaceHudWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	if (!Model)
	{
		return;
	}
	// Presentation-only polling. Barring widgets disabled at author
	// time, this never contributes to the gameplay tick graph.
	const float RateHz = FMath::Clamp(Model->GetConfig().UpdateRateHz, 1.0f, 1000.0f);
	Accumulator += InDeltaTime;
	if (Accumulator >= 1.0f / RateHz)
	{
		Accumulator = 0.0f;
		Model->Refresh();
	}
	if (CountdownText)
	{
		const FString Text = Model->GetCountdownText();
		CountdownText->SetText(FText::FromString(Text));
		CountdownText->SetVisibility(Text.IsEmpty()
			? ESlateVisibility::Collapsed
			: ESlateVisibility::HitTestInvisible);
	}
}