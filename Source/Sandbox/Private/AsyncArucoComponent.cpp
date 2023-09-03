// Fill out your copyright notice in the Description page of Project Settings.


#include "AsyncArucoComponent.h"

#include "Engine/TextureRenderTarget2D.h"
#if WITH_OPENCV

#include <vector>

#include "PreOpenCVHeaders.h"

#include "opencv2/aruco.hpp"
#include "opencv2/calib3d.hpp"
#include "opencv2/imgproc.hpp"

#include "PostOpenCVHeaders.h"

#endif	// WITH_OPENCV

DEFINE_LOG_CATEGORY(LogARUCO);

// Sets default values for this component's properties
UAsyncArucoComponent::UAsyncArucoComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	// ...
}


// Called when the game starts
void UAsyncArucoComponent::BeginPlay()
{
	Super::BeginPlay();
	OutTexture = UTexture2D::CreateTransient(1, 1, PF_B8G8R8A8);
	OutDetectedMarkers = TArray<FOpenCVArucoDetectedMarker>();
	InLensDistortionParameters = FOpenCVLensDistortionParametersBase();
	Pixels.AddDefaulted(1920*1080);
}


// Called every frame
void UAsyncArucoComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (FirstRun == true)
	{
		FirstRun = false;
		
		AsyncTask(ENamedThreads::AnyBackgroundThreadNormalTask, [this]()
		{
			FMemory::Memcpy(Pixels.GetData(),
				RawKinectComponent->KinectDevice->ColorImagePixelBuffer.GetData(), Pixels.Num() * sizeof(FColor));
			OpenCVArucoDetectMarkers(FIntPoint(1920,1080));
			FirstRun = true;
		});
	}
}

FIntPoint UAsyncArucoComponent::ReadRenderTarget(const UTextureRenderTarget2D* InRenderTarget)
{
	if (InRenderTarget == nullptr)
	{
		UE_LOG(LogARUCO, Error, TEXT("OpenCVArucoDetectMarkers: Invalid InRenderTarget"));
		return 0;
	}
	
	if (InRenderTarget->RenderTargetFormat != ETextureRenderTargetFormat::RTF_RGBA8)
	{
		UE_LOG(LogARUCO, Error, TEXT("OpenCVArucoDetectMarkers: Invalid InRenderTarget texture format - only RGBA8 is currently supported"));
		return 0;
	}

	FRenderTarget* RenderTarget = ((UTextureRenderTarget2D*)InRenderTarget)->GameThread_GetRenderTargetResource();
	if (RenderTarget == nullptr)
	{
		UE_LOG(LogARUCO, Error, TEXT("OpenCVArucoDetectMarkers: Can't get render target resource from InRenderTarget"));
		return 0;
	}

	// Copy the pixels from the RenderTarget
	if (!RenderTarget->ReadPixels(Pixels))
	{
		UE_LOG(LogARUCO, Error, TEXT("OpenCVArucoDetectMarkers: Failed to read pixels from RenderTarget"));
		return 0;
	}

	return RenderTarget->GetSizeXY();
}



