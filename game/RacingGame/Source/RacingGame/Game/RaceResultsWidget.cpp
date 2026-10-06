// See header.

#include "RaceResultsWidget.h"
#include "RaceGameMode.h"
#include "RaceManager.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Kismet/GameplayStatics.h"

TSharedRef<SWidget> URaceResultsWidget::RebuildWidget()
{
	if (WidgetTree->RootWidget == nullptr)
	{
		BuildResultsTree();
	}
	return Super::RebuildWidget();
}

void URaceResultsWidget::BuildResultsTree()
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
	Title->SetText(FText::FromString(TEXT("RESULTS")));
	Title->SetJustification(ETextJustify::Center);
	if (UVerticalBoxSlot* TitleSlot = Box->AddChildToVerticalBox(Title))
	{
		TitleSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 24.0f));
	}

	RowsBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	Box->AddChildToVerticalBox(RowsBox);

	UButton* RestartButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass());
	UTextBlock* RestartLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	RestartLabel->SetText(FText::FromString(TEXT("Restart")));
	RestartLabel->SetJustification(ETextJustify::Center);
	RestartButton->AddChild(RestartLabel);
	RestartButton->OnClicked.AddDynamic(this, &URaceResultsWidget::OnRestart);
	if (UVerticalBoxSlot* RestartSlot = Box->AddChildToVerticalBox(RestartButton))
	{
		RestartSlot->SetPadding(FMargin(0.0f, 24.0f, 0.0f, 0.0f));
	}

	UE_LOG(LogTemp, Display, TEXT("RACEGAME: results widget constructed"));
}

void URaceResultsWidget::ShowResults(ARaceManager* Manager)
{
	if (!Manager || !Manager->HasResults() || !RowsBox)
	{
		return;
	}
	RowsBox->ClearChildren();
	const FRaceResults& Snapshot = Manager->GetResults();
	int32 Place = 0;
	for (const FRaceResultEntry& E : Snapshot.Ordered)
	{
		++Place;
		const FString Line = FString::Printf(TEXT("%d. P%d%s - %d laps - best %.2f s"),
			Place, E.ParticipantIndex + 1,
			E.ParticipantIndex == 0 ? TEXT(" (YOU)") : TEXT(""),
			E.CompletedLaps, E.BestLapTime);
		UTextBlock* Row = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		Row->SetText(FText::FromString(Line));
		Row->SetJustification(ETextJustify::Center);
		RowsBox->AddChildToVerticalBox(Row);
		UE_LOG(LogTemp, Display, TEXT("RACEGAME: results row %s"), *Line);
	}
	UE_LOG(LogTemp, Display, TEXT("RACEGAME: results shown entries=%d"), Snapshot.Ordered.Num());
}

void URaceResultsWidget::OnRestart()
{
	UE_LOG(LogTemp, Display, TEXT("RACEGAME: Restart pressed"));
	if (ARaceGameMode* Mode = Cast<ARaceGameMode>(UGameplayStatics::GetGameMode(this)))
	{
		Mode->RestartRace();
	}
}
