#pragma once

#include "CoreMinimal.h"
#include "ProcCityGeometry/Polygon2D.h"
#include "ProcCityGeometry/PathCurve.h"
#include "PathNetwork.generated.h"

/**
 * Road hierarchy. Drives width, setback, frontage class and, later, which
 * building archetypes are eligible on a given block edge.
 * Values are ordered by importance so comparisons like `Class >= Arterial` work.
 */

UENUM(BlueprintType)
enum class EPathClass : uint8
{
	Undefined  = 0,
	/** Pedestrian only: alley, promenade, park trail */
	Footpath   = 1,
	/** Block-internal service road */
	Local      = 2,
	/** District-internal distributor */
	Collector  = 3,
	/** District boundary; City Sample calls these arteries */
	Arterial   = 4,
	/** City-scale, limited access */
	Highway    = 5
};

/**
 * Cross-section of a road, measured from the centreline.
 *
 * ==== Side convention ====
 * "Left" and "Right" are relative to travelling from NodeA to NodeB.
 * Left is the +90 degree (CCW) side, matching FPathSample::LeftNormal.
 *
 * A bounded face traced CCW always keeps its interior on the LEFT of each
 * half-edge it walks. So a face walking A->B yields LeftTotal(), and a face
 * walking B->A yields RightTotal(). FPathFace::BoundaryEdgeForward records the
 * traversal direction so this can be resolved without ambiguity.
 */
// CHANGE (A1): explicit asymmetric road cross-section.
USTRUCT(BlueprintType)
struct PROCCITYGEOMETRY_API FRoadCrossSection
{
	GENERATED_BODY()

	UPROPERTY() double LeftHalfWidth  = 400.0;
	UPROPERTY() double RightHalfWidth = 400.0;
	UPROPERTY() double LeftSidewalk   = 200.0;
	UPROPERTY() double RightSidewalk  = 200.0;

	double LeftTotal() const { return LeftHalfWidth + LeftSidewalk; }
	double RightTotal() const { return RightHalfWidth + RightSidewalk; }
	double FullWidth() const { return LeftTotal() + RightTotal(); }
	double CarriagewayOffset() const { return (LeftHalfWidth - RightHalfWidth) * 0.5; }

	static FRoadCrossSection Symmetric(double HalfWidth, double Sidewalk)
	{
		FRoadCrossSection Section;
		Section.LeftHalfWidth = HalfWidth;
		Section.RightHalfWidth = HalfWidth;
		Section.LeftSidewalk = Sidewalk;
		Section.RightSidewalk = Sidewalk;
		return Section;
	}
};

/** A node in the planar network: an intersection, an endpoint, or a curve break. */
USTRUCT(BlueprintType)
struct PROCCITYGEOMETRY_API FPathNode
{
	GENERATED_BODY()
	
	UPROPERTY() FVector2D Position = FVector2D::ZeroVector;
	UPROPERTY() double    Elevation = 0.0;
	
	/** Indices into FPathNetwork::Edges, sorted CCW by outgoing direction. */
	UPROPERTY() TArray<int32> IncidentEdges;
	
	/** Highest class among incident edges; used for intersection geometry. */
	UPROPERTY() EPathClass MaxClass = EPathClass::Undefined;
	
	int32 Degree() const { return IncidentEdges.Num(); }
	bool IsIntersection() const { return IncidentEdges.Num() >= 3; }
	bool IsDeadEnd() const { return IncidentEdges.Num() == 1; }
};

/**
 * A straight, non-self-intersecting segment between two nodes.
 * Curved roads are represented as chains of edges sharing a SourceCurveId;
 * this keeps the planar graph strictly straight-line (required for robust face
 * extraction) while preserving the original curve identity for rendering.
 */
USTRUCT(BlueprintType)
struct PROCCITYGEOMETRY_API FPathEdge
{
	GENERATED_BODY()
	
	UPROPERTY() int32 NodeA = INDEX_NONE;
	UPROPERTY() int32 NodeB = INDEX_NONE;
	
	UPROPERTY() EPathClass Class = EPathClass::Local;

	// CHANGE (A2): asymmetric cross-section replaces symmetric HalfWidth/SidewalkWidth.
	UPROPERTY() FRoadCrossSection Section;
	
	/** Index of the originating FPathCurve, or INDEX_NONE for synthetic edges. */
	UPROPERTY() int32 SourceCurveId = INDEX_NONE;
	
