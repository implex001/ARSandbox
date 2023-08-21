// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "RawKinectComponent.h"
#include "Components/DynamicMeshComponent.h"
#include "ProceduralMeshComponent.h"
#include "DynamicMesh/DynamicMesh3.h"
#include "KinectDepthLandscapeActor.generated.h"

UCLASS()
class SANDBOX_API AKinectDepthLandscapeActor : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AKinectDepthLandscapeActor();

	// UPROPERTY(BlueprintReadOnly)
	URawKinectComponent* RawKinectComponent;

	UPROPERTY(EditAnywhere)
	UDynamicMeshComponent* MeshComponent;

	UPROPERTY(BlueprintReadOnly)
	UProceduralMeshComponent* ProceduralMeshComponent;


protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;
	

};

