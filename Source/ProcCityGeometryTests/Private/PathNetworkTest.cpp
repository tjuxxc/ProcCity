#include "Misc/AutomationTest.h"
#include "ProcCityGeometry/PathNetwork.h"
#include "ProcCityGeometry/SpatialIndex.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	constexpr EAutomationTestFlags TestFlags =
		EAutomationTestFlags::EditorContext |
		EAutomationTestFlags::CommandletContext |
		EAutomationTestFlags::ClientContext |
		EAutomationTestFlags::EngineFilter;
	
	/** N x M grid of streets spanning [0, (N-1)*Spacing] in both axes. */
	void BuildGrid(FPathNetwork& Net, int32 NX, int32 NY, double Spacing, 
		double HalfWidth = 400.0, double Sidewalk = 200.0)
	{
		const double MaxX = (NX - 1) * Spacing;
		const double MaxY = (NY - 1) * Spacing;
		for (int32 i = 0; i < NX; ++i)
		{
			const double X = i * Spacing;
			Net.AddPath(FPathCurve::MakeLine(FVector2D(X, 0), FVector2D(X, MaxY)),
						EPathClass::Local, HalfWidth, Sidewalk);
		}
		for (int32 j = 0; j < NY; ++j)
		{
			const double Y = j * Spacing;
			Net.AddPath(FPathCurve::MakeLine(FVector2D(0, Y), FVector2D(MaxX, Y)),
						EPathClass::Local, HalfWidth, Sidewalk);
		}
	}
}

// ------------- Spatial index ------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSpatialGridTest,
	"ProcCity.Geometry.SpatialGrid.Basic", TestFlags)

bool FSpatialGridTest::RunTest(const FString& Parameters)
{
	FSpatialGrid2D Grid;
	Grid.Reset(100.0);
	
	for (int32 i = 0; i < 10; ++i)
	{
		Grid.Insert(FVector2D(i * 50.0, 0.0));
	}
	TestEqual(TEXT("10 Points"), Grid.Num(), 10);
	
	TArray<int32> Found;
	Grid.QueryRadius(FVector2D(100.0, 0.0), 60.0, Found);
	TestEqual(TEXT("3 in radius"), Found.Num(), 3);
	TestTrue(TEXT("Results ascending"),
				 Found[0] < Found[1] && Found[1] < Found[2]);
	
	TestEqual(TEXT("Nearest"), Grid.FindNearest(
		FVector2D(103.0, 0.0), 50.0), 2);
	TestEqual(TEXT("Nothing in range"), Grid.FindNearest(
		FVector2D(10000, 0), 10.0), INDEX_NONE);
	
	// Welding
	bool bInserted = false;
	const int32 Welded = Grid.InsertUnique(
		FVector2D(100.5, 0.0), 5.0, bInserted);
	TestFalse(TEXT("Welded, not inserted"), bInserted);
	TestEqual(TEXT("Welded to existing index"), Welded, 2);
	TestEqual(TEXT("Count unchanged"), Grid.Num(), 10);
	
	const int32 Fresh = Grid.InsertUnique(
		FVector2D(-9999, -9999), 5.0, bInserted);
	TestTrue(TEXT("Far point inserted"), bInserted);
	TestEqual(TEXT("New index"), Fresh, 10);
	
	// Far from origin: grid must not assume anything about absolute position
	FSpatialGrid2D Far;
	Far.Reset(100.0);
	const FVector2D Base(1.0e6, -7.5e5);
	Far.Insert(Base);
	Far.Insert(Base + FVector2D(30, 0));
	Far.QueryRadius(Base, 50.0, Found);
	TestEqual(TEXT("Far-from-origin query works"), Found.Num(), 2);
	
	return true;
}

// ------------- Planarization ------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPathNetworkCrossTest,
	"ProcCity.Geometry.PathNetwork.Planarize", TestFlags)

