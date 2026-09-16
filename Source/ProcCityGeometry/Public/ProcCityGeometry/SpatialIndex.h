#pragma once

#include "CoreMinimal.h"

/**
 * Uniform grid spatial index for 2D points.
 *
 * Purpose: node welding during network planarization, prop overlap rejection,
 * nearest-neighbor lookups. Deliberately simple - at city scale with a cell size
 * near the mean query radius this beats a KD-tree for build cost and has
 * fully deterministic iteration order, which matters for golden tests.
 *
 * Contract:
 *  - Query results are returned in ascending point-index order (deterministic).
 *  - No UObject / Engine dependency; safe inside ParallelFor as long as a single
 *    instance is not written concurrently.
 */

class PROCCITYGEOMETRY_API FSpatialGrid2D
{
public:
	FSpatialGrid2D() = default;
	
	/** @param InCellSize  cell edge length in cm; should be >= typical query radius */
	void Reset(double InCellSize);
	
	/** Returns the index assigned to the point */
	int32 Insert(const FVector2D& Point);
	
	void Reserve(int32 Count) { Points.Reserve(Count); }
	
	int32 Num() const { return Points.Num(); }
	
	const FVector2D& GetPoint(int32 Index) const { return Points[Index]; }
	
	/** All indices whose point lies within Radius of Query. Sorted ascending. */
	void QueryRadius(const FVector2D& Query, 
		double Radius, TArray<int32>& OutIndices) const;
	
	/**
	 * Nearest point within Radius, or INDEX_NONE.
	 * Ties are broken by lowest index so the result is deterministic.
	 */
	int32 FindNearest(const FVector2D& Query, 
		double Radius, double* OutDistance = nullptr) const;
	
	/**
	 * Insert only if no existing point lies within WeldRadius; otherwise return the
	 * existing index. This is the primitive used to weld coincident network nodes.
	 * @param bOutInserted  true when a new point was added
	 */
	int32 InsertUnique(const FVector2D& Point, double WeldRadius, bool& bOutInserted);
	
private:
	struct FCellKey
	{
		int32 X = 0;
		int32 Y = 0;
		bool operator==(const FCellKey& Other) const
		{
			return X == Other.X && Y == Other.Y;
		}
		friend uint32 GetTypeHash(const FCellKey& K)
		{
			return HashCombine(::GetTypeHash(K.X), ::GetTypeHash(K.Y));
		}
	};
	
	FCellKey ToCell(const FVector2D& P) const;
	
	double CellSize = 100.0;
	TArray<FVector2D> Points;
	TMap<FCellKey, TArray<int32>> Cells;
};