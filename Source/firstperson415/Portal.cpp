// Fill out your copyright notice in the Description page of Project Settings.


#include "Portal.h"
#include "Kismet/GameplayStatics.h"
#include "firstperson415Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Components/CapsuleComponent.h"
#include "Engine/World.h"

// Sets default values
APortal::APortal()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Portal Mesh"));
	BoxComp = CreateDefaultSubobject<UBoxComponent>(TEXT("Box Component"));
	SceneCapture = CreateDefaultSubobject<USceneCaptureComponent2D>(TEXT("Scene Capture Component"));
	RootArrow = CreateDefaultSubobject<UArrowComponent>(TEXT("Root Arrow"));

	RootComponent = BoxComp;
	Mesh->SetupAttachment(BoxComp);
	SceneCapture->SetupAttachment(Mesh);
	RootArrow->SetupAttachment(RootComponent);

	// Configure portal mesh to not block anything
	Mesh->SetCollisionResponseToAllChannels(ECR_Ignore);

	// Configure box component for overlap-only detection
	if (BoxComp)
	{
		BoxComp->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		BoxComp->SetCollisionObjectType(ECollisionChannel::ECC_WorldDynamic);
		BoxComp->SetCollisionResponseToAllChannels(ECR_Overlap);
		BoxComp->SetGenerateOverlapEvents(true);
	}

	// Configure scene capture defaults
	if (SceneCapture)
	{
		SceneCapture->bCaptureEveryFrame = true;
		SceneCapture->bCaptureOnMovement = true;
		SceneCapture->LODDistanceFactor = 3.0f;
		SceneCapture->bUseCustomProjectionMatrix = false;
	}
}

// Called when the game starts or when spawned
void APortal::BeginPlay()
{
	Super::BeginPlay();

	// Bind the overlap event
	if (BoxComp)
	{
		BoxComp->OnComponentBeginOverlap.AddDynamic(this, &APortal::OnOverlapBegin);
	}

	if (Mesh)
	{
		Mesh->SetHiddenInSceneCapture(true);
		Mesh->CastShadow = false;

		// Set up material for portal if available
		if (Mat)
		{
			Mesh->SetMaterial(0, Mat);
		}
	}

	// Configure scene capture for gameplay
	if (SceneCapture && RenderTarget)
	{
		SceneCapture->TextureTarget = RenderTarget;
		SceneCapture->CaptureSource = ESceneCaptureSource::SCS_SceneColorHDR;
		SceneCapture->bCaptureEveryFrame = true;
		SceneCapture->bCaptureOnMovement = true;

		// Hide other portal's mesh from this capture
		if (OtherPortal && OtherPortal->Mesh)
		{
			SceneCapture->HiddenActors.Add(OtherPortal);
		}

		UE_LOG(LogTemp, Log, TEXT("Portal %s: Render target assigned: %s"),
			*GetName(), RenderTarget ? *RenderTarget->GetName() : TEXT("NULL"));
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Portal %s: Missing SceneCapture or RenderTarget!"), *GetName());
	}
}

// Called every frame
void APortal::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Update portal view every frame
	UpdatePortals();

	// Clean up old teleport records
	CleanupTeleportRecords();
}

