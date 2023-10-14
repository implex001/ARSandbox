// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "FGrid.h"
#include "UBasicFireSimComponent.h"

#include "FireSimComponent.generated.h"

template<class T>
struct SANDBOX_API BoundsProbe
{
	T grid_x_minus;
	T grid_x_plus;
	T grid_y_minus;
	T grid_y_plus;
};

UENUM()
enum class ETextureDisplayType : uint8
{
	Fire,
	DistanceField
};

UENUM()
enum class EFireState : uint8
{
	Unburned,
	Burning,
	Burned
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

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	ETextureDisplayType DisplayType;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	int BurnTime;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	bool AdaptiveReinitialize;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	int AdaptiveErrorTolerance = 1;

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
	const float SteadyThreshold = 0.005f;
	FGrid<double> PhiGrid;
	FGrid<double> TempPhiGrid;

	FGrid<EFireState> FireStateGrid;
	FGrid<int> BurnTimeGrid;

	int Time;

	int AdaptiveErrorCount = 0;

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

	template<class T>
	static BoundsProbe<T> ProbeBounds(FGrid<T>& Grid, FIntVector2 Coordinate);
};


