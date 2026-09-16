#include "ProcCityGeometry/PathNetwork.h"

#include "ProcCityGeometry/SpatialIndex.h"
#include "ProcCityGeometry/PolygonOffset.h"
#include "Intersection/IntrSegment2Segment2.h"
#include "Algo/Reverse.h"
#include "Algo/Unique.h"


using namespace ProcCityGeometry;

namespace 
{
	/** Angle in [0, 2PI) measured CCW from +X. */
	FORCEINLINE double DirectionAngle(const FVector2D& D)
	{
		const double A = FMath::Atan2(D.Y, D.X);
		return (A < 0.0) ? A + UE_TWO_PI : A;
	}
}

void FPathNetwork::Reset()
{
	Inputs.Reset();
	Nodes.Reset();
	Edges.Reset();
	bBuilt = false;
}

int32 FPathNetwork::AddPath(const FPathCurve& Curve, 
	EPathClass Class, double HalfWidth, double SidewalkWidth)
{
	// CHANGE (A3): keep legacy overload by forwarding to a symmetric section.
	return AddPath(Curve, Class, FRoadCrossSection::Symmetric(HalfWidth, SidewalkWidth));
}

int32 FPathNetwork::AddPath(const FPathCurve& Curve, EPathClass Class, const FRoadCrossSection& Section)
{
	FPathNetworkInput In;
	In.Curve = Curve;
	In.Class = Class;
	In.Section = Section;
	return AddPath(In);
}

int32 FPathNetwork::AddPath(const FPathNetworkInput& Input)
{
	bBuilt = false;
	if (!Input.Curve.IsValid()){ return INDEX_NONE; }
	return Inputs.Add(Input);
}

int32 FPathNetwork::AddPolygonBoundary(const FPolygon2D& Polygon, 
	EPathClass Class, double HalfWidth, double SidewalkWidth)
{
	// CHANGE (A3): keep legacy overload by forwarding to a symmetric section.
	return AddPolygonBoundary(
		Polygon, Class, FRoadCrossSection::Symmetric(HalfWidth, SidewalkWidth));
}

int32 FPathNetwork::AddPolygonBoundary(const FPolygon2D& Polygon,
	EPathClass Class, const FRoadCrossSection& Section)
{
	if (!Polygon.IsValid()){ return INDEX_NONE; }
	return AddPath(
		FPathCurve::FromPolygonOuter(Polygon), 
		Class, Section);
}

// ------------- tessellate ------------------

void FPathNetwork::TessellateInputs(
	const FPathNetworkBuildParams& Params, 
	TArray<FRawSegment>& Out) const
{
	Out.Reset();
	for (int32 i = 0; i < Inputs.Num(); ++i)
	{
		const FPathNetworkInput& In = Inputs[i];
		TArray<FPathSample> Samples;
		In.Curve.SampleAdaptive(Params.CurveTessellationError, Samples);
		if (Samples.Num() < 2) { continue; }
		
		for (int32 s = 0; s + 1 < Samples.Num(); ++s)
		{
			if (FVector2D::Distance(Samples[s].Position, Samples[s + 1].Position)
				< Params.NodeWeldRadius)
			{
				continue; // collapsed by welding anyway
			}
			
			Out.Add({
				Samples[s].Position, Samples[s + 1].Position, i,
				Samples[s].Distance, 
				Samples[s + 1].Distance});
		}
		// Closed curves: SampleAdaptive already emits the wrap-around sample,
		// so no explicit closing segment processing is required here.
	}
}

// ------------- planarize ------------------

