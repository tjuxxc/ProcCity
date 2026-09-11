#include "Misc/AutomationTest.h"
#include "ProcCityGeometry/PolygonMeshAdapter.h"

#if WITH_DEV_AUTOMATION_TESTS

using namespace ProcCityGeometry;

namespace
{
	constexpr EAutomationTestFlags TestFlags =
		EAutomationTestFlags::EditorContext |
		EAutomationTestFlags::CommandletContext |
		EAutomationTestFlags::ClientContext |
		EAutomationTestFlags::EngineFilter;
	
	/**
	 * Replicates UE's face normal algorithm: Cross(C-A, B-A), i.e. the reverse of 
	 * the mathematical convention.
	 * 
	 * Basis: (Edge02 ^ Edge01) in UProceduralMeshComponent::CalculateTangentsForMesh.
	 * Corollary: UE treats "numerically CW" as front-facing.
	*/
	FVector UnrealFaceNormal(const FVector& A, const FVector& B, const FVector& C)
	{
		return FVector::CrossProduct(C - A, B - A).GetSafeNormal();
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSurfaceWindingTest,
	"ProcCity.Geometry.Adapter.SurfaceWinding", TestFlags)

bool FSurfaceWindingTest::RunTest(const FString& Parameters)
{
	const FPolygon2D Poly = FPolygon2D::MakeRect(
		FVector2D(1000, 2000), 
		FVector2D(500, 300), 
		25.0);
	
	FSurfaceMeshData Ground;
	TestTrue(TEXT("Build up-facing surface"),
			 BuildSurfaceMesh(Poly, 0.0, 
			 	ESurfaceFacing::Up, 100.0, Ground));
	
	for (int32 t = 0; t < Ground.NumTriangles(); ++t)
	{
		const FVector& A = Ground.Positions[Ground.Indices[t * 3 + 0]];
		const FVector& B = Ground.Positions[Ground.Indices[t * 3 + 1]];
		const FVector& C = Ground.Positions[Ground.Indices[t * 3 + 2]];
		TestTrue(TEXT("UE face normal points up"), 
			UnrealFaceNormal(A, B, C).Z > 0.9);
	}

	FSurfaceMeshData Ceiling;
	BuildSurfaceMesh(Poly, 0.0, ESurfaceFacing::Down, 100.0, Ceiling);
	for (int32 t = 0; t < Ceiling.NumTriangles(); ++t)
	{
		const FVector& A = Ceiling.Positions[Ceiling.Indices[t * 3 + 0]];
		const FVector& B = Ceiling.Positions[Ceiling.Indices[t * 3 + 1]];
		const FVector& C = Ceiling.Positions[Ceiling.Indices[t * 3 + 2]];
		TestTrue(TEXT("UE face normal points down"), 
			UnrealFaceNormal(A, B, C).Z < -0.9);
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEdgeTransformTest,
	"ProcCity.Geometry.Adapter.EdgeTransform", TestFlags)

bool FEdgeTransformTest::RunTest(const FString& Parameters)
{
	const FPolygon2D Block = FPolygon2D::MakeRect(
		FVector2D::ZeroVector, 
		FVector2D(1000, 600), 
		37.0);
	
	TArray<FPolyEdge> Edges;
	Block.GetOuterEdges(Edges);
	
	for (const FPolyEdge& E : Edges)
	{
		const FTransform Xf = MakeEdgeTransform(E, 0.5, 0.0);
		
		const FVector Fwd   = Xf.GetUnitAxis(EAxis::X);
		const FVector Right = Xf.GetUnitAxis(EAxis::Y);
		const FVector Up    = Xf.GetUnitAxis(EAxis::Z);
		
		// Outer normal points to forward (local X+)
		TestTrue(TEXT("Forward is outward normal"),
			Fwd.Equals(
				FVector(E.Normal.X, E.Normal.Y, 0.0), 
				1e-4));
		
		// Edge direction (E.End - E.Start) point to right (local Y+)
		const FVector2D Dir = E.Direction(); 
		TestTrue(TEXT("Right runs along edge direction"),
				 Right.Equals(
				 	FVector(Dir.X, Dir.Y, 0.0), 
				 	1e-4));
		
		TestTrue(TEXT("Up is world up"), 
			Up.Equals(FVector::UpVector, 1e-4));
		
		const FVector Out = Xf.GetLocation() + Fwd * 10.0;
		TestFalse(TEXT("Forward exits polygon"), 
			Block.Contains(FVector2D(Out.X, Out.Y)));
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FYawConventionTest,
	"ProcCity.Geometry.Adapter.YawConvention", TestFlags)

bool FYawConventionTest::RunTest(const FString& Parameters)
{
	// Assertion: Assert that FPolygon2D's angle has the same sign as UE Yaw, 
	// to prevent anyone from "casually" adding a negative sign in the future
	constexpr double YawDeg = 40.0;
	const FPolygon2D R = FPolygon2D::MakeRect(
		FVector2D::ZeroVector, 
		FVector2D(200, 50), YawDeg);
	
	const FOrientedBox2D OBB = R.MinAreaOBB();
	
	TestTrue(TEXT("OBB yaw matches construction yaw"),
			 FMath::IsNearlyEqual(
				 FMath::RadiansToDegrees(OBB.Rotation), 
				 YawDeg, 0.1));
	
	const FRotator Rot(0.0, YawDeg, 0.0);
	const FVector UeRotated = Rot.RotateVector(FVector::ForwardVector);
	const FVector2D GeoAxis = OBB.AxisX();
	
	const bool bMatchX = FMath::IsNearlyEqual(
		UeRotated.X, GeoAxis.X, 1e-4);
	const bool bMatchY = FMath::IsNearlyEqual(
		UeRotated.Y, GeoAxis.Y, 1e-4);
	
	TestTrue(TEXT("Geometry axis matches UE yaw rotation"),
			 bMatchX && bMatchY);
	
	return true;
}

#endif
