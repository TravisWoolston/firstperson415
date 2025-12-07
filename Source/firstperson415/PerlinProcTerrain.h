// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ProceduralMeshComponent.h"
#include "PerlinProcTerrain.generated.h"

class UProceduralMeshComponent;
class UMaterialInterface;

UCLASS()
class FIRSTPERSON415_API APerlinProcTerrain : public AActor
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	APerlinProcTerrain();

	UPROPERTY(EditAnywhere, Meta = (ClampMin = 0))
	int XSize = 0;

	UPROPERTY(EditAnywhere, Meta = (ClampMin = 0))
	int YSize = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Meta = (ClampMin = 0.0f))
	float ZMultiplier = 1.0f;

	UPROPERTY(EditAnywhere, Meta = (ClampMin = 0))
	float NoiseScale = 1.0f;

	UPROPERTY(EditAnywhere, Meta = (ClampMin = 0.000001))
	float Scale = 0;

	UPROPERTY(EditAnywhere, Meta = (ClampMin = 0.000001))
	float UVScale = 0;

	UPROPERTY(EditAnywhere)
	float radius;

	UPROPERTY(EditAnywhere)
	FVector Depth;

	UPROPERTY(EditAnywhere, Category = "Portals")
	TSubclassOf<class APortal> PortalClass;

	UPROPERTY(EditAnywhere, Category = "Portals")
	int32 NumPortalsToSpawn = 0;

	UPROPERTY(EditAnywhere, Category = "Portals")
	UMaterialInterface* PortalMaterial;

	// Template Render Targets (optional, overrides resolution setting if provided)
	UPROPERTY(EditAnywhere, Category = "Portals")
	UTextureRenderTarget2D* RenderTargetTemplate;

	// Template Materials for specific RT assignment if needed
	UPROPERTY(EditAnywhere, Category = "Portals")
	UMaterialInterface* PortalMaterialTemplate;

	UPROPERTY(EditAnywhere, Category = "Portals")
	FName PortalTextureParameterName = "RenderTargetParam";

	// Capture Source: Use LDR (Final Color) to avoid exposure issues, or HDR (Scene Color) for high dynamic range
	UPROPERTY(EditAnywhere, Category = "Portals")
	TEnumAsByte<ESceneCaptureSource> PortalCaptureSource = ESceneCaptureSource::SCS_FinalColorLDR;

	UPROPERTY(EditAnywhere, Category = "Portals")
	int32 PortalRenderTargetResolution = 256;

	// Button to generate portals in the editor
	UFUNCTION(CallInEditor, Category = "Portals")
	void GeneratePortals();

	// Button to clear portals in the editor
	UFUNCTION(CallInEditor, Category = "Portals")
	void ClearPortals();

protected:
	// Track spawned portals
	UPROPERTY(VisibleAnywhere, Category = "Portals")
	TArray<class APortal*> SpawnedPortals;

	// Called when the game starts or when spawned
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	// Called in editor when properties change
	virtual void OnConstruction(const FTransform& Transform) override;

	UPROPERTY(EditAnywhere)
	UMaterialInterface* Mat;
public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	UFUNCTION(BlueprintCallable)
	void AlterMesh(FVector impactPoint);

private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Mesh", meta = (AllowPrivateAccess = "true"))
	UProceduralMeshComponent* ProcMesh;
	
	TArray<FVector> Vertices;
	TArray<int> Triangles;
	TArray<FVector> Normals;
	TArray<FVector2D> UV0;
	TArray<FLinearColor> UpVertexColors;
	TArray<FProcMeshTangent> Tangents;

	int SectionID = 0;

	void CreateVertices();
	void CreateTriangles();
	void GenerateMesh();
	void SpawnPortals(); // Internal spawn logic
	void ConfigurePortals();
};
