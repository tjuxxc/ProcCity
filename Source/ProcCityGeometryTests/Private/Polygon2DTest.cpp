#include "Misc/AutomationTest.h"
#include "ProcCityGeometry/Polygon2D.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	constexpr EAutomationTestFlags TestFlags =
		EAutomationTestFlags::EditorContext |
		EAutomationTestFlags::CommandletContext |
		EAutomationTestFlags::ClientContext |
		EAutomationTestFlags::EngineFilter;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPolygon2DRectTest, 
	"ProcCity.Geometry.Polygon2D.Rect", TestFlags)

// ---- Basic metric tests

bool FPolygon2DRectTest::RunTest(const FString& Parameters)
{
	const FPolygon2D R = FPolygon2D::MakeRect(
		FVector2D(100, 200), 
		FVector2D(50, 25), 
		0.0
	);
	
	TestEqual(TEXT("VertexCount"), R.Outer.NumVertices(), 4);
	TestTrue(TEXT("IsValid"), R.IsValid());
	TestEqual(TEXT("Winding is CCW"), 
		R.Outer.GetWinding(), EPolyWinding::CounterClockwise);
	TestTrue(TEXT("Area = 100*50"), 
		FMath::IsNearlyEqual(R.Area(), 100.0 * 50.0, 1e-6));
	TestTrue(TEXT("Perimeter"), 
		FMath::IsNearlyEqual(R.Perimeter(), 2.0 * (100.0 + 50.0), 1e-6));
	TestTrue(TEXT("Centroid"), 
		R.Centroid().Equals(FVector2D(100, 200), 1e-6));
	TestTrue(TEXT("Contains center"), 
		R.Contains(FVector2D(100, 200)));
	TestFalse(TEXT("Excludes outside"), 
		R.Contains(FVector2D(1000, 200)));
	TestFalse(TEXT("No self-intersection"), R.HasSelfIntersection());
	
	return true;
}

// Rotated rectangle: area and centroid must remain unchanged (rotation invariants)
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPolygon2DRotatedRectTest,
	"ProcCity.Geometry.Polygon2D.RotatedRect", TestFlags)

bool FPolygon2DRotatedRectTest::RunTest(const FString& Parameters)
{
	for (double Yaw : {0.0, 17.0, 45.0, 90.0, 133.0, -60.0})
	{
		const FPolygon2D R = FPolygon2D::MakeRect(
			FVector2D(0, 0), 
			FVector2D(40, 10), 
			Yaw
		);
		
		TestTrue(*FString::Printf(TEXT("Area invariant @%.0f"), Yaw),
				 FMath::IsNearlyEqual(R.Area(), 80.0 * 20.0, 1e-6));
		TestTrue(*FString::Printf(TEXT("Centroid invariant @%.0f"), Yaw),
				 R.Centroid().Equals(FVector2D::ZeroVector, 1e-6));
		TestEqual(TEXT("CCW preserved"), 
			R.Outer.GetWinding(), EPolyWinding::CounterClockwise);
	}
	return true;
}

// ---- Winding tests

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPolygon2DWindingTest,
	"ProcCity.Geometry.Polygon2D.Winding", TestFlags)

bool FPolygon2DWindingTest::RunTest(const FString& Parameters)
{
	// Deliberately constructed in CW order
	TArray<FVector2D> CW = {
		FVector2D(0, 0), FVector2D(0, 100), 
		FVector2D(100, 100), FVector2D(100, 0)
	};
	
	const FPolygon2D P(CW);
	TestEqual(TEXT("Normalized to CCW"), 
		P.Outer.GetWinding(), EPolyWinding::CounterClockwise);
	
	TestTrue(TEXT("Area positive"), P.Area() > 0.0);
	TestTrue(TEXT("Area = 10000"), 
		FMath::IsNearlyEqual(P.Area(), 10000.0, 1e-6));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPolygon2DTriangulationWindingTest,
	"ProcCity.Geometry.Polygon2D.TriangulationWinding", TestFlags)

