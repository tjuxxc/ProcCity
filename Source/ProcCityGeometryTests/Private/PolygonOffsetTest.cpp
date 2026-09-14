#include "Misc/AutomationTest.h"
#include "ProcCityGeometry/PolygonOffset.h"

#if WITH_DEV_AUTOMATION_TESTS

using namespace ProcCityGeometry;

namespace
{
	namespace
	{
		constexpr EAutomationTestFlags TestFlags =
			EAutomationTestFlags::EditorContext |
			EAutomationTestFlags::CommandletContext |
			EAutomationTestFlags::ClientContext |
			EAutomationTestFlags::EngineFilter;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOffsetRectTest,
	"ProcCity.Geometry.Offset.Rect", TestFlags)

// ----- Polygon offset tests

bool FOffsetRectTest::RunTest(const FString& Parameters)
{
	const FPolygon2D R = FPolygon2D::MakeRect(
		FVector2D(0, 0), 
		FVector2D(500, 300));
	
	// Inset 100 -> 800 x 400
	FPolygon2D MiterIn;
	TestTrue(TEXT("Inset ok"), OffsetPolygonSingle(
				R, FPolygonOffsetParams::Inset(100.0), MiterIn));
	
	TestTrue(TEXT("Inset area = 800*400"), 
		FMath::IsNearlyEqual(
			MiterIn.Area(), 800.0 * 400.0, R.Area() * 1e-5));
	AddInfo(FString::Printf(
		TEXT("Area after miter inset: %.3f."), MiterIn.Area()));
	
	TestEqual(TEXT("Inset stays CCW"), 
		MiterIn.Outer.GetWinding(), 
		EPolyWinding::CounterClockwise);
	
	TestTrue(TEXT("Inset contained in original"), 
		R.Contains(MiterIn));
	
	// Outset 100 (miter) -> 1200 x 800
	FPolygon2D MiterOut;
	TestTrue(TEXT("Outset ok"), OffsetPolygonSingle(
			R, FPolygonOffsetParams::Outset(100.0), MiterOut));
	
	TestTrue(TEXT("Outset area = 1200*800"), 
		FMath::IsNearlyEqual(
			MiterOut.Area(), 1200.0 * 800.0, R.Area() * 1e-5));
	AddInfo(FString::Printf(
		TEXT("Area after miter outset: %.3f."), MiterOut.Area()));
	
	TestEqual(TEXT("Outset stays CCW"), 
		MiterOut.Outer.GetWinding(), 
		EPolyWinding::CounterClockwise);
	
	TestTrue(TEXT("Original contained in outset"), 
		MiterOut.Contains(R));
	
	// Area[Round outset] should lie between Area[Square/Beveled] and Area(Miter)
	{
		FPolygon2D Round;
		TestTrue(TEXT("Round outset ok"),
			OffsetPolygonSingle(
			R, FPolygonOffsetParams::Rounded(100.0, 1.0), 
			Round));

		TestTrue(TEXT("Round area < miter area"), 
			Round.Area() < MiterOut.Area());
		
		TestTrue(TEXT("Round area > original"), 
			Round.Area() > R.Area());
		
		AddInfo(FString::Printf(
			TEXT("Area after round outset: %.3f."), Round.Area()));
	}
	
	return true;
}

// Excessive inward offset must cleanly return 0 results, rather than 
// crashing or returning an inverted polygon
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOffsetVanishTest,
	"ProcCity.Geometry.Offset.Vanish", TestFlags)

bool FOffsetVanishTest::RunTest(const FString& Parameters)
{
	const FPolygon2D R = FPolygon2D::MakeRect(
		FVector2D::ZeroVector, 
		FVector2D(100, 100));
	
	TArray<FPolygon2D> Out;
	TestFalse(TEXT("Over-inset yields nothing"),
			  OffsetPolygon(
			  	R, FPolygonOffsetParams::Inset(200.0), Out));
	
	TestEqual(TEXT("Result array empty"), Out.Num(), 0);
	return true;
}

// Key point: an inward-offset dumbbell shape splits into two pieces 
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOffsetSplitTest,
	"ProcCity.Geometry.Offset.Dumbbell", TestFlags)