void FPathNetwork::SplitAtIntersections(
	TArray<FRawSegment>& InOutSegments, double Tolerance) const
{
	/**
	 * - Collect split parameters per segment first, then rebuild. Splitting in place 
	 *   while iterating would invalidate indices and make the result order-dependent. 
	 *   
	 * - Complexity is O(n^2). At district scale (n in the low thousands after 
	 *   tessellation) this runs in well under a second. 
	 *   
	 * - If city-scale arterial networks ever become the bottleneck, swap 
	 *   in a sweep-line or reuse UE::Geometry::FArrangement2d here. 
	 *   The interface does not change. */
	
	const int32 N = InOutSegments.Num();
	TArray<TArray<double>> SplitParams;
	SplitParams.SetNum(N);
	
	for (int32 i = 0; i < N; ++i)
	{
		for (int32 j = i + 1; j < N; ++j)
		{
			const FRawSegment& S0 = InOutSegments[i];
			const FRawSegment& S1 = InOutSegments[j];
			
			// Cheap reject
			const FBox2D B0(
				FVector2D::Min(S0.A, S0.B) - FVector2D(Tolerance, Tolerance), 
				FVector2D::Max(S0.A, S0.B) + FVector2D(Tolerance, Tolerance));
			const FBox2D B1(FVector2D::Min(S1.A, S1.B), FVector2D::Max(S1.A, S1.B));
			if (!B0.Intersect(B1)) { continue; }
			
			UE::Geometry::FIntrSegment2Segment2d Intr(
				UE::Geometry::FSegment2d(S0.A, S0.B),
				UE::Geometry::FSegment2d(S1.A, S1.B));
			Intr.SetIntervalThreshold(Tolerance);
			if (!Intr.Find()) { continue; };
			
			// Collinear overlap produces a segment of intersection; take both ends.
			const int32 IntersectionCount = FMath::Max(1, Intr.Quantity);
			for (int32 k = 0; k < IntersectionCount; ++k)
			{
				FVector2D P = (k == 0) ? Intr.Point0 : Intr.Point1;
				
				auto AddParam = [&](int32 SegIdx)
				{
					const FRawSegment& S = InOutSegments[SegIdx];
					const FVector2D D = S.B - S.A;
					const double LenSq = D.SizeSquared();
					if (LenSq < UE_KINDA_SMALL_NUMBER) { return; }
					const double T = FVector2D::DotProduct(P - S.A, D) / LenSq;
					// Skip endpoints: they weld naturally and would create zero-length pieces
					if (T > 1e-6 && T < 1.0 - 1e-6)
					{
						SplitParams[SegIdx].Add(T);
					}
				};
				
				AddParam(i); 
				AddParam(j);
			}
		}
	}
	TArray<FRawSegment> Result;
	Result.Reserve(N * 2);
	for (int32 i = 0; i < N; ++i)
	{
		TArray<double>& Ts = SplitParams[i];
		if (Ts.Num() == 0)
		{
			Result.Add(InOutSegments[i]); 
			continue;
		}
		
		Ts.Sort();
		const FRawSegment& S = InOutSegments[i];
		double PrevT = 0.0;
		auto Emit = [&](double T0, double T1)
		{
			if (T1 - T0 < 1e-9) { return; }
			FRawSegment Piece;
			Piece.A = FMath::Lerp(S.A, S.B, T0);
			Piece.B = FMath::Lerp(S.A, S.B, T1);
			// Index of input description of associated road
			Piece.InputIndex = S.InputIndex;
			Piece.StartDistance = 
				FMath::Lerp(S.StartDistance, S.EndDistance, T0);
			Piece.EndDistance = 
				FMath::Lerp(S.StartDistance, S.EndDistance, T1);
			Result.Add(Piece);
		};
		
		for (double T : Ts)
		{
			if (T - PrevT > 1e-9) { Emit(PrevT, T); }
			PrevT = T;
		}
		Emit(PrevT, 1.0);
	}
	InOutSegments = MoveTemp(Result);
}

// ------------- graph ------------------