	/** Arc-length range on the source curve that this edge covers. */
	UPROPERTY() double SourceStartDistance = 0.0;
	UPROPERTY() double SourceEndDistance = 0.0;
	
	// CHANGE (A2): deprecated compatibility shim for legacy callers.
	double TotalHalfWidth() const { return FMath::Max(Section.LeftTotal(), Section.RightTotal()); }
	
	int32 OtherNode(int32 Node) const
	{
		return (Node == NodeA) ? NodeB : NodeA;
	}
};

/** A face of the planar subdivision. Bounded faces become city blocks. */
USTRUCT(BlueprintType)
struct PROCCITYGEOMETRY_API FPathFace
{
	GENERATED_BODY()
	
	/** Boundary polygon, CCW, in traversal order. */
	UPROPERTY() FPolygon2D Shape;
	
	/** Node indices along the boundary, parallel to Shape.Outer.Vertices. */
	UPROPERTY() TArray<int32> BoundaryNodes;
	
	/**
	 * Edge index for boundary segment i (from vertex i to vertex i+1).
	 * Parallel to Shape.Outer.Vertices. This is how block edges learn their
	 * frontage class: FBlockEdgeFrontage reads Edges[BoundaryEdges[i]].Class.
	 */
	UPROPERTY() TArray<int32> BoundaryEdges;

	/**
	 * True when boundary segment i traverses its edge from NodeA to NodeB.
	 * Parallel to BoundaryEdges. Determines which side of the road this face
	 * borders, and therefore which half-width it must yield.
	 */
	UPROPERTY() TArray<bool> BoundaryEdgeForward;

	/**
	 * Edges traversed in both directions during the face walk, i.e. graph bridges
	 * (cut edges) that dangle into this face rather than bounding it.
	 *
	 * Removed from Shape/BoundaryNodes/BoundaryEdges so the boundary stays a simple
	 * polygon. Kept here because they still consume land: ExtractBlockPolygons
	 * subtracts their road corridor, which is how a cul-de-sac carves a slot into
	 * an otherwise solid block.
	 */
	UPROPERTY() TArray<int32> InteriorEdges;
	
	/** True for the single unbounded face; it must be discarded by callers. */
	UPROPERTY() bool bIsOuterFace = false;
	
	/** Unsigned area */
	UPROPERTY() double Area = 0.0;
};

USTRUCT(BlueprintType)
struct PROCCITYGEOMETRY_API FPathNetworkBuildParams
{
	GENERATED_BODY()
	
	/** Nodes closer than this are welded into one. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ProcCity|Geometry")
	double NodeWeldRadius = 10.0;
	
	/** Chord error used when tessellating input curves into straight edges. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ProcCity|Geometry")
	double CurveTessellationError = 50.0;
	
	/** Edges shorter than this are collapsed after welding. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ProcCity|Geometry")
	double MinEdgeLength = 20.0;
	
	/** Faces smaller than this are dropped (slivers from near-tangent crossings). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ProcCity|Geometry")
	double MinFaceArea = 10000.0;   // 1 m^2
	
	/** Remove degree-1 chains before face extraction. Dead ends cannot bound a face
	 *  and would otherwise appear as zero-area spikes in the traversal. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ProcCity|Geometry")
	bool bPruneDeadEnds = true;
	
	/**
	 * Dead-end chains longer than this are kept as-is instead of being pruned.
	 * Real cul-de-sacs and service spurs are legitimate network features; only
	 * short stubs (tessellation leftovers, curves ending mid-block) are noise.
	 * Set to 0 to prune every dead end.
	 *
	 * CHANGE (B2): kept spurs still cannot bound a face. ExtractFaces must explicitly
	 * collapse out-and-back bridge excursions so Shape/BoundaryEdges stay aligned;
	 * this cannot be deferred to FPolyRing::Simplify.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ProcCity|Geometry")
	double MaxPrunedSpurLength = 0.0;
};

/** Input description of one road before planarization. */
USTRUCT(BlueprintType)
struct PROCCITYGEOMETRY_API FPathNetworkInput
{
	GENERATED_BODY()
	
