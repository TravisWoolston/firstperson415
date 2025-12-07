// Fill out your copyright notice in the Description page of Project Settings.


#include "PerlinProcTerrain.h"
#include "ProceduralMeshComponent.h"
#include "KismetProceduralMeshLibrary.h"
#include "Portal.h"
#include "Kismet/KismetRenderingLibrary.h"
#include "Materials/MaterialInstanceDynamic.h"

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
	
	// Disable async cooking at runtime to ensure collision is ready immediately
	// Async cooking can cause timing issues with character movement
	ProcMesh->bUseAsyncCooking = false;
	
	// Regenerate mesh at runtime to ensure proper collision initialization
	// This is critical for character movement to work correctly
	GenerateMesh();
	
	// Force update bounds and mark for refresh
	ProcMesh->UpdateBounds();
	ProcMesh->MarkRenderStateDirty();

	// Only spawn portals at runtime if none exist
	if (SpawnedPortals.Num() == 0)
	{
		SpawnPortals();
	}
	
	// Configure portals (create RTs, Materials, etc) for runtime
	ConfigurePortals();
}

// Called in editor when properties change or actor is moved
void APerlinProcTerrain::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	
	// Generate mesh in editor for preview
	GenerateMesh();
}

void APerlinProcTerrain::GenerateMesh()
{
	if (!ProcMesh)
	{
		return;
	}

	// Clear existing data
	Vertices.Empty();
	Triangles.Empty();
	Normals.Empty();
	UV0.Empty();
	UpVertexColors.Empty();
	Tangents.Empty();

	// Clear existing mesh sections and physics state
	ProcMesh->ClearAllMeshSections();
	
	// Temporarily disable collision while rebuilding
	ProcMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// Generate mesh data
	CreateVertices();
	CreateTriangles();

	// Ensure we have valid mesh data
	if (Vertices.Num() == 0 || Triangles.Num() == 0)
	{
		return;
	}

	// Generate normals and tangents using UE's built-in function
	UKismetProceduralMeshLibrary::CalculateTangentsForMesh(Vertices, Triangles, UV0, Normals, Tangents);

	// Generate vertex colors (white by default)
	for (int i = 0; i < Vertices.Num(); i++)
	{
		UpVertexColors.Add(FLinearColor(1.0f, 1.0f, 1.0f, 1.0f));
	}

	// Configure collision BEFORE creating mesh section
	ProcMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	ProcMesh->SetCollisionObjectType(ECollisionChannel::ECC_WorldStatic);
	ProcMesh->SetCollisionResponseToAllChannels(ECR_Block);
	ProcMesh->SetWalkableSlopeOverride(FWalkableSlopeOverride(WalkableSlope_Default, 0.f));
	ProcMesh->CanCharacterStepUpOn = ECB_Yes;

	// Create the mesh section with collision enabled (true parameter)
	ProcMesh->CreateMeshSection_LinearColor(SectionID, Vertices, Triangles, Normals, UV0, UpVertexColors, Tangents, true);

	// Set material if provided
	if (Mat)
	{
		ProcMesh->SetMaterial(0, Mat);
	}

	// Update bounds and mark for refresh
	ProcMesh->UpdateBounds();
	ProcMesh->MarkRenderStateDirty();

	// Recreate physics state to update collision
	// This is especially important at runtime for proper character interaction
	ProcMesh->RecreatePhysicsState();
}

