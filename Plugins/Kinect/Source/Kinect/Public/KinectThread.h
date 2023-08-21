#pragma once
#include "KinectDevice.h"

DECLARE_LOG_CATEGORY_EXTERN(KinectThreadLog, Log, All);

class KINECT_API KinectThread : FRunnable
{
public:
	KinectThread(UKinectDevice* Device);
	virtual ~KinectThread() override;

	// Begin FRunnable interface.
	virtual bool Init() override;
	virtual uint32 Run() override;
	virtual void Stop() override;
	// End FRunnable interface

	FCriticalSection* GetCriticalSection();

private:
	FRunnableThread* Thread;

	FThreadSafeCounter StopTaskCounter;
	
	UKinectDevice* KinectDevice;
	
	FCriticalSection CriticalSection;
	
};
