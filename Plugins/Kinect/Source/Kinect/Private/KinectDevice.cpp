#include "KinectDevice.h"

#include "stdafx.h"
#include "VectorTypes.h"
#include "RHICommandList.h"

#define COLOR_RAW_WIDTH 1920
#define COLOR_RAW_HEIGHT 1080

DEFINE_LOG_CATEGORY(KinectDeviceLog)

UKinectDevice::UKinectDevice() :
KinectSensor(nullptr)
{
	  // Get the depth frame size from the NUI_IMAGE_RESOLUTION enum
    // You can use NUI_IMAGE_RESOLUTION_640x480 or NUI_IMAGE_RESOLUTION_320x240 in this sample
    // Smaller resolutions will be faster in per-frame computations, but show less detail in reconstructions.
    DepthWidth = NUI_DEPTH_RAW_WIDTH;
	Width = NUI_DEPTH_RAW_WIDTH;
	Height = NUI_DEPTH_RAW_HEIGHT;
    DepthHeight = NUI_DEPTH_RAW_HEIGHT;
    DepthImagePixels = DepthWidth * DepthHeight;
	
	DepthImage = UTexture2D::CreateTransient(DepthWidth, DepthHeight);
	DepthData.Init(0, DepthImagePixels + 5);

	ColorWidth = COLOR_RAW_WIDTH;
	ColorHeight = COLOR_RAW_HEIGHT;

	ColorImage = UTexture2D::CreateTransient(COLOR_RAW_WIDTH, COLOR_RAW_HEIGHT);
	ColorImagePixelBuffer.Init(0, COLOR_RAW_WIDTH * COLOR_RAW_HEIGHT * 4);

	// We don't know these at object creation time, so we use nominal values.
	// These will later be updated in response to the CoordinateMappingChanged event.
	CameraParameters.focalLengthX = NUI_KINECT_DEPTH_NORM_FOCAL_LENGTH_X;
	CameraParameters.focalLengthY = NUI_KINECT_DEPTH_NORM_FOCAL_LENGTH_Y;
	CameraParameters.principalPointX = NUI_KINECT_DEPTH_NORM_PRINCIPAL_POINT_X;
	CameraParameters.principalPointY = NUI_KINECT_DEPTH_NORM_PRINCIPAL_POINT_Y;

	unsigned int frameSize[2] = {DepthWidth, DepthHeight};
	Filter = new FrameFilter(frameSize,30);
	Filter->setValidElevationInterval(MinDepthDistance,MaxDepthDistance);
	Filter->setStableParameters(10,2);
	Filter->setHysteresis(0.1);
	Filter->setSpatialFilter(true);
	
	HRESULT hr;
	hr = GetDefaultKinectSensor(&KinectSensor);
	if (FAILED(hr))
	{
		UE_LOG(KinectDeviceLog, Error, TEXT("Failed loading default kinect sensor: %ld"), hr)
		return;
	}
}

UKinectDevice::~UKinectDevice()
{
	Shutdown();
}

