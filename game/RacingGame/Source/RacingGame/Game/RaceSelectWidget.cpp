// See header.

#include "RaceSelectWidget.h"
#include "RaceGameMode.h"
#include "RaceSelectData.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Kismet/GameplayStatics.h"

TSharedRef<SWidget> URaceSelectWidget::RebuildWidget()
{
	if (WidgetTree->RootWidget == nullptr)
	{
		BuildSelectTree();
	}
	return Super::RebuildWidget();
}

TSharedPtr<SWidget> URaceSelectWidget::GetFocusTarget() const
{
	return ConfirmButton ? ConfirmButton->TakeWidget() : TSharedPtr<SWidget>();
}

void URaceSelectWidget::BuildSelectTree()
{
	UCanvasPanel* Root = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass());
	WidgetTree->RootWidget = Root;

	UVerticalBox* Box = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	if (UCanvasPanelSlot* RootSlot = Root->AddChildToCanvas(Box))
	{
		RootSlot->SetAnchors(FAnchors(0.5f, 0.5f, 0.5f, 0.5f));
		RootSlot->SetAlignment(FVector2D(0.5f, 0.5f));
		RootSlot->SetAutoSize(true);
	}

	UTextBlock* Title = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	Title->SetText(FText::FromString(TEXT("SELECT")));
	Title->SetJustification(ETextJustify::Center);
	Box->AddChildToVerticalBox(Title);

	auto AddRow = [this](UVerticalBox* Parent, TObjectPtr<UTextBlock>& OutLabel, int32 RowId)
	{
		UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
		Parent->AddChildToVerticalBox(Row);
		UButton* Prev = MakeNavButton(TEXT("<"));
		UButton* Next = MakeNavButton(TEXT(">"));
		if (RowId == 0)
		{
			Prev->OnClicked.AddDynamic(this, &URaceSelectWidget::OnCarPrev);
			Next->OnClicked.AddDynamic(this, &URaceSelectWidget::OnCarNext);
		}
		else
		{
			Prev->OnClicked.AddDynamic(this, &URaceSelectWidget::OnTrackPrev);
			Next->OnClicked.AddDynamic(this, &URaceSelectWidget::OnTrackNext);
		}
		Row->AddChildToHorizontalBox(Prev);
		OutLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		OutLabel->SetJustification(ETextJustify::Center);
		if (UHorizontalBoxSlot* LabelSlot = Row->AddChildToHorizontalBox(OutLabel))
		{
			LabelSlot->SetPadding(FMargin(16.0f, 0.0f, 16.0f, 0.0f));
		}
		Row->AddChildToHorizontalBox(Next);
	};

	AddRow(Box, CarLabel, 0);
	AddRow(Box, TrackLabel, 1);

	ConfirmButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass());
	UTextBlock* ConfirmLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	ConfirmLabel->SetText(FText::FromString(TEXT("Start Race")));
	ConfirmLabel->SetJustification(ETextJustify::Center);
	ConfirmButton->AddChild(ConfirmLabel);
	ConfirmButton->OnClicked.AddDynamic(this, &URaceSelectWidget::OnConfirm);
	Box->AddChildToVerticalBox(ConfirmButton);

	UButton* BackButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass());
	UTextBlock* BackLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	BackLabel->SetText(FText::FromString(TEXT("Back")));
	BackLabel->SetJustification(ETextJustify::Center);
	BackButton->AddChild(BackLabel);
	BackButton->OnClicked.AddDynamic(this, &URaceSelectWidget::OnBack);
	Box->AddChildToVerticalBox(BackButton);

	RefreshLabels();
	UE_LOG(LogTemp, Display, TEXT("RACEGAME: select widget constructed"));
}

UButton* URaceSelectWidget::MakeNavButton(const TCHAR* LabelText)
{
	UButton* Button = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass());
	UTextBlock* Label = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	Label->SetText(FText::FromString(LabelText));
	Button->AddChild(Label);
	return Button;
}

void URaceSelectWidget::RefreshLabels()
{
	const TArray<FRaceCarPreset> Cars = GetRaceCarPresets();
	const TArray<FRaceTrackEntry> Tracks = GetRaceTrackEntries();
	if (CarLabel && Cars.Num() > 0)
	{
		CarLabel->SetText(FText::FromString(Cars[CarIdx % Cars.Num()].Name));
	}
	if (TrackLabel && Tracks.Num() > 0)
	{
		TrackLabel->SetText(FText::FromString(Tracks[TrackIdx % Tracks.Num()].Name));
	}
}

void URaceSelectWidget::OnCarPrev()
{
	const int32 N = GetRaceCarPresets().Num();
	CarIdx = (CarIdx + N - 1) % N;
	RefreshLabels();
}

void URaceSelectWidget::OnCarNext()
{
	const int32 N = GetRaceCarPresets().Num();
	CarIdx = (CarIdx + 1) % N;
	RefreshLabels();
}

void URaceSelectWidget::OnTrackPrev()
{
	const int32 N = GetRaceTrackEntries().Num();
	TrackIdx = (TrackIdx + N - 1) % N;
	RefreshLabels();
}

void URaceSelectWidget::OnTrackNext()
{
	const int32 N = GetRaceTrackEntries().Num();
	TrackIdx = (TrackIdx + 1) % N;
	RefreshLabels();
}

void URaceSelectWidget::OnConfirm()
{
	UE_LOG(LogTemp, Display, TEXT("RACEGAME: selection confirmed car=%d track=%d"), CarIdx, TrackIdx);
	if (ARaceGameMode* Mode = Cast<ARaceGameMode>(UGameplayStatics::GetGameMode(this)))
	{
		Mode->StartSelectedRace(CarIdx, TrackIdx);
	}
}

void URaceSelectWidget::OnBack()
{
	if (ARaceGameMode* Mode = Cast<ARaceGameMode>(UGameplayStatics::GetGameMode(this)))
	{
		Mode->OpenMenu();
	}
}
