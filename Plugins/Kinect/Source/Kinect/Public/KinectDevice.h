#pragma once

#include "Kinect.h"
#include "NuiKinectFusionApi.h"
#include "Kinect/Private/FrameFilter.h"
#include "RHIResources.h"
#include "RHI.h"
#undef UpdateResource
#include "Engine/CanvasRenderTarget2D.h"

#include "KinectDevice.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(KinectDeviceLog, Log, All);

UCLASS()
class KINECT_API UKinectDevice : public UObject
{
	GENERATED_BODY()

	static const int            cBytesPerPixel = 4; // for depth float and int-per-pixel raycast images
	static const int            cResetOnTimeStampSkippedMilliseconds = 1000;  // ms
	static const int            cResetOnNumberOfLostFrames = 100;
	static const int            cStatusMessageMaxLen = MAX_PATH*2;
	static const int            cTimeDisplayInterval = 10;
public:
	UKinectDevice();
	virtual ~UKinectDevice() override;

	UFUNCTION(BlueprintCallable, Category="IO")
	void LoadDefault();

	UPROPERTY(BlueprintReadOnly)
	UTexture2D* DepthImage;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UCanvasRenderTarget2D* DepthImageRT;

	UPROPERTY(BlueprintReadOnly)
	UTexture2D* ColorImage;

	UFUNCTION(BlueprintCallable, Category="IO")
	void Update();

	UFUNCTION(BlueprintCallable, Category="IO")
	void Shutdown();

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	double MinDepthDistance = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	double MaxDepthDistance = 2000;

	UPROPERTY(BlueprintReadOnly)
	TArray<int> DepthData;

	// Width and Height are only updated on construction
	UPROPERTY(BlueprintReadOnly)
	int Width;

	UPROPERTY(BlueprintReadOnly)
	int Height;


private:

	HRESULT SetupUndistortion();
	void ProcessDepth(UINT16* buffer, UINT size);
	void ProcessColor(UINT8* buffer, UINT size);
	HRESULT OnCoordinateMappingChanged();
	void UpdateTextureRegion(FTextureRHIRef TextureRHI, int32 MipIndex, uint32 NumRegions, FUpdateTextureRegion2D Region, uint32 SrcPitch, uint32 SrcBpp, uint8* SrcData);
	void DrawOnCanvas(UCanvas* Canvas, int32 Width, int32 Height);
	
	IKinectSensor* KinectSensor;

	// Frames from depth
	IDepthFrameReader* DepthFrameReader;
	
	UINT DepthWidth;
	UINT DepthHeight;
	UINT DepthImagePixels;
	UINT16* DepthImagePixelBuffer;
	
	IColorFrameReader* ColorFrameReader;
	TArray<uint8> ColorImagePixelBuffer;

	IMultiSourceFrameReader* MultiSourceFrameReader;

	// For filtering
	FrameFilter* Filter;
	
	//For depth distortion correction
	ICoordinateMapper* Mapper;
	DepthSpacePoint* DepthDistortionMap;
	UINT* DepthDistortionLT;
	WAITABLE_HANDLE CoordinateMappingChangedEvent;

	/// Kinect camera parameters.
	NUI_FUSION_CAMERA_PARAMETERS CameraParameters;
	
};