bool FOffsetSplitTest::RunTest(const FString& Parameters)
{
	// Two 400x400 squares connected by a 40-wide narrow neck
	FPolygon2D Dumbbell;
	Dumbbell.Outer.Vertices = {
		FVector2D(-500,-200), FVector2D(-100,-200), 
		FVector2D(-100,-20),  FVector2D( 100, -20), 
		FVector2D( 100,-200), FVector2D( 500,-200),
		FVector2D( 500, 200), FVector2D( 100, 200), 
		FVector2D( 100,  20), FVector2D(-100,  20), 
		FVector2D(-100, 200), FVector2D(-500, 200) 
	};
	
	Dumbbell.Normalize();
	TestTrue(TEXT("Dumbbell valid"), Dumbbell.IsValid());
	
	TArray<FPolygon2D> Parts;
	TestTrue(TEXT("Inset ok"), OffsetPolygon(Dumbbell, 
			FPolygonOffsetParams::Inset(40.0), Parts));
	TestEqual(TEXT("Neck severed -> 2 pieces"), 
		Parts.Num(), 2);
	TestTrue(TEXT("Sorted by area descending"), 
		Parts[0].Area() >= Parts[1].Area());
	
	for (const FPolygon2D& P : Parts)
	{
		TestEqual(TEXT("Piece is CCW"), 
			P.Outer.GetWinding(), 
			EPolyWinding::CounterClockwise);
	}
	
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOffsetHoleTest,
	"ProcCity.Geometry.Offset.Hole", TestFlags)

bool FOffsetHoleTest::RunTest(const FString& Parameters)
{
	FPolygon2D P = FPolygon2D::MakeRect(FVector2D::ZeroVector, 
		FVector2D(500, 500));
	FPolyRing Hole;
	Hole.Vertices = { 
		FVector2D(-200,-200), FVector2D(200,-200),
		FVector2D(200,200),   FVector2D(-200,200) };
	
	P.Holes.Add(Hole);
	P.Normalize();
	
	FPolygon2D In;
	TestTrue(TEXT("Inset with hole"), OffsetPolygonSingle(
			P, FPolygonOffsetParams::Inset(50.0), In));
	TestEqual(TEXT("Hole preserved"), In.Holes.Num(), 1);
	// Inset decreases outer, enlarge hole
	TestTrue(TEXT("Area shrinks"), In.Area() < P.Area());
	TestEqual(TEXT("Area equal expected"), In.Area(),
		900.0 * 900.0 - 500.0 * 500.0, P.Area() * 1e-5);
	
	AddInfo(FString::Printf(
		TEXT("Area after inset: %.3f."), In.Area()));
	
	TestEqual(TEXT("Hole still CW"), 
		In.Holes[0].GetWinding(), EPolyWinding::Clockwise);
	
	return true;
}

// ----- Boolean tests

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBooleanTest,
	"ProcCity.Geometry.Offset.Boolean", TestFlags)

bool FBooleanTest::RunTest(const FString& Parameters)
{
	const FPolygon2D A = FPolygon2D::MakeRect(
		FVector2D(0, 0),   
		FVector2D(200, 200));
	const FPolygon2D B = FPolygon2D::MakeRect(
		FVector2D(200, 0), 
		FVector2D(200, 200));
	
	TArray<FPolygon2D> OutArray;
	{
		TestTrue(TEXT("Union"), 
			UnionPolygons(MakeArrayView(&A, 1), 
				MakeArrayView(&B, 1), OutArray));
		TestEqual(TEXT("Union -> 1 piece"), OutArray.Num(), 1);
		
		TestTrue(TEXT("Union area"), 
			FMath::IsNearlyEqual(OutArray[0].Area(), 
				600.0 * 400.0, A.Area() * 1e-5));
		
		TestTrue(TEXT("Intersect"), IntersectPolygons(
			MakeArrayView(&A,1), 
			MakeArrayView(&B,1), OutArray));
		TestTrue(TEXT("Intersect area = 200*400"), 
			FMath::IsNearlyEqual(OutArray[0].Area(), 
				200.0*400.0, 10.0));
		
		TestTrue(TEXT("Subtract"), SubtractPolygons(
			MakeArrayView(&A,1), 
			MakeArrayView(&B,1), 
			OutArray));
		
		TestTrue(TEXT("Diff area = 200*400"), 
			FMath::IsNearlyEqual(OutArray[0].Area(), 
				200.0*400.0, 10.0));
	}
	
	{
		// Subtract small rect from big produces a hole
		const FPolygon2D Big = FPolygon2D::MakeRect(
			FVector2D::ZeroVector, 
			FVector2D(500, 500));
		const FPolygon2D Small = FPolygon2D::MakeRect(
			FVector2D::ZeroVector, 
			FVector2D(100, 100));
		
		TestTrue(TEXT("Punch hole"), SubtractPolygons(
			MakeArrayView(&Big,1), 
			MakeArrayView(&Small,1), OutArray));
		TestEqual(TEXT("One piece with a hole"), OutArray.Num(), 1);
		TestEqual(TEXT("Hole count"), OutArray[0].Holes.Num(), 1);
		
		TestTrue(TEXT("Net area"), 
			FMath::IsNearlyEqual(OutArray[0].Area(), 
				1000.0*1000.0 - 200.0*200.0, Big.Area() * 1e-5));
		
		// Small subtract big -> Empty
		TestFalse(TEXT("Small subtract big"), SubtractPolygons(
			MakeArrayView(&Small,1), 
			MakeArrayView(&Big,1), OutArray));
		
		TestEqual(TEXT("Result array empty"), OutArray.Num(), 0);
	}
	return true;
}

