// Fill out your copyright notice in the Description page of Project Settings.


#include "Sandbox/Public/RawKinectComponent.h"

#include "Async/IAsyncTask.h"

// Sets default values for this component's properties
URawKinectComponent::URawKinectComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;


	// ...
}

void URawKinectComponent::OnRegister()
{
	Super::OnRegister();
	KinectDevice = NewObject<UKinectDevice>();
	KinectDevice->AddToRoot();
}

// Called when the game starts
void URawKinectComponent::BeginPlay()
{
	Super::BeginPlay();
	KinectDevice->LoadDefault();
	
	// ...
	
}

void URawKinectComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	KinectDevice->Shutdown();
	Super::EndPlay(EndPlayReason);
}


// Called every frame
void URawKinectComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (FirstLaunch == true)
	{
		FirstLaunch = false;
		AsyncTask(ENamedThreads::GameThread, [this]() {
			KinectDevice->Update();
			FirstLaunch = true;
		});
	}
// 	AsyncTask(ENamedThreads::GameThread, [this]() {
// 	KinectDevice->Update();
// 	FirstLaunch = true;
// });

	
}

