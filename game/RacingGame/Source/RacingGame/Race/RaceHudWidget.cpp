// See header.

#include "RaceHudWidget.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Blueprint/UserWidget.h"

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
}