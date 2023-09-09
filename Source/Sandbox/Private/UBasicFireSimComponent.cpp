#include "UBasicFireSimComponent.h"

UBasicFireSimComponent::UBasicFireSimComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UBasicFireSimComponent::BeginPlay()
{
	Super::BeginPlay();
}

void UBasicFireSimComponent::TickComponent(float DeltaTime, ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}

double UBasicFireSimComponent::GetSpeed(FGrid<double>& Grid, FIntVector2 Coordinate, FVector2d AdvectNormalVector)
{
	return 1.0 + CalculateWind(AdvectNormalVector);
}

double UBasicFireSimComponent::CalculateWind(FVector2D normal)
{
	double product =  FVector2D::DotProduct(normal, WindVector);
	if (product <= 0)
		return 0;
	return product;
}