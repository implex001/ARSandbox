#include "UCheneyFireSimComponent.h"

#include "KinectDevice.h"

#if WITH_OPENCV
#include <vector>

#include "PreOpenCVHeaders.h"

#include "opencv2/calib3d.hpp"
#include "opencv2/imgproc.hpp"

#include "PostOpenCVHeaders.h"

#endif	// WITH_OPENCV

UCheneyFireSimComponent::UCheneyFireSimComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UCheneyFireSimComponent::BeginPlay()
{
	Super::BeginPlay();
}

void UCheneyFireSimComponent::TickComponent(float DeltaTime, ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (KinectDevice == nullptr || DebugTexture == nullptr)
	{
		return;
	}

	const int DepthGridInputXSize = KinectDevice->Width;
	const int DepthGridInputYSize = KinectDevice->Height;
	
	cv::Mat TempDepthArray(cv::Size(DepthGridInputXSize, DepthGridInputYSize), CV_32SC1, KinectDevice->DepthData.GetData());
	cv::Mat DepthMat(cv::Size(SimSize.X, SimSize.Y), CV_32SC1, DepthGrid.GetData());
	cv::resize(TempDepthArray, DepthMat, DepthMat.size(), 0, 0, cv::INTER_NEAREST );
	cv::flip(DepthMat, DepthMat, 1);

	//Apply min max to NormalGrid
	double max = FGenericPlatformMath::Max(NormalGrid);
	double min = FGenericPlatformMath::Min(NormalGrid);
	// Debug Texture
	FTexturePlatformData* platformData = DebugTexture->GetPlatformData();
	FTexture2DMipMap* MipMap = &platformData->Mips[0];
	FByteBulkData* ImageData = &MipMap->BulkData;
	uint8* RawImageData = (uint8*)ImageData->Lock(LOCK_READ_WRITE);

	for(int i = 0; i < SimSize.X; i++)
	{
		for(int j = 0; j < SimSize.Y; j++)
		{
			double value = (NormalGrid[j * SimSize.Y + i] - min) / (max-min) * 255;
			RawImageData[4 * (j * SimSize.Y+ i)] = value;		// b
			RawImageData[4 * (j * SimSize.Y+ i) + 1] = DepthGrid[j * SimSize.Y + i];	// G
			RawImageData[4 * (j * SimSize.Y + i) + 2] = value;	// r
			RawImageData[4 * (j * SimSize.Y + i) + 3] = 255;
		}
	}

	//release the lock
	ImageData->Unlock();
	DebugTexture->UpdateResource();
}

void UCheneyFireSimComponent::DefaultSetUp(FVector2D wind, EChenySubclass Subclass, double Temp, double Humidity, double curing, FVector2D size)
{
	WindVector = wind;
	temperature = Temp;
	rel_hum = Humidity;

	CuringGrid.Init(curing, size.X * size.Y);
	SubclassGrid.Init(Subclass, size.X * size.Y);
	DepthGrid.Init(0, size.X*size.Y);
	NormalGrid.Init(0, size.X*size.Y);
	DebugTexture = UTexture2D::CreateTransient(size.X, size.Y);

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

	double slope_coefficient = 1;
	if (gradient_multiplier != 0)
	{
		FVector2d gradient_elevation = CalculateGradient(DepthGrid, Coordinate, Grid.Size);
		double slope_in_normal = atan(FVector2d::DotProduct(AdvectNormalVector, gradient_elevation)) * 180 / PI;
		slope_in_normal = FMath::Min(FMath::Max(slope_in_normal, -20), 20);
		slope_coefficient = FMath::Exp(0.069 * slope_in_normal) * gradient_multiplier;
	}
	const double total_speed = speed * slope_coefficient;
	NormalGrid[Coordinate.Y * Grid.Size.Y + Coordinate.X] = slope_coefficient;
	return total_speed;
}

FVector2D UCheneyFireSimComponent::CalculateGradient(TArray<int>& Grid, FIntVector2 Coordinate, FIntVector2 Bounds)
{
	int y = Coordinate.Y;
	int x = Coordinate.X;

	double grid_x_minus;
	double grid_x_plus;
	double grid_y_minus;
	double grid_y_plus;
	
	if (y == 0)
	{
		grid_x_minus = Grid[y * Bounds.Y + x];
	} else
	{
		grid_x_minus = Grid[(y - 1) * Bounds.Y + x];
	}

	if (y == Bounds.Y - 1)
	{
		grid_x_plus = Grid[y * Bounds.Y + x];
	}
	else
	{
		grid_x_plus = Grid[(y + 1) * Bounds.Y + x];
	}

	if (x == 0)
	{
		grid_y_minus = Grid[y * Bounds.Y + x];
	}
	else
	{
		grid_y_minus = Grid[y * Bounds.Y + (x - 1)];
	}

	if (x == Bounds.X - 1)
	{
		grid_y_plus = Grid[y * Bounds.Y + x];
	}
	else
	{
		grid_y_plus = Grid[y * Bounds.Y + (x + 1)];
	}

	double grid_x = (grid_x_plus - grid_x_minus);
	double grid_y = (grid_y_plus - grid_y_minus);
	return FVector2D(grid_y, grid_x);
}
