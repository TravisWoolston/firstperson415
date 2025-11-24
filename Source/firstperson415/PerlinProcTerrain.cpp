// Fill out your copyright notice in the Description page of Project Settings.


#include "PerlinProcTerrain.h"
#include "ProceduralMeshComponent.h"
#include "KismetProceduralMeshLibrary.h"

// Sets default values
APerlinProcTerrain::APerlinProcTerrain()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	ProcMesh = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("GeneratedMesh"));
	RootComponent = ProcMesh;
	ProcMesh->bUseAsyncCooking = true;

	// Enable collision
	ProcMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	ProcMesh->SetCollisionObjectType(ECollisionChannel::ECC_WorldStatic);
	ProcMesh->SetCollisionResponseToAllChannels(ECR_Block);

	// Make the surface walkable for characters
	ProcMesh->SetWalkableSlopeOverride(FWalkableSlopeOverride(WalkableSlope_Default, 0.f));
	ProcMesh->CanCharacterStepUpOn = ECB_Yes;
}

// Called when the game starts or when spawned
void APerlinProcTerrain::BeginPlay()
{
	Super::BeginPlay();
}

// Called in editor when properties change or actor is moved
void APerlinProcTerrain::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	// Clear existing data
	Vertices.Empty();
	Triangles.Empty();
	Normals.Empty();
	UV0.Empty();
	UpVertexColors.Empty();
	Tangents.Empty();

	// Clear existing mesh sections
	ProcMesh->ClearAllMeshSections();

	// Generate mesh data
	CreateVertices();
	CreateTriangles();

	// Generate normals and tangents using UE's built-in function
	UKismetProceduralMeshLibrary::CalculateTangentsForMesh(Vertices, Triangles, UV0, Normals, Tangents);

	// Generate vertex colors (white by default)
	for (int i = 0; i < Vertices.Num(); i++)
	{
		UpVertexColors.Add(FLinearColor(1.0f, 1.0f, 1.0f, 1.0f));
	}

	// Create the mesh section with collision enabled (true parameter)
	ProcMesh->CreateMeshSection_LinearColor(SectionID, Vertices, Triangles, Normals, UV0, UpVertexColors, Tangents, true);

	// Set material if provided
	if (Mat)
	{
		ProcMesh->SetMaterial(0, Mat);
	}

	// Recreate physics state to update collision
	ProcMesh->RecreatePhysicsState();
}

// Called every frame
void APerlinProcTerrain::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void APerlinProcTerrain::AlterMesh(FVector impactPoint) {
	for (int i = 0; i < Vertices.Num(); i++)
	{
		FVector tempVector = impactPoint - this->GetActorLocation();
		if (FVector(Vertices[i] - tempVector).Size() < radius) {
			Vertices[i] -= Depth;
		}
	}

	// Recalculate normals and tangents after mesh alteration
	Normals.Empty();
	Tangents.Empty();
	UKismetProceduralMeshLibrary::CalculateTangentsForMesh(Vertices, Triangles, UV0, Normals, Tangents);

	// Convert FLinearColor to FColor for UpdateMeshSection
	TArray<FColor> VertexColors;
	VertexColors.Reserve(UpVertexColors.Num());
	for (const FLinearColor& LinearColor : UpVertexColors)
	{
		VertexColors.Add(LinearColor.ToFColor(false));
	}

	// Update the mesh
	ProcMesh->UpdateMeshSection(SectionID, Vertices, Normals, UV0, VertexColors, Tangents);

	// Recreate physics state to update collision after mesh deformation
	ProcMesh->RecreatePhysicsState();
}

void APerlinProcTerrain::CreateVertices()
{
	for (int x = 0; x <= XSize; x++) {
		for (int y = 0; y <= YSize; y++)
		{
			float z = FMath::PerlinNoise2D(FVector2D(x * NoiseScale + .1, y * NoiseScale + .1)) * ZMultiplier;
			Vertices.Add(FVector(x * Scale, y * Scale, z));
			UV0.Add(FVector2D(x * UVScale, y * UVScale));
		}
	}
}

void APerlinProcTerrain::CreateTriangles()
{
	int Vertex = 0;
	for (int x = 0; x < XSize; x++) {
		for (int y = 0; y < YSize; y++)
		{
			Triangles.Add(Vertex);
			Triangles.Add(Vertex + YSize + 1);
			Triangles.Add(Vertex + 1);
			Triangles.Add(Vertex + 1);
			Triangles.Add(Vertex + YSize + 1);
			Triangles.Add(Vertex + YSize + 2);
			Vertex++;
		}
		Vertex++;
	}
}