/** Fill Nodes and Edges using RawSegments produced by SplitAtIntersections*/
void FPathNetwork::BuildGraph(
	const TArray<FRawSegment>& Segments, 
	const FPathNetworkBuildParams& Params)
{
	Nodes.Reset();
	Edges.Reset();
	
	// Grid cell sized to the weld radius keeps neighbour scans to 9 cells.
	FSpatialGrid2D Grid;
	Grid.Reset(FMath::Max(Params.NodeWeldRadius * 2.0, 10.0));
	
	auto GetOrAddNode = [&](const FVector2D& P, double Elevation) -> int32
	{
		bool bInserted = false;
		const int32 Index = 
			Grid.InsertUnique(P, Params.NodeWeldRadius, bInserted);
		if (bInserted)
		{
			FPathNode Node;
			Node.Position = P;
			Node.Elevation = Elevation;
			Nodes.Add(Node);
			checkSlow(Nodes.Num() - 1 == Index);
		}
		return Index;
	};
	
	// Deduplicate edges: two inputs overlapping exactly must not create a doubled
	// edge, which would break face traversal.
	TSet<TPair<int32, int32>> SeenEdges;
	
	for (const FRawSegment& S : Segments)
	{
		const FPathNetworkInput& In = Inputs[S.InputIndex];
		
		const int32 IdxA = GetOrAddNode(S.A, 0.0);
		const int32 IdxB = GetOrAddNode(S.B, 0.0);
		if (IdxA == IdxB) { continue; } // collapsed by welding
		
		if (FVector2D::Distance(Nodes[IdxA].Position, Nodes[IdxB].Position) 
			< Params.MinEdgeLength)
		{
			continue;
		}
		
		const TPair<int32, int32> 
			Key(FMath::Min(IdxA, IdxB), FMath::Max(IdxA, IdxB));
		
		if (SeenEdges.Contains(Key))
		{
			// Keep the higher road class when duplicates collide.
			continue;
		}
		SeenEdges.Add(Key);
		
		FPathEdge E;
		E.NodeA = IdxA;
		E.NodeB = IdxB;
		E.Class = In.Class;
		// CHANGE (A3): persist per-edge asymmetric cross-section.
		E.Section = In.Section;
		E.SourceCurveId = S.InputIndex;
		E.SourceStartDistance = S.StartDistance;
		E.SourceEndDistance = S.EndDistance;
		const int32 EdgeIndex = Edges.Add(E);
		
		Nodes[IdxA].IncidentEdges.Add(EdgeIndex);
		Nodes[IdxB].IncidentEdges.Add(EdgeIndex);
		Nodes[IdxA].MaxClass = FMath::Max(Nodes[IdxA].MaxClass, In.Class);
		Nodes[IdxB].MaxClass = FMath::Max(Nodes[IdxB].MaxClass, In.Class);
	}
}

FVector2D FPathNetwork::OutgoingDirection(int32 NodeIndex, int32 EdgeIndex) const
{
	const FPathEdge& E = Edges[EdgeIndex];
	const int32 Other = E.OtherNode(NodeIndex);
	return (Nodes[Other].Position - Nodes[NodeIndex].Position).GetSafeNormal();
}

void FPathNetwork::SortIncidentEdges()
{
	for (int32 n = 0; n < Nodes.Num(); ++n)
	{
		FPathNode& Node = Nodes[n];
		// Edge incident direction is the aligned with [Node.Position - Other.Position]
		// Sort ascent w.r.t. edge incident angle (Node is start)
		Node.IncidentEdges.Sort(
			[&] (int32 L, int32 R)
			{
				const double AL = DirectionAngle(
					OutgoingDirection(n, L));
				const double AR = DirectionAngle(
					OutgoingDirection(n, R));
				// Equal angles (overlapping edges) fall back to index for determinism.
				return (FMath::Abs(AL - AR) > 1e-12) ? (AL < AR) : (L < R);
			}
		);
	}
}

