// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "FGrid.h"
#include "UBasicFireSimComponent.h"

#include "FireSimComponent.generated.h"

struct SANDBOX_API BoundsProbe
{
	double grid_x_minus;
	double grid_x_plus;
	double grid_y_minus;
	double grid_y_plus;
};

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class SANDBOX_API UFireSimComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UFireSimComponent();
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FVector2D SimulationSize;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FVector2D> InterfaceLocations;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UBasicFireSimComponent* Simulation;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float SimStepSize;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int ReInitializeStep;
	
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere)
	UTexture2D* FireTexture;

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UFUNCTION(BlueprintCallable)
	void StepSimulation();

	UFUNCTION(BlueprintCallable)
	void RefreshGrid();

	UFUNCTION(BlueprintCallable)
	void ResetSimulation();
private:
	const float SteadyThreshold = 0.01f;
	FGrid<double> PhiGrid;
	FGrid<double> TempPhiGrid;

	int Time;

	void InitDistanceGrid(FGrid<double>& Grid);
	void TimeStep(FGrid<double>& Grid);
	double CellInitStep(FGrid<double>& Grid, FIntVector2 Coordinate);
	double CellTimeStep(FGrid<double>& Grid, FIntVector2 Coordinate, double StepSize);
	double ChangeInPhi(FGrid<double>& Grid, FIntVector2 Coordinate);
	double RunEulerMethod(FGrid<double>& Grid, FIntVector2 Coordinate, double StepSize);

	void ConvertToTexture(FGrid<double>& Grid);
public:
	static FVector2d GradientPhi(FGrid<double>& Grid, FIntVector2 Coordinate, double Unit = 1);
	static double MagnitudePhi(FGrid<double>& Grid, FIntVector2 Coordinate, double Unit = 1);
	static FVector2d NormalizePhi(FGrid<double>& Grid, FIntVector2 Coordinate, double Unit = 1);
	static BoundsProbe ProbeBounds(FGrid<double>& Grid, FIntVector2 Coordinate);
};