bool FPathNetworkCrossTest::RunTest(const FString& Parameters)
{
	// Two crossing lines must produce a node at the intersection
	FPathNetwork Net;
	Net.AddPath(FPathCurve::MakeLine(FVector2D(-1000, 0), FVector2D(1000, 0)),
				EPathClass::Arterial, 500.0, 200.0);
	Net.AddPath(FPathCurve::MakeLine(FVector2D(0, -1000), FVector2D(0, 1000)),
				EPathClass::Arterial, 500.0, 200.0);
	
	FPathNetworkBuildParams Params;
	Params.bPruneDeadEnds = false; // keep the four spurs so we can count them
	TestTrue(TEXT("Build Success"), Net.Build(Params));
	
	// 4 endpoints + 1 centre
	TestEqual(TEXT("5 nodes"), Net.GetNodes().Num(), 5);
	TestEqual(TEXT("4 edges"), Net.GetEdges().Num(), 4);
	
	FString Error;
	TestTrue(*FString::Printf(TEXT("Topology valid: %s"), *Error),
			 Net.ValidateTopology(&Error));
	
	// The centre node must have degree 4 and its incident edges sorted CCW
	int32 CentreIdx = INDEX_NONE;
	for (int32 i = 0; i < Net.GetNodes().Num(); ++i)
	{
		if (Net.GetNodes()[i].Position.IsNearlyZero(0.1))
		{
			CentreIdx = i; 
			break;
		}
	}
	TestNotEqual(TEXT("Centre node exists"), 
		CentreIdx, static_cast<int32>(INDEX_NONE));
	TestEqual(TEXT("Centre degree 4"), 
		Net.GetNodes()[CentreIdx].Degree(), 4);
	TestTrue(TEXT("Centre is intersection"), 
		Net.GetNodes()[CentreIdx].IsIntersection());
	
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPathNetworkPruneTest,
	"ProcCity.Geometry.PathNetwork.PruneDeadEnds", TestFlags)

bool FPathNetworkPruneTest::RunTest(const FString& Parameters)
{
	// A closed square plus a spur sticking out; the spur must be pruned
	FPathNetwork Net;
	const TArray<FVector2D> Square = {
		FVector2D(0,0), FVector2D(1000,0), 
		FVector2D(1000,1000), FVector2D(0,1000) };
	
	Net.AddPath(FPathCurve::MakePolyline(Square, /*bClosed=*/true),
				EPathClass::Local, 300.0, 150.0);
	Net.AddPath(FPathCurve::MakeLine(FVector2D(1000, 500), FVector2D(1800, 500)),
				EPathClass::Local, 300.0, 150.0);
	
	TestTrue(TEXT("Build"), Net.Build());
	
	int32 DeadEndCount = 0;
	for (const FPathNode& N : Net.GetNodes())
	{
		if (N.Degree() == 0)
		{
			DeadEndCount += 1;
		}
		TestFalse(TEXT("No dead ends remain"), N.Degree() == 1);
	}
	
	// The dead end is still in Net.Nodes, but its degree reduces to 0.
	TestTrue("One dead end found. Its degree reduces to 0", DeadEndCount == 1);
	TestEqual("Edge Count = 5", Net.GetEdges().Num(), 5);
	
	return true;
}

// ------------- Face extraction ------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPathNetworkSingleFaceTest,
	"ProcCity.Geometry.PathNetwork.SingleFace", TestFlags)

