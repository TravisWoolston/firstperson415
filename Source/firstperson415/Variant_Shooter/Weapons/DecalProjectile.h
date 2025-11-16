// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/SphereComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "DecalProjectile.generated.h"

class UNiagaraSystem;
UCLASS()
class FIRSTPERSON415_API ADecalProjectile : public AActor
{
	GENERATED_BODY()

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:
	// Sets default values for this actor's properties
	ADecalProjectile();
	// Collision component
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Projectile")
	USphereComponent* CollisionComponent;

	// Projectile mesh
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Projectile")
	UStaticMeshComponent* ProjectileMesh;

	// Projectile movement component
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Projectile")
	UProjectileMovementComponent* ProjectileMovement;

	// Use collision profile instead of manual collision setup
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile|Collision")
	bool bUseCollisionProfile = false;

	// Collision profile name to use (only if bUseCollisionProfile is true)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile|Collision", meta = (EditCondition = "bUseCollisionProfile"))
	FName CollisionProfileName = FName("BlockAll");

	// Base material for the projectile (will be used to create dynamic material instance)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile|Material")
	UMaterialInterface* BaseMaterial;

	// Dynamic material instance for the projectile
	UPROPERTY()
	UMaterialInstanceDynamic* ProjectileMaterialInstance;

	// Base material for the decal (will be used to create dynamic material instance)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Decal|Material")
	UMaterialInterface* BaseDecalMaterial;

	// Array of decal textures to randomize from
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Decal|Material")
	TArray<UTexture*> DecalTextures;

	// Size of the decal to spawn
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Decal")
	FVector DecalSize = FVector(10.0f, 10.0f, 10.0f);

	// Decal lifetime in seconds (0 = infinite)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Decal", meta = (ClampMin = "0.0"))
	float DecalLifeSpan = 10.0f;

	// Randomized color for both projectile and decal
	FLinearColor RandomColor;
	UPROPERTY(EditAnywhere)
	UNiagaraSystem* colorP;

	// Handle collision with objects
	UFUNCTION()
	void OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);
private:
	bool DecalSpawned = false;

protected:
	// Generate random color
	void GenerateRandomColor();

	// Spawn decal at hit location
	void SpawnDecal(const FVector& HitLocation, const FVector& HitNormal);
};