void UKinectDevice::LoadDefault()
{
	HRESULT hr;

	
	if (KinectSensor)
	{
		hr = KinectSensor->Open();

		if (SUCCEEDED(hr))
		{
			hr = KinectSensor->get_CoordinateMapper(&Mapper);
			UE_LOG(KinectDeviceLog, Display, TEXT("Loaded Kinect Mapper"));
		}

		hr = KinectSensor->OpenMultiSourceFrameReader(FrameSourceTypes_Depth | FrameSourceTypes_Color, &MultiSourceFrameReader);

		if (SUCCEEDED(hr))
		{
			hr = Mapper->SubscribeCoordinateMappingChanged(&CoordinateMappingChangedEvent);
		}
	}
    if (nullptr == KinectSensor || FAILED(hr))
    {
        UE_LOG(KinectDeviceLog, Error, TEXT("Failed loading default kinect sensor: %ld"), hr)
    }

	UE_LOG(KinectDeviceLog, Display, TEXT("Loaded Kinect"));

	// Depth Image output
	SAFE_DELETE_ARRAY(DepthImagePixelBuffer);
	DepthImagePixelBuffer = new(std::nothrow) UINT16[DepthImagePixels];
	if (nullptr == DepthImagePixelBuffer)
	{
		UE_LOG(KinectDeviceLog, Error, TEXT("Failed to initialize Kinect Fusion depth image pixel buffer."));
	}

	SAFE_DELETE_ARRAY(DepthDistortionMap);
	DepthDistortionMap = new(std::nothrow) DepthSpacePoint[DepthImagePixels];
	if (nullptr == DepthDistortionMap)
	{
		UE_LOG(KinectDeviceLog, Error, TEXT("Failed to initialize Kinect Fusion depth image distortion map."));
	}
	

	// Lookup table for depth image pixel mapping
	SAFE_DELETE_ARRAY(DepthDistortionLT);
	DepthDistortionLT = new(std::nothrow) UINT[DepthImagePixels];

	if (nullptr == DepthDistortionLT)
	{
		UE_LOG(KinectDeviceLog, Error, TEXT("Failed to initialize Kinect Fusion depth image distortion Lookup Table."));
	}

	ColorSpacePoints.Init(ColorSpacePoint(), DepthImagePixels);
	DepthSpacePoints.Init(DepthSpacePoint(), COLOR_RAW_WIDTH*COLOR_RAW_HEIGHT);

	OnCoordinateMappingChanged();
	
}

void UKinectDevice::Shutdown()
{
	SafeRelease(DepthFrameReader);
	SafeRelease(ColorFrameReader);

	if (Mapper != nullptr)
		Mapper->UnsubscribeCoordinateMappingChanged(CoordinateMappingChangedEvent);
	
	SafeRelease(Mapper);
	SAFE_DELETE_ARRAY(DepthImagePixelBuffer);
	SAFE_DELETE_ARRAY(DepthDistortionMap);
	SAFE_DELETE_ARRAY(DepthDistortionLT);

	if(KinectSensor)
	{
		KinectSensor->Close();
		KinectSensor->Release();
	}
	
}

void UKinectDevice::Update()
{
    if (nullptr == KinectSensor)
    {
        return;
    }

	if  (CoordinateMappingChangedEvent != NULL &&
		WAIT_OBJECT_0 == WaitForSingleObject((HANDLE)(CoordinateMappingChangedEvent) , 0))
	{
		OnCoordinateMappingChanged();
		ResetEvent((HANDLE)(CoordinateMappingChangedEvent));
	}

    // if (!DepthFrameReader)
    // {
    //     return;
    // }

	IMultiSourceFrame* pMultiSourceFrame = NULL;
	HRESULT hr = MultiSourceFrameReader->AcquireLatestFrame(&pMultiSourceFrame);

	if (FAILED(hr))
	{
		return;
	}

	// Get Color Frame
	IColorFrameReference* pColorFrameReference = NULL;
	IColorFrame* pColorFrame = NULL;
	pMultiSourceFrame->get_ColorFrameReference(&pColorFrameReference);
	hr = pColorFrameReference->AcquireFrame(&pColorFrame);
	
	if (FAILED(hr))
	{
		UE_LOG(KinectDeviceLog, Error, TEXT("Failed to acquire color frame: %ld"), hr);
	}
	
	if (SUCCEEDED(hr))
	{
		UINT nBufferSize = 0;
		UINT8 *pBuffer = NULL;
		
		//pColorFrame->AccessRawUnderlyingBuffer(&nBufferSize, &pBuffer);
		hr = pColorFrame->CopyConvertedFrameDataToArray(COLOR_RAW_WIDTH*COLOR_RAW_HEIGHT*4, ColorImagePixelBuffer.GetData(), ColorImageFormat_Bgra);
		
		if (FAILED(hr))
		{
			UE_LOG(KinectDeviceLog, Error, TEXT("Failed to acquire color frame: %ld"), hr);
		}
	
		if (SUCCEEDED(hr))
		{
			ProcessColor(ColorImagePixelBuffer.GetData(), ColorImagePixelBuffer.Num());
		}
	}

	SafeRelease(pColorFrame);
	SafeRelease(pColorFrameReference);

	// Depth Frame
	IDepthFrameReference* pDepthFrameReference = NULL;
	IDepthFrame* pDepthFrame = NULL;
	pMultiSourceFrame->get_DepthFrameReference(&pDepthFrameReference);
	hr = pDepthFrameReference->AcquireFrame(&pDepthFrame);
	
	
	if (FAILED(hr))
	{
		//UE_LOG(KinectDeviceLog, Error, TEXT("Failed to acquire depth frame: %ld"), hr);
	}

    if (SUCCEEDED(hr))
    {
        UINT nBufferSize = 0;
        UINT16 *pBuffer = NULL;
    	
    	hr = pDepthFrame->AccessUnderlyingBuffer(&nBufferSize, &pBuffer);

    	if (FAILED(hr))
    	{
    		UE_LOG(KinectDeviceLog, Error, TEXT("Failed to acquire buffer frame: %ld"), hr);
		}

        if (SUCCEEDED(hr))
        {
            ProcessDepth(pBuffer, nBufferSize);
        }
    }
	
    SafeRelease(pDepthFrame);
}