bool FPathNetworkSingleFaceTest::RunTest(const FString& Parameters)
{
	FPathNetwork Net;
	const TArray<FVector2D> Square = {
		FVector2D(0,0), FVector2D(2000,0), 
		FVector2D(2000,2000), FVector2D(0,2000) };
	
	Net.AddPath(FPathCurve::MakePolyline(Square, true),
		EPathClass::Local, 300.0, 100.0);
	TestTrue(TEXT("Build"), Net.Build());
	
	TArray<FPathFace> Faces;
	TestTrue(TEXT("Extract faces"), Net.ExtractFaces(Faces));
	
	// Exactly one bounded + one unbounded
	int32 Bounded = 0, Outer = 0;
	
	for (const FPathFace& Face : Faces)
	{
		Face.bIsOuterFace? ++Outer : ++Bounded;
	}
	TestEqual(TEXT("1 bounded face"), Bounded, 1);
	TestEqual(TEXT("1 outer face"), Outer, 1);
	
	for (const FPathFace& F : Faces)
	{
		TestTrue(TEXT("Face area = 2000^2"),
		FMath::IsNearlyEqual(F.Area, 2000.0 * 2000.0, 1.0));
		
		TestEqual(TEXT("Face is CCW"),
				F.Shape.Outer.GetWinding(), EPolyWinding::CounterClockwise);
		TestEqual(TEXT("BoundaryEdges parallel to vertices"),
						  F.BoundaryEdges.Num(), 
						  F.Shape.Outer.NumVertices());
		TestEqual(TEXT("BoundaryNodes parallel to vertices"),
				  F.BoundaryNodes.Num(),
				  F.Shape.Outer.NumVertices());
		
		// Each boundary edge must actually connect consecutive boundary nodes
		const int32 N = F.BoundaryNodes.Num();
		for (int32 i = 0; i < N; ++i)
		{
			const FPathEdge& E = Net.GetEdges()[F.BoundaryEdges[i]];
			const int32 V0 = F.BoundaryNodes[i];
			const int32 V1 = F.BoundaryNodes[(i + 1) % N];
			const bool bMatches = (E.NodeA == V0 && E.NodeB == V1) ||
								  (E.NodeB == V0 && E.NodeA == V1);
			TestTrue(*FString::Printf(TEXT("Edge %d connects nodes %d-%d"), i, V0, V1), 
					 bMatches);
		}
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPathNetworkGridFacesTest,
	"ProcCity.Geometry.PathNetwork.GridFaces", TestFlags)

bool FPathNetworkGridFacesTest::RunTest(const FString& Parameters)
{
	// 4x4 street grid -> 3x3 = 9 bounded faces
	FPathNetwork Net;
	BuildGrid(Net, 4, 4, 10000.0);
	TestTrue(TEXT("Build"), Net.Build());
	
	FString Error;
	TestTrue(*FString::Printf(TEXT("Topology: %s"), *Error), 
		Net.ValidateTopology(&Error));
	TestEqual(TEXT("16 nodes"), Net.GetNodes().Num(), 16);
	TestEqual(TEXT("24 edges"), Net.GetEdges().Num(), 24);
	
	TArray<FPathFace> Faces;
	TestTrue(TEXT("Extract faces"), Net.ExtractFaces(Faces));
	
	int32 Bounded = 0;
	double TotalArea = 0.0;
	for (const FPathFace& F : Faces)
	{
		if (F.bIsOuterFace) { continue; }
		++Bounded;
		TotalArea += F.Area;
		TestTrue(TEXT("Cell area = 100m x 100m"),
				 FMath::IsNearlyEqual(
				 	F.Area, 10000.0 * 10000.0, 1.0));
		TestEqual(TEXT("Quad face"), 
			F.Shape.Outer.NumVertices(), 4);
	}
	TestEqual(TEXT("9 bounded faces"), Bounded, 9);
	TestTrue(TEXT("Faces tile the grid"),
			 FMath::IsNearlyEqual(
			 	TotalArea, 30000.0 * 30000.0, 1.0));
	
	// Euler check: V - E + F = 2 for a connected planar 
	// graph (F counts the outer face)
	const int32 V = Net.GetNodes().Num();
	const int32 E = Net.GetEdges().Num();
	const int32 F = Faces.Num();
	TestEqual(TEXT("Euler V-E+F=2"), V - E + F, 2);
	
	return true;
}

// Mixed road classes: wide arterials must eat more land than local streets
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPathNetworkMixedClassTest,
	"ProcCity.Geometry.PathNetwork.MixedClass", TestFlags)

bool FPathNetworkMixedClassTest::RunTest(const FString& Parameters)
{
	FPathNetwork Net;
	// Left/bottom are arterials (wide), right/top are local (narrow)
	Net.AddPath(FPathCurve::MakeLine(
		FVector2D(0, 0), FVector2D(0, 10000)),
				EPathClass::Arterial, 1200.0, 300.0);
	Net.AddPath(FPathCurve::MakeLine(
		FVector2D(0, 0), FVector2D(10000, 0)),
				EPathClass::Arterial, 1200.0, 300.0);
	Net.AddPath(FPathCurve::MakeLine(
		FVector2D(10000, 0), FVector2D(10000, 10000)),
				EPathClass::Local, 300.0, 100.0);
	Net.AddPath(FPathCurve::MakeLine(
		FVector2D(0, 10000), FVector2D(10000, 10000)),
				EPathClass::Local, 300.0, 100.0);
	
	TestTrue(TEXT("Build"), Net.Build());
	
	TArray<FPolygon2D> Blocks;
	TArray<FPathFace> Faces;
	TestTrue(TEXT("Extract"), 
		Net.ExtractBlockPolygons(10000.0, Blocks, &Faces));
	TestEqual(TEXT("1 block"), Blocks.Num(), 1);
	
	const FBox2D B = Blocks[0].Bounds();
	// Arterial side: 1200 + 300 = 1500 ; local side: 300 + 100 = 400
	TestTrue(TEXT("Left inset 1500"),  
		FMath::IsNearlyEqual(B.Min.X, 1500.0, 1.0));
	TestTrue(TEXT("Bottom inset 1500"),
		FMath::IsNearlyEqual(B.Min.Y, 1500.0, 1.0));
	TestTrue(TEXT("Right inset 400"),  
		FMath::IsNearlyEqual(B.Max.X, 9600.0, 1.0));
	TestTrue(TEXT("Top inset 400"),    
		FMath::IsNearlyEqual(B.Max.Y, 9600.0, 1.0));
	
	// Frontage recovery: every boundary edge must expose its road class
	bool bFoundArterial = false;
	bool bFoundLocal = false;
	for (int32 EdgeIdx : Faces[0].BoundaryEdges)
	{
		const EPathClass C = Net.GetEdges()[EdgeIdx].Class;
		bFoundArterial |= (C == EPathClass::Arterial);
		bFoundLocal    |= (C == EPathClass::Local);
	}
	TestTrue(TEXT("Arterial frontage recoverable"), bFoundArterial);
	TestTrue(TEXT("Local frontage recoverable"), bFoundLocal);
	
	return true;
}

