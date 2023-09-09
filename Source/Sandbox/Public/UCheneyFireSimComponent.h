#pragma once

#include "CoreMinimal.h"
#include "UBasicFireSimComponent.h"
#include "FGrid.h"
#include "UCheneyFireSimComponent.Generated.h"

UENUM()
enum EChenySubclass
{
	Eaten = 1,
	Cut = 2,
	Natural = 3,
	Woodland = 4,
	OpenForest = 5
};

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class SANDBOX_API UCheneyFireSimComponent : public UBasicFireSimComponent
{

	GENERATED_BODY()
	
public:

	UCheneyFireSimComponent();
	
protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	
	UPROPERTY(BlueprintReadWrite)
	TArray<double> CuringGrid;

	UPROPERTY(BlueprintReadWrite)
	TArray<int> SubclassGrid;

	UPROPERTY(BlueprintReadWrite)
	TArray<int> DepthGrid;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	double temperature;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	double rel_hum;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	UTexture2D* DepthTexture;
	
	virtual double GetSpeed(FGrid<double>& Grid, FIntVector2 Coordinate, FVector2d AdvectNormalVector) override;

	UFUNCTION(BlueprintCallable)
	void DefaultSetUp(FVector2D wind, EChenySubclass Subclass, double Temp, double Humidity, double curing, FVector2D size);

private:
	FIntVector2 SimSize;
	
	FVector2D CalculateGradient(TArray<int>& Grid, FIntVector2 Coordinate, FIntVector2 Bounds);
};
