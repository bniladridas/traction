// See header.

#include "RaceTrack.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "UObject/ConstructorHelpers.h"

ARaceTrack::ARaceTrack()
{
	USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("TrackRoot"));
	RootComponent = Root;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeAsset(TEXT("/Engine/BasicShapes/Cube"));
	if (CubeAsset.Succeeded())
	{
		CubeMesh = CubeAsset.Object;
	}
}

void ARaceTrack::BeginPlay()
{
	Super::BeginPlay();
	BuildTrack();
	UE_LOG(LogTemp, Display, TEXT("RACETRACK: built %s segs=%d checkpoints=%d length=%.1f"),
		*TrackConfig.TrackName, RoadSegmentCount, Checkpoints.Num(), TrackLength);
}

void ARaceTrack::BuildTrack()
{
	const TArray<FVector>& Ctrl = TrackConfig.ControlPoints;
	if (Ctrl.Num() < 4)
	{
		return;
	}

	// Dense centerline: length-based subdivision so pursuit indices map
	// to roughly uniform distance (~250 cm) on straights and curves.
	TArray<FVector> Dense;
	Dense.Add(Ctrl[0]);
	for (int32 i = 0; i < Ctrl.Num(); ++i)
	{
		const FVector A = Ctrl[i];
		const FVector B = Ctrl[(i + 1) % Ctrl.Num()];
		const float SpanLen = (B - A).Size2D();
		if (SpanLen < 1.0f)
		{
			continue;
		}
		const int32 N = FMath::Clamp(FMath::CeilToInt(SpanLen / 250.0f), 1, 16);
		for (int32 k = 1; k <= N; ++k)
		{
			Dense.Add(A + (B - A) * (static_cast<float>(k) / static_cast<float>(N)));
		}
	}

	// Centerline with cumulative distance. Last dense point repeats the
	// first positionally, marking closure.
	CenterPoints.Reset();
	CenterPoints.Reserve(Dense.Num());
	float S = 0.0f;
	for (int32 i = 0; i < Dense.Num(); ++i)
	{
		FRaceTrackCenterPoint P;
		P.Position = FVector(Dense[i].X, Dense[i].Y, TrackConfig.RoadTopZ);
		const FVector Next = Dense[(i + 1) % Dense.Num()];
		const FVector Prev = Dense[(i + Dense.Num() - 1) % Dense.Num()];
		FVector Dir = Next - Dense[i];
		if (Dir.Size2D() < 1.0f)
		{
			Dir = Dense[i] - Prev;
		}
		Dir.Z = 0.0f;
		P.Forward = Dir.GetSafeNormal();
		if (i > 0)
		{
			S += (Dense[i] - Dense[i - 1]).Size2D();
		}
		P.Distance = S;
		CenterPoints.Add(P);
	}
	TrackLength = S;

	const int32 SegCount = Dense.Num() - 1;
	for (int32 i = 0; i < SegCount; ++i)
	{
		const FVector A = Dense[i];
		const FVector B = Dense[i + 1];
		const float Len = (B - A).Size2D();
		if (Len < 1.0f)
		{
			continue;
		}
		// Joint extension capped at half the segment length. A fixed
		// extension overshoots short curve micros and throws wall stubs
		// across the lane (observed pin at the hairpin entry); the cap
		// keeps every tip near its own segment while still covering joints.
		const float Ext = FMath::Min(150.0f, Len * 0.5f);
		const FVector Mid = (A + B) * 0.5f;
		const FVector Dir = (B - A).GetSafeNormal2D();
		const float Yaw = FMath::RadiansToDegrees(FMath::Atan2(Dir.Y, Dir.X));
		const FRotator Rot(0.0f, Yaw, 0.0f);

		// Road collision: top surface exactly at RoadTopZ.
		UBoxComponent* Road = NewObject<UBoxComponent>(this, FName(*FString::Printf(TEXT("Road_%d"), i)));
		Road->SetBoxExtent(FVector((Len + Ext * 2.0f) * 0.5f, TrackConfig.TrackWidth * 0.5f, 50.0f));
		Road->SetWorldLocationAndRotation(
			FVector(Mid.X, Mid.Y, TrackConfig.RoadTopZ - 50.0f), Rot, false, nullptr, ETeleportType::None);
		Road->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		Road->SetCollisionObjectType(ECC_WorldStatic);
		Road->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Block);
		Road->AttachToComponent(RootComponent, FAttachmentTransformRules::KeepWorldTransform);
		Road->RegisterComponent();

		// Boundary walls on both edges.
		const FVector Side(-Dir.Y, Dir.X, 0.0f);
		for (int32 SideSign = -1; SideSign <= 1; SideSign += 2)
		{
			const FVector WallC = FVector(Mid.X, Mid.Y, TrackConfig.RoadTopZ + TrackConfig.WallHeight * 0.5f)
				+ Side * (static_cast<float>(SideSign) * (TrackConfig.TrackWidth * 0.5f + TrackConfig.WallThickness * 0.5f));
			UBoxComponent* Wall = NewObject<UBoxComponent>(this,
				FName(*FString::Printf(TEXT("Wall_%d_%s"), i, (SideSign < 0) ? TEXT("L") : TEXT("R"))));
			Wall->SetBoxExtent(FVector((Len + Ext * 2.0f) * 0.5f, TrackConfig.WallThickness * 0.5f, TrackConfig.WallHeight * 0.5f));
			Wall->SetWorldLocationAndRotation(WallC, Rot, false, nullptr, ETeleportType::None);
			Wall->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
			Wall->SetCollisionObjectType(ECC_WorldStatic);
			Wall->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Block);
			Wall->AttachToComponent(RootComponent, FAttachmentTransformRules::KeepWorldTransform);
			Wall->RegisterComponent();
		}

		// Plain road visual matching the collision box. Appearance only;
		// no polished art in this task.
		if (CubeMesh)
		{
			UStaticMeshComponent* Vis = NewObject<UStaticMeshComponent>(this);
			Vis->SetStaticMesh(CubeMesh);
			Vis->SetWorldLocationAndRotation(
				FVector(Mid.X, Mid.Y, TrackConfig.RoadTopZ - 50.0f), Rot, false, nullptr, ETeleportType::None);
			Vis->SetWorldScale3D(FVector((Len + Ext * 2.0f) / 100.0f, TrackConfig.TrackWidth / 100.0f, 1.0f));
			Vis->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			Vis->AttachToComponent(RootComponent, FAttachmentTransformRules::KeepWorldTransform);
			Vis->RegisterComponent();
		}
		RoadSegmentCount++;
	}

	// Checkpoints evenly spaced by distance; index 0 is the start line.
	Checkpoints.Reset();
	const int32 NCheck = FMath::Max(1, TrackConfig.CheckpointCount);
	for (int32 k = 0; k < NCheck; ++k)
	{
		const float At = FMath::Fmod(TrackConfig.StartLineDistance + static_cast<float>(k) * TrackLength / static_cast<float>(NCheck), TrackLength);
		const FRaceTrackCenterPoint P = SampleAtDistance(At);
		FRaceTrackCheckpoint CP;
		CP.Index = k;
		CP.Position = P.Position;
		CP.Forward = P.Forward;
		CP.Width = TrackConfig.TrackWidth;
		Checkpoints.Add(CP);
	}

	// Start pose on the line; spawn sits behind it.
	const FRaceTrackCenterPoint Line = SampleAtDistance(TrackConfig.StartLineDistance);
	const FRaceTrackCenterPoint Back = SampleAtDistance(TrackConfig.StartLineDistance - TrackConfig.SpawnBackoff);
	StartPosition = FVector(Back.Position.X, Back.Position.Y, 60.0f);
	StartYawDeg = FMath::RadiansToDegrees(FMath::Atan2(Line.Forward.Y, Line.Forward.X));
}