// Curved arterials: the whole point of Part 2's FPathCurve
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPathNetworkCurvedTest,
	"ProcCity.Geometry.PathNetwork.CurvedRoads", TestFlags)

bool FPathNetworkCurvedTest::RunTest(const FString& Parameters)
{
	FPathNetwork Net;
	// A curved boulevard crossing a straight street
	TArray<FVector2D> Curve;
	constexpr int32 NumVerts = 8.0;
	for (int32 i = 0; i <= NumVerts; ++i)
	{
		const double T = i / static_cast<double>(NumVerts);
		Curve.Add(FVector2D(
			T * 20000.0, FMath::Sin(T * PI) * 4000.0));
	}
	
	Net.AddPath(FPathCurve::MakeSpline(Curve), 
		EPathClass::Arterial, 800.0, 300.0);
	Net.AddPath(FPathCurve::MakeLine(
		FVector2D(10000, -5000), FVector2D(10000, 10000)),
				EPathClass::Local, 400.0, 200.0);
	
	FPathNetworkBuildParams Params;
	Params.CurveTessellationError = 100.0;
	Params.bPruneDeadEnds = false;
	TestTrue(TEXT("Build"), Net.Build(Params));
	
	FString Error;
	TestTrue(*FString::Printf(TEXT("No crossings without nodes: %s"), *Error),
			 Net.ValidateTopology(&Error));
	
	// The straight street must have been split by the curve
	bool bHasDegree4 = false;
	for (const FPathNode& N : Net.GetNodes())
	{
		bHasDegree4 |= (N.Degree() == 4);
	}
	TestTrue(TEXT("Curve/street intersection created a degree-4 node"), bHasDegree4);
	
	return true;
}

// Determinism: identical inputs must hash identically regardless of run
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPathNetworkDeterminismTest,
	"ProcCity.Geometry.PathNetwork.Determinism", TestFlags)

bool FPathNetworkDeterminismTest::RunTest(const FString& Parameters)
{
	uint64 First = 0;
	for (int32 Run = 0; Run < 3; ++Run)
	{
		FPathNetwork Net;
		BuildGrid(Net, 4, 4, 8000.0);
		TestTrue(TEXT("Build"), Net.Build());
		
		const uint64 H = Net.ComputeStableHash();
		if (Run == 0)
		{
			First = H;
		}
		else
		{
			TestEqual(TEXT("Hash stable across runs"), H, First);
		}
		
		TArray<FPolygon2D> Blocks;
		Net.ExtractBlockPolygons(10000.0, Blocks);
		TestEqual(TEXT("Block count stable"), Blocks.Num(), 9);
	}
	
	// Far from origin must work identically
	FPathNetwork Far;
	const FVector2D Offset(2.0e6, -1.5e6);
	
	for (int32 i = 0; i < 3; ++i)
	{
		const double X = Offset.X + i * 10000.0;
		Far.AddPath(FPathCurve::MakeLine(FVector2D(X, Offset.Y), 
			FVector2D(X, Offset.Y + 20000.0)), 
			EPathClass::Local, 400.0, 200.0);
		
		const double Y = Offset.Y + i * 10000.0;
		Far.AddPath(FPathCurve::MakeLine(FVector2D(Offset.X, Y),
			FVector2D(Offset.X + 20000.0, Y)),
			EPathClass::Local, 400.0, 200.0);
	}
	TestTrue(TEXT("Far build"), Far.Build());
	TArray<FPolygon2D> FarBlocks;
	TestTrue(TEXT("Far blocks extracted"), 
		Far.ExtractBlockPolygons(10000.0, FarBlocks));
	TestEqual(TEXT("4 far blocks"), FarBlocks.Num(), 4);
	
	return true;
}

#endif