// Called every frame
void APerlinProcTerrain::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void APerlinProcTerrain::AlterMesh(FVector impactPoint) {
	if (Vertices.Num() == 0) return;

	// Convert impact point to local space
	FVector LocalPoint = GetActorTransform().InverseTransformPosition(impactPoint);

	// Optimize: Only check vertices within the radius range
	// Calculate grid indices based on Scale
	int32 CenterX = FMath::RoundToInt(LocalPoint.X / Scale);
	int32 CenterY = FMath::RoundToInt(LocalPoint.Y / Scale);
	int32 RadiusInSteps = FMath::CeilToInt(radius / Scale) + 1; // +1 buffer

	// Clamp bounds
	int32 MinX = FMath::Clamp(CenterX - RadiusInSteps, 0, XSize);
	int32 MaxX = FMath::Clamp(CenterX + RadiusInSteps, 0, XSize);
	int32 MinY = FMath::Clamp(CenterY - RadiusInSteps, 0, YSize);
	int32 MaxY = FMath::Clamp(CenterY + RadiusInSteps, 0, YSize);

	bool bModified = false;
	float RadiusSq = radius * radius;

	// Only iterate the relevant subset of vertices
	for (int x = MinX; x <= MaxX; x++) {
		for (int y = MinY; y <= MaxY; y++) {
			int32 Index = x * (YSize + 1) + y;
			
			if (Vertices.IsValidIndex(Index))
			{
				if (FVector::DistSquared(Vertices[Index], LocalPoint) < RadiusSq) {
					Vertices[Index] -= Depth;
					bModified = true;
				}
			}
		}
	}

	if (bModified)
	{
		// OPTIMIZATION: Recalculate normals ONLY for the affected area instead of the whole mesh
		// UKismetProceduralMeshLibrary::CalculateTangentsForMesh is O(N) and very slow for dynamic updates
		
		int32 NormalMinX = FMath::Clamp(MinX - 1, 0, XSize);
		int32 NormalMaxX = FMath::Clamp(MaxX + 1, 0, XSize);
		int32 NormalMinY = FMath::Clamp(MinY - 1, 0, YSize);
		int32 NormalMaxY = FMath::Clamp(MaxY + 1, 0, YSize);

		for (int x = NormalMinX; x <= NormalMaxX; x++) {
			for (int y = NormalMinY; y <= NormalMaxY; y++) {
				int32 Idx = x * (YSize + 1) + y;
				
				// Get neighbors (clamp to edges)
				int32 L_Idx = FMath::Clamp(x - 1, 0, XSize) * (YSize + 1) + y;
				int32 R_Idx = FMath::Clamp(x + 1, 0, XSize) * (YSize + 1) + y;
				int32 D_Idx = x * (YSize + 1) + FMath::Clamp(y - 1, 0, YSize);
				int32 U_Idx = x * (YSize + 1) + FMath::Clamp(y + 1, 0, YSize);
				
				FVector vL = Vertices[L_Idx];
				FVector vR = Vertices[R_Idx];
				FVector vD = Vertices[D_Idx];
				FVector vU = Vertices[U_Idx];
				
				// Compute gradients
				FVector TanX = vR - vL;
				FVector TanY = vU - vD;
				
				// Normal = Cross(TanX, TanY) normalized
				// Assuming grid X is Forward, Y is Right, Z is Up
				FVector NewNormal = FVector::CrossProduct(TanX, TanY).GetSafeNormal();
				
				// Update Normal
				if (Normals.IsValidIndex(Idx))
				{
					Normals[Idx] = NewNormal;
				}
				
				// Update Tangent (approximate from X gradient)
				if (Tangents.IsValidIndex(Idx))
				{
					Tangents[Idx] = FProcMeshTangent(TanX.GetSafeNormal(), false);
				}
			}
		}

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
			// First triangle - counter-clockwise when viewed from above (normal pointing up)
			Triangles.Add(Vertex);
			Triangles.Add(Vertex + 1);
			Triangles.Add(Vertex + YSize + 1);
			
			// Second triangle - counter-clockwise when viewed from above
			Triangles.Add(Vertex + 1);
			Triangles.Add(Vertex + YSize + 2);
			Triangles.Add(Vertex + YSize + 1);
			Vertex++;
		}
		Vertex++;
	}
}

void APerlinProcTerrain::GeneratePortals()
{
	ClearPortals();
	
	// Ensure mesh data is available (especially in editor)
	if (Vertices.Num() == 0)
	{
		GenerateMesh();
	}

	SpawnPortals();
	
	// Configure immediately for editor visualization
	ConfigurePortals();
}

void APerlinProcTerrain::ClearPortals()
{
	for (APortal* Portal : SpawnedPortals)
	{
		if (Portal)
		{
			Portal->Destroy();
		}
	}
	SpawnedPortals.Empty();
}

void APerlinProcTerrain::SpawnPortals()
{
	if (!PortalClass || NumPortalsToSpawn <= 0 || Vertices.Num() == 0)
	{
		return;
	}

	// Clean up any null pointers in the array just in case
	SpawnedPortals.RemoveAll([](APortal* P) { return P == nullptr; });

	// Determine how many more we need
	int32 Needed = NumPortalsToSpawn - SpawnedPortals.Num();
	if (Needed <= 0) return;

	
	// Create a random stream for consistent results if needed, or just use FMath
	for (int32 i = 0; i < Needed; i++)
	{
		// Pick a random vertex
		int32 RandomVertexIndex = FMath::RandRange(0, Vertices.Num() - 1);
		FVector LocalLocation = Vertices[RandomVertexIndex];
		
		// Convert to world space
		FVector WorldLocation = GetActorTransform().TransformPosition(LocalLocation);
		
		// Add a small offset to ensure it's slightly above ground
		WorldLocation.Z += 150.0f; // Increased from 50.0f to prevent clipping

		FActorSpawnParameters SpawnParams;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
#if WITH_EDITOR
		SpawnParams.bTemporaryEditorActor = false; // Make them persistent if generated in editor
#endif

		APortal* NewPortal = GetWorld()->SpawnActor<APortal>(PortalClass, WorldLocation, FRotator::ZeroRotator, SpawnParams);
		if (NewPortal)
		{
#if WITH_EDITOR
			// Ensure it's attached or grouped if desired, or just set label
			NewPortal->SetActorLabel(FString::Printf(TEXT("Portal_%d"), SpawnedPortals.Num()));
			NewPortal->AttachToActor(this, FAttachmentTransformRules::KeepWorldTransform);
#endif
			SpawnedPortals.Add(NewPortal);
		}
	}

	// Link pairs of portals
	for (int32 i = 0; i < SpawnedPortals.Num() - 1; i += 2)
	{
		if (SpawnedPortals.IsValidIndex(i) && SpawnedPortals.IsValidIndex(i + 1))
		{
			APortal* PortalA = SpawnedPortals[i];
			APortal* PortalB = SpawnedPortals[i + 1];

			if (PortalA && PortalB)
			{
				// Link them
				PortalA->OtherPortal = PortalB;
				PortalB->OtherPortal = PortalA;

				// Assign Material property so it persists
				if (PortalMaterial)
				{
					PortalA->Mat = PortalMaterial;
					PortalB->Mat = PortalMaterial;
				}
			}
		}
	}
}

