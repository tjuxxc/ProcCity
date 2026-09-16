#include "ProcCityGeometry/SpatialIndex.h"

void FSpatialGrid2D::Reset(double InCellSize)
{
	CellSize = FMath::Max(InCellSize, 1.0);
	Points.Reset();
	Cells.Reset();
}

FSpatialGrid2D::FCellKey FSpatialGrid2D::ToCell(const FVector2D& P) const
{
	return FCellKey{
		static_cast<int32>(FMath::RoundToDouble(P.X / CellSize)),
		static_cast<int32>(FMath::RoundToDouble(P.Y / CellSize))
	};
}

int32 FSpatialGrid2D::Insert(const FVector2D& Point)
{
	const int32 Index = Points.Add(Point);
	Cells.FindOrAdd(ToCell(Point)).Add(Index);
	return Index;
}

void FSpatialGrid2D::QueryRadius(const FVector2D& Query, 
	double Radius, TArray<int32>& OutIndices) const
{
	OutIndices.Reset();
	if (Points.Num() == 0 || Radius < 0.0) { return; }
	
	const int32 Span = FMath::Max(1, 
		FMath::CeilToInt(Radius / CellSize));
	const FCellKey Base = ToCell(Query);
	const double RadiusSq = Radius * Radius;
	
	for (int32 Y = -Span; Y <= Span; ++Y)
	{
		for (int32 X = -Span; X <= Span; ++X)
		{
			const TArray<int32>* Bucket = 
				Cells.Find(FCellKey(Base.X + X, Base.Y + Y));
			if (!Bucket) { continue; }
			for (int32 Idx : *Bucket)
			{
				if (FVector2D::DistSquared(Points[Idx], Query) < RadiusSq)
				{
					OutIndices.Add(Idx);
				}
			}
		}
	}
	// Cell iteration order depends on TMap internals, so sort for determinism.
	OutIndices.Sort(); // ascending
}

int32 FSpatialGrid2D::FindNearest(const FVector2D& Query, 
	double Radius, double* OutDistance) const
{
	TArray<int32> Candidates;
	QueryRadius(Query, Radius, Candidates);
	
	int32 Best = INDEX_NONE;
	double BestDistSq = TNumericLimits<double>::Max();
	// Indices stores in ascent order. Ties resolve to lowest index
	for (int32 Idx : Candidates)
	{
		const double Dsq = FVector2D::DistSquared(Points[Idx], Query);
		if (Dsq < BestDistSq)
		{
			BestDistSq = Dsq;
			Best = Idx;
		}
	}
	if (OutDistance)
	{
		*OutDistance = (Best != INDEX_NONE) 
			? BestDistSq : TNumericLimits<double>::Max();;
	}
	return Best;
}

int32 FSpatialGrid2D::InsertUnique(const FVector2D& Point, 
	double WeldRadius, bool& bOutInserted)
{
	const int32 Existing = FindNearest(Point, WeldRadius);
	if (Existing != INDEX_NONE)
	{
		bOutInserted = false;
		return Existing;
	}
	bOutInserted = true;
	return Insert(Point);
}