void FPathNetwork::PruneDeadEnds(double MaxSpurLength)
{
	// Iteratively strip degree-1 nodes. A dead-end chain cannot bound a face;
	// leaving it in would make the traversal walk out and back along the spur,
	// producing a zero-width sliver in the face polygon.
	
	TArray<bool> EdgeAlive;
	EdgeAlive.Init(true, Edges.Num());
	TArray<int32> Degree;
	Degree.SetNum(Nodes.Num());
	
	for (int32 n = 0; n < Nodes.Num(); ++n)
	{
		Degree[n] = Nodes[n].IncidentEdges.Num();
	}
	
	// Accumulated chain length reaching each node along the spur being peeled.
	// A spur is only cut while its total length stays under MaxPrunedSpurLength, so a
	// long cul-de-sac survives while a short stub is removed entirely.
	TArray<double> SpurLength;
	SpurLength.Init(0.0, Nodes.Num());
	const bool bPruneAll = (MaxSpurLength <= 0.0);
	
	bool bChanged = true;
	while (bChanged)
	{
		bChanged = false;
		for (int32 n = 0; n < Nodes.Num(); ++n)
		{
			if (Degree[n] != 1) { continue; }
			for (int32 EdgeIdx : Nodes[n].IncidentEdges) // redundant for?
			{
				if (!EdgeAlive[EdgeIdx]) { continue; }
				
				const int32 Other = Edges[EdgeIdx].OtherNode(n);
				const double EdgeLen = FVector2D::Distance(
					Nodes[n].Position, Nodes[Other].Position);
				const double Accum = SpurLength[n] + EdgeLen;
				
				if (!bPruneAll && Accum > MaxSpurLength)
				{
					// Long enough to be a genuine cul-de-sac: stop peeling this chain.
					// CHANGE (B1): freeze marker avoids non-converging re-examination.
					Degree[n] = -1; // mark as frozen so we do not revisit
					break;
				}
					
				EdgeAlive[EdgeIdx] = false;
				--Degree[n];
				--Degree[Other];
				SpurLength[Other] = FMath::Max(SpurLength[Other], Accum);
				bChanged = true;
				break;
			}
		}
	}
	
	// Compact: remove edges associated to dead ends
	TArray<FPathEdge> NewEdges;
	NewEdges.Reserve(Edges.Num());
	TArray<int32> Remap;
	Remap.Init(INDEX_NONE, Edges.Num());
	for (int32 e = 0; e < Edges.Num(); ++e)
	{
		if (EdgeAlive[e])
		{
			// e-th slot of Remap store the new index of Edge(e) in NewEdges
			Remap[e] = NewEdges.Add(Edges[e]);
		}
	}
	Edges = MoveTemp(NewEdges);
	
	for (FPathNode& Node : Nodes)
	{
		TArray<int32> Kept;
		Kept.Reserve(Node.IncidentEdges.Num());
		for (int32 e : Node.IncidentEdges)
		{
			if (Remap[e] != INDEX_NONE) { Kept.Add(Remap[e]); } 
		}
		Node.IncidentEdges = MoveTemp(Kept);
	}
}

bool FPathNetwork::Build(const FPathNetworkBuildParams& Params)
{
	Nodes.Reset();
	Edges.Reset();
	bBuilt = false;
	
	if (Inputs.Num() == 0) { return false; }
	
	TArray<FRawSegment> Segments;
	TessellateInputs(Params, Segments);
	if (Segments.Num() == 0) { return false; }
	SplitAtIntersections(Segments, Params.NodeWeldRadius);
	// Build Edges and Nodes from Segments. Remove duplicate/too short edges
	BuildGraph(Segments, Params);
	
	if (Params.bPruneDeadEnds)
	{
		// CHANGE (B1): wire build params through to chain-length spur pruning.
		PruneDeadEnds(Params.MaxPrunedSpurLength);
	}
	
	SortIncidentEdges();
	bBuilt = Edges.Num() > 0;
	return bBuilt;
}