void UKinectDevice::ProcessColor(UINT8* buffer, UINT size)
{
	FTexturePlatformData* platformData = ColorImage->GetPlatformData();
	FTexture2DMipMap* MipMap = &platformData->Mips[0];
	FByteBulkData* ImageData = &MipMap->BulkData;
	uint8* RawImageData = (uint8*)ImageData->Lock(LOCK_READ_WRITE);

	FMemory::Memcpy(RawImageData, buffer, size);

	//release the lock
	ImageData->Unlock();
	ColorImage->UpdateResource();
}

/// <summary>
/// Handle new depth data 
/// </summary>
void UKinectDevice::ProcessDepth(UINT16* buffer, UINT size)
{
	Filter->setValidElevationInterval(MinDepthDistance,MaxDepthDistance);

	// Lens correction
	UINT16 * pDepth = DepthImagePixelBuffer;
	for(UINT i = 0; i < size; i++, pDepth++)
	{
		const UINT id = DepthDistortionLT[i];
		*pDepth = id < size? buffer[id] : 0;
	}

	// Copy depth data to a new frame buffer for filter
	Kinect::FrameBuffer* kbuffer = new Kinect::FrameBuffer(DepthWidth, DepthHeight, DepthImagePixels* sizeof(UINT16));
	FMemory::Memcpy(kbuffer->getData<UINT16>(), DepthImagePixelBuffer, DepthImagePixels * sizeof(UINT16));

	// Filter the depth data
	Filter->receiveRawFrame(*kbuffer);
	Kinect::FrameBuffer output = Filter->filterThreadMethod();
	float* rawOutput = output.getData<float>();

	// Get min and max for minimax
	float min = static_cast<float>(MinDepthDistance);
	float max = static_cast<float>(MaxDepthDistance);

	// Map Raw depth points to color spac
	Mapper->MapDepthFrameToColorSpace(DepthImagePixels, buffer, DepthImagePixels, ColorSpacePoints.GetData());
	Mapper->MapColorFrameToDepthSpace(DepthImagePixels, buffer, COLOR_RAW_WIDTH*COLOR_RAW_HEIGHT, DepthSpacePoints.GetData());
	FTexturePlatformData* platformData = DepthImage->GetPlatformData();
	FTexture2DMipMap* MipMap = &platformData->Mips[0];
	FByteBulkData* ImageData = &MipMap->BulkData;
	uint8* RawImageData = (uint8*)ImageData->Lock(LOCK_READ_WRITE);

	// Copy the depth frame data onto our texture. Each int in the frame represents a brightness value for a pixel.
	for (UINT i = 0; i < DepthImagePixels; ++i)
	{
		// Get the depth for this pixel
		uint16 Depth = static_cast<uint16>(rawOutput[i]);
		if (Depth == 2048U)
		{
			Depth = static_cast<uint16>(max);
		}
		DepthData[i] = 255 - static_cast<uint8>((Depth - min) / (max - min) * 255);

		uint8 Intensity = 255 - static_cast<uint8>((Depth - min) / (max - min) * 255);
		if (Intensity == 74)
		{
			Intensity = 255;
		}

		ColorSpacePoint colorPoint = ColorSpacePoints[i];
		const int colorX = static_cast<int>(colorPoint.X);
		const int colorY = static_cast<int>(colorPoint.Y);
		int r = 0;
		int g = 0;
		int b = 0;
		int colorIndex = (colorX + colorY * COLOR_RAW_WIDTH) * 4;
		if (colorX > 0 && colorY > 0 && colorX < COLOR_RAW_WIDTH && colorY < COLOR_RAW_HEIGHT && colorIndex > 0) {
			b = ColorImagePixelBuffer[colorIndex];
			g = ColorImagePixelBuffer[colorIndex + 1];
			r = ColorImagePixelBuffer[colorIndex + 2];
		}
		
		// Write the texture data
		RawImageData[4 * i] = b;		// b
		RawImageData[4 * i + 1] = g;	// G
		RawImageData[4 * i + 2] = r;	// r
		RawImageData[4 * i + 3] = Intensity;			// A
	}
	
	//release the lock
	ImageData->Unlock();
	DepthImage->UpdateResource();
}


