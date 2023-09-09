// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "FGrid.h"
#include "Components/ActorComponent.h"
#include "UBasicFireSimComponent.Generated.h"

/**
 * 
 */
UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class SANDBOX_API UBasicFireSimComponent : public UActorComponent
{

	GENERATED_BODY()
	
public:

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FVector2D WindVector;
	
	UBasicFireSimComponent();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	virtual double GetSpeed(FGrid<double>& Grid, FIntVector2 Coordinate, FVector2d AdvectNormalVector);
private:
	double CalculateWind(FVector2D normal);
};