void APortal::OnOverlapBegin(class UPrimitiveComponent* OverlappedComp, class AActor* OtherActor,
	class UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (!OtherActor || !OtherPortal || OtherActor == this || OtherActor == OtherPortal)
	{
		return;
	}

	// Debug log to see what's overlapping
	UE_LOG(LogTemp, Log, TEXT("Portal overlap: %s"), *OtherActor->GetName());

	// Check if actor recently teleported
	float* LastTeleportTime = TeleportedActors.Find(OtherActor);
	if (LastTeleportTime && (GetWorld()->GetTimeSeconds() - *LastTeleportTime) < TeleportCooldown)
	{
		UE_LOG(LogTemp, Log, TEXT("Actor %s is in cooldown"), *OtherActor->GetName());
		return;
	}

	// Check if actor is actually crossing the portal plane
	FVector RelativePos;
	if (!IsActorCrossingPortal(OtherActor, RelativePos))
	{
		UE_LOG(LogTemp, Log, TEXT("Actor %s not crossing portal plane (X: %.2f)"), *OtherActor->GetName(), RelativePos.X);
		return;
	}

	// Handle character teleportation
	Afirstperson415Character* PlayerChar = Cast<Afirstperson415Character>(OtherActor);
	if (PlayerChar)
	{
		if (!PlayerChar->IsTeleporting)
		{
			UE_LOG(LogTemp, Log, TEXT("Teleporting player character"));
			TeleportActor(PlayerChar);

			// Use shorter timer for smoother re-teleportation
			FTimerHandle TimerHandle;
			FTimerDelegate TimerDelegate;
			TimerDelegate.BindUFunction(this, "SetBool", PlayerChar);
			GetWorld()->GetTimerManager().SetTimer(TimerHandle, TimerDelegate, TeleportCooldown, false);
		}
	}
	else
	{
		// Handle all other actors (projectiles, physics objects, etc.)
		UE_LOG(LogTemp, Log, TEXT("Teleporting non-character: %s"), *OtherActor->GetName());
		TeleportActor(OtherActor);
	}
}

void APortal::SetBool(Afirstperson415Character* PlayerChar)
{
	if (PlayerChar)
	{
		PlayerChar->IsTeleporting = false;
	}
}

void APortal::TeleportActor(AActor* ActorToTeleport)
{
	if (!ActorToTeleport || !OtherPortal)
	{
		return;
	}

	// Mark character as teleporting if applicable
	Afirstperson415Character* PlayerChar = Cast<Afirstperson415Character>(ActorToTeleport);
	if (PlayerChar)
	{
		PlayerChar->IsTeleporting = true;
	}

	// Get actor's current transform
	FVector ActorLocation = ActorToTeleport->GetActorLocation();
	FRotator ActorRotation = ActorToTeleport->GetActorRotation();
	FVector ActorVelocity = ActorToTeleport->GetVelocity();

	// Calculate relative transform from this portal
	FTransform ThisPortalTransform = GetActorTransform();
	FTransform ActorTransform(ActorRotation, ActorLocation);
	FTransform RelativeTransform = ActorTransform.GetRelativeTransform(ThisPortalTransform);

	// Mirror the relative location through portal
	FVector RelativeLocation = RelativeTransform.GetLocation();
	RelativeLocation.X = -RelativeLocation.X; // Flip through portal

	// Mirror the relative rotation
	FRotator RelativeRotation = RelativeTransform.Rotator();
	RelativeRotation.Yaw += 180.0f;
	RelativeRotation.Pitch = -RelativeRotation.Pitch;
	RelativeRotation.Roll = -RelativeRotation.Roll;

	// Calculate new world transform at other portal
	FTransform OtherPortalTransform = OtherPortal->GetActorTransform();
	FTransform MirroredTransform(RelativeRotation, RelativeLocation);
	FTransform NewWorldTransform = MirroredTransform * OtherPortalTransform;

	// Prevent spawning under floor
	FVector TargetLocation = NewWorldTransform.GetLocation();
	FHitResult Hit;
	FVector Start = TargetLocation + FVector(0, 0, 200.0f);
	FVector End = TargetLocation - FVector(0, 0, 1000.0f);
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(ActorToTeleport);
	Params.AddIgnoredActor(this);
	Params.AddIgnoredActor(OtherPortal);

	// Adjust collision channel if needed (e.g. ECC_Visibility or ECC_WorldStatic)
	if (GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_WorldStatic, Params))
	{
		float CapsuleHalfHeight = 0.0f;
		ACharacter* Char = Cast<ACharacter>(ActorToTeleport);
		if (Char)
		{
			CapsuleHalfHeight = Char->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
		}
		else
		{
			// Estimate for non-character
			FVector Origin, BoxExtent;
			ActorToTeleport->GetActorBounds(true, Origin, BoxExtent);
			CapsuleHalfHeight = BoxExtent.Z;
		}

		TargetLocation.Z = Hit.Location.Z + CapsuleHalfHeight + 5.0f;
		NewWorldTransform.SetLocation(TargetLocation);
	}

	// Transform velocity through portal
	FVector RelativeVelocity = ThisPortalTransform.InverseTransformVector(ActorVelocity);
	RelativeVelocity.X = -RelativeVelocity.X; // Mirror velocity
	FVector NewVelocity = OtherPortalTransform.TransformVector(RelativeVelocity);

	// Teleport the actor - use Rotator() method, not GetRotator()
	ActorToTeleport->SetActorLocationAndRotation(
		NewWorldTransform.GetLocation(),
		NewWorldTransform.Rotator(),
		false,
		nullptr,
		ETeleportType::TeleportPhysics
	);

	// Apply velocity based on actor type
	UCharacterMovementComponent* CharMovement = ActorToTeleport->FindComponentByClass<UCharacterMovementComponent>();
	if (CharMovement)
	{
		CharMovement->Velocity = NewVelocity;
	}
	else
	{
		UProjectileMovementComponent* ProjectileMovement = ActorToTeleport->FindComponentByClass<UProjectileMovementComponent>();
		if (ProjectileMovement)
		{
			ProjectileMovement->Velocity = NewVelocity;
		}
		else
		{
			// For physics objects
			UPrimitiveComponent* PrimComp = Cast<UPrimitiveComponent>(ActorToTeleport->GetRootComponent());
			if (PrimComp && PrimComp->IsSimulatingPhysics())
			{
				PrimComp->SetPhysicsLinearVelocity(NewVelocity);
			}
		}
	}

	// Record teleport time
	TeleportedActors.Add(ActorToTeleport, GetWorld()->GetTimeSeconds());

	UE_LOG(LogTemp, Log, TEXT("Teleported %s through portal"), *ActorToTeleport->GetName());
}

