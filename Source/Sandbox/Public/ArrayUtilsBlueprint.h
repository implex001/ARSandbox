// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "ArrayUtilsBlueprint.generated.h"

/**
 * 
 */
UCLASS()
class SANDBOX_API UArrayUtilsBlueprint : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintCallable, Category = "ArrayUtils")
	static TArray<float> bilinearInterpolation(TArray<int> inArray, TArray<float> outArray, int inWidth, int inHeight, int outWidth, int outHeight);
	
};
