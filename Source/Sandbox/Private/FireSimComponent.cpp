// Fill out your copyright notice in the Description page of Project Settings.


#include "FireSimComponent.h"

// Sets default values for this component's properties
UFireSimComponent::UFireSimComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;
	FireTexture = UTexture2D::CreateTransient(SimulationSize.X, SimulationSize.Y);
	// ...
}


// Called when the game starts
void UFireSimComponent::BeginPlay()
{
	Super::BeginPlay();
	FireTexture = UTexture2D::CreateTransient(SimulationSize.X, SimulationSize.Y);
	ResetSimulation();
}


// Called every frame
void UFireSimComponent::TickComponent(float DeltaTime, ELevelTick TickType,
                                      FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
}

void UFireSimComponent::StepSimulation()
{
	Time++;
	if (ReInitializeStep > 0 && Time % ReInitializeStep == 0)
	{
		InitDistanceGrid(PhiGrid);
	}
	TimeStep(PhiGrid);
	ConvertToTexture(PhiGrid);
}

void UFireSimComponent::RefreshGrid()
{
	InitDistanceGrid(PhiGrid);
	ConvertToTexture(PhiGrid);
}

void UFireSimComponent::ResetSimulation()
{
	PhiGrid = FGrid<double>(0, SimulationSize.X, SimulationSize.Y);
	TempPhiGrid = FGrid<double>(0, SimulationSize.X, SimulationSize.Y);
	FireStateGrid = FGrid<EFireState>(EFireState::Unburned, SimulationSize.X, SimulationSize.Y);
	BurnTimeGrid = FGrid<int>(BurnTime, SimulationSize.X, SimulationSize.Y);
	AdaptiveErrorCount = 0;
	InitDistanceGrid(PhiGrid);
	ConvertToTexture(PhiGrid);
}


void UFireSimComponent::InitDistanceGrid(FGrid<double>& Grid)
{
	Grid = FGrid<double>(FMath::Sqrt(SimulationSize.X * SimulationSize.Y), SimulationSize.X, SimulationSize.Y);
	for (auto Element : InterfaceLocations)
	{
		FireStateGrid.Get(Element.X, Element.Y) = EFireState::Burning;
		Grid.Get(Element.X, Element.Y) = -1;
	}

	for (int i = 0; i < Grid.Size.X; i++)
	{
		for (int j = 0; j < Grid.Size.Y; j++)
		{
			if (FireStateGrid.Get(i, j) != EFireState::Unburned)
			{
				Grid.Get(i,j) = -1;
			} 
		}
	}

	int error = INT_MAX;
	while (error > SteadyThreshold)
	{
		error = 0;
		TempPhiGrid = Grid;
		for (int i = 0; i < Grid.Size.X; i++)
		{
			for (int j = 0; j < Grid.Size.Y; j++)
			{
				if (FireStateGrid.Get(i, j) != EFireState::Unburned)
					continue;

				double difference = CellInitStep(TempPhiGrid, FIntVector2(i, j));
				error += difference;
				Grid.Get(i, j) += difference;
			}
		}
	}
}