bool FPathNetwork::ExtractFaces(TArray<FPathFace>& OutFaces) const
{
	OutFaces.Reset();
	if (!bBuilt) { return false; }
	
	// Half-edge id: EdgeIndex * 2 + (0 => A->B, 1 => B->A)
	const int32 NumHalf = Edges.Num() * 2;
	TArray<bool> Visited;
	Visited.Init(false, NumHalf);
	
	auto HalfSource = [&](int32 H)
	{
		return (H & 1) ? Edges[H >> 1].NodeB : Edges[H >> 1].NodeA;
	};
	auto HalfTarget = [&](int32 H)
	{
		return (H & 1) ? Edges[H >> 1].NodeA : Edges[H >> 1].NodeB;
	};
	
	// CHANGE (B2): Twin helper is used by both traversal and bridge collapsing.
	auto Twin = [](int32 H)
	{
		return H ^ 1;
	};
	
	/**
	 * Next half-edge in the face walk.
	 *
	 * Standard planar-subdivision rule: arriving at V along H, take the twin
	 * (V -> U) and step to the *previous* entry in V's CCW-sorted incidence list,
	 * i.e. turn as far clockwise as possible. This traces bounded faces
	 * counter-clockwise and the unbounded face clockwise, which is exactly the
	 * discriminator used below.
	 */
	auto NextHalfEdge = [&](int32 H) -> int32
	{
		const int32 V = HalfTarget(H);
		const FPathNode& Node = Nodes[V];
		
		// twin(H) leaves V along the edge we arrived on.
		const int32 T = Twin(H);
		const int32 Pos = Node.IncidentEdges.IndexOfByKey(T >> 1);
		if (Pos == INDEX_NONE) { return INDEX_NONE; }
		
		const int32 Count = Node.IncidentEdges.Num();
		const int32 PrevPos = (Pos + Count - 1) % Count;
		const int32 NextEdge = Node.IncidentEdges[PrevPos];

		// Orient the chosen edge so that it leaves V.
		return (Edges[NextEdge].NodeA == V) ? (NextEdge * 2) : (NextEdge * 2 + 1);
	};
	
	for (int32 Start = 0; Start < NumHalf; ++Start)
	{
		if (Visited[Start]) { continue; }
		
		TArray<int32> Cycle;
		int32 H = Start;
		bool bOk = true;
		
		// Guard against malformed graphs; a valid face cannot exceed NumHalf steps.
		for (int32 Guard = 0; Guard <= NumHalf; ++Guard)
		{
			if (H == INDEX_NONE || Visited[H])
			{
				bOk = (H == Start && Cycle.Num() > 0); 
				break;
			}
			Visited[H] = true;
			Cycle.Add(H);
			H = NextHalfEdge(H);
			if (H == Start) { break; }
		}
		
		if (!bOk || Cycle.Num() < 3) { continue; }

		// CHANGE (B2): collapse out-and-back excursions. Removing one twin pair can
		// expose another, so this is bracket matching with a stack.
		TArray<int32> Cleaned;
		TArray<int32> Removed;
		Cleaned.Reserve(Cycle.Num());
		for (int32 Hi : Cycle)
		{
			if (Cleaned.Num() > 0 && Cleaned.Last() == Twin(Hi))
			{
				Removed.Add(Cleaned.Pop() >> 1);
				continue;
			}
			Cleaned.Add(Hi);
		}
		while (Cleaned.Num() >= 2 && Cleaned[0] == Twin(Cleaned.Last()))
		{
			Removed.Add(Cleaned.Pop() >> 1);
			Cleaned.RemoveAt(0);
		}
		if (Cleaned.Num() < 3) { continue; }
		Cycle = MoveTemp(Cleaned);
		
		FPathFace Face;
		Face.BoundaryNodes.Reserve(Cycle.Num());
		Face.BoundaryEdges.Reserve(Cycle.Num());
		Face.BoundaryEdgeForward.Reserve(Cycle.Num());
		TArray<FVector2D> Verts;
		Verts.Reserve(Cycle.Num());
		
		for (int32 HalfEdgeIdx : Cycle)
		{
			const int32 Src = HalfSource(HalfEdgeIdx);
			Verts.Add(Nodes[Src].Position);
			Face.BoundaryNodes.Add(Src);
			Face.BoundaryEdges.Add(HalfEdgeIdx >> 1);
			// CHANGE (A4): even half-edge id means NodeA -> NodeB.
			Face.BoundaryEdgeForward.Add((HalfEdgeIdx & 1) == 0);
		}

		Removed.Sort();
		const int32 UniqueEnd = Algo::Unique(Removed);
		Removed.SetNum(UniqueEnd);
		Face.InteriorEdges = MoveTemp(Removed);
		// Signed area decides bounded vs unbounded, and must be computed BEFORE
		// FPolygon2D normalization flips the winding.
		FPolyRing Ring;
		Ring.Vertices = Verts;
		const double SignedArea = Ring.SignedArea();
		
		Face.bIsOuterFace = (SignedArea < 0.0);
		Face.Area = FMath::Abs(SignedArea);
		
		Face.Shape.Outer.Vertices = MoveTemp(Verts);
		// Normalize() would also Simplify, which desynchronizes BoundaryEdges
		// from the vertex array. Only fix the winding here.
		
		if (Face.bIsOuterFace)
		{
			Face.Shape.Outer.Reverse();
			Algo::Reverse(Face.BoundaryNodes);
			Algo::Reverse(Face.BoundaryEdges);
			Algo::Reverse(Face.BoundaryEdgeForward);
			// After reversing vertices, boundary edge i must still map to the
			// segment (v_i -> v_i+1); rotate by one to restore that alignment.
			if (Face.BoundaryEdges.Num() > 1)
			{
				const int32 First = Face.BoundaryEdges[0];
				Face.BoundaryEdges.RemoveAt(0);
				Face.BoundaryEdges.Add(First);
			}
			if (Face.BoundaryEdgeForward.Num() > 1)
			{
				// CHANGE (A4): after Algo::Reverse edges are [e_{n-1} ... e_0], but slot i
				// must hold e_{n-2-i}; this is a left rotation by one.
				const bool FirstForward = Face.BoundaryEdgeForward[0];
				Face.BoundaryEdgeForward.RemoveAt(0);
				Face.BoundaryEdgeForward.Add(FirstForward);
			}
			// CHANGE (A4): reversing the loop flips every traversal direction.
			for (int32 DirIdx = 0; DirIdx < Face.BoundaryEdgeForward.Num(); ++DirIdx)
			{
				Face.BoundaryEdgeForward[DirIdx] = !Face.BoundaryEdgeForward[DirIdx];
			}
		}
		OutFaces.Add(MoveTemp(Face));
	}
	return OutFaces.Num() > 0;
}