FRaceTrackCenterPoint ARaceTrack::SampleAtDistance(float S) const
{
	FRaceTrackCenterPoint Out;
	if (CenterPoints.Num() == 0 || TrackLength <= 0.0f)
	{
		return Out;
	}
	float Clamped = FMath::Fmod(S, TrackLength);
	if (Clamped < 0.0f)
	{
		Clamped += TrackLength;
	}
	for (int32 i = 0; i < CenterPoints.Num(); ++i)
	{
		const FRaceTrackCenterPoint& A = CenterPoints[i];
		const FRaceTrackCenterPoint& B = CenterPoints[(i + 1) % CenterPoints.Num()];
		float SpanEnd = (i + 1 < CenterPoints.Num()) ? B.Distance : TrackLength;
		if (Clamped >= A.Distance && Clamped <= SpanEnd)
		{
			const float Span = FMath::Max(1.0f, SpanEnd - A.Distance);
			const float T = (Clamped - A.Distance) / Span;
			Out.Position = FMath::Lerp(A.Position, B.Position, T);
			Out.Forward = (B.Position - A.Position).GetSafeNormal();
			if (Out.Forward.IsNearlyZero())
			{
				Out.Forward = A.Forward;
			}
			Out.Distance = Clamped;
			return Out;
		}
	}
	return CenterPoints[0];
}
