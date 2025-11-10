// Fill out your copyright notice in the Description page of Project Settings.

#include "DecalProjectile.h"
#include "Components/DecalComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"

// Sets default values
ADecalProjectile::ADecalProjectile()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

	// Create collision component
	CollisionComponent = CreateDefaultSubobject<USphereComponent>(TEXT("Collision Component"));
	RootComponent = CollisionComponent;
	CollisionComponent->SetSphereRadius(4.0f);
	CollisionComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	CollisionComponent->SetCollisionObjectType(ECC_WorldDynamic);
	CollisionComponent->SetCollisionResponseToAllChannels(ECR_Block);
	CollisionComponent->CanCharacterStepUpOn = ECanBeCharacterBase::ECB_No;
	CollisionComponent->bReturnMaterialOnMove = true;

	// Create projectile mesh
	ProjectileMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Projectile Mesh"));
	ProjectileMesh->SetupAttachment(RootComponent);
	ProjectileMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// Create projectile movement component
	ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("Projectile Movement"));
	ProjectileMovement->UpdatedComponent = CollisionComponent;
	ProjectileMovement->InitialSpeed = 3000.0f;
	ProjectileMovement->MaxSpeed = 3000.0f;
	ProjectileMovement->bShouldBounce = false;
	ProjectileMovement->ProjectileGravityScale = 0.0f;
}

// Called when the game starts or when spawned
void ADecalProjectile::BeginPlay()
{
	Super::BeginPlay();

	// Apply collision profile if specified
	if (bUseCollisionProfile && CollisionProfileName != NAME_None)
	{
		CollisionComponent->SetCollisionProfileName(CollisionProfileName);
	}

	// Generate random color for this projectile
	GenerateRandomColor();

	// Create dynamic material instance for the projectile mesh
	if (BaseMaterial && ProjectileMesh)
	{
		ProjectileMaterialInstance = UMaterialInstanceDynamic::Create(BaseMaterial, this);
		if (ProjectileMaterialInstance)
		{
			// Set the random color on the material
			ProjectileMaterialInstance->SetVectorParameterValue(FName("Color"), RandomColor);

			// Apply the material to the mesh
			ProjectileMesh->SetMaterial(0, ProjectileMaterialInstance);
		}
	}

	// Ignore the pawn that shot this projectile
	if (GetInstigator())
	{
		CollisionComponent->IgnoreActorWhenMoving(GetInstigator(), true);
	}
}

void ADecalProjectile::NotifyHit(UPrimitiveComponent* MyComp, AActor* Other, UPrimitiveComponent* OtherComp, bool bSelfMoved, FVector HitLocation, FVector HitNormal, FVector NormalImpulse, const FHitResult& Hit)
{
	Super::NotifyHit(MyComp, Other, OtherComp, bSelfMoved, HitLocation, HitNormal, NormalImpulse, Hit);

	// Debug logging

	if (!DecalSpawned)
		// Spawn decal at hit location
		SpawnDecal(Hit.ImpactPoint, Hit.ImpactNormal);

}

void ADecalProjectile::GenerateRandomColor()
{
	// Generate random RGB values between 0 and 1
	float R = UKismetMathLibrary::RandomFloatInRange(0.0f, 1.0f);
	float G = UKismetMathLibrary::RandomFloatInRange(0.0f, 1.0f);
	float B = UKismetMathLibrary::RandomFloatInRange(0.0f, 1.0f);

	// Store the random color
	RandomColor = FLinearColor(R, G, B, 1.0f);
}

void ADecalProjectile::SpawnDecal(const FVector& HitLocation, const FVector& HitNormal)
{
	DecalSpawned = true;
	if (!BaseDecalMaterial)
	{
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red, TEXT("DecalProjectile: No BaseDecalMaterial set!"));
		}
		return;
	}

	// Offset the decal slightly away from the surface to prevent z-fighting
	FVector DecalLocation = HitLocation + HitNormal * 0.5f;

	// Calculate rotation from normal vector
	// The decal should face away from the surface (negative X axis points into the surface)
	FRotator DecalRotation = (HitNormal).Rotation();

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Green,
			FString::Printf(TEXT("Spawning Decal at: %s with rotation: %s"),
				*DecalLocation.ToString(),
				*DecalRotation.ToString()));
	}

	// Spawn decal component
	UDecalComponent* DecalComponent = UGameplayStatics::SpawnDecalAtLocation(
		GetWorld(),
		BaseDecalMaterial,
		DecalSize,
		DecalLocation,
		DecalRotation,
		DecalLifeSpan
	);

	if (DecalComponent)
	{
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Green, TEXT("Decal spawned successfully!"));
		}

		// Create dynamic material instance for the decal
		UMaterialInstanceDynamic* DecalMaterialInstance = DecalComponent->CreateDynamicMaterialInstance();

		if (DecalMaterialInstance)
		{
			// Set the same random color as the projectile
			DecalMaterialInstance->SetVectorParameterValue(FName("Color"), RandomColor);

			// Randomize the decal texture if we have any textures to choose from
			if (DecalTextures.Num() > 0)
			{
				// Pick a random texture from the array
				int32 RandomIndex = UKismetMathLibrary::RandomIntegerInRange(0, DecalTextures.Num() - 1);
				UTexture* RandomTexture = DecalTextures[RandomIndex];

				if (RandomTexture)
				{
					// Set the random texture on the decal material
					DecalMaterialInstance->SetTextureParameterValue(FName("DecalTexture"), RandomTexture);

					if (GEngine)
					{
						GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Cyan,
							FString::Printf(TEXT("Applied texture: %s"), *RandomTexture->GetName()));
					}
				}
			}
		}
		else if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Orange, TEXT("Failed to create dynamic material instance for decal"));
		}
	}
	else if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red, TEXT("Failed to spawn decal!"));
	}
}
