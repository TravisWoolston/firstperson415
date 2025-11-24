// Fill out your copyright notice in the Description page of Project Settings.


#include "ProcMeshFromStatic.h"
#include "KismetProceduralMeshLibrary.h"
// Sets default values
AProcMeshFromStatic::AProcMeshFromStatic()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;
	ProcMesh = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("GeneratedMesh"));
	BaseMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BaseMesh"));

	RootComponent = ProcMesh;
	BaseMesh->SetupAttachment(ProcMesh);

}

// Called when the game starts or when spawned
void AProcMeshFromStatic::BeginPlay()
{
	Super::BeginPlay();

}

void AProcMeshFromStatic::PostActorCreated() {
	Super::PostActorCreated();
	GetMeshData();
	if (PlaneMat) {
		ProcMesh->SetMaterial(0, PlaneMat);
	}
}

void AProcMeshFromStatic::PostLoad() {
	Super::PostLoad();
	GetMeshData();
	if (PlaneMat) {
		ProcMesh->SetMaterial(0, PlaneMat);
	}
}

// Called every frame
void AProcMeshFromStatic::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}
void AProcMeshFromStatic::CreateMesh() {
	ProcMesh->CreateMeshSection(0, Vertices, Triangles, Normals, UV0, UpVertexColors, Tangents, true);
}
void AProcMeshFromStatic::GetMeshData() {
	UStaticMesh* mesh = BaseMesh->GetStaticMesh();
	if (mesh) {
		UKismetProceduralMeshLibrary::GetSectionFromStaticMesh(mesh, 0, 0, Vertices, Triangles, Normals, UV0, Tangents);
		ProcMesh->UpdateMeshSection(0, Vertices, Normals, UV0, UpVertexColors, Tangents);
		CreateMesh();
	}
}