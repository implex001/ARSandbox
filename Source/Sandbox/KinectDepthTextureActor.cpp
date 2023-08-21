// Fill out your copyright notice in the Description page of Project Settings.


#include "KinectDepthTextureActor.h"


// Sets default values
AKinectDepthTextureActor::AKinectDepthTextureActor()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	PlaneMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Plane"));
	PlaneMesh->SetStaticMesh(ConstructorHelpers::FObjectFinder<UStaticMesh>(TEXT("StaticMesh'/Engine/BasicShapes/Plane.Plane'")).Object);
	PlaneMesh->SetMaterial(0, ConstructorHelpers::FObjectFinder<UMaterial>(TEXT("/Script/Engine.Material'/Game/Materials/DepthDisplayMaterial.DepthDisplayMaterial'")).Object);
	PlaneMesh->SetupAttachment(RootComponent);
}

// Called when the game starts or when spawned
void AKinectDepthTextureActor::BeginPlay()
{
	Super::BeginPlay();
	KinectDevice = NewObject<UKinectDevice>();
	KinectDevice->AddToRoot();
	KinectDevice->LoadDefault();

	// Thread = new KinectThread(KinectDevice);
	// Thread->Init();
}

// Called every frame
void AKinectDepthTextureActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	KinectDevice->Update();

	// set material to kinect depth texture
	UMaterialInstanceDynamic* DynamicMat = PlaneMesh->CreateDynamicMaterialInstance(0, PlaneMesh->GetMaterial(0));
	if (DynamicMat)
	{
		DynamicMat->SetTextureParameterValue("KinectDepthTexture", KinectDevice->DepthImage);
	}
}

void AKinectDepthTextureActor::EndPlay(EEndPlayReason::Type EndPlayReason)
{
	KinectDevice->Shutdown();
}