bool FPathNetwork::ExtractBlockPolygons(double MinBlockArea, 
	TArray<FPolygon2D>& OutBlocks, TArray<FPathFace>* OutFaces) const
{
	OutBlocks.Reset();
	if (OutFaces) { OutFaces->Reset(); }
	
	TArray<FPathFace> Faces;
	if (!ExtractFaces(Faces)) { return false; }
	
	for (const FPathFace& Face : Faces)
	{
		if (Face.bIsOuterFace) { continue; }
		if (Face.Area < MinBlockArea) { continue; }
		if (!Face.Shape.IsValid()) { continue; }
		
		// Per-edge inset by that road's total half-width: wide arterials eat more
		// land than narrow local streets, which is what produces the characteristic
		// non-uniform block shapes in real cities.
		TArray<double> Insets;
		Insets.Reserve(Face.BoundaryEdges.Num());
		for (int32 i = 0; i < Face.BoundaryEdges.Num(); ++i)
		{
			if (!Edges.IsValidIndex(Face.BoundaryEdges[i]))
			{
				Insets.Add(0.0);
				continue;
			}
			const FPathEdge& E = Edges[Face.BoundaryEdges[i]];
			// CHANGE (A5): bounded face yields the width on its traversed side.
			Insets.Add(Face.BoundaryEdgeForward[i] ? E.Section.LeftTotal() : E.Section.RightTotal());
		}
		
		TArray<FPolygon2D> Pieces;
		if (!InsetPolygonPerEdge(Face.Shape, Insets, MinBlockArea, Pieces))
		{
			continue;  // road corridors consumed the whole face
		}

		// CHANGE (B3): carve corridors for bridges dangling into this face.
		if (Face.InteriorEdges.Num() > 0)
		{
			TArray<FPolygon2D> Corridors;
			for (int32 EdgeIdx : Face.InteriorEdges)
			{
				if (!Edges.IsValidIndex(EdgeIdx)) { continue; }
				const FPathEdge& E = Edges[EdgeIdx];
				const FPathCurve Seg = FPathCurve::MakeLine(
					Nodes[E.NodeA].Position, Nodes[E.NodeB].Position);
				FPolygon2D Corridor;
				if (Seg.BuildRibbonSingle(
						FWidthProfile::Asymmetric(E.Section.LeftTotal(), E.Section.RightTotal()),
						ProcCityGeometry::DefaultMaxChordError, Corridor))
				{
					Corridors.Add(MoveTemp(Corridor));
				}
			}
			if (Corridors.Num() > 0)
			{
				TArray<FPolygon2D> Carved;
				if (SubtractPolygons(Pieces, Corridors, Carved))
				{
					Pieces = MoveTemp(Carved);
				}
			}
		}
		
		for (FPolygon2D& P : Pieces)
		{
			if (P.Area() >= MinBlockArea)
			{
				OutBlocks.Add(MoveTemp(P));
				if (OutFaces) { OutFaces->Add(Face); }
			}
		}
	}
	return OutBlocks.Num() > 0;
}