// ----- Split tests

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSplitTest,
	"ProcCity.Geometry.Offset.Split.Line", TestFlags)

bool FSplitTest::RunTest(const FString& Parameters)
{
	const FPolygon2D R = FPolygon2D::MakeRect(
		FVector2D::ZeroVector, 
		FVector2D(600, 200));
	
	TArray<FPolygon2D> Pos, Neg;
	{
		TestTrue(TEXT("Split by X=0"), 
		SplitByLine(R, 
			FVector2D(1, 0), 
			100, 
			Pos, Neg));
	
		TestEqual(TEXT("Positive side 1 piece"), Pos.Num(), 1);
		TestEqual(TEXT("Negative side 1 piece"), Neg.Num(), 1);
	
		TestTrue(TEXT("Halves sum to whole"),
			FMath::IsNearlyEqual(Pos[0].Area() + Neg[0].Area(), 
			R.Area(), R.Area() * 1e-5));
	
		TestTrue(TEXT("Positive is at right side"), 
			Pos[0].Centroid().X > Neg[0].Centroid().X);
		
		AddInfo(FString::Printf(
			TEXT("Pos Area: %.3f. Neg Area: %.3f. Total Area: %.3f."), 
				Pos[0].Area(), Neg[0].Area(), R.Area()));
		
		TestTrue(TEXT("Halves sum to whole"),
			FMath::IsNearlyEqual(Pos[0].Area() + Neg[0].Area(), 
			R.Area(), R.Area() * 1e-5));
		bool bAHasCorrectArea = FMath::IsNearlyEqual(
			Pos[0].Area(), 500.0 * 400.0, R.Area() * 1e-5);
		bool bBHasCorrectArea = FMath::IsNearlyEqual(
			Neg[0].Area(), 700.0 * 400.0, R.Area() * 1e-5);
		
		TestTrue(TEXT("Each part has correct area (SplitByLine)"), 
			bAHasCorrectArea && bBHasCorrectArea);
	}
	
	// Cut along longest axis, obtain two 600 x 400 rects
	{
		TestTrue(TEXT("Split along longest axis"), 
			SplitAlongLongestAxis(R, 0.6, Pos, Neg));
	
		TestTrue(TEXT("Area conserved"),
				 FMath::IsNearlyEqual(Pos[0].Area() + Neg[0].Area(), 
					 R.Area(), R.Area() * 1e-5));
	
		bool bAHasCorrectArea = FMath::IsNearlyEqual(
			Pos[0].Area(), R.Area() * 0.4, R.Area() * 1e-5);
		bool bBHasCorrectArea = FMath::IsNearlyEqual(
			Neg[0].Area(), R.Area() * 0.6, R.Area() * 1e-5);

		TestTrue(TEXT("Each part has correct area (SplitAlongLongestAxis)"), 
			bAHasCorrectArea && bBHasCorrectArea);
	}
	
	return true;
}


// For city scale, polygon is far from origin.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSplitFarFromOriginTest,
	"ProcCity.Geometry.Offset.Split.FarFromOrigin", TestFlags)

bool FSplitFarFromOriginTest::RunTest(const FString& Parameters)
{
	const FVector2D Far(500000.0, -300000.0);
	const FPolygon2D R = FPolygon2D::MakeRect(
		Far, FVector2D(600, 200), 33.0);
	
	for (double AngleDeg : {0.0, 33.0, 77.0, 145.0, -60.0})
	{
		const double Ang = FMath::DegreesToRadians(AngleDeg);
		const FVector2D N(FMath::Cos(Ang), FMath::Sin(Ang));
		
		// Splitting line passes centroid
		const double Offset = FVector2D::DotProduct(N, R.Centroid());
		
		TArray<FPolygon2D> Pos, Neg;
		TestTrue(*FString::Printf(TEXT("Split succeeds @%.0f"), AngleDeg),
			SplitByLine(R, N, Offset, Pos, Neg));
		TestTrue(*FString::Printf(TEXT("Both sides non-empty @%.0f"), AngleDeg),
				 Pos.Num() > 0 && Neg.Num() > 0);
		
		double Sum = 0.0;
		for (const FPolygon2D& P : Pos) { Sum += P.Area(); }
		for (const FPolygon2D& P : Neg) { Sum += P.Area(); }
		
		TestTrue(*FString::Printf(
			TEXT("Area conserved @%.0f (%.1f vs %.1f)"), AngleDeg, Sum, R.Area()), 
			FMath::IsNearlyEqual(Sum, R.Area(), R.Area() * 1e-3));
		
		// Unnormalized normal should give the same result
		TArray<FPolygon2D> Pos2, Neg2;
		SplitByLine(R, N * 7.3, Offset * 7.3, 
			Pos2, Neg2);
		
		bool bSamePositiveArea = FMath::IsNearlyEqual(
			Pos2[0].Area(), Pos[0].Area(), R.Area() * 1e-3);
		bool bSameNegativeArea = FMath::IsNearlyEqual(
			Neg2[0].Area(), Neg[0].Area(), R.Area() * 1e-3);
		
		TestTrue(TEXT("Unnormalized normal equivalent"),
				 Pos2.Num() == Pos.Num() 
				 && bSamePositiveArea 
				 && bSameNegativeArea);
	}
	
	// Per-edge inset should also work (city scale, far from origin)
	constexpr double Insets[4] = { 50.0, 0.0, 80.0, 20.0 };
	TArray<FPolygon2D> Out;
	TestTrue(TEXT("Per-edge inset far from origin"),
			 InsetPolygonPerEdge(R, 
			 	MakeArrayView(Insets, 4), 
			 	100.0, Out));
	
	TestTrue(TEXT("Result smaller but non-empty"),
			 Out.Num() == 1 
			 && Out[0].Area() < R.Area() 
			 && Out[0].Area() > 0.0);

	return true;
}