bool FPolygon2DTriangulationWindingTest::RunTest(const FString& Parameters)
{
	// Covers the four cases — convex / concave / with holes / rotated — 
	// ensuring normalization takes effect in all branches
	TArray<TPair<FString, FPolygon2D>> Cases;
	
	Cases.Emplace(TEXT("Rect"),
		FPolygon2D::MakeRect(
			FVector2D(1000, 2000), 
			FVector2D(500, 300), 25.0));
	
	Cases.Emplace(TEXT("Regular9"),
		FPolygon2D::MakeRegular(
			FVector2D::ZeroVector, 800.0, 9));
	
	// L-shape concave polygon
	{   
		FPolygon2D L;
		L.Outer.Vertices = {
			FVector2D(0,0), FVector2D(600,0), 
			FVector2D(600,200), FVector2D(200,200), 
			FVector2D(200,600), FVector2D(0,600) };
		
		L.Normalize();
		Cases.Emplace(TEXT("LShape"), MoveTemp(L));
	}
	
	// With hole
	{   
		FPolygon2D H = FPolygon2D::MakeRect(
			FVector2D::ZeroVector, 
			FVector2D(500, 500));
		
		FPolyRing Hole;
		Hole.Vertices = { FVector2D(-150,-150), FVector2D(150,-150),
						  FVector2D(150,150),   FVector2D(-150,150) };
		
		H.Holes.Add(Hole);
		H.Normalize();
		
		Cases.Emplace(TEXT("WithHole"), MoveTemp(H));
	}
	
	for (const auto& Case : Cases)
	{
		FPolygonTriangulation2D Tri;
		TestTrue(*FString::Printf(TEXT("%s: triangulated"), *Case.Key),
				 Case.Value.Triangulate(Tri));
		
		// Contract 1: all triangles are CCW
		TestTrue(*FString::Printf(TEXT("%s: all triangles CCW"), *Case.Key),
				 Tri.IsAllCounterClockwise());
		
		// Contract 2: area conversation
		double Sum = 0.0;
		for (int32 T = 0; T < Tri.NumTriangles(); ++T)
		{
			Sum += Tri.SignedArea(T);   
		}
		
		const double Expected = Case.Value.Area();
		TestTrue(*FString::Printf(
			TEXT("%s: area conserved (%.2f vs %.2f)"), *Case.Key, Sum, Expected),
			FMath::IsNearlyEqual(Sum, Expected, Expected * 1e-3));
	}
	return true;
}


// ---- Inner holes tests

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPolygon2DHoleTest,
	"ProcCity.Geometry.Polygon2D.Holes", TestFlags)

bool FPolygon2DHoleTest::RunTest(const FString& Parameters)
{
	FPolygon2D P = FPolygon2D::MakeRect(
		FVector2D(0, 0), FVector2D(100, 100));
	
	FPolyRing Hole;
	// Deliberately constructed in CCW order
	Hole.Vertices = { 
		FVector2D(-20,-20), 
		FVector2D(20,-20), 
		FVector2D(20,20), 
		FVector2D(-20,20) 
	};
	
	P.Holes.Add(Hole);
	P.Normalize();
	
	TestEqual(TEXT("Hole is CW"), 
		P.Holes[0].GetWinding(), EPolyWinding::Clockwise);
	TestTrue(TEXT("Net area = 200*200 - 40*40"),
			 FMath::IsNearlyEqual(P.Area(), 40000.0 - 1600.0, 1e-6));
	TestFalse(TEXT("Point inside hole excluded"), 
		P.Contains(FVector2D(0, 0)));
	TestTrue(TEXT("Point in ring included"), 
		P.Contains(FVector2D(50, 50)));
	
	TArray<FPolyEdge> Edges;
	P.GetEdges(Edges);
	TestEqual(TEXT("Total edges = 4 + 4"), Edges.Num(), 8);
	
	return true;
}

// ---- Edge and normal tests

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPolygon2DEdgeNormalTest,
	"ProcCity.Geometry.Polygon2D.EdgeNormal", TestFlags)

bool FPolygon2DEdgeNormalTest::RunTest(const FString& Parameters)
{
	const FPolygon2D R = FPolygon2D::MakeRect(
		FVector2D::ZeroVector, FVector2D(50, 50));
	
	TArray<FPolyEdge> Edges;
	R.GetOuterEdges(Edges);
	TestEqual(TEXT("4 edges"), Edges.Num(), 4);
	
	// Critical invariants, edge normal must point to polygon external.
	for (const FPolyEdge& E : Edges)
	{
		TestTrue(TEXT("Normal is unit"), 
			FMath::IsNearlyEqual(E.Normal.Size(), 1.0, 1e-6));
		
		const FVector2D Probe = E.Midpoint() + E.Normal * 1.0;
		TestFalse(TEXT("Outward normal exits polygon"), R.Contains(Probe));
		
		const FVector2D Inward = E.Midpoint() - E.Normal * 1.0;
		TestTrue(TEXT("Inward stays inside"), R.Contains(Inward));
		
		TestTrue(TEXT("Edge length = 100"), 
			FMath::IsNearlyEqual(E.Length, 100.0, 1e-6));
	}
	return true;
}

// ---- Minimum OBB tests

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPolygon2DMinAreaOBBTest,
	"ProcCity.Geometry.Polygon2D.MinAreaOBB", TestFlags)