void UFireSimComponent::TimeStep(FGrid<double>& Grid)
{
	TempPhiGrid = Grid;
	for (int i = 0; i < Grid.Size.X; i++)
	{
		for (int j = 0; j < Grid.Size.Y; j++)
		{
			double newDistance = CellTimeStep(TempPhiGrid, FIntVector2(i, j), SimStepSize);
			EFireState fireState = FireStateGrid.Get(i, j);
			Grid.Get(i, j) = newDistance;
			if (newDistance > 0 || fireState == EFireState::Burned)
			{
				continue;
			}

			if (fireState == EFireState::Burning)
			{
				BurnTimeGrid.Get(i, j) -= 1;
				if (BurnTimeGrid.Get(i, j) <= 0)
				{
					FireStateGrid.Get(i, j) = EFireState::Burned;
				}
				continue;
			}

			BoundsProbe probe = ProbeBounds(FireStateGrid, FIntVector2(i, j));
			
			if (probe.grid_x_minus == EFireState::Burning || probe.grid_x_plus == EFireState::Burning ||
				probe.grid_y_minus == EFireState::Burning || probe.grid_y_plus == EFireState::Burning)
			{
				FireStateGrid.Get(i, j) = EFireState::Burning;
				continue;
			}

			// Distance cell negative out of order, reinitialise distance field if above error count
			if (AdaptiveReinitialize && AdaptiveErrorCount >= AdaptiveErrorTolerance)
			{
				AdaptiveErrorCount = 0;
				Grid = TempPhiGrid;
				InitDistanceGrid(Grid);
				return;
			}

			AdaptiveErrorCount++;
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
	return -(Simulation->GetSpeed(Grid, Coordinate, NormalizePhi(Grid, Coordinate)) *
		MagnitudePhi(Grid, Coordinate, 2));
}

double UFireSimComponent::RunEulerMethod(FGrid<double>& Grid, FIntVector2 Coordinate, double StepSize)
{
	return ChangeInPhi(Grid, Coordinate) * StepSize + Grid[Coordinate];
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
	BoundsProbe probe = ProbeBounds(Grid, Coordinate);
	double grid_x1 = (Grid[Coordinate] - probe.grid_x_minus) / Unit;
	double grid_x2 = (Grid[Coordinate] - probe.grid_x_plus) / Unit;
	double grid_y1 = (Grid[Coordinate] - probe.grid_y_minus) / Unit;
	double grid_y2 = (Grid[Coordinate] - probe.grid_y_plus) / Unit;

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

template<class T>
BoundsProbe<T> UFireSimComponent::ProbeBounds(FGrid<T>& Grid, FIntVector2 Coordinate)
{
	int x = Coordinate.X;
	int y = Coordinate.Y;
	BoundsProbe<T> probe;

	if (x == 0)
	{
		probe.grid_x_minus = Grid[Coordinate];
	}
	else
	{
		probe.grid_x_minus = Grid.Get(x - 1, y);
	}

	if (x == Grid.GetSize()[0] - 1)
	{
		probe.grid_x_plus = Grid[Coordinate];
	}
	else
	{
		probe.grid_x_plus = Grid.Get(x + 1, y);
	}

	if (y == 0)
	{
		probe.grid_y_minus = Grid[Coordinate];
	}
	else
	{
		probe.grid_y_minus = Grid.Get(x, y - 1);
	}

	if (y == Grid.GetSize()[1] - 1)
	{
		probe.grid_y_plus = Grid[Coordinate];
	}
	else
	{
		probe.grid_y_plus = Grid.Get(x, y + 1);
	}

	return probe;
}

void UFireSimComponent::ConvertToTexture(FGrid<double>& Grid)
{
	const double max = FGenericPlatformMath::Max(Grid.Grid);
	const double min = FGenericPlatformMath::Min(Grid.Grid);
	FTexturePlatformData* platformData = FireTexture->GetPlatformData();
	FTexture2DMipMap* MipMap = &platformData->Mips[0];
	FByteBulkData* ImageData = &MipMap->BulkData;
	uint8* RawImageData = (uint8*)ImageData->Lock(LOCK_READ_WRITE);
	switch (DisplayType)
	{
	case ETextureDisplayType::Fire:
		for (int i = 0; i < Grid.Size.X * Grid.Size.Y; i++)
		{
			const int index = i * 4;
			const EFireState fire = FireStateGrid.Grid[i];

			switch (fire)
			{
			case EFireState::Burning:
				RawImageData[index] = 0;
				RawImageData[index + 1] = 0;
				RawImageData[index + 2] = 255;
				RawImageData[index + 3] = 255;
				break;
			case EFireState::Burned:
				RawImageData[index] = 0;
				RawImageData[index + 1] = 0;
				RawImageData[index + 2] = 0;
				RawImageData[index + 3] = 255;
				break;
			case EFireState::Unburned:
				RawImageData[index] = 50;
				RawImageData[index + 1] = 50;
				RawImageData[index + 2] = 50;
				RawImageData[index + 3] = 255;
				break;
			}
		}
		break;

	case ETextureDisplayType::DistanceField:
		for (int i = 0; i < Grid.Size.X * Grid.Size.Y; i++)
		{
			const int index = i * 4;
			const int fire = Grid.Grid[i];
			const int distance_norm = (FMath::Abs(fire) - 0) / (max - 0) * 255;
			const int distance_norm_min = (FMath::Abs(fire) - 0) / (FMath::Abs(min) - 0) * 255;
			if (fire <= 0)
			{
				RawImageData[index] = 0;
				RawImageData[index + 1] = 0;
				RawImageData[index + 2] = 255;
				RawImageData[index + 3] = 255;
			}
			else
			{
				RawImageData[index] = 0;
				RawImageData[index + 1] = distance_norm;
				RawImageData[index + 2] = 0;
				RawImageData[index + 3] = 255;
			}
		}
		break;
	}


	//release the lock
	ImageData->Unlock();
	FireTexture->UpdateResource();
}