	UPROPERTY() FPathCurve Curve;
	UPROPERTY() EPathClass Class = EPathClass::Local;
	// CHANGE (A3): input now carries asymmetric road section data.
	UPROPERTY() FRoadCrossSection Section;
};

/**
 * Planar straight-line graph of the road network, plus face extraction.
 *
 * ==== Pipeline ====
 *   AddPath(...) x N          - queue input curves
 *   Build(Params)             - tessellate, split at intersections, weld, prune
 *   ExtractFaces(...)         - minimal cycles -> candidate city blocks
 *   ExtractBlockPolygons(...) - faces inset by per-edge road half-width
 *
 * ==== Contracts ====
 *  - After Build(), no two edges intersect except at shared nodes.
 *  - Node::IncidentEdges is sorted CCW by outgoing direction.
 *  - ExtractFaces returns exactly one face with bIsOuterFace == true per
 *    connected component that encloses area; callers must skip those.
 *  - All bounded faces are CCW and satisfy the FPolygon2D contract.
 *  - Deterministic: results depend only on input order and Params, never on
 *    hash iteration order.
 */

class PROCCITYGEOMETRY_API FPathNetwork
{
	
public:
	void Reset();
	
	/** Queue a road. Returns the SourceCurveId assigned. */
	int32 AddPath(const FPathCurve& Curve, EPathClass Class, 
		double HalfWidth, double SidewalkWidth);
	int32 AddPath(const FPathCurve& Curve, EPathClass Class, const FRoadCrossSection& Section);
	int32 AddPath(const FPathNetworkInput& Input);
	
	/** Convenience: add every outer edge of a polygon as a closed loop of roads. */
	int32 AddPolygonBoundary(const FPolygon2D& Polygon, 
		EPathClass Class, double HalfWidth, double SidewalkWidth);
	int32 AddPolygonBoundary(const FPolygon2D& Polygon, EPathClass Class, const FRoadCrossSection& Section);
	
	/** Tessellate, planarize, weld, prune. Must be called before queries. */
	bool Build(const FPathNetworkBuildParams& Params = FPathNetworkBuildParams());
	bool IsBuilt() const { return bBuilt; }
	
	const TArray<FPathNode>& GetNodes() const { return Nodes; }
	const TArray<FPathEdge>& GetEdges() const { return Edges; }
	
	/** Minimal cycles of the planar graph. Includes the outer face(s). */
	bool ExtractFaces(TArray<FPathFace>& OutFaces) const;
	
	/**
	 * Bounded faces inset by each boundary edge's TotalHalfWidth, i.e. the land
	 * that remains once road corridors are subtracted. This is the primary
	 * producer of FBlockDefinition::Shape.
	 *
	 * @param MinBlockArea  faces whose inset result falls below this are dropped
	 * @param OutFaces      optional; the pre-inset face for each output polygon,
	 *                      parallel to OutBlocks. Needed to recover per-edge
	 *                      frontage class.
	 */
	bool ExtractBlockPolygons(double MinBlockArea, 
		TArray<FPolygon2D>& OutBlocks, 
		TArray<FPathFace>* OutFaces = nullptr) const;
	
	/**
	 * Road surface ribbons, one polygon set per edge class. Feeds the
	 * environment resolver's surface features.
	 */
	bool BuildRoadSurfaces(bool bIncludeSidewalk, TArray<FPolygon2D>& Out) const;
	
	FBox2D Bounds() const;
	uint64 ComputeStableHash() const;
	
	/** Diagnostic: verify the post-Build contracts. Slow; test/debug only. */
	bool ValidateTopology(FString* OutError = nullptr) const;
	
private:
	struct FRawSegment
	{
		FVector2D A, B;
		int32 InputIndex;
		double StartDistance, EndDistance;
	};
	
	void TessellateInputs(const FPathNetworkBuildParams& Params, 
		TArray<FRawSegment>& Out) const;
	
	void SplitAtIntersections(
		TArray<FRawSegment>& InOutSegments, 
		double Tolerance) const;
	
	/** Fill Nodes and Edges using RawSegments produced by SplitAtIntersections*/
	void BuildGraph(const TArray<FRawSegment>& Segments, 
		const FPathNetworkBuildParams& Params);
	
	/** 
	 * Sort Node.IncidentEdges such that edge indices are arrange ascent w.r.t. 
	 * edge incident angle (Angle between X+ and Edge started from Node.Position).
	 * This ensures incident edges sorted CCW
	 */
	void SortIncidentEdges();
	void PruneDeadEnds(double MaxSpurLength);
	
	/** Outgoing direction of Edge as seen from Node. */
	FVector2D OutgoingDirection(int32 NodeIndex, int32 EdgeIndex) const;
	
	TArray<FPathNetworkInput> Inputs;
	TArray<FPathNode> Nodes;
	TArray<FPathEdge> Edges;
	bool bBuilt = false;
};





