void APerlinProcTerrain::ConfigurePortals()
{
	// Iterate valid pairs
	for (int32 i = 0; i < SpawnedPortals.Num() - 1; i += 2)
	{
		if (SpawnedPortals.IsValidIndex(i) && SpawnedPortals.IsValidIndex(i + 1))
		{
			APortal* PortalA = SpawnedPortals[i];
			APortal* PortalB = SpawnedPortals[i + 1];

			if (PortalA && PortalB)
			{
				// Create and Assign Render Targets
				UTextureRenderTarget2D* RTA = nullptr;
				UTextureRenderTarget2D* RTB = nullptr;

				if (RenderTargetTemplate)
				{
					// Duplicate the template if provided (preserves settings like format, clear color, etc.)
					// Ensure unique names to avoid conflicts
					FName NameA = MakeUniqueObjectName(this, UTextureRenderTarget2D::StaticClass(), FName("PortalRT_A"));
					FName NameB = MakeUniqueObjectName(this, UTextureRenderTarget2D::StaticClass(), FName("PortalRT_B"));
					
					RTA = DuplicateObject<UTextureRenderTarget2D>(RenderTargetTemplate, this, NameA);
					RTB = DuplicateObject<UTextureRenderTarget2D>(RenderTargetTemplate, this, NameB);
				}
				else if (PortalRenderTargetResolution > 0)
				{
					// Create fresh if no template
					RTA = UKismetRenderingLibrary::CreateRenderTarget2D(this, PortalRenderTargetResolution, PortalRenderTargetResolution);
					RTB = UKismetRenderingLibrary::CreateRenderTarget2D(this, PortalRenderTargetResolution, PortalRenderTargetResolution);
				}

				if (RTA && RTB)
				{
					PortalA->RenderTarget = RTA;
					PortalB->RenderTarget = RTB;

					// Assign render target to SceneCapture
					if (PortalA->SceneCapture) 
					{
						PortalA->SceneCapture->TextureTarget = RTA;
						PortalA->SceneCapture->CaptureSource = PortalCaptureSource;
					}
					if (PortalB->SceneCapture) 
					{
						PortalB->SceneCapture->TextureTarget = RTB;
						PortalB->SceneCapture->CaptureSource = PortalCaptureSource;
					}

					// Create Dynamic Materials
					// Prioritize Template -> PortalMaterial -> Instance Material
					// Determine material for each portal individually
					UMaterialInterface* BaseMatA = PortalMaterialTemplate ? PortalMaterialTemplate : (PortalMaterial ? PortalMaterial : PortalA->Mat);
					UMaterialInterface* BaseMatB = PortalMaterialTemplate ? PortalMaterialTemplate : (PortalMaterial ? PortalMaterial : PortalB->Mat);
					
					if (BaseMatA && BaseMatB)
					{
						UMaterialInstanceDynamic* DMI_A = UMaterialInstanceDynamic::Create(BaseMatA, this);
						UMaterialInstanceDynamic* DMI_B = UMaterialInstanceDynamic::Create(BaseMatB, this);

						if (DMI_A && DMI_B)
						{
							// Portal A should display what Portal B sees (RTB)
							DMI_A->SetTextureParameterValue(PortalTextureParameterName, RTB);
							
							// Portal B should display what Portal A sees (RTA)
							DMI_B->SetTextureParameterValue(PortalTextureParameterName, RTA);

							// Verify assignment
							UTexture* CheckTexA = nullptr;
							DMI_A->GetTextureParameterValue(PortalTextureParameterName, CheckTexA);
							if (CheckTexA != RTB)
							{
								UE_LOG(LogTemp, Warning, TEXT("Failed to set parameter '%s' on Portal A material. Check material parameter name!"), *PortalTextureParameterName.ToString());
							}

							// Assign to meshes
							if (PortalA->Mesh) PortalA->Mesh->SetMaterial(0, DMI_A);
							if (PortalB->Mesh) PortalB->Mesh->SetMaterial(0, DMI_B);
						}
					}
				}

				// Force update to apply changes and ensure capture is active
				if (PortalA->SceneCapture) PortalA->SceneCapture->bCaptureEveryFrame = true;
				if (PortalB->SceneCapture) PortalB->SceneCapture->bCaptureEveryFrame = true;
				
				PortalA->UpdatePortals();
				PortalB->UpdatePortals();
			}
		}
	}
}