HRESULT UKinectDevice::SetupUndistortion()
{
    HRESULT hr = E_UNEXPECTED;

    if (CameraParameters.principalPointX != 0)
    {

        CameraSpacePoint cameraFrameCorners[4] = //at 1 meter distance. Take into account that depth frame is mirrored
        {
            /*LT*/{ -CameraParameters.principalPointX / CameraParameters.focalLengthX, CameraParameters.principalPointY / CameraParameters.focalLengthY, 1.f },
            /*RT*/{ (1.f - CameraParameters.principalPointX) / CameraParameters.focalLengthX, CameraParameters.principalPointY / CameraParameters.focalLengthY, 1.f },
            /*LB*/{ -CameraParameters.principalPointX / CameraParameters.focalLengthX, (CameraParameters.principalPointY - 1.f) / CameraParameters.focalLengthY, 1.f },
            /*RB*/{ (1.f - CameraParameters.principalPointX) / CameraParameters.focalLengthX, (CameraParameters.principalPointY - 1.f) / CameraParameters.focalLengthY, 1.f }
        };

        for (UINT rowID = 0; rowID < DepthHeight; rowID++)
        {
            const float rowFactor = float(rowID) / float(DepthHeight - 1);
            const CameraSpacePoint rowStart =
            {
                cameraFrameCorners[0].X + (cameraFrameCorners[2].X - cameraFrameCorners[0].X) * rowFactor,
                cameraFrameCorners[0].Y + (cameraFrameCorners[2].Y - cameraFrameCorners[0].Y) * rowFactor,
                1.f
            };

            const CameraSpacePoint rowEnd =
            {
                cameraFrameCorners[1].X + (cameraFrameCorners[3].X - cameraFrameCorners[1].X) * rowFactor,
                cameraFrameCorners[1].Y + (cameraFrameCorners[3].Y - cameraFrameCorners[1].Y) * rowFactor,
                1.f
            };

            const float stepFactor = 1.f / float(DepthWidth - 1);
            const CameraSpacePoint rowDelta =
            {
                (rowEnd.X - rowStart.X) * stepFactor,
                (rowEnd.Y - rowStart.Y) * stepFactor,
                0
            };

            _ASSERT(DepthWidth == NUI_DEPTH_RAW_WIDTH);
            CameraSpacePoint cameraCoordsRow[NUI_DEPTH_RAW_WIDTH];

            CameraSpacePoint currentPoint = rowStart;
            for (UINT i = 0; i < DepthWidth; i++)
            {
                cameraCoordsRow[i] = currentPoint;
                currentPoint.X += rowDelta.X;
                currentPoint.Y += rowDelta.Y;
            }

            hr = Mapper->MapCameraPointsToDepthSpace(DepthWidth, cameraCoordsRow, DepthWidth, &DepthDistortionMap[rowID * DepthWidth]);
            if (FAILED(hr))
            {
                UE_LOG(KinectDeviceLog, Error, TEXT("Failed to initialize Kinect Coordinate Mapper."));
                return hr;
            }
        }

        if (nullptr == DepthDistortionLT)
        {
            UE_LOG(KinectDeviceLog, Error, TEXT("Failed to initialize Kinect Fusion depth image distortion Lookup Table."));
            return E_OUTOFMEMORY;
        }

        UINT* pLT = DepthDistortionLT;
        for (UINT i = 0; i < DepthImagePixels; i++, pLT++)
        {
            //nearest neighbor depth lookup table 
            UINT x = UINT(DepthDistortionMap[i].X + 0.5f);
            UINT y = UINT(DepthDistortionMap[i].Y + 0.5f);

            *pLT = (x < DepthWidth && y < DepthHeight) ? x + y * DepthWidth : UINT_MAX;
        }
    }

    return S_OK;
}