// Copied and modified from OpenCVBlueprintFunctionLibrary
int32 UAsyncArucoComponent::OpenCVArucoDetectMarkers(const FIntPoint Size)
{
#if WITH_OPENCV

	if (bEstimatePose)
	{
		if (InMarkerLengthInMeters <= 0.0)
		{
			UE_LOG(LogARUCO, Error, TEXT("OpenCVArucoDetectMarkers: bEstimatePose specified, but InMarkerLengthInMeters is <= 0"));
			return 0;
		}
	}
	
	// Check for invalid dimensions
	if ((Size.X <= 0) || (Size.Y <= 0) || (Pixels.Num() != (Size.X * Size.Y)))
	{
		UE_LOG(LogARUCO, Error, TEXT("OpenCVArucoDetectMarkers: Invalid number of pixels read from RenderTarget"));
		return 0;
	}

	// Create OpenCV Mat with those pixels and convert to grayscale for image processing
	cv::Mat CvFrameColor(cv::Size(Size.X, Size.Y), CV_8UC4, Pixels.GetData());
	cv::flip(CvFrameColor, CvFrameColor, 1);
	cv::Mat CvFrameGray;
	cv::cvtColor(CvFrameColor, CvFrameGray, cv::COLOR_BGRA2GRAY);

	// Select the requested marker dictionary
	cv::aruco::PREDEFINED_DICTIONARY_NAME DictionaryName = cv::aruco::DICT_ARUCO_ORIGINAL;
	switch (InDictionary)
	{
	case EOpenCVArucoDictionary::Dict4x4:		switch (InDictionarySize)
	{
	case EOpenCVArucoDictionarySize::DictSize50:	DictionaryName = cv::aruco::DICT_4X4_50; break;
	case EOpenCVArucoDictionarySize::DictSize100:	DictionaryName = cv::aruco::DICT_4X4_100; break;
	case EOpenCVArucoDictionarySize::DictSize250:	DictionaryName = cv::aruco::DICT_4X4_250; break;
	case EOpenCVArucoDictionarySize::DictSize1000:	DictionaryName = cv::aruco::DICT_4X4_1000; break;
	}
		break;
	case EOpenCVArucoDictionary::Dict5x5:		switch (InDictionarySize)
	{
	case EOpenCVArucoDictionarySize::DictSize50:	DictionaryName = cv::aruco::DICT_5X5_50; break;
	case EOpenCVArucoDictionarySize::DictSize100:	DictionaryName = cv::aruco::DICT_5X5_100; break;
	case EOpenCVArucoDictionarySize::DictSize250:	DictionaryName = cv::aruco::DICT_5X5_250; break;
	case EOpenCVArucoDictionarySize::DictSize1000:	DictionaryName = cv::aruco::DICT_5X5_1000; break;
	}
		break;
	case EOpenCVArucoDictionary::Dict6x6:		switch (InDictionarySize)
	{
	case EOpenCVArucoDictionarySize::DictSize50:	DictionaryName = cv::aruco::DICT_6X6_50; break;
	case EOpenCVArucoDictionarySize::DictSize100:	DictionaryName = cv::aruco::DICT_6X6_100; break;
	case EOpenCVArucoDictionarySize::DictSize250:	DictionaryName = cv::aruco::DICT_6X6_250; break;
	case EOpenCVArucoDictionarySize::DictSize1000:	DictionaryName = cv::aruco::DICT_6X6_1000; break;
	}
		break;
	case EOpenCVArucoDictionary::Dict7x7:		switch (InDictionarySize)
	{
	case EOpenCVArucoDictionarySize::DictSize50:	DictionaryName = cv::aruco::DICT_7X7_50; break;
	case EOpenCVArucoDictionarySize::DictSize100:	DictionaryName = cv::aruco::DICT_7X7_100; break;
	case EOpenCVArucoDictionarySize::DictSize250:	DictionaryName = cv::aruco::DICT_7X7_250; break;
	case EOpenCVArucoDictionarySize::DictSize1000:	DictionaryName = cv::aruco::DICT_7X7_1000; break;
	}
		break;
	case EOpenCVArucoDictionary::DictOriginal:	DictionaryName = cv::aruco::DICT_ARUCO_ORIGINAL; break;
	}
	cv::Ptr<cv::aruco::Dictionary> Dictionary = cv::aruco::getPredefinedDictionary(DictionaryName);

	// Find all ArUco markers
	std::vector<int> MarkerIds;
	std::vector<std::vector<cv::Point2f>> MarkerCorners;
	const cv::Ptr<cv::aruco::DetectorParameters> DetectorParameters = cv::aruco::DetectorParameters::create();
	cv::aruco::detectMarkers(CvFrameGray, Dictionary, MarkerCorners, MarkerIds, DetectorParameters);

	static const uint32 kMaxCorners = 4;
	const int32 NumMarkersFound = MarkerIds.size();
	if (NumMarkersFound > 0)
	{
		// Validate all return data
		if (NumMarkersFound != MarkerCorners.size())
		{
			UE_LOG(LogARUCO, Error, TEXT("OpenCVArucoDetectMarkers: Number of detected MarkerIds and MarkerCorners don't match"));
			return 0;
		}

		for (const std::vector<cv::Point2f>& Corner : MarkerCorners)
		{
			if (Corner.size() != kMaxCorners)
			{
				UE_LOG(LogARUCO, Error, TEXT("OpenCVArucoDetectMarkers: Number of detected corners per marker must be exactly %d"), kMaxCorners);
				return 0;
			}
		}

		// Get the estimated 3D pose data relative to the camera if requested
		std::vector<cv::Vec3d> RotationVectors;
		std::vector<cv::Vec3d> TranslationVectors;
		cv::Mat CameraMatrix = cv::Mat::eye(3, 3, CV_64F);
		cv::Mat DistortionCoeffs = cv::Mat::zeros(1, 8, CV_64F);
		if (bEstimatePose)
		{
			CameraMatrix = InLensDistortionParameters.CreateOpenCVCameraMatrix(FVector2D(Size));
			DistortionCoeffs = InLensDistortionParameters.ConvertToOpenCVDistortionCoefficients();

			cv::aruco::estimatePoseSingleMarkers(MarkerCorners, InMarkerLengthInMeters, CameraMatrix, DistortionCoeffs, RotationVectors, TranslationVectors);
		}

		// Draw debug info onto the frame if requested
		if (bDebugDrawMarkers)
		{
			// We need to convert to RGB temporarily because drawDetectedMarkers only supports grayscale or RGB
			cv::cvtColor(CvFrameColor, CvFrameColor, cv::COLOR_RGBA2RGB);
			cv::aruco::drawDetectedMarkers(CvFrameColor, MarkerCorners, MarkerIds, cv::Scalar(0, 255, 255));

			if (bEstimatePose)
			{
				static const float AxisLengthInMeters = 0.05;
				for (int32 Index = 0; Index < RotationVectors.size(); Index++)
				{
					// drawAxis only does one at a time - it will crash if more than one RotationVector or TranslationVector is passed in
					cv::aruco::drawAxis(CvFrameColor, CameraMatrix, DistortionCoeffs, RotationVectors[Index], TranslationVectors[Index], AxisLengthInMeters);
				}
			}

			cv::cvtColor(CvFrameColor, CvFrameColor, cv::COLOR_RGB2RGBA);

			OutTexture = FOpenCVHelper::TextureFromCvMat(CvFrameColor);
		}

		// Copy the OpenCV data into the Unreal types for return
		TArray<FOpenCVArucoDetectedMarker> markers;
		markers.Reserve(NumMarkersFound);
		for (int32 Index = 0; Index < NumMarkersFound; Index++)
		{
			FOpenCVArucoDetectedMarker NewMarker;
			NewMarker.Id = MarkerIds[Index];
			NewMarker.Corners.Reserve(kMaxCorners);
			for (const cv::Point2f& Corner : MarkerCorners[Index])
			{
				// Normalize each corner to the input image size
				NewMarker.Corners.Add(FVector2D(Corner.x / Size.X, Corner.y / Size.Y));
			}

			// Convert the returned pose into the Unreal coordinate system
			FTransform NewPose = FTransform::Identity;
			if (bEstimatePose)
			{
				// Convert the Rodrigues rotation into a 3x3 rotation matrix
				cv::Mat RotationMatrix = cv::Mat::eye(3, 3, CV_64F);
				cv::Rodrigues(RotationVectors[Index], RotationMatrix);

				// Populate the Unreal rotation matrix
				FMatrix UnrealMatrix = FMatrix::Identity;
				for (int32 Column = 0; Column < 3; Column++)
				{
					UnrealMatrix.SetColumn(Column, FVector(RotationMatrix.at<double>(Column, 0), RotationMatrix.at<double>(Column, 1), RotationMatrix.at<double>(Column, 2)));
				}

				// Scale our translation and populate the FTransform
				static const double MetersToUnrealUnits = 100.0;
				FVector Translation(TranslationVectors[Index][0], TranslationVectors[Index][1], TranslationVectors[Index][2]);
				NewPose = FTransform(UnrealMatrix.GetColumn(0), UnrealMatrix.GetColumn(1), UnrealMatrix.GetColumn(2), Translation * MetersToUnrealUnits);

				// Convert to Unreal axes
				FOpenCVHelper::ConvertOpenCVToUnreal(NewPose);
			}

			NewMarker.Pose = NewPose;
			markers.Add(NewMarker);
		}
		OutDetectedMarkers = markers;
	}
	
	return NumMarkersFound;
}
#else
	{
		UE_LOG(LogOpenCVBlueprintFunctionLibrary, Error, TEXT("OpenCVArucoDetectMarkers: Requires OpenCV"));
		return 0;
	}
