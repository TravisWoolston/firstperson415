// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ProceduralMeshComponent.h"
#include "ProcMeshFromStatic.generated.h"

UCLASS()
class FIRSTPERSON415_API AProcMeshFromStatic : public AActor
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	AProcMeshFromStatic();



protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	virtual void PostActorCreated() override;

	virtual void PostLoad() override;

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	UPROPERTY()
	TArray<FVector> Normals;

	UPROPERTY(EditAnywhere)
	TArray<FVector> Vertices;

	UPROPERTY(EditAnywhere)
	TArray<int> Triangles;

	UPROPERTY(EditAnywhere)
	UMaterialInterface* PlaneMat;

	UPROPERTY(EditAnywhere)
	TArray<FVector2D> UV0;

	TArray<FProcMeshTangent> Tangents;

	UPROPERTY()
	TArray<FLinearColor> VertexColors;

	TArray<FColor> UpVertexColors;

	UPROPERTY(EditAnywhere)
	UStaticMeshComponent* BaseMesh;

private:
	UProceduralMeshComponent* ProcMesh;
	void GetMeshData();
	void CreateMesh();
};