// Check the case splitting line does not path through the polygon
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSplitNoIntersectionTest,
	"ProcCity.Geometry.Offset.Split.NoIntersection", TestFlags)

bool FSplitNoIntersectionTest::RunTest(const FString& Parameters)
{
	const FPolygon2D R = FPolygon2D::MakeRect(
		FVector2D(1000, 1000), 
		FVector2D(200, 200));
	
	// Whole polygon is at positive side of splitting line
	{
		TArray<FPolygon2D> Pos, Neg;
		TestFalse(TEXT("No real split"), SplitByLine(R, 
			FVector2D(1, 0), 0.0, 
			Pos, Neg));
		TestEqual(TEXT("Whole polygon on positive side"), Pos.Num(), 1);
		TestEqual(TEXT("Negative side empty"), Neg.Num(), 0);
		TestTrue(TEXT("Area intact (positive)"), 
			FMath::IsNearlyEqual(
				Pos[0].Area(), R.Area(), R.Area() * 1e-5));
	}
	
	// The opposite case
	{
		TArray<FPolygon2D> Pos, Neg;
		TestFalse(TEXT("No real split"), 
			SplitByLine(R, FVector2D(1, 0), 
				5000.0, Pos, Neg));
		TestEqual(TEXT("Positive side empty"), Pos.Num(), 0);
		TestEqual(TEXT("Whole polygon on negative side"), Neg.Num(), 1);
		TestTrue(TEXT("Area intact (negative)"), 
			FMath::IsNearlyEqual(
				Neg[0].Area(), R.Area(), R.Area() * 1e-5));
	}
	return true;
}

// ----- Per edge inset tests

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPerEdgeInsetTest,
	"ProcCity.Geometry.Offset.PerEdge", TestFlags)

bool FPerEdgeInsetTest::RunTest(const FString& Parameters)
{
	const FPolygon2D R = FPolygon2D::MakeRect(
		FVector2D::ZeroVector, 
		FVector2D(500, 500));
	
	// bottom(0), right(1), top(2), left(3)
	constexpr double Insets[4] = { 100.0, 0.0, 200.0, 50.0 };
	
	TArray<FPolygon2D> Out;
	TestTrue(TEXT("Per-edge inset ok"),
			 InsetPolygonPerEdge(
			 	R, MakeArrayView(Insets, 4), 
			 	100.0, Out));
	TestEqual(TEXT("Single piece"), Out.Num(), 1);

	const FBox2D B = Out[0].Bounds();
	TestTrue(TEXT("Bottom inset 100"),  
		FMath::IsNearlyEqual(B.Min.Y, -400.0, B.GetExtent().Y * 1e-4));
	TestTrue(TEXT("Right inset 0"),     
		FMath::IsNearlyEqual(B.Max.X,  500.0, B.GetExtent().X * 1e-4));
	TestTrue(TEXT("Top inset 200"),   
		FMath::IsNearlyEqual(B.Max.Y,  300.0, B.GetExtent().Y * 1e-4));
	TestTrue(TEXT("Left inset 50"),    
		FMath::IsNearlyEqual(B.Min.X, -450.0, B.GetExtent().X * 1e-4));
	
	// Excessive setback -> empty
	constexpr double Huge[4] = { 600.0, 600.0, 600.0, 600.0 };
	TArray<FPolygon2D> Empty;
	TestFalse(TEXT("Excessive setback results in empty"),
			  InsetPolygonPerEdge(R, 
			  	MakeArrayView(Huge, 4), 
			  	100.0, Empty));
	
	return true;
}




#endif
