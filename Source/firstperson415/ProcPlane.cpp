// Fill out your copyright notice in the Description page of Project Settings.


#include "ProcPlane.h"
#include "ProceduralMeshComponent.h"

// Sets default values
AProcPlane::AProcPlane()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

	ProcMesh = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("Procedural Mesh"));
	RootComponent = ProcMesh;
}

// Called when the game starts or when spawned
void AProcPlane::BeginPlay()
{
	Super::BeginPlay();
}

void AProcPlane::PostActorCreated() 
{
	Super::PostActorCreated();
	
	// Only create mesh if we have valid data and component
	if (ProcMesh && Vertices.Num() > 0 && Triangles.Num() > 0)
	{
		CreateMesh();
		if (PlaneMat) 
		{
			ProcMesh->SetMaterial(0, PlaneMat);
		}
	}
}

void AProcPlane::PostLoad() 
{
	Super::PostLoad();
	
	// Only create mesh if we have valid data and component
	if (ProcMesh && Vertices.Num() > 0 && Triangles.Num() > 0)
	{
		CreateMesh();
		if (PlaneMat) 
		{
			ProcMesh->SetMaterial(0, PlaneMat);
		}
	}
}

// Called every frame
void AProcPlane::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void AProcPlane::CreateMesh() 
{
	if (!ProcMesh)
	{
		UE_LOG(LogTemp, Warning, TEXT("CreateMesh called but ProcMesh is null"));
		return;
	}
	
	if (Vertices.Num() == 0 || Triangles.Num() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("CreateMesh called but Vertices or Triangles are empty"));
		return;
	}
	
	ProcMesh->CreateMeshSection(0, Vertices, Triangles, TArray<FVector>(), UV0, TArray<FColor>(), TArray<FProcMeshTangent>(), true);
}