#include "UCheneyFireSimComponent.h"

#include "GeometryScript/TextureMapFunctions.h"

UCheneyFireSimComponent::UCheneyFireSimComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UCheneyFireSimComponent::BeginPlay()
{
	Super::BeginPlay();
}

void UCheneyFireSimComponent::TickComponent(float DeltaTime, ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (DepthTexture != nullptr)
	{
		for(int i = 0; i < SimSize.X; i++)
		{
			for(int j = 0; j < SimSize.Y; j++)
			{
				FGeometryScriptUVList UVList = FGeometryScriptUVList();
				UVList.List->Add(FVector2D(i, j));
				FGeometryScriptSampleTextureOptions Options = FGeometryScriptSampleTextureOptions();
				FGeometryScriptColorList ColorList = FGeometryScriptColorList();
				UGeometryScriptLibrary_TextureMapFunctions::SampleTexture2DAtUVPositions(UVList, DepthTexture, Options, ColorList);
				DepthGrid[i * SimSize.Y + j] = ColorList.List->operator[](0).A;
			}
		}
	}
}

void UCheneyFireSimComponent::DefaultSetUp(FVector2D wind, EChenySubclass Subclass, double Temp, double Humidity, double curing, FVector2D size)
{
	WindVector = wind;
	temperature = Temp;
	rel_hum = Humidity;

	CuringGrid.Init(curing, size.X * size.Y);
	SubclassGrid.Init(Subclass, size.X * size.Y);
	DepthGrid.Init(0, size.X * size.Y);

	SimSize = FIntVector2(size.X, size.Y);
}

