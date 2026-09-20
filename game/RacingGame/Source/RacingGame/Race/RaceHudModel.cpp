// See header.

#include "RaceHudModel.h"
#include "RaceManager.h"

void URaceHudModel::Init(ARaceManager* InManager, const FRaceHudConfig& InConfig)
{
	Manager = InManager;
	Config = InConfig;
	Refresh();
}

void URaceHudModel::Refresh()
{
	if (!Manager)
	{
		return;
	}

	TotalLaps = Manager->RaceConfig.LapCount;
	Lap = Manager->GetParticipantLaps(0);

	const float Remaining = Manager->GetCountdownRemaining();
	CountdownSeconds = FMath::CeilToInt32(Remaining);

	const int32 Field = Manager->GetParticipantCount();
	Position = FMath::Clamp(Manager->GetPosition(Manager->GetParticipantVehicle(0)), 1, FMath::Max(Field, 1));
	FieldSize = Field > 0 ? Field : 1;

	const bool bRacing = (Manager->GetPhase() == ERacePhase::Racing);
	bHasFinish = Manager->HasResults();

	CountdownText = (Config.bShowCountdown && Manager->GetPhase() == ERacePhase::Countdown && CountdownSeconds > 0)
		? ApplyTemplate(Config.CountdownTemplate, {
			{ TEXT("{Countdown}"), FString::FromInt(CountdownSeconds) } })
		: FString();

	const bool bShowLap = Config.bShowLap && bRacing;
	LapText = bShowLap
		? ApplyTemplate(Config.LapTemplate, {
			{ TEXT("{Lap}"), FString::FromInt(Lap) },
			{ TEXT("{Total}"), FString::FromInt(TotalLaps) } })
		: FString();

	const bool bShowPos = Config.bShowPosition && bRacing;
	PositionText = bShowPos
		? ApplyTemplate(Config.PositionTemplate, {
			{ TEXT("{Position}"), FString::FromInt(Position) },
			{ TEXT("{Field}"), FString::FromInt(FieldSize) } })
		: FString();

	const bool bShowFinish = Config.bShowFinish && bHasFinish;
	FinishText = bShowFinish
		? ApplyTemplate(Config.FinishTemplate, {
			{ TEXT("{Position}"), FString::FromInt(Position) },
			{ TEXT("{Field}"), FString::FromInt(FieldSize) } })
		: FString();
}

FString URaceHudModel::ApplyTemplate(const FString& Template, const TMap<FString, FString>& Tokens) const
{
	FString Out = Template;
	for (const TPair<FString, FString>& Token : Tokens)
	{
		Out = Out.Replace(*Token.Key, *Token.Value);
	}
	return Out;
}