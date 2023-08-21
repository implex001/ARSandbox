// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "KinectDevice.h"
#include "KinectThread.h"
#include "KinectDepthTextureActor.generated.h"

UCLASS()
class SANDBOX_API AKinectDepthTextureActor : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AKinectDepthTextureActor();
	UPROPERTY(EditAnywhere)
	UKinectDevice* KinectDevice;

	// Editable Minimum and maximum depth distance in millimetres
	UPROPERTY(EditAnywhere)
	uint16 MinDepthDistance = 0;

	UPROPERTY(EditAnywhere)
	uint16 MaxDepthDistance = 500;


protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	
	virtual void EndPlay(EEndPlayReason::Type EndPlayReason) override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	KinectThread* Thread;

	UPROPERTY(EditAnywhere)
	UStaticMeshComponent* PlaneMesh;
	

};