double UCheneyFireSimComponent::GetSpeed(FGrid<double>& Grid, FIntVector2 Coordinate, FVector2d AdvectNormalVector)
{
	// // CSIRO grasslands models - Cheney et al. (1998)


	// Subclasses defined as the following sub type of grasslands:
	// 1 - Eaten out
	// 2 - Cut / grazed
	// 3 - Natural / undisturbed
	// 4 - Woodland
	// 5 - Open forest

	// -------------------------------------------
	// Model parameters
	// 1. Temperature, 'temp'  (input)
	// 2. Relative humidity, 'rel_hum' (input)
	// 3. Curing value, 'curing'
	// -------------------------------------------

	// Calculating the wind speed which is used to calculate head fire ROS
	float wind_speed = WindVector.Length();
	FVector2d NormalisedWindVector = WindVector;
	NormalisedWindVector.Normalize();

	// Calculating the normalised dot product between the wind vector and the normal to the fire perimeter
	float wdot = FVector2d::DotProduct(NormalisedWindVector, AdvectNormalVector);

	// Calculate length-to-breadth ratio (LBR) which varies with wind speed
	// Equations are curve fits adapted from Taylor (1997)
	float LBR = 1.0;
	if (wind_speed < 5)
	{
		LBR = 1.0;
	}
	else
	{
		LBR = 1.1 * pow(wind_speed, 0.464);
	}

	// Determine coefficient for backing and flanking rank of spread using elliptical equations
	// Where R_backing = cb * R_head, R_flanking = cf * R_head,
	float cc = sqrt(1.0 - pow(LBR, -2.0));
	float cb = (1.0 - cc) / (1.0 + cc);
	float a_LBR = 0.5 * (cb + 1.0);
	float cf = a_LBR / LBR;

	// Determine shape parameters 
	float f = 0.5 * (1.0 + cb);
	float g = 0.5 * (1.0 - cb);
	float h = cf;

	// Now calculate a speed coefficient using normal flow formula
	float speed_fraction = (g * wdot + sqrt(h * h + (f * f - h * h) * wdot * wdot));

	// Calculate curing coefficient from Cruz et al. (2015)
	float curing_coeff;
	float curing = CuringGrid[Coordinate.Y + Coordinate.X * Grid.Size.Y];
	if (curing < 20)
		curing_coeff = 0;
	else
		curing_coeff = 1.036 / (1 + 103.989 * exp(-0.0996 * (curing - 20)));

	// Fuel moisture content approximated using McArthur (1966)
	float GMf = 9.58 - (0.205 * temperature) + (0.138 * rel_hum);

	// Calculate moisture coefficient from Cheney et al. (1998)
	float moisture_coeff;
	if (GMf <= 12)
	{
		moisture_coeff = exp(-0.108 * GMf);
	}
	else if (wind_speed <= 10)
	{
		moisture_coeff = 0.684 - 0.0342 * GMf;
	}
	else
	{
		moisture_coeff = 0.547 - 0.0228 * GMf;
	}

	// Defining coefficients for the various grasslands sub-types using different subclasses.
	float head_speed;
	float CF_Backing_Slow;
	float CF_Backing_Fast;
	float CF_Wind_Slow;
	float CF_Wind_Fast;
	float speed_factor = 1;

	int subclass = SubclassGrid[Coordinate.Y + Coordinate.X * Grid.Size.Y];

	if (subclass == 1) // Eaten out
	{
		CF_Backing_Slow = 0.027;
		CF_Backing_Fast = 0.55;
		CF_Wind_Slow = 0.1045;
		CF_Wind_Fast = 0.3575;
	}
	else if (subclass == 2) // Cut or grazed 
	{
		CF_Backing_Slow = 0.054;
		CF_Backing_Fast = 1.1;
		CF_Wind_Slow = 0.209;
		CF_Wind_Fast = 0.715;
	}
	else if (subclass == 3) // Natural or undisturbed
	{
		CF_Backing_Slow = 0.054;
		CF_Backing_Fast = 1.4;
		CF_Wind_Slow = 0.269;
		CF_Wind_Fast = 0.838;
	}
	else if (subclass == 4) // Woodland  
	{
		CF_Backing_Slow = 0.054;
		CF_Backing_Fast = 1.4;
		CF_Wind_Slow = 0.269;
		CF_Wind_Fast = 0.838;
		speed_factor = 0.5;
	}
	else if (subclass == 5) // Open forest
	{
		CF_Backing_Slow = 0.054;
		CF_Backing_Fast = 1.4;
		CF_Wind_Slow = 0.269;
		CF_Wind_Fast = 0.838;
		speed_factor = 0.3;
	}

	// Calculate spread rate from Cheney et al. (1998) (converting spread rate to m/s from km/hr) 
	if (wind_speed >= 5.0)
		head_speed = (CF_Backing_Fast + CF_Wind_Fast * pow((wind_speed - 5), 0.844)) * moisture_coeff * curing_coeff /
			3.6;
	else
		head_speed = (CF_Backing_Slow + CF_Wind_Slow * wind_speed) * moisture_coeff * curing_coeff / 3.6;

	// Adjust speed based on canopy layer for Northern Australia grassland types (woodland and open forest) 
	// Based on Cheney and Sullivan (2008)
	head_speed = head_speed * speed_factor;

	// Adjust for calculated speed coefficient for fire flanks
	double speed = head_speed * speed_fraction;

	FVector2d gradient_elevation = CalculateGradient(DepthGrid, Coordinate, Grid.Size);
	double slope_in_normal = atan(FVector2d::DotProduct(AdvectNormalVector, gradient_elevation));
	slope_in_normal = FMath::Min(FMath::Max(slope_in_normal, -20), 20);
	double slope_cooefficient = FMath::Exp(0.069 * slope_in_normal);
	return speed * slope_cooefficient;
}

FVector2D UCheneyFireSimComponent::CalculateGradient(TArray<int>& Grid, FIntVector2 Coordinate, FIntVector2 Bounds)
{
	int x = Coordinate.X;
	int y = Coordinate.Y;

	double grid_x_minus;
	double grid_x_plus;
	double grid_y_minus;
	double grid_y_plus;
	
	if (x == 0)
	{
		grid_x_minus = Grid[x * Bounds.Y + y];
	} else
	{
		grid_x_minus = Grid[(x - 1) * Bounds.Y + y];
	}

	if (x == Bounds.X - 1)
	{
		grid_x_plus = Grid[x * Bounds.Y + y];
	}
	else
	{
		grid_x_plus = Grid[(x + 1) * Bounds.Y + y];
	}

	if (y == 0)
	{
		grid_y_minus = Grid[x * Bounds.Y + y];
	}
	else
	{
		grid_y_minus = Grid[x * Bounds.Y + (y - 1)];
	}

	if (y == Bounds.Y - 1)
	{
		grid_y_plus = Grid[x * Bounds.Y + y];
	}
	else
	{
		grid_y_plus = Grid[x * Bounds.Y + (y + 1)];
	}

	double grid_x = (grid_x_plus - grid_x_minus);
	double grid_y = (grid_y_plus - grid_y_minus);
	return FVector2D(grid_x, grid_y);
}
