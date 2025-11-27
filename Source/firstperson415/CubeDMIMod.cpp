// Fill out your copyright notice in the Description page of Project Settings.


#include "CubeDMIMod.h"
#include "firstperson415Character.h"
#include "Kismet/KismetMathLibrary.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h"
#include "Portal.h"
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
		FLinearColor LinearRandColor(ranNumx, ranNumy, ranNumz, 1.0f);
		FVector4 randColor = FVector4(ranNumx, ranNumy, ranNumz, 1.0f);

		if (dmiMat)
		{
			dmiMat->SetVectorParameterValue("Color", randColor);
			dmiMat->SetScalarParameterValue("Darkness", ranNumx);
			// Correct Niagara spawn: use full signature to satisfy overload
			if (colorP && OtherComp)
			{
				UNiagaraComponent* particleComp = UNiagaraFunctionLibrary::SpawnSystemAttached(
					colorP,                      // System template
					OtherComp,                   // Attach to component
					NAME_None,                   // Attach point name
					FVector::ZeroVector,         // Location offset
					FRotator::ZeroRotator,       // Rotation offset
					EAttachLocation::KeepRelativeOffset,
					true,                        // bAutoDestroy
					true,                        // bAutoActivate
					ENCPoolMethod::None,         // Pooling method
					true                         // bPreCullCheck
				);
				if (particleComp)
					{
					// Set Niagara user parameters to match cube color (system must expose these parameters)
					particleComp->SetNiagaraVariableLinearColor(TEXT("RandomColor"), LinearRandColor);
					particleComp->SetNiagaraVariableLinearColor(TEXT("ParticleColor"), LinearRandColor);
				}
			}
		}
	}
}

