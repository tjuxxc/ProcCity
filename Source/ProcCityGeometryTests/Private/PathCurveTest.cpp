#include "Misc/AutomationTest.h"
#include "ProcCityGeometry/PathCurve.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	constexpr EAutomationTestFlags TestFlags =
		EAutomationTestFlags::EditorContext |
		EAutomationTestFlags::CommandletContext |
		EAutomationTestFlags::ClientContext |
		EAutomationTestFlags::EngineFilter;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPathCurveLineTest,
	"ProcCity.Geometry.PathCurve.Line", TestFlags)

bool FPathCurveLineTest::RunTest(const FString& Parameters)
{
	const FPathCurve C = FPathCurve::MakeLine(
		FVector2D(0, 0), FVector2D(1000, 0));
	
	const double Len = C.GetLength();
	TestTrue( TEXT("Length = 1000"), 
		FMath::IsNearlyEqual(Len, 1000.0, 0.1));
	
	const FPathSample M = C.EvalAtDistance(500.0);
	TestTrue(TEXT("Midpoint"), 
		M.Position.Equals(FVector2D(500.0, 0.0), Len * 1e-4));
	
	TestTrue(TEXT("Tangent +X"), 
		M.Tangent.Equals(FVector2D(1, 0), 1e-6));
	
	// Left normal points to Y+ if forward is X+.
	TestTrue(TEXT("LeftNormal +Y"), 
		M.LeftNormal.Equals(FVector2D(0, 1), 1e-6));
	
	// Clamp behavior
	TestTrue(TEXT("Clamp start"), 
		C.EvalAtDistance(-100.0).Position.Equals(
			FVector2D(0,0), Len * 1e-4));
	TestTrue(TEXT("Clamp end"),   
		C.EvalAtDistance(9999.0).Position.Equals(
			FVector2D(1000,0), Len * 1e-4));
	
	return true;
}

// Core: arc-length parameterization must be uniform —— this is the 
// prerequisite for evenly spaced placement of streetlights / street trees
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPathCurveArcLengthTest,
	"ProcCity.Geometry.PathCurve.ArcLength", TestFlags)

bool FPathCurveArcLengthTest::RunTest(const FString& Parameters)
{
	// Deliberately make a polyline with wildly unequal segment lengths: 10 / 1000 / 10
	const TArray<FVector2D> Pts = {
		FVector2D(0, 0), FVector2D(10, 0), 
		FVector2D(1010, 0), FVector2D(1010, 10) };
	const FPathCurve C = FPathCurve::MakePolyline(Pts);
	
	TArray<FPathSample> S;
	C.SampleUniform(102.0, S);
	TestEqual(TEXT("11 samples"), S.Num(), 11);
	
	for (int32 i = 0; i + 1 < S.Num(); ++i)
	{
		const double Step = FVector2D::Distance(S[i].Position, S[i + 1].Position);
		// At polyline corners the straight-line distance is slightly less than the 
		// arc length, so the tolerance is relaxed
		TestTrue(*FString::Printf(
			TEXT("Uniform spacing at %d (%.2f)"), i, Step),
				 FMath::IsNearlyEqual(Step, 102.0, 10.0));
	}
	
	// Distance from EvalAtDistance must be accurately equal the input
	const FPathSample X = C.EvalAtDistance(333.0);
	TestTrue(TEXT("Distance field exact"), 
		FMath::IsNearlyEqual(X.Distance, 333.0, 1e-9));

	return true;
}

// Semicircular arc: verifies Catmull-Rom tessellation and length accuracy
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPathCurveSplineTest,
	"ProcCity.Geometry.PathCurve.Spline", TestFlags)