HRESULT UKinectDevice::OnCoordinateMappingChanged()
{
	HRESULT hr = E_UNEXPECTED;

	//Calculate the dowbn sampled image sizes
	CameraIntrinsics intrinsics = {};

	Mapper->GetDepthCameraIntrinsics(&intrinsics);

	float focalLengthX = intrinsics.FocalLengthX / NUI_DEPTH_RAW_WIDTH;
	float focalLengthY = intrinsics.FocalLengthY / NUI_DEPTH_RAW_HEIGHT;
	float principalPointX = intrinsics.PrincipalPointX / NUI_DEPTH_RAW_WIDTH;
	float principalPointY = intrinsics.PrincipalPointY / NUI_DEPTH_RAW_HEIGHT;

	if(CameraParameters.focalLengthX == focalLengthX &&
		CameraParameters.focalLengthY == focalLengthY &&
		CameraParameters.principalPointX == principalPointX &&
		CameraParameters.principalPointY == principalPointY)
	{
		return S_OK;
	}

	CameraParameters.focalLengthX = focalLengthX;
	CameraParameters.focalLengthY = focalLengthY;
	CameraParameters.principalPointX = principalPointX;
	CameraParameters.principalPointY = principalPointY;

	_ASSERT(CameraParameters.principalPointX !=0);
	if (nullptr == DepthDistortionMap)
	{
		UE_LOG(KinectDeviceLog, Error, TEXT("Failed to initialize Kinect Fusion depth image pixel buffer."));
	}

	hr = SetupUndistortion();
	return hr;
}


void UKinectDevice::UpdateTextureRegion(FTextureRHIRef TextureRHI, int32 MipIndex, uint32 NumRegions, FUpdateTextureRegion2D Region, uint32 SrcPitch, uint32 SrcBpp, uint8* SrcData)
{
	ENQUEUE_RENDER_COMMAND(UpdateTextureRegionsData)(
		[=](FRHICommandListImmediate& RHICmdList)
		{
			check(TextureRHI.IsValid());
			RHIUpdateTexture2D(
				TextureRHI,
				MipIndex,
				Region,
				SrcPitch,
				SrcData
				+ Region.SrcY * SrcPitch
				+ Region.SrcX * SrcBpp
			);
			
		});
}

void UKinectDevice::ResetFilter()
{
	Filter->resetFilter();
}


void UpdateIntrinsics(NUI_FUSION_IMAGE_FRAME * pImageFrame, NUI_FUSION_CAMERA_PARAMETERS * params)
{
	if (pImageFrame != nullptr && pImageFrame->pCameraParameters != nullptr && params != nullptr)
	{
		pImageFrame->pCameraParameters->focalLengthX = params->focalLengthX;
		pImageFrame->pCameraParameters->focalLengthY = params->focalLengthY;
		pImageFrame->pCameraParameters->principalPointX = params->principalPointX;
		pImageFrame->pCameraParameters->principalPointY = params->principalPointY;
	}

	// Confirm we are called correctly
	_ASSERT(pImageFrame != nullptr && pImageFrame->pCameraParameters != nullptr && params != nullptr);
}



