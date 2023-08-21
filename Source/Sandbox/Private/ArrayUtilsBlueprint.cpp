// Fill out your copyright notice in the Description page of Project Settings.


#include "ArrayUtilsBlueprint.h"

TArray<float> UArrayUtilsBlueprint::bilinearInterpolation(TArray<int> inArray, TArray<float> outArray, int inWidth, int inHeight, int outWidth, int outHeight)
{
	// Standard Bilinear Interpolation
	// Check in array length
	if (inArray.Num() == 0)
	{
		return outArray;
	}
	
	float x_ratio;
	
	if (outWidth > 1)
	{
		x_ratio = static_cast<float>(inWidth - 1) / static_cast<float>(outWidth - 1); 
	} else
	{
		x_ratio = 0;
	}

	float y_ratio;

	if (outHeight > 1)
	{
		y_ratio = static_cast<float>(inHeight - 1) / static_cast<float>(outHeight - 1);
	} else
	{
		y_ratio = 0;
	}



	for (int i = 0; i < outHeight; i++)
	{
		for (int j = 0; j < outWidth; j++)
		{
			float x_l = floor(x_ratio * (float) j);
			float x_h = ceil(x_ratio * (float) j);
			float y_l = floor(y_ratio * (float) i);
			float y_h = ceil(y_ratio * (float) i);

			float x_weight = (x_ratio * (float) j) - x_l;
			float y_weight = (y_ratio * (float) i) - y_l;

			float a = static_cast<float>(inArray[y_l * inWidth + (int) x_l]);
			float b = static_cast<float>(inArray[y_l * inWidth + (int) x_h]);
			float c = static_cast<float>(inArray[y_h * inWidth + (int) x_l]);
			float d = static_cast<float>(inArray[y_h * inWidth + (int) x_h]);

			float pixel = a * (1 - x_weight) * (1 - y_weight) + b * x_weight * (1 - y_weight) + c * y_weight * (1 - x_weight) + d * x_weight * y_weight;

			outArray[i * outWidth + j] = pixel;
		}
	}

	return outArray;
}