bool FPolygon2DMinAreaOBBTest::RunTest(const FString& Parameters)
{
	// For rotated rect, minimum OBB area should equal rect area
	for (double Yaw : {0.0, 30.0, 45.0, 72.0})
	{
		const FPolygon2D R = FPolygon2D::MakeRect(
			FVector2D(500, -300), 
			FVector2D(80, 20), 
			Yaw
		);
		
		const FOrientedBox2D OBB = R.MinAreaOBB();
		
		TestTrue(*FString::Printf(TEXT("OBB area matches @%.0f"), Yaw),
				 FMath::IsNearlyEqual(
				 	OBB.Area(), R.Area(), ProcCityGeometry::AreaTolerance));
		TestTrue(*FString::Printf(TEXT("OBB center matches @%.0f"), Yaw),
				 OBB.Center.Equals(R.Centroid(), ProcCityGeometry::PositionTolerance));
		
		TestTrue(TEXT("Long axis is X"), OBB.Extent.X >= OBB.Extent.Y);
		TestTrue(TEXT("Extent matches (80,20)"),
				 FMath::IsNearlyEqual(
					OBB.Extent.X, 80.0, ProcCityGeometry::PositionTolerance
				 ) &&
				 FMath::IsNearlyEqual(
					OBB.Extent.Y, 20.0, ProcCityGeometry::PositionTolerance
				 ));
	}
	
	// All vertices must be in OBB
	const FPolygon2D Tri(
		TArray<FVector2D>{
			FVector2D(0, 0), 
			FVector2D(300, 40), 
			FVector2D(120, 250) 
		});
	
	const FOrientedBox2D OBB = Tri.MinAreaOBB();
	for (const FVector2D& V : Tri.Outer.Vertices)
	{
		const FVector2D L = OBB.WorldToLocal(V);
		
		bool bInX = FMath::Abs(L.X) <= 
			OBB.Extent.X + ProcCityGeometry::PositionTolerance;
		bool bInY = FMath::Abs(L.Y) <=
			OBB.Extent.Y + ProcCityGeometry::PositionTolerance;
		
		TestTrue(TEXT("Vertex within OBB"),bInX && bInY);
	}
	return true;
}

// ---- Simplify function tests

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPolygon2DSimplifyCriteriaTest,
	"ProcCity.Geometry.Polygon2D.SimplifyCriteria", TestFlags)

bool FPolygon2DSimplifyCriteriaTest::RunTest(const FString& Parameters)
{
	TArray<FVector2D> V = {
		FVector2D(0, 0), 
		FVector2D(2500, 50), 
		FVector2D(5000, 0),
		FVector2D(5000, 3000), 
		FVector2D(0, 3000) 
	};
	
	FPolygon2D P;
	P.Outer.Vertices = V;
	P.Outer.EnsureWinding(EPolyWinding::CounterClockwise);
	
	FPolygon2D A = P;
	A.Simplify(FPolySimplifyParams::Cleanup());
	TestEqual(TEXT("Cleanup preserves the 50cm bump"), 
		A.Outer.NumVertices(), 5);

	FPolygon2D B = P;
	B.MergeCollinearEdges(100.0, 0.05);
	TestEqual(TEXT("Coarse merges the bump"), 
		B.Outer.NumVertices(), 4);
	
	FPolygon2D C = P;
	C.MergeCollinearEdges(100.0, 0.001);
	TestEqual(TEXT("AND semantics keeps vertex when angle fails"), 
		C.Outer.NumVertices(), 5);
	
	return true;
}


IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPolygon2DSimplifyTest,
	"ProcCity.Geometry.Polygon2D.Simplify", TestFlags)

bool FPolygon2DSimplifyTest::RunTest(const FString& Parameters)
{
	// Insert redundant point on edges
	TArray<FVector2D> Verts = {
		FVector2D(0, 0), 
		FVector2D(50, 0), 
		FVector2D(50, 0), 
		FVector2D(100, 0),
		FVector2D(100, 50), 
		FVector2D(100, 100),
		FVector2D(0, 100)
	};
	
	FPolygon2D P(Verts);  // Normalize -> Simplify is invoked in Construction
	
	TestEqual(TEXT("Reduced to 4 corners"), 
		P.Outer.NumVertices(), 4);
	
	TestTrue(TEXT("Area preserved"), 
		FMath::IsNearlyEqual(P.Area(), 10000.0, 1e-6));
	
	return true;
}

// ---- Self intersect tests

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPolygon2DSelfIntersectTest,
	"ProcCity.Geometry.Polygon2D.SelfIntersection", TestFlags)

