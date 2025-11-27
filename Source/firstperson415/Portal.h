// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Engine/TextureRenderTarget2D.h"
#include "firstperson415Character.h"
#include "Components/BoxComponent.h"
#include "Components/ArrowComponent.h"
#include "Portal.generated.h"

UCLASS()
class FIRSTPERSON415_API APortal : public AActor
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	APortal();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Portal Components
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Portal Components")
	UBoxComponent* BoxComp;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Portal Components")
	UStaticMeshComponent* Mesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Portal Components")
	USceneCaptureComponent2D* SceneCapture;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Portal Components")
	UArrowComponent* RootArrow;

	// Portal Settings
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Portal Settings")
	UTextureRenderTarget2D* RenderTarget;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Portal Settings")
	APortal* OtherPortal;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Portal Settings")
	UMaterialInterface* Mat;

	// Portal Functions
	UFUNCTION()
	void OnOverlapBegin(class UPrimitiveComponent* OverlappedComp, class AActor* OtherActor,
		class UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void SetBool(Afirstperson415Character* PlayerChar);

	UFUNCTION()
	void UpdatePortals();
};