bool FPathNetwork::BuildRoadSurfaces(
	bool bIncludeSidewalk, TArray<FPolygon2D>& Out) const
{
	Out.Reset();
	if (!bBuilt) { return false; }
	
	TArray<FPolygon2D> Quads;
	Quads.Reserve(Edges.Num());
	
	for (const FPathEdge& E : Edges)
	{
		const FPathCurve Seg = FPathCurve::MakeLine(
			Nodes[E.NodeA].Position, Nodes[E.NodeB].Position,
			Nodes[E.NodeA].Elevation, Nodes[E.NodeB].Elevation);
		
		FPolygon2D Ribbon;
		// CHANGE (A6): use left/right widths directly from per-edge cross-section.
		const FWidthProfile Profile = FWidthProfile::Asymmetric(
			bIncludeSidewalk ? E.Section.LeftTotal() : E.Section.LeftHalfWidth,
			bIncludeSidewalk ? E.Section.RightTotal() : E.Section.RightHalfWidth);
		if (Seg.BuildRibbonSingle(Profile, 
			DefaultMaxChordError, Ribbon))
		{
			Quads.Add(MoveTemp(Ribbon));
		}
	}
	if (Quads.Num() == 0) { return false; }
	
	// CHANGE (A7): single Clipper union over all ribbons. The previous pairwise
	// loop was O(n) Clipper invocations, each reprocessing the accumulated result.
	if (!UnionPolygons(Quads, TArrayView<const FPolygon2D>(), Out))
	{
		Out = MoveTemp(Quads);   // degenerate fallback: keep overlapping quads
	}
	return Out.Num() > 0;
	
}

FBox2D FPathNetwork::Bounds() const
{
	FBox2D B(ForceInit);
	for (const FPathNode& N : Nodes) { B += N.Position; }
	return B;
}

