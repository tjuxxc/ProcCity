#include "ProcCityGeometry/PolygonMeshAdapter.h"

namespace ProcCityGeometry
{
	void FlipTriangleWinding(TArray<int32>& Indices)
	{
		for (int32 i = 0; i + 2 < Indices.Num(); i += 3)
		{
			Swap(Indices[i + 1], Indices[i + 2]);
		}
	}
	
	bool BuildSurfaceMesh(const FPolygonTriangulation2D& Tri, double Z, 
		ESurfaceFacing Facing, double UvScale, FSurfaceMeshData& OutMesh)
	{
		OutMesh = FSurfaceMeshData{};
		if (!Tri.IsValid()) { return false; }
		
		const bool bUp = (Facing == ESurfaceFacing::Up);
		const FVector N(0.0, 0.0, bUp ? 1.0 : -1.0);
		const double InvUvScale = 
			(UvScale > UE_KINDA_SMALL_NUMBER) ? (1.0 / UvScale) : 0.01;
		
		OutMesh.Positions.Reserve(Tri.Vertices.Num());
		OutMesh.Normals.Reserve(Tri.Vertices.Num());
		OutMesh.UVs.Reserve(Tri.Vertices.Num());
		
		for (const FVector2D& V : Tri.Vertices)
		{
			OutMesh.Positions.Add(FVector(V.X, V.Y, Z));
			OutMesh.Normals.Add(N);
			
			OutMesh.UVs.Add(FVector2f(
				static_cast<float>(V.X * InvUvScale),
				static_cast<float>(V.Y * InvUvScale)));
		}
		
		OutMesh.Indices = Tri.Indices;
		
		// FPolygon2D triangulation output is algebraically CCW.
		// UE treats numerically CW as front-facing, so 
		// - upward-facing surfaces need flipping;
		// - downward-facing surfaces keep the original order.
		if (bUp)
		{
			FlipTriangleWinding(OutMesh.Indices);
		}
		
		return OutMesh.IsValid();
	}
	
	bool BuildSurfaceMesh(const FPolygon2D& Polygon, double Z, 
		ESurfaceFacing Facing, double UvScale, FSurfaceMeshData& OutMesh)
	{
		FPolygonTriangulation2D Tri;
		
		if (!Polygon.Triangulate(Tri)) { return false; }
		return BuildSurfaceMesh(Tri, Z, Facing, UvScale, OutMesh);
	}
	
	FTransform MakeEdgeTransform(const FPolyEdge& Edge, double AlongT, double Z)
	{
		const FVector2D P = Edge.PointAt(AlongT);
		const FVector Location(P.X, P.Y, Z);
		
		if (Edge.Normal.IsNearlyZero())
		{
			return FTransform(FRotator::ZeroRotator, Location);
		}
		
		const FVector Forward(Edge.Normal.X, Edge.Normal.Y, 0.0);
		const FVector Up = FVector::UpVector; // (0, 0, 1)
		
		return FTransform(
			FRotationMatrix::MakeFromXZ(Forward, Up).Rotator(), 
			Location);
	}
	
	FTransform MakeBoxTransform(const FOrientedBox2D& Box, double Z)
	{
		const FRotator R(0.0, 
			FMath::RadiansToDegrees(Box.Rotation), 0.0);
		return FTransform(R, FVector(Box.Center.X, Box.Center.Y, Z));
	}
	
}





























