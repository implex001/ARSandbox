// Fill out your copyright notice in the Description page of Project Settings.


#include "FireSimComponent.h"

// Sets default values for this component's properties
UFireSimComponent::UFireSimComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	// ...
}


// Called when the game starts
void UFireSimComponent::BeginPlay()
{
	Super::BeginPlay();
	PhiGrid = FGrid<double>(SimulationSize.X, SimulationSize.X);
	TempPhiGrid = FGrid<double>(SimulationSize.X, SimulationSize.X);
}


// Called every frame
void UFireSimComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
}

void UFireSimComponent::InitDistanceGrid(FGrid<double>& Grid)
{
	int error = INT_MAX;
	while (error > SteadyThreshold)
	{
		error = 0;
		TempPhiGrid = Grid;
		for (int i = 0; i < Grid.Rows(); i++)
		{
			for (int j = 0; j < Grid[i].Num(); j++)
			{
				if (Grid[i][j] < 0 || InterfaceLocations.Contains(FVector2D(i,j)))
					continue;

				double difference = CellInitStep(TempPhiGrid, FIntVector2(i, j));
				error += difference;
				Grid[i][j] += difference;
			}
		}
	}
}

void UFireSimComponent::TimeStep(FGrid<double>& Grid)
{
	TempPhiGrid = Grid;
	for (int i = 0; i < Grid.Rows(); i++)
	{
		for (int j = 0; j < Grid[i].Num(); j++)
		{
			Grid[i][j] = CellTimeStep(TempPhiGrid, FIntVector2(i, j), 1);
		}
	}
}

double UFireSimComponent::CellInitStep(FGrid<double>& Grid, FIntVector2 Coordinate)
{
	return 1 - MagnitudePhi(Grid, Coordinate, 2);
}

double UFireSimComponent::CellTimeStep(FGrid<double>& Grid, FIntVector2 Coordinate, double StepSize)
{
	return RunEulerMethod(Grid, Coordinate, StepSize);
}

double UFireSimComponent::ChangeInPhi(FGrid<double>& Grid, FIntVector2 Coordinate)
{
	return -(Simulation->GetSpeed(Grid, Coordinate, NormalizePhi(Grid, Coordinate)) * MagnitudePhi(Grid, Coordinate, 2));
}

double UFireSimComponent::CalculateWind(FGrid<double>& Grid, FIntVector2 Coordinate)
{
	FVector2d normal = NormalizePhi(Grid, Coordinate);
	double product =  FVector2D::DotProduct(normal, WindVector);
	if (product <= 0)
		return 0;
	return product;
}

double UFireSimComponent::RunEulerMethod(FGrid<double>& Grid, FIntVector2 Coordinate, double StepSize)
{
	return ChangeInPhi(Grid, Coordinate) * StepSize + Grid[Coordinate[0]][Coordinate[1]];
}

FVector2d UFireSimComponent::GradientPhi(FGrid<double>& Grid, FIntVector2 Coordinate, double Unit)
{
	BoundsProbe probe = ProbeBounds(Grid, Coordinate);

	double grid_x = (probe.grid_x_plus - probe.grid_x_minus) / (2 * Unit);
	double grid_y = (probe.grid_y_plus - probe.grid_y_minus) / (2 * Unit);
	return FVector2D(grid_x, grid_y);
}

double UFireSimComponent::MagnitudePhi(FGrid<double>& Grid, FIntVector2 Coordinate, double Unit)
{
	int x = Coordinate.X;
	int y = Coordinate.Y;
	BoundsProbe probe = ProbeBounds(Grid, Coordinate);
	double grid_x1 = (Grid[x][y] - probe.grid_x_minus) / Unit;
	double grid_x2 = (Grid[x][y] - probe.grid_x_plus) / Unit;
	double grid_y1 = (Grid[x][y] - probe.grid_y_minus) / Unit;
	double grid_y2 = (Grid[x][y] - probe.grid_y_plus) / Unit;

	double dx = FMath::Max3(grid_x1, grid_x2, 0.0);
	double dy = FMath::Max3(grid_y1, grid_y2, 0.0);
	return FMath::Sqrt(dx * dx + dy * dy);
}

FVector2d UFireSimComponent::NormalizePhi(FGrid<double>& Grid, FIntVector2 Coordinate, double Unit)
{
	FVector2D gradient = GradientPhi(Grid, Coordinate, Unit);
	double magnitude = MagnitudePhi(Grid, Coordinate, Unit);
	if (magnitude == 0)
	{
		return FVector2D(0, 0);
	}
	return FVector2D(gradient.X / magnitude, gradient.Y / magnitude);
}

BoundsProbe UFireSimComponent::ProbeBounds(FGrid<double>& Grid, FIntVector2 Coordinate)
{
	int x = Coordinate.X;
	int y = Coordinate.Y;
	BoundsProbe probe;
	
	if (x == 0)
	{
		probe.grid_x_minus = Grid[x][y];
	} else
	{
		probe.grid_x_minus = Grid[x - 1][y];
	}

	if (x == SimulationSize[0])
	{
		probe.grid_x_plus = Grid[x][y];
	}
	else
	{
		probe.grid_x_plus = Grid[x + 1][y];
	}

	if (y == 0)
	{
		probe.grid_y_minus = Grid[x][y];
	}
	else
	{
		probe.grid_y_minus = Grid[x][y - 1];
	}

	if (y == SimulationSize[1])
	{
		probe.grid_y_plus = Grid[x][y];
	}
	else
	{
		probe.grid_y_plus = Grid[x][y + 1];
	}
	
	return probe;
}

