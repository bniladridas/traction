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

	auto AddCornerText = [this](UCanvasPanel* Parent, float X, float Y, float AlignX, TObjectPtr<UTextBlock>& Out)
	{
		Out = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		Out->SetJustification(ETextJustify::Center);
		if (UCanvasPanelSlot* Slot = Parent->AddChildToCanvas(Out))
		{
			Slot->SetAnchors(FAnchors(X, Y, X, Y));
			Slot->SetAlignment(FVector2D(AlignX, 0.0f));
			Slot->SetAutoSize(true);
		}
		Out->SetVisibility(ESlateVisibility::Collapsed);
	};

	CountdownText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	CountdownText->SetJustification(ETextJustify::Center);
	if (UCanvasPanelSlot* Slot = Root->AddChildToCanvas(CountdownText))
	{
		Slot->SetAnchors(FAnchors(0.5f, 0.0f, 0.5f, 0.0f));
		Slot->SetAlignment(FVector2D(0.5f, 0.0f));
		Slot->SetAutoSize(true);
	}
	CountdownText->SetVisibility(ESlateVisibility::Collapsed);

	AddCornerText(Root, 0.0f, 0.0f, 0.0f, LapText);
	AddCornerText(Root, 1.0f, 0.0f, 1.0f, PositionText);

	FinishText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	FinishText->SetJustification(ETextJustify::Center);
	if (UCanvasPanelSlot* FinishSlot = Root->AddChildToCanvas(FinishText))
	{
		FinishSlot->SetAnchors(FAnchors(0.5f, 0.5f, 0.5f, 0.5f));
		FinishSlot->SetAlignment(FVector2D(0.5f, 0.5f));
		FinishSlot->SetAutoSize(true);
	}
	FinishText->SetVisibility(ESlateVisibility::Collapsed);
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
	auto BindField = [](TObjectPtr<UTextBlock> Block, const FString& Text)
	{
		if (Block)
		{
			Block->SetText(FText::FromString(Text));
			Block->SetVisibility(Text.IsEmpty()
				? ESlateVisibility::Collapsed
				: ESlateVisibility::HitTestInvisible);
		}
	};
	BindField(CountdownText, Model->GetCountdownText());
	BindField(LapText, Model->GetLapText());
	BindField(PositionText, Model->GetPositionText());
	BindField(FinishText, Model->GetFinishText());
}