uint64 FPathNetwork::ComputeStableHash() const
{
	uint64 H = 1469598103934665603ull;
	auto Mix = [&H](uint64 V) { H ^= V; H *= 1099511628211ull; };
	auto Q = [](double V) -> int64
	{
		return static_cast<int64>(
			FMath::RoundToDouble(V / PositionTolerance));
	};
	
	Mix(static_cast<uint64>(Nodes.Num()));
	for (const FPathNode& N : Nodes)
	{
		Mix(static_cast<uint64>(Q(N.Position.X)));
		Mix(static_cast<uint64>(Q(N.Position.Y)));
	}
	
	Mix(static_cast<uint64>(Edges.Num()));
	for (const FPathEdge& E : Edges)
	{
		Mix(static_cast<uint64>(E.NodeA));
		Mix(static_cast<uint64>(E.NodeB));
		Mix(static_cast<uint64>(E.Class));
	}
	
	return H;
}

bool FPathNetwork::ValidateTopology(FString* OutError) const
{
	auto Fail = [&](const FString& Msg) -> bool
	{
		if (OutError) { *OutError = Msg; }	
		return false;
	};
	
	if (!bBuilt) { return Fail(TEXT("Network not built")); }
	
	for (int32 e = 0; e < Edges.Num(); ++e)
	{
		const FPathEdge& E = Edges[e];
		if (!Nodes.IsValidIndex(E.NodeA) || !Nodes.IsValidIndex(E.NodeB))
		{
			return Fail(FString::Printf(
				TEXT("Edge %d references invalid node"), e));
		}
		if (E.NodeA == E.NodeB)
		{
			return Fail(FString::Printf(TEXT("Edge %d is a self-loop"), e));
		}
		if (!Nodes[E.NodeA].IncidentEdges.Contains(e) ||
			!Nodes[E.NodeB].IncidentEdges.Contains(e))
		{
			return Fail(FString::Printf(TEXT("Edge %d missing from node incidence"), e));
		}
	}
	
	// No two edges may cross except at shared nodes.
	for (int32 i = 0; i < Edges.Num(); ++i)
	{
		for (int32 j = i + 1; j < Edges.Num(); ++j)
		{
			const FPathEdge& A = Edges[i];
			const FPathEdge& B = Edges[j];
			if (A.NodeA == B.NodeA || A.NodeA == B.NodeB ||
				A.NodeB == B.NodeA || A.NodeB == B.NodeB)
			{
				continue;   // share a node: touching is legal
			}
			UE::Geometry::FIntrSegment2Segment2d Intr(
				UE::Geometry::FSegment2d(Nodes[A.NodeA].Position, Nodes[A.NodeB].Position),
				UE::Geometry::FSegment2d(Nodes[B.NodeA].Position, Nodes[B.NodeB].Position));
			if (Intr.Find())
			{
				return Fail(FString::Printf(
					TEXT("Edges %d and %d cross without a node"), i, j));
			}
		}
	}

	TArray<FPathFace> Faces;
	if (ExtractFaces(Faces))
	{
		for (int32 FaceIdx = 0; FaceIdx < Faces.Num(); ++FaceIdx)
		{
			const FPathFace& Face = Faces[FaceIdx];
			if (Face.Shape.HasSelfIntersection())
			{
				// CHANGE (B4): extracted face boundaries must be simple polygons.
				return Fail(FString::Printf(TEXT("Face %d has self-intersection"), FaceIdx));
			}
			const int32 N = Face.Shape.Outer.NumVertices();
			if (Face.BoundaryEdges.Num() != N)
			{
				return Fail(FString::Printf(TEXT("Face %d boundary-edge count mismatch"), FaceIdx));
			}
			if (Face.BoundaryEdgeForward.Num() != N)
			{
				return Fail(FString::Printf(TEXT("Face %d boundary-direction count mismatch"), FaceIdx));
			}
		}
	}
	return true;
}