#endif	// WITH_OPENCV

void UAsyncArucoComponent::ConvertMarkerToDepthSpace(TArray<FOpenCVArucoDetectedMarker> InMarkers, TArray<FOpenCVArucoDetectedMarker>& OutMarkers)
{
	int width = RawKinectComponent->KinectDevice->Width;
	int height = RawKinectComponent->KinectDevice->Height;

	int colorWidth = RawKinectComponent->KinectDevice->ColorWidth;
	int colorHeight = RawKinectComponent->KinectDevice->ColorHeight;

	OutMarkers.Reserve(InMarkers.Num());
	for(FOpenCVArucoDetectedMarker marker : InMarkers)
	{
		for (int i = 0; i < marker.Corners.Num(); i++)
		{
			float cornerX = (1 - marker.Corners[i].X) * colorWidth;
			float cornerY = marker.Corners[i].Y * colorHeight;
			DepthSpacePoint depthSpacePoint = RawKinectComponent->KinectDevice->DepthSpacePoints[floor(cornerX + cornerY * colorWidth)];
			if (depthSpacePoint.X > 0 && depthSpacePoint.Y > 0)
			{
				marker.Corners[i] = FVector2D(1 - (depthSpacePoint.X / width), depthSpacePoint.Y / height);
			}
		}
		OutMarkers.Add(marker);
	}
}


