// Fill out your copyright notice in the Description page of Project Settings.


#include "Portal.h"
#include "Kismet/GameplayStatics.h"
#include "firstperson415Character.h"

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

	Mesh->SetCollisionResponseToAllChannels(ECR_Ignore);
}

// Called when the game starts or when spawned
void APortal::BeginPlay()
{
	Super::BeginPlay();

	// Bind the overlap event
	if (BoxComp)
	{
		BoxComp->OnComponentBeginOverlap.AddDynamic(this, &APortal::OnOverlapBegin);
		Mesh->SetHiddenInSceneCapture(true);

		Mesh->CastShadow = false;
	}

	// Set up material for portal if available
	if (Mesh && Mat)
	{
		Mesh->SetMaterial(0, Mat);
	}
}

// Called every frame
void APortal::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Update portal view every frame
	UpdatePortals();

}
void APortal::OnOverlapBegin(class UPrimitiveComponent* OverlappedComp, class AActor* OtherActor,
	class UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	// Check if the overlapping actor is the player characte
	Afirstperson415Character* PlayerChar = Cast<Afirstperson415Character>(OtherActor);
	if (PlayerChar) {
		if (OtherPortal) {
			if (!PlayerChar->IsTeleporting) {
				PlayerChar->IsTeleporting = true;
				FVector loc = OtherPortal->RootArrow->GetComponentLocation();
				PlayerChar->SetActorLocation(loc);

				FTimerHandle TimerHandle;
				FTimerDelegate TimerDelegate;
				TimerDelegate.BindUFunction(this, "SetBool", PlayerChar);
				GetWorld()->GetTimerManager().SetTimer(TimerHandle, TimerDelegate, 1, false);
			}
		}
	}
}
void APortal::SetBool(Afirstperson415Character* PlayerChar)
{
	if (PlayerChar)
	{
		PlayerChar->IsTeleporting = false;
	}
}
void APortal::UpdatePortals()
{
	FVector Location = this->GetActorLocation() - OtherPortal->GetActorLocation();
	FVector camLocation = UGameplayStatics::GetPlayerCameraManager(GetWorld(), 0)->GetTransformComponent()->GetComponentLocation();
	FRotator camRotation = UGameplayStatics::GetPlayerCameraManager(GetWorld(), 0)->GetTransformComponent()->GetComponentRotation();
	FVector CombinedLocation = camLocation + Location;

	SceneCapture->SetWorldLocationAndRotation(CombinedLocation, camRotation);
}