bool FPathCurveSplineTest::RunTest(const FString& Parameters)
{
	constexpr double R = 1000.0;
	TArray<FVector2D> Pts;
	for (int32 i = 0; i <= 8; ++i)
	{
		const double Ang = PI * i / 8.0;
		Pts.Add(FVector2D(
			FMath::Cos(Ang), FMath::Sin(Ang)) * R);
	}
	const FPathCurve C = FPathCurve::MakeSpline(Pts);

	// Half perimeter = πR ≈ 3141.6
	TestTrue(*FString::Printf(
		TEXT("Length ~= PI*R (got %.1f)"), C.GetLength()),
		FMath::IsNearlyEqual(C.GetLength(), PI * R, PI * R * 0.02));
	
	// Adaptively tessellated points must all lie close to the circle
	TArray<FPathSample> S;
	C.SampleAdaptive(2.0, S);
	TestTrue(TEXT("Adaptive produced points"), S.Num() > Pts.Num());
	for (const FPathSample& Smp : S)
	{
		TestTrue(TEXT("Sample near circle"),
			FMath::IsNearlyEqual(Smp.Position.Size(), R, R * 0.01));
		
		TestTrue(TEXT("Tangent is unit"), 
			FMath::IsNearlyEqual(Smp.Tangent.Size(), 1.0, 1e-6));
		
		TestTrue(TEXT("Tangent perpendicular to radius"),
				 FMath::Abs(FVector2D::DotProduct(
				 	Smp.Tangent, Smp.Position.GetSafeNormal())) < 0.2);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPathCurveOffsetTest,
	"ProcCity.Geometry.PathCurve.OffsetLateral", TestFlags)

bool FPathCurveOffsetTest::RunTest(const FString& Parameters)
{
	const FPathCurve C = FPathCurve::MakeLine(
		FVector2D(0, 0), FVector2D(1000, 0));
	
	const FPathCurve L = C.OffsetLateral(200.0);
	TestTrue(TEXT("Left offset to +Y"),
		L.EvalAtDistance(L.GetLength() * 0.5).Position.Equals(
				FVector2D(500, 200), 1.0));
	
	const FPathCurve Rt = C.OffsetLateral(-200.0);
	TestTrue(TEXT("Right offset to -Y"),
			 Rt.EvalAtDistance(Rt.GetLength() * 0.5).Position.Equals(
			 	FVector2D(500, -200), 1.0));
	
	// Circular arc: radius -> (R + Offset) after outward offset
	constexpr double R = 1000.0;
	TArray<FVector2D> Arc;
	for (int32 i = 0; i <= 8; ++i)
	{
		const double A = PI * i / 8.0;
		Arc.Add(FVector2D(FMath::Cos(A), FMath::Sin(A)) * R);
	}
	
	// CCW arc. Inverse of left normal points outside of circle
	const FPathCurve Curve = FPathCurve::MakeSpline(Arc);
	const FPathCurve Outer = Curve.OffsetLateral(-100.0);
	for (const FPathControlPoint& P : Outer.ControlPoints)
	{
		TestTrue(TEXT("Outer offset radius = R+100"),
				 FMath::IsNearlyEqual(
				 	P.Position.Size(), R + 100.0, 10.0));
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPathCurveRibbonTest,
	"ProcCity.Geometry.PathCurve.Ribbon", TestFlags)

bool FPathCurveRibbonTest::RunTest(const FString& Parameters)
{
	// Straight line ribbon: area = lenght * total width
	{
		const FPathCurve C = FPathCurve::MakeLine(
		FVector2D(0, 0), FVector2D(2000, 0));
		FPolygon2D Road;
		TestTrue(TEXT("Ribbon built"),
			C.BuildRibbonSingle(
				FWidthProfile::Uniform(350.0), 2.0, Road));
		
		TestTrue(TEXT("Area = 2000 * 700"),
				 FMath::IsNearlyEqual(
				 	Road.Area(), 2000.0 * 700.0, 2000.0 * 700.0 * 1e-3));
		
		TestEqual(TEXT("Ribbon is CCW"), 
			Road.Outer.GetWinding() /*Compute signed area*/, 
			EPolyWinding::CounterClockwise);
		TestFalse(TEXT("No self-intersection"), Road.HasSelfIntersection());
		TestTrue(TEXT("Centerline inside"), 
			Road.Contains(FVector2D(1000, 0)));
		TestFalse(TEXT("Beyond width excluded"), 
			Road.Contains(FVector2D(1000, 400)));
	}
	
	// Asymmetric road. only the left side has a sidewalk
	{
		const FPathCurve C = FPathCurve::MakeLine(
			FVector2D(0, 0), FVector2D(1000, 0));
		FPolygon2D SideWalk;
		TestTrue(TEXT("Asymmetric ribbon"),
				 C.BuildRibbonSingle(
				 	FWidthProfile::Asymmetric(300.0, 0.0), 
				 	2.0, SideWalk));
		TestTrue(TEXT("Area = 1000 * 300"),
				 FMath::IsNearlyEqual(SideWalk.Area(), 
				 	1000.0 * 300.0, 1000.0 * 300.0 * 1e-3));
		TestTrue(TEXT("Left side included"),  
			SideWalk.Contains(FVector2D(500,  150)));
		TestFalse(TEXT("Right side excluded"), 
			SideWalk.Contains(FVector2D(500, -150)));
	}
	
	// Sharp turn + large width: the inner self-intersection must be resolved 
	// by union, and must not produce a self-intersecting polygon
	{
		const TArray<FVector2D> Hairpin = {
			FVector2D(0, 0), FVector2D(500, 0), 
			FVector2D(600, 200), FVector2D(500, 400), 
			FVector2D(0, 400) };
		
		const FPathCurve C = FPathCurve::MakeSpline(Hairpin);
		TArray<FPolygon2D> Parts;
		TestTrue(TEXT("Hairpin ribbon built"),
			C.BuildRibbon(FWidthProfile::Uniform(250.0), 
				2.0, Parts));
		
		for (const FPolygon2D& P: Parts)
		{
			TestTrue(TEXT("Piece valid"), P.IsValid());
			TestEqual(TEXT("Piece CCW"), 
				P.Outer.GetWinding(), EPolyWinding::CounterClockwise);
			TestFalse(TEXT("Piece simple"), P.HasSelfIntersection());
			TestTrue(TEXT("Piece has positive area"), P.Area() > 0.0);
		}
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPathCurveScatterTest,
	"ProcCity.Geometry.PathCurve.Scatter", TestFlags)

bool FPathCurveScatterTest::RunTest(const FString& Parameters)
{
	const FPathCurve C = FPathCurve::MakeLine(
		FVector2D(0, 0), FVector2D(1000, 0));
	
	TArray<FTransform> Xf;
	C.ScatterAlong(250.0, 400.0, 0.0, Xf);
	TestEqual(TEXT("5 props at 0/250/500/750/1000"), 
		Xf.Num(), 5);
	
	for (int32 i = 0; i < Xf.Num(); ++i)
	{
		FVector P = Xf[i].GetLocation();
		TestTrue(TEXT("X spacing"), 
			FMath::IsNearlyEqual(P.X, i * 250.0, 1.0));
		TestTrue(TEXT("Lateral offset to +Y"), 
			FMath::IsNearlyEqual(P.Y, 400.0, 1.0));
		// X+ aligns with road direction
		TestTrue(TEXT("Forward along path"),
				 Xf[i].GetUnitAxis(EAxis::X).Equals(
				 	FVector::ForwardVector, 1e-4));
	}
	return true;
}

// Cross-check against UE rotation. Prevent anyone from "casually adding a negative sign" 
// to the tangent in the future
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPathCurveTransformConventionTest,
	"ProcCity.Geometry.PathCurve.TransformConvention", TestFlags)

bool FPathCurveTransformConventionTest::RunTest(const FString& Parameters)
{
	// For path toward Y+, UE Yaw should be 90 degree
	const FPathCurve C = FPathCurve::MakeLine(
		FVector2D(0, 0), FVector2D(0, 1000));
	const FTransform Xf = C.GetTransformAtDistance(500.0);
	TestTrue(TEXT("Yaw = 90"),
		FMath::IsNearlyEqual(
			FRotator::NormalizeAxis(Xf.Rotator().Yaw), 90.0, 1.0));
	TestTrue(TEXT("Forward = +Y"),
		Xf.GetUnitAxis(EAxis::X).Equals(
			FVector(0.0, 1.0, 0.0), 1e-4));
	TestTrue(TEXT("Up = +Z"),
		Xf.GetUnitAxis(EAxis::Z).Equals(
			FVector::UpVector, 1e-4));
	// Left normal (Transform Y+) should be (-1, 0, 0)
	TestTrue(TEXT("Transform +Y is left normal"),
		Xf.GetUnitAxis(EAxis::Y).Equals(
			-FVector::ForwardVector, 1e-4));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPathCurveClosedTest,
	"ProcCity.Geometry.PathCurve.Closed", TestFlags)

bool FPathCurveClosedTest::RunTest(const FString& Parameters)
{
	const FPolygon2D Block = FPolygon2D::MakeRect(
		FVector2D::ZeroVector, FVector2D(500, 300));
	const FPathCurve Loop = FPathCurve::FromPolygonOuter(Block);
	
	TestTrue(TEXT("Closed"), Loop.bClosed);
	TestTrue(TEXT("Loop length = perimeter"),
		FMath::IsNearlyEqual(
			Loop.GetLength(), Block.Perimeter(), 
			Block.Perimeter() * 1e-3));
	
	// CCW along Outer. Left normal points polygon inside.
	TArray<FPathSample> S;
	Loop.SampleUniform(100.0, S);
	for (const FPathSample& Smp : S)
	{
		const FVector2D Inward = Smp.Position + Smp.LeftNormal * 10.0;
		
		// AddInfo(FString::Printf(TEXT("Signed dist %.3f"), 
		// 	Block.SignedDistanceToBoundary(Inward)));
		
		// TODO: Wrong condition
		// if (Block.SignedDistanceToBoundary(Smp.Position) > -1.0)
		
		// Skip corner regions (where the normal is not uniquely defined)
		if (FMath::Abs(Block.SignedDistanceToBoundary(Inward)) > 1.0)
		{
			TestTrue(TEXT("Left normal points inward on CCW loop"), 
				Block.Contains(Inward));
		}
	}
	return true;
}

#endif
