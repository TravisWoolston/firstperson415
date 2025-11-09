// Fill out your copyright notice in the Description page of Project Settings.


#include "CubeDMIMod.h"
#include "firstperson415Character.h"
#include "Kismet/KismetMathLibrary.h"

// Sets default values
ACubeDMIMod::ACubeDMIMod()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	boxComp = CreateDefaultSubobject<UBoxComponent>(TEXT("Box Component"));
	cubeMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Cube Mesh"));

	RootComponent = boxComp;
	cubeMesh->SetupAttachment(RootComponent);

}

// Called when the game starts or when spawned
void ACubeDMIMod::BeginPlay()
{
	Super::BeginPlay();

	boxComp->OnComponentBeginOverlap.AddDynamic(this, &ACubeDMIMod::OnOverlapBegin);
	if (baseMat)
	{
		dmiMat = UMaterialInstanceDynamic::Create(baseMat, this);
		//if (dmiMat)
		//{
		//	cubeMesh->SetMaterial(0, dmiMat);
		//}
	}
	if (cubeMesh) {
		cubeMesh->SetMaterial(0, dmiMat);
	}
}

// Called every frame
void ACubeDMIMod::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void ACubeDMIMod::OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	ACharacter* overlappedActor = Cast<ACharacter>(OtherActor);

	if (overlappedActor) {


		float ranNumx = UKismetMathLibrary::RandomFloatInRange(0.0f, 1.0f);
		float ranNumy = UKismetMathLibrary::RandomFloatInRange(0.0f, 1.0f);
		float ranNumz = UKismetMathLibrary::RandomFloatInRange(0.0f, 1.0f);
		FVector4 randColor = FVector4(ranNumx, ranNumy, ranNumz, 1.0f);

		if (dmiMat)
		{
			dmiMat->SetVectorParameterValue("Color", randColor);
			dmiMat->SetScalarParameterValue("Darkness", ranNumx);
		}
	}
}
