#pragma once

template<typename T>
struct SANDBOX_API FGrid
{
	TArray<TArray<T>> Grid;
	FGrid(int32 x, int32 y)
	{
		Grid = TArray<TArray<T>>();
		Grid.Reserve(x);
		for (int i = 0; i < x; i++)
		{
			TArray<T> row;
			row.Reserve(y);
			Grid.Add(row);
		}
	}

	FGrid()
	{
		Grid = TArray<TArray<T>>();
	}

	FORCEINLINE TArray<T> operator[](int32 x) const
	{
		return Grid[x];
	}

	FORCEINLINE int Rows()
	{
		return Grid.Num();
	}

	FORCEINLINE void Init(T element)
	{
		for (int i = 0; i < Grid.Num(); i++)
		{
			for (int j = 0; j < Grid[i].Num(); j++)
			{
				Grid[i][j] = element;
			}
		}
	}

	FORCEINLINE void Copy(FGrid& GridToCopy)
	{
		for (int i = 0; i < Grid.Num(); i++)
		{
			Grid[i] = GridToCopy[i];
		}
	}
};
