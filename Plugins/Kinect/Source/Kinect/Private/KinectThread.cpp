#include "KinectThread.h"

DEFINE_LOG_CATEGORY(KinectThreadLog);
KinectThread::KinectThread(UKinectDevice* Device) :
	Thread(nullptr),
	StopTaskCounter(0),
	KinectDevice(Device)
{
	Thread = FRunnableThread::Create(this, TEXT("KinectThread"));
	if (Thread == nullptr)
	{
		UE_LOG(LogTemp, Error, TEXT("KinectThread could not be created!"));
	}
}

KinectThread::~KinectThread()
{
	if (Thread != nullptr)
	{
		delete Thread;
		Thread = nullptr;
	}
}

bool KinectThread::Init()
{
	UE_LOG(KinectThreadLog, Log, TEXT("KinectThread::Init()"));
	return true;
}

uint32 KinectThread::Run()
{
	//Check for KinectDevice null
	if (KinectDevice == nullptr)
	{
		UE_LOG(KinectThreadLog, Error, TEXT("KinectDevice is null!"));
		return 1;
	}
	
	while (StopTaskCounter.GetValue() == 0)
	{
		KinectDevice->Update();
	}
	return 0;
}

void KinectThread::Stop()
{
	UE_LOG(KinectThreadLog, Log, TEXT("KinectThread::Stop()"));
	StopTaskCounter.Increment();
}

FCriticalSection* KinectThread::GetCriticalSection()
{
	return &CriticalSection;
}