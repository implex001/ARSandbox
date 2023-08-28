// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "OpenCVBlueprintFunctionLibrary.h"
#include "Components/ActorComponent.h"
#include "AsyncArucoComponent.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogARUCO, Log, All);
UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class SANDBOX_API UAsyncArucoComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UAsyncArucoComponent();

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	UTextureRenderTarget2D* RenderTargetIn;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	EOpenCVArucoDictionary InDictionary;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	EOpenCVArucoDictionarySize InDictionarySize;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	bool bDebugDrawMarkers;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	bool bEstimatePose;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float InMarkerLengthInMeters;

	UPROPERTY(BlueprintReadWrite)
	FOpenCVLensDistortionParametersBase InLensDistortionParameters;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere)
	UTexture2D* OutTexture;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere)
	TArray<FOpenCVArucoDetectedMarker> OutDetectedMarkers;

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

	UE::Tasks::FTask Task;

	bool FirstRun = true;

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
private:
	int32 OpenCVArucoDetectMarkers(const FIntPoint Size);

	// Read render target, can only be called from game thread
	FIntPoint ReadRenderTarget(const UTextureRenderTarget2D* InRenderTarget);

	TArray<FColor> Pixels;
		
};