bool APortal::IsActorCrossingPortal(AActor* Actor, FVector& OutRelativePosition) const
{
	if (!Actor)
	{
		return false;
	}

	// Get actor's position relative to this portal
	FVector ActorLocation = Actor->GetActorLocation();
	FTransform PortalTransform = GetActorTransform();
	FVector RelativeLocation = PortalTransform.InverseTransformPosition(ActorLocation);

	OutRelativePosition = RelativeLocation;

	// Actor is crossing if it's near or past the portal plane (with threshold for fast-moving objects)
	// Allow entry from both sides (Front and Back) by ignoring direction check
	return true; // RelativeLocation.X > CrossingThreshold;
}

void APortal::CleanupTeleportRecords()
{
	if (TeleportedActors.Num() == 0)
	{
		return;
	}

	float CurrentTime = GetWorld()->GetTimeSeconds();
	TArray<AActor*> ActorsToRemove;

	for (auto& Pair : TeleportedActors)
	{
		// Remove records older than cooldown period
		if (CurrentTime - Pair.Value > TeleportCooldown)
		{
			ActorsToRemove.Add(Pair.Key);
		}
		// Also remove invalid actors
		else if (!IsValid(Pair.Key))
		{
			ActorsToRemove.Add(Pair.Key);
		}
	}

	for (AActor* Actor : ActorsToRemove)
	{
		TeleportedActors.Remove(Actor);
	}
}

void APortal::UpdatePortals()
{
	// Prevent camera movement in Editor to preserve relative location
	if (GetWorld() && !GetWorld()->IsGameWorld())
	{
		return;
	}

	if (!OtherPortal || !SceneCapture)
	{
		return;
	}

	APlayerCameraManager* CameraManager = UGameplayStatics::GetPlayerCameraManager(GetWorld(), 0);
	if (!CameraManager)
	{
		return;
	}

	// Simple offset approach - works best for portal rendering
	FVector Location = this->GetActorLocation() - OtherPortal->GetActorLocation();
	FVector camLocation = CameraManager->GetCameraLocation();
	FRotator camRotation = CameraManager->GetCameraRotation();
	FVector CombinedLocation = camLocation + Location;

	SceneCapture->SetWorldLocationAndRotation(CombinedLocation, camRotation);

	// Update FOV to match player camera
	SceneCapture->FOVAngle = CameraManager->GetFOVAngle();
}
