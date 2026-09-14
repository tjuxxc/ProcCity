#pragma once

#include "CoreMinimal.h"
#include "Polygon2D.h"
#include "PathCurve.generated.h"

UENUM(BlueprintType)
enum class EPathInterp : uint8
{
	// Polyline. A straight road segment is just a Linear with 
	// only 2 control points —— no longer a special case
	Linear,
	// Catmull-Rom with automatic tangents
	CatmullRom,
	// Cubic Hermite with explicit tangents (corresponds to 
	// USplineComponent's CurveCustomTangent)
	Hermite
};

USTRUCT(BlueprintType)
struct PROCCITYGEOMETRY_API FPathControlPoint
{
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ProcCity|Geometry")
	FVector2D Position = FVector2D::ZeroVector;
	
	// Elevation (cm). Used for road grades, bridges, and terrain conformance
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ProcCity|Geometry")
	double Elevation = 0.0;
	
	// Used only in Hermite mode
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ProcCity|Geometry")
	FVector2D ArriveTangent = FVector2D::ZeroVector;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ProcCity|Geometry")
	FVector2D LeaveTangent = FVector2D::ZeroVector;
	
	// Tilt angle in degree around the tangent, used for superelevation / road crown 
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ProcCity|Geometry")
	double RollDegrees = 0.0;
	
	FVector GetLocation3D() const
	{
		return FVector(Position.X, Position.Y, Elevation);
	}
};

/** One sample on curve */
USTRUCT(BlueprintType)
struct PROCCITYGEOMETRY_API FPathSample
{
	GENERATED_BODY()
	
	UPROPERTY() FVector2D Position = FVector2D::ZeroVector;
	UPROPERTY() double    Elevation = 0.0;
	// Normalized tangent vector
	UPROPERTY() FVector2D Tangent = FVector2D(1, 0);
	// Normalized left normal (-Tangent.Y, Tangent.X)
	UPROPERTY() FVector2D LeftNormal = FVector2D(0, 1);
	// Arc length (cm) 
	UPROPERTY() double    Distance = 0.0;
	// Normalization parameter [0,1]
	UPROPERTY() double    Alpha = 0.0;
	UPROPERTY() double    RollDegrees = 0.0;
	
	FVector GetLocation3D() const
	{
		return FVector(Position.X, Position.Y, Elevation);
	}
};

/** Width profile that varies along the path (left and right may 
 * be asymmetric: median strips, one-sided sidewalks) */
USTRUCT(BlueprintType)
struct PROCCITYGEOMETRY_API FWidthProfile
{
	GENERATED_BODY()
	
	// Key: (Alpha, LeftWidth, RightWidth). If empty, a constant width is used
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ProcCity|Geometry")
	TArray<FVector> Keys;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ProcCity|Geometry")
	double ConstantLeftWidth = 0.0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ProcCity|Geometry")
	double ConstantRightWidth = 0.0;
	
	static FWidthProfile Uniform(double HalfWidth)
	{
		FWidthProfile P;
		P.ConstantLeftWidth = HalfWidth;
		P.ConstantRightWidth = HalfWidth;
		return P;
	}
	
	static FWidthProfile Asymmetric(double Left, double Right)
	{
		FWidthProfile P;
		P.ConstantLeftWidth = Left;
		P.ConstantRightWidth = Right;
		return P;
	}
	
	void Evaluate(double Alpha, double& OutLeft, double& OutRight) const;
	double MaxHalfWidth() const;
};

/**
 * Unified path primitive: straight segments, polylines, and splines are all this.
 *
 * ==== Contract ====
 * - Units are cm, XY plane + Elevation
 * - Tangent points in the direction of increasing parameter; LeftNormal = Rot90CCW(Tangent)
 * 
 * - The arc-length table is built lazily on demand (on the first call to GetLength/EvalAtDistance).
 * - After modifying Points, you must call Invalidate() (or use SetPoints).
 * - No UObject / no Engine dependency, can be used inside ParallelFor 
 *   (as long as the same instance is not written concurrently)
 */
USTRUCT(BlueprintType)
struct PROCCITYGEOMETRY_API FPathCurve
{
	GENERATED_BODY()
	
	FPathCurve() = default;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ProcCity|Geometry")
	TArray<FPathControlPoint> ControlPoints;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ProcCity|Geometry")
	EPathInterp Interp = EPathInterp::Linear;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ProcCity|Geometry")
	bool bClosed = false;
	
	// --------------- Build --------------------
	static FPathCurve MakeLine(const FVector2D& A, 
		const FVector2D& B, double ZA = 0.0, double ZB = 0.0);
	
