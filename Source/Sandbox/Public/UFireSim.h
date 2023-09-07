// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "FGrid.h"
#include "UFireSim.Generated.h"

/**
 * 
 */
UCLASS()
class SANDBOX_API UFireSim : public UObject
{

	GENERATED_BODY()
	
public:
	virtual ~UFireSim() override;
	virtual double GetSpeed(FGrid<double>& Grid, FIntVector2 Coordinate, FVector2d AdvectNormalVector);
};


class SANDBOX_API UCheneyFireSim : UFireSim
{
public:
	FVector2d WindVector;
	FGrid<double> CuringGrid;
	FGrid<int> SubclassGrid;
	double temperature;
	double rel_hum;
	
	virtual double GetSpeed(FGrid<double>& Grid, FIntVector2 Coordinate, FVector2d AdvectNormalVector) override;
};