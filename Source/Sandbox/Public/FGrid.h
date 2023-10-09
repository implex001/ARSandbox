#pragma once

template<typename T>
struct SANDBOX_API FGrid
{
	TArray<T> Grid;
	FIntVector2 Size;
	FGrid(int32 x, int32 y)
	{
		Grid = TArray<T>();
		Grid.Reserve(x * y);
		Size = FIntVector2(x, y);
	}

	FGrid(T default_object, int32 x, int32 y)
	{
		Grid = TArray<T>();
		Grid.Init(default_object, x * y);
		Size = FIntVector2(x, y);
	}

	FGrid(TArray<T> grid, int32 x, int32 y)
	{
		Grid = grid;
		Size = FIntVector2(x, y);
	}

	FGrid()
	{
		Grid = TArray<T>();
		Size = FIntVector2(0, 0);
	}

	FORCEINLINE T& operator[](FIntVector2 vector)
	{
		return Grid[vector.Y * Size.Y + vector.X];
	}

	FORCEINLINE T& Get(int x, int y)
	{
		return Grid[y * Size.Y + x];
	}
	

	FORCEINLINE void Init(T element)
	{
		Grid.Init(element, Size.X * Size.Y);
	}

	FORCEINLINE void Copy(FGrid& GridToCopy)
	{
		for (int i = 0; i < Grid.Num(); i++)
		{
			Grid[i] = GridToCopy[i];
		}
	}

	FORCEINLINE FIntVector2 GetSize()
	{
		return Size;
	}
};
