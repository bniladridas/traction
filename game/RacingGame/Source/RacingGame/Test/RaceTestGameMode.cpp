// See header.

#include "RaceTestGameMode.h"
#include "RaceVehicle.h"
#include "Task2Probe.h"
#include "Engine/World.h"
#include "TimerManager.h"

ARaceTestGameMode::ARaceTestGameMode()
{
	DefaultPawnClass = ARaceVehicle::StaticClass();
}

void ARaceTestGameMode::BeginPlay()
{
	Super::BeginPlay();

	// Spawn the E2E probe after pawn possession has had a chance to happen.
	FTimerHandle H;
	GetWorldTimerManager().SetTimer(H, [this]()
	{
		FActorSpawnParameters P;
		P.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		GetWorld()->SpawnActor<ATask2Probe>(ATask2Probe::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator, P);
		UE_LOG(LogTemp, Display, TEXT("TASK2E2E: probe spawned"));
	}, 1.0f, false);
}