	static FPathCurve MakePolyline(
		TArrayView<const FVector2D> InPoints, 
		bool bInClosed = false);
	
	static FPathCurve MakeSpline(
		TArrayView<const FVector2D> InPoints, 
		bool bInClosed = false);
	
	// Construct closed path from polygon outer ring
	static FPathCurve FromPolygonOuter(const FPolygon2D& Poly);
	
	void SetControlPoints(TArray<FPathControlPoint>&& InPoints)
	{
		ControlPoints = MoveTemp(InPoints); 
		Invalidate();
	}
	
	// Must be called after modifying control points
	void Invalidate() const
	{
		CachedLength = -1.0; 
		ArcTable.Reset();
	}
	
	// --------------- Query --------------------
	bool IsValid() const
	{
		return ControlPoints.Num() >= 2 
			|| (bClosed && ControlPoints.Num() >= 3);
	}
	
	int32 NumSegments() const;
	
	// Get total arc length
	double GetLength() const;
	
	// Evaluate based on normalization factor (fast) TODO: Eval what 
	FPathSample EvalAtAlpha(double Alpha) const;
	
	// Evaluate by arc length (uniform, slightly slower, uses lookup table (LUT) 
	// binary search). This is the correct way to place evenly spaced 
	// props such as streetlights and trees. TODO: Eval what 
	FPathSample EvalAtDistance(double Distance) const;
	
	// Converts to a UE world transform: +X = tangent, +Z = up (including Roll)
	FTransform GetTransformAtDistance(double Distance) const;
	
	// Nearest-point query, returns the arc length corresponding the near 
	// point of query on path
	double FindNearestDistance(const FVector2D& Query, 
		double* OutDistanceToCurve = nullptr) const;
	
	// --------------- Sample --------------------
	
	// Adaptive tessellation: recursively bisect until the chord height 
	// error < MaxChordError
	void SampleAdaptive(double MaxChordError, TArray<FPathSample>& Out) const;
	
	// Uniform arc-length sampling, with spacing approximately Spacing (
	// first and last points always included)
	void SampleUniform(double Spacing, TArray<FPathSample>& Out) const;
	
	// Fixed-count sampling
	void SampleCount(int32 Count, TArray<FPathSample>& Out) const;
	
	// --------------- Derived geometry --------------------

	/**
	 * - Offsets laterally (横向) to produce a new curve. Positive values 
	 *   go toward LeftNormal.
	 * - Used for: generating lane markings, curbs, and inner/outer sidewalk 
	 *   boundaries from a road centerline.
	 * - Note: large offsets + small-radius curves can self-intersect; callers 
	 *   should check for this or use BuildRibbon to go through Clipper cleanup.
	 */
	FPathCurve OffsetLateral(double Offset, 
		double MaxChordError = ProcCityGeometry::DefaultMaxChordError) const;

	/**
	 * - Generates a ribbon (带状) polygon with width along the path (source 
	 *   of road / sidewalk surfaces).
	 * - Output satisfies the FPolygon2D contract (outer ring CCW).
	 * - Self-intersecting ribbons are automatically cleaned up by Clipper union.
	 */
	bool BuildRibbon(const FWidthProfile& Profile, 
		double MaxChordError, TArray<FPolygon2D>& Out) const;

	/** Single-result version, taking the largest area */
	bool BuildRibbonSingle(const FWidthProfile& Profile, 
		double MaxChordError, FPolygon2D& Out) const;

	/** Sequence of transforms for scattering props along the path at a given spacing */
	void ScatterAlong(double Spacing, double LateralOffset, 
		double StartOffset, TArray<FTransform>& Out) const;

	FBox2D Bounds2D() const;
	uint64 ComputeStableHash() const;
	
private:
	/** Arc-length LUT: Alpha -> Distance, built lazily */
	struct FArcEntry
	{
		double Alpha; 
		double Distance; 
		FVector2D Position;
	};
	
	mutable TArray<FArcEntry> ArcTable;
	mutable double CachedLength = -1.0;
	
	// Max depth for recursively bisect tessellation, used in SampleAdaptive
	int32 MaxTessellateDepth = 12;
	
	void BuildArcTable() const;
	FVector2D EvalPosition(double Alpha) const;
	FVector2D EvalTangent(double Alpha) const;
	double EvalElevation(double Alpha) const;
	double EvalRoll(double Alpha) const;
	void GetSegment(double Alpha, int32& OutSeg, double& OutLocalT) const;
};







