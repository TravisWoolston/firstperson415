// Fill out your copyright notice in the Description page of Project Settings.

#include "DecalProjectile.h"
#include "Components/DecalComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h"

// Sets default values
ADecalProjectile::ADecalProjectile()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

	// Create collision component
	CollisionComponent = CreateDefaultSubobject<USphereComponent>(TEXT("Collision Component"));
	CollisionComponent->InitSphereRadius(5.0f);
	CollisionComponent->BodyInstance.SetCollisionProfileName(TEXT("Projectile"));
	CollisionComponent->OnComponentHit.AddDynamic(this, &ADecalProjectile::OnHit);
	CollisionComponent->SetWalkableSlopeOverride(FWalkableSlopeOverride(WalkableSlope_Unwalkable, 0.f));
	CollisionComponent->CanCharacterStepUpOn = ECB_No;

	// Set as root component
	RootComponent = CollisionComponent;

	// Create and attach mesh component
	ProjectileMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Projectile Mesh"));
	if (ProjectileMesh)
	{
		ProjectileMesh->SetupAttachment(RootComponent);
		ProjectileMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	// Create movement component
	ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("Projectile Movement"));
	ProjectileMovement->UpdatedComponent = CollisionComponent;
	ProjectileMovement->InitialSpeed = 3000.f;
	ProjectileMovement->bRotationFollowsVelocity = true;
	ProjectileMovement->bShouldBounce = true;

	InitialLifeSpan = 3.0f;
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

void ADecalProjectile::OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	if ((OtherActor != nullptr) && (OtherActor != this) && (OtherComp != nullptr) && OtherComp->IsSimulatingPhysics()) {
		OtherComp->AddImpulseAtLocation(GetVelocity() * 100.0f, GetActorLocation());
		Destroy();
	}
	if (OtherActor != nullptr) {
		// Spawn Niagara impact system using the projectile's RandomColor so visuals match.
		if (colorP)
		{
			// Compute orientation relative to surface and incoming direction
			const FVector SurfaceNormal = Hit.Normal.GetSafeNormal();
			FVector IncomingVelocity = ProjectileMovement ? ProjectileMovement->Velocity : GetVelocity();
			// Project incoming velocity onto the surface plane to get tangential direction
			FVector TangentDir = (IncomingVelocity - FVector::DotProduct(IncomingVelocity, SurfaceNormal) * SurfaceNormal).GetSafeNormal();
			if (TangentDir.IsNearlyZero())
			{
				// Fallback: build any tangent orthogonal to the normal
				TangentDir = FVector::CrossProduct(SurfaceNormal, FVector::UpVector);
				if (TangentDir.IsNearlyZero())
				{
					TangentDir = FVector::CrossProduct(SurfaceNormal, FVector::RightVector);
				}
				TangentDir.Normalize();
			}

			// Build rotation: X (forward) along Tangent, Z (up) along Normal, plus a small random spin
			const FRotator BasisRot = FRotationMatrix::MakeFromXZ(TangentDir, SurfaceNormal).Rotator();
			const float RandSpin = UKismetMathLibrary::RandomFloatInRange(-20.f, 20.f);
			const FQuat SpinQuat(SurfaceNormal, FMath::DegreesToRadians(RandSpin));
			const FRotator ImpactRotation = (SpinQuat * BasisRot.Quaternion()).Rotator();

			UNiagaraComponent* ParticleComp = UNiagaraFunctionLibrary::SpawnSystemAttached(
				colorP,
				HitComp,
				NAME_None,
				FVector::ZeroVector,
				ImpactRotation,
				EAttachLocation::KeepRelativeOffset,
				true,   // bAutoDestroy
				false,  // bAutoActivate (set params first)
				ENCPoolMethod::None,
				true    // bPreCullCheck
			);
			if (ParticleComp)
			{
				// Set direction and color so Niagara can drive initial velocity and tint
				ParticleComp->SetNiagaraVariableVec3(TEXT("User.ImpactNormal"), SurfaceNormal);
				ParticleComp->SetNiagaraVariableVec3(TEXT("User.ImpactDirection"), TangentDir);
				ParticleComp->SetNiagaraVariableFloat(TEXT("User.ImpactSpeed"), IncomingVelocity.Size());
				ParticleComp->SetNiagaraVariableLinearColor(TEXT("User.ParticleColor"), RandomColor);
				ParticleComp->SetNiagaraVariableLinearColor(TEXT("User.RandomColor"), RandomColor);
				ParticleComp->Activate(true);
			}

			// Update projectile material again (optional if it was destroyed later)
			if (ProjectileMaterialInstance)
			{
				ProjectileMaterialInstance->SetVectorParameterValue(FName("Color"), RandomColor);
			}
		}

		// Random frame selection (keep decal frame variety) but use existing RandomColor for decal
		float frameNum = UKismetMathLibrary::RandomFloatInRange(0.0f, 3.0f);

		if (BaseDecalMaterial)
		{
			FVector SafeNormal = Hit.Normal.GetSafeNormal();
			if (!SafeNormal.IsNearlyZero())
			{
				float RandomAngleDeg = UKismetMathLibrary::RandomFloatInRange(0.f, 360.f);
				FQuat SurfaceQuat = SafeNormal.ToOrientationQuat();
				FQuat SpinQuat(SafeNormal, FMath::DegreesToRadians(RandomAngleDeg));
				FRotator DecalRotation = (SpinQuat * SurfaceQuat).Rotator();

				if (UDecalComponent* Decal = UGameplayStatics::SpawnDecalAtLocation(GetWorld(), BaseDecalMaterial, FVector(UKismetMathLibrary::RandomFloatInRange(20.0f, 40.f)), Hit.Location, DecalRotation, 0.f))
				{
					if (UMaterialInstanceDynamic* MatInstance = Decal->CreateDynamicMaterialInstance())
					{
						MatInstance->SetVectorParameterValue("Color", RandomColor);
						MatInstance->SetScalarParameterValue("Frame", frameNum);
						if (DecalTextures.Num() > 0)
						{
							int32 RandomIndex = UKismetMathLibrary::RandomIntegerInRange(0, DecalTextures.Num() - 1);
							if (UTexture* RandomTexture = DecalTextures[RandomIndex])
							{
								MatInstance->SetTextureParameterValue(FName("Decal"), RandomTexture);
							}
						}
					}
				}
			}
		}
	}
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

	// Random spin around surface normal
	float RandomAngleDeg = UKismetMathLibrary::RandomFloatInRange(0.f, 360.f);
	FQuat SurfaceQuat = HitNormal.GetSafeNormal().ToOrientationQuat();
	FQuat SpinQuat(HitNormal.GetSafeNormal(), FMath::DegreesToRadians(RandomAngleDeg));
	FRotator DecalRotation = (SpinQuat * SurfaceQuat).Rotator();

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Green,
			FString::Printf(TEXT("Spawning Decal at: %s with rotation: %s (Angle %.1f)"),
				*DecalLocation.ToString(),
				*DecalRotation.ToString(), RandomAngleDeg));
	}

	// Spawn decal component
	UDecalComponent* DecalComponent = UGameplayStatics::SpawnDecalAtLocation(
		GetWorld(),
		BaseDecalMaterial,
		DecalSize,
		DecalLocation,
		DecalRotation,
		0.0f // LifeSpan; 0 = infinite
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