bool FPolygon2DSelfIntersectTest::RunTest(const FString& Parameters)
{
	FPolygon2D Good = FPolygon2D::MakeRegular(
		FVector2D::ZeroVector, 100.0, 7);
	TestFalse(TEXT("Regular polygon is simple"), 
		Good.HasSelfIntersection());
	
	FPolygon2D Bow;
	Bow.Outer.Vertices = {
		FVector2D(0, 0), 
		FVector2D(100, 100), 
		FVector2D(100, 0), 
		FVector2D(0, 100) 
	};
	TestTrue(TEXT("Bowtie detected"), Bow.HasSelfIntersection());
	
	return true;
}

// ---- Triangularization tests

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPolygon2DTriangulateTest,
	"ProcCity.Geometry.Polygon2D.Triangulate", TestFlags)

bool FPolygon2DTriangulateTest::RunTest(const FString& Parameters)
{
	// No hole
	{
		const FPolygon2D P = FPolygon2D::MakeRegular(
			FVector2D(0, 0), 100.0, 6);
		
		FPolygonTriangulation2D Mesh;
		TestTrue(TEXT("Triangulate succeeded"), P.Triangulate(Mesh));
		
		// n-Gon -> n-2 triangles
		TestEqual(TEXT("Triangle Count"), 
			Mesh.NumTriangles(), P.Outer.NumVertices() - 2);
		
		// Area conversation
		double Sum = 0.0;
		for (int32 t = 0; t < Mesh.NumTriangles(); ++t)
		{
			const FVector2D& A = Mesh.Vertices[Mesh.Indices[t*3+0]];
			const FVector2D& B = Mesh.Vertices[Mesh.Indices[t*3+1]];
			const FVector2D& C = Mesh.Vertices[Mesh.Indices[t*3+2]];
			Sum += FMath::Abs((B.X-A.X)*(C.Y-A.Y) - (C.X-A.X)*(B.Y-A.Y)) * 0.5;
		}
		TestTrue(TEXT("Triangle area sums to polygon area"),
				 FMath::IsNearlyEqual(Sum, P.Area(), P.Area() * 1e-4));
		
	}
	
	// With holes
	{
		FPolygon2D P = FPolygon2D::MakeRect(
			FVector2D::ZeroVector, FVector2D(100, 100));
		
		FPolyRing Hole;
		Hole.Vertices = { 
			FVector2D(-30,-30), 
			FVector2D(30,-30),
			FVector2D(30,30),
			FVector2D(-30,30) 
		};
		P.Holes.Add(Hole);
		P.Normalize();
		
		FPolygonTriangulation2D Mesh;
		TestTrue(TEXT("Triangulate with hole"), P.Triangulate(Mesh));
		
		double Sum = 0.0;
		for (int32 t = 0; t < Mesh.NumTriangles(); ++t)
		{
			const FVector2D& A = Mesh.Vertices[Mesh.Indices[t*3+0]];
			const FVector2D& B = Mesh.Vertices[Mesh.Indices[t*3+1]];
			const FVector2D& C = Mesh.Vertices[Mesh.Indices[t*3+2]];
			Sum += FMath::Abs((B.X-A.X)*(C.Y-A.Y) - (C.X-A.X)*(B.Y-A.Y)) * 0.5;
		}
		TestTrue(TEXT("Hole excluded from triangulation"),
			FMath::IsNearlyEqual(
				Sum, P.Area(), P.Area() * 1e-3));
	}
	return true;
}

// ---- Stable hash test (golden test)
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPolygon2DStableHashTest,
	"ProcCity.Geometry.Polygon2D.StableHash", TestFlags)

bool FPolygon2DStableHashTest::RunTest(const FString& Parameters)
{
	const FPolygon2D A = FPolygon2D::MakeRect(
		FVector2D(10, 20), 
		FVector2D(5, 5), 
		30.0);
	const FPolygon2D B = FPolygon2D::MakeRect(
		FVector2D(10, 20), 
		FVector2D(5, 5), 
		30.0);
	TestEqual(TEXT("Identical polygons hash equal"), 
		A.ComputeStableHash(), B.ComputeStableHash());
	
	// Sub-tolerance perturbations should not change the hash
	FPolygon2D C = A;
	C.Outer.Vertices[0] += FVector2D(0.0001, 0.0001);
	TestEqual(TEXT("Sub-tolerance jitter ignored"), 
		A.ComputeStableHash(), C.ComputeStableHash());
	
	const FPolygon2D D = FPolygon2D::MakeRect(
		FVector2D(10, 20), 
		FVector2D(5, 6), 
		30.0);
	TestNotEqual(TEXT("Different polygons hash differently"), 
		A.ComputeStableHash(), D.ComputeStableHash());
	
	return true;
}


#endif
