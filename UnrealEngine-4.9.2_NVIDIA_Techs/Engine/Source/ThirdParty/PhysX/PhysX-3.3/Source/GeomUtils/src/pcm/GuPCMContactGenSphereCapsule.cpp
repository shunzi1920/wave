/*
 * Copyright (c) 2008-2015, NVIDIA CORPORATION.  All rights reserved.
 *
 * NVIDIA CORPORATION and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA CORPORATION is strictly prohibited.
 */
// Copyright (c) 2004-2008 AGEIA Technologies, Inc. All rights reserved.
// Copyright (c) 2001-2004 NovodeX AG. All rights reserved.  

#include "GuGeometryUnion.h"
#include "GuPCMContactGen.h"
#include "GuPCMShapeConvex.h"
#include "CmRenderOutput.h"
#include "GuPCMContactGenUtil.h"
#include "PsVecMath.h"
#include "GuVecCapsule.h"
#include "GuVecBox.h"

#ifdef	PCM_LOW_LEVEL_DEBUG
#include "CmRenderOutput.h"
extern physx::Cm::RenderOutput* gRenderOutPut;
#endif

#ifdef PX_WIIU  
#pragma ghs nowarning 1656 //within a function using alloca or VLAs, alignment of local variables
#endif

using namespace physx;
using namespace Gu;


	//Precompute the convex data
	//     7+------+6			0 = ---
	//     /|     /|			1 = +--
	//    / |    / |			2 = ++-
	//   / 4+---/--+5			3 = -+-
	// 3+------+2 /    y   z	4 = --+
	//  | /    | /     |  /		5 = +-+
	//  |/     |/      |/		6 = +++
	// 0+------+1      *---x	7 = -++

namespace physx
{

namespace Gu
{

	static bool testSATCapsulePoly(const Gu::CapsuleV& capsule, const Gu::PolygonalData& polyData, SupportLocal* map,  const Ps::aos::FloatVArg contactDist, Ps::aos::FloatV& minOverlap, Ps::aos::Vec3V& seperatingAxis)
	{
		using namespace Ps::aos;
		FloatV _minOverlap = FMax();//minOverlap;
		FloatV min0, max0;
		FloatV min1, max1;
		Vec3V tempAxis = V3UnitY();
		const BoolV bTrue = BTTTT();

		//ML: project capsule to polydata axis 
		if(!testPolyDataAxis(capsule, polyData, map, contactDist, _minOverlap, tempAxis))
			return false;

		const Vec3V capsuleAxis = V3Sub(capsule.p1, capsule.p0);
	
		for(PxU32 i=0;i<polyData.mNbPolygons;i++)
		{
			const Gu::HullPolygonData& polygon = polyData.mPolygons[i];
			const PxU8* inds1 = polyData.mPolygonVertexRefs + polygon.mVRef8;

			for(PxU32 lStart = 0, lEnd =PxU32(polygon.mNbVerts-1); lStart<polygon.mNbVerts; lEnd = lStart++)
			{

				//in the vertex space
				const Vec3V p10 = V3LoadU(polyData.mVerts[inds1[lStart]]);
				const Vec3V p11 = V3LoadU(polyData.mVerts[inds1[lEnd]]);
				
				const Vec3V vertexSpaceV = V3Sub(p11, p10);

				//transform v to shape space
				const Vec3V shapeSpaceV = M33TrnspsMulV3(map->shape2Vertex, vertexSpaceV);
			
				const Vec3V normal = V3Normalize(V3Cross(capsuleAxis, shapeSpaceV));

				map->doSupport(normal, min0, max0);

				const FloatV tempMin = V3Dot(capsule.p0, normal);
				const FloatV tempMax = V3Dot(capsule.p1, normal);
				min1 = FMin(tempMin, tempMax);
				max1 = FMax(tempMin, tempMax);

				min1 = FSub(min1, capsule.radius);
				max1 = FAdd(max1, capsule.radius);

				const BoolV con = BOr(FIsGrtr(min1, FAdd(max0, contactDist)), FIsGrtr(min0, FAdd(max1, contactDist)));

				if(BAllEq(con, bTrue))
					return false;

			
				const FloatV tempOverlap = FSub(max0, min1);

				if(FAllGrtr(_minOverlap, tempOverlap))
				{
					_minOverlap = tempOverlap;
					tempAxis = normal;
				}
			}
		}
		
		seperatingAxis = tempAxis;
		minOverlap = _minOverlap;

		return true;

	}


	void generatedCapsuleBoxFaceContacts(const Gu::CapsuleV& capsule,  Gu::PolygonalData& polyData,const Gu::HullPolygonData& referencePolygon, Gu::SupportLocal* map, const Ps::aos::PsMatTransformV& aToB, Gu::PersistentContact* manifoldContacts, PxU32& numContacts, 
		const Ps::aos::FloatVArg contactDist, const Ps::aos::Vec3VArg normal)
	{

		using namespace Ps::aos;

		const BoolV bTrue = BTTTT();
		const FloatV radius = FAdd(capsule.radius, contactDist);

		//calculate the intersect point of ray to plane
		const Vec3V planeNormal = V3Normalize(M33TrnspsMulV3(map->shape2Vertex, V3LoadU(referencePolygon.mPlane.n)));
		const PxU8* inds = polyData.mPolygonVertexRefs + referencePolygon.mVRef8;
		const Vec3V a = M33MulV3(map->vertex2Shape,V3LoadU(polyData.mVerts[inds[0]]));
	
		const FloatV denom0 = V3Dot(planeNormal, V3Sub(capsule.p0, a)); 
		const FloatV denom1 = V3Dot(planeNormal, V3Sub(capsule.p1, a)); 
		const FloatV numer= FRecip(V3Dot(planeNormal, normal));
		const FloatV t0 = FMul(denom0, numer);
		const FloatV t1 = FMul(denom1, numer);

		const BoolV con0 = FIsGrtrOrEq(radius, t0);
		const BoolV con1 = FIsGrtrOrEq(radius, t1);
		if(BAllEq(BOr(con0, con1), bTrue))
		{

			const Mat33V rot = findRotationMatrixFromZAxis(planeNormal);
			Vec3V* points0In0 = (Vec3V*)PxAllocaAligned(sizeof(Vec3V)*referencePolygon.mNbVerts, 16);
			map->populateVerts(inds, referencePolygon.mNbVerts, polyData.mVerts, points0In0);

			Vec3V rPolygonMin = V3Splat(FMax());
			Vec3V rPolygonMax = V3Neg(rPolygonMin);
			for(PxU32 i=0; i<referencePolygon.mNbVerts; ++i)
			{
				points0In0[i] = M33MulV3(rot, points0In0[i]);
				rPolygonMin = V3Min(rPolygonMin, points0In0[i]);
				rPolygonMax = V3Max(rPolygonMax, points0In0[i]);
			}

			if(BAllEq(con0, bTrue))
			{
				//add contact
				const Vec3V proj = V3NegScaleSub(normal, t0, capsule.p0);//V3ScaleAdd(t0, normal, capsule.p0);
				//transform proj0 to 2D
				const Vec3V point = M33MulV3(rot, proj);

				if(contains(points0In0, referencePolygon.mNbVerts, point, rPolygonMin, rPolygonMax))
				{
					manifoldContacts[numContacts].mLocalPointA = aToB.transformInv(capsule.p0);
					manifoldContacts[numContacts].mLocalPointB = proj;
					manifoldContacts[numContacts++].mLocalNormalPen = V4SetW(Vec4V_From_Vec3V(normal), t0);
				}

			}

			if(BAllEq(con1, bTrue))
			{
				const Vec3V proj = V3NegScaleSub(normal, t1, capsule.p1);//V3ScaleAdd(t1, normal, capsule.p1);
				//transform proj0 to 2D
				const Vec3V point = M33MulV3(rot, proj);

				if(contains(points0In0, referencePolygon.mNbVerts, point, rPolygonMin, rPolygonMax))
				{
					manifoldContacts[numContacts].mLocalPointA = aToB.transformInv(capsule.p1);
					manifoldContacts[numContacts].mLocalPointB = proj;
					manifoldContacts[numContacts++].mLocalNormalPen = V4SetW(Vec4V_From_Vec3V(normal),t1);
				}
			}
		}

	}


	static void generatedFaceContacts(const Gu::CapsuleV& capsule,  Gu::PolygonalData& polyData, Gu::SupportLocal* map, const Ps::aos::PsMatTransformV& aToB, Gu::PersistentContact* manifoldContacts, PxU32& numContacts, 
		const Ps::aos::FloatVArg contactDist, const Ps::aos::Vec3VArg normal)
	{
		using namespace Ps::aos;

		const FloatV zero = FZero();
		FloatV tEnter=zero, tExit=zero;
		const FloatV inflatedRadius = FAdd(capsule.radius, contactDist);
		const Vec3V dir = V3Neg(normal);
		const Vec3V vertexSpaceDir = M33MulV3(map->shape2Vertex, dir);
		const Vec3V p0 = M33MulV3(map->shape2Vertex, capsule.p0);

		if(intersectSegmentPolyhedron(p0, vertexSpaceDir, polyData, tEnter, tExit) && FAllGrtrOrEq(inflatedRadius, tEnter))
		{
			manifoldContacts[numContacts].mLocalPointA = aToB.transformInv(capsule.p0);
			manifoldContacts[numContacts].mLocalPointB = V3ScaleAdd(dir, tEnter, capsule.p0);
			manifoldContacts[numContacts++].mLocalNormalPen = V4SetW(Vec4V_From_Vec3V(normal), tEnter);
		}

		const Vec3V p1 = M33MulV3(map->shape2Vertex, capsule.p1);
		if(intersectSegmentPolyhedron(p1, vertexSpaceDir, polyData, tEnter, tExit) && FAllGrtrOrEq(inflatedRadius, tEnter))
		{
			manifoldContacts[numContacts].mLocalPointA = aToB.transformInv(capsule.p1);
			manifoldContacts[numContacts].mLocalPointB = V3ScaleAdd(dir, tEnter, capsule.p1);
			manifoldContacts[numContacts++].mLocalNormalPen = V4SetW(Vec4V_From_Vec3V(normal), tEnter);
		}
		
	}


	static void generateEE(const Ps::aos::Vec3VArg p, const Ps::aos::Vec3VArg q, const Ps::aos::Vec3VArg normal, const Ps::aos::Vec3VArg a, const Ps::aos::Vec3VArg b,
		const Ps::aos::PsMatTransformV& aToB,  Gu::PersistentContact* manifoldContacts, PxU32& numContacts, const Ps::aos::FloatVArg inflatedRadius)
	{
		using namespace Ps::aos;
		const FloatV zero = FZero();
		const FloatV expandedRatio = FLoad(0.005f);
		const Vec3V ab = V3Sub(b, a);
		const Vec3V n = V3Cross(ab, normal);
		const FloatV d = V3Dot(n, a);
		const FloatV np = V3Dot(n, p);
		const FloatV nq = V3Dot(n,q);
		const FloatV signP = FSub(np, d);
		const FloatV signQ = FSub(nq, d);
		const FloatV temp = FMul(signP, signQ);
		if(FAllGrtr(temp, zero)) return;//If both points in the same side as the plane, no intersect points
		
		// if colliding edge (p3,p4) and plane are parallel return no collision
		const Vec3V pq = V3Sub(q, p);
		const FloatV npq= V3Dot(n, pq); 
		if(FAllEq(npq, zero))	return;


		//calculate the intersect point in the segment pq with plane n(x - a).
		const FloatV segTValue = FDiv(FSub(d, np), npq);
		const Vec3V localPointA = V3ScaleAdd(pq, segTValue, p);

		//ML: ab, localPointA and normal is in the same plane, so that we can do 2D segment segment intersection
		//calculate a normal perpendicular to ray localPointA + normal*t, then do 2D segment segment intersection
		const Vec3V perNormal = V3Cross(normal, pq);
		const Vec3V ap = V3Sub(localPointA, a);
		const FloatV nom = V3Dot(perNormal, ap);
		const FloatV denom = V3Dot(perNormal, ab);

		const FloatV tValue = FDiv(nom, denom);

		const FloatV max = FAdd(FOne(), expandedRatio);
		const FloatV min = FSub(zero, expandedRatio);
		if(FAllGrtr(tValue, max) || FAllGrtr(min, tValue))
			return;

		const Vec3V v = V3NegScaleSub(ab, tValue, ap);
		const FloatV signedDist = V3Dot(v, normal);
		if(FAllGrtrOrEq(inflatedRadius, signedDist))
		{
			const Vec3V localPointB = V3Sub(localPointA, v);
			manifoldContacts[numContacts].mLocalPointA = aToB.transformInv(localPointA);
			manifoldContacts[numContacts].mLocalPointB = localPointB;
			manifoldContacts[numContacts++].mLocalNormalPen = V4SetW(Vec4V_From_Vec3V(normal), signedDist);
			
		}
	}


	void generatedContactsEEContacts(const Gu::CapsuleV& capsule, Gu::PolygonalData& polyData, const Gu::HullPolygonData& referencePolygon, Gu::SupportLocal* map,  const Ps::aos::PsMatTransformV& aToB, Gu::PersistentContact* manifoldContacts, 
		PxU32& numContacts, const Ps::aos::FloatVArg contactDist, const Ps::aos::Vec3VArg contactNormal)
	{

		using namespace Ps::aos;

		const PxU8* inds = polyData.mPolygonVertexRefs + referencePolygon.mVRef8;

		Vec3V* points0In0 = (Vec3V*)PxAllocaAligned(sizeof(Vec3V)*referencePolygon.mNbVerts, 16);


		//Transform all the verts from vertex space to shape space
		map->populateVerts(inds, referencePolygon.mNbVerts, polyData.mVerts, points0In0);

		const FloatV inflatedRadius = FAdd(capsule.radius, contactDist);

		for (PxU32 rStart = 0, rEnd = PxU32(referencePolygon.mNbVerts - 1); rStart < referencePolygon.mNbVerts; rEnd = rStart++) 
		{
			generateEE(capsule.p0, capsule.p1, contactNormal,points0In0[rStart], points0In0[rEnd], aToB,  manifoldContacts, numContacts, inflatedRadius);
		
		}
	}


	bool generateCapsuleBoxFullContactManifold(const Gu::CapsuleV& capsule,  Gu::PolygonalData& polyData, Gu::SupportLocal* map, const Ps::aos::PsMatTransformV& aToB, Gu::PersistentContact* manifoldContacts, PxU32& numContacts,
		const Ps::aos::FloatVArg contactDist, Ps::aos::Vec3V& normal, const bool doOverlapTest)
	{
		using namespace Ps::aos;
		
		const PxU32 originalContacts = numContacts;
		if(doOverlapTest)
		{
			Ps::aos::FloatV minOverlap;
			//overwrite the normal
			if(!testSATCapsulePoly(capsule, polyData, map, contactDist, minOverlap, normal))
				return false;
		}

		const Gu::HullPolygonData& referencePolygon = polyData.mPolygons[getPolygonIndex(polyData, map, V3Neg(normal))];
		generatedCapsuleBoxFaceContacts(capsule, polyData, referencePolygon, map, aToB, manifoldContacts, numContacts, contactDist, normal);
	
		const PxU32 faceContacts = numContacts - originalContacts;
		if(faceContacts < 2)
		{
			generatedContactsEEContacts(capsule, polyData, referencePolygon, map, aToB, manifoldContacts, numContacts, contactDist, normal);
		}

		return true;
	}

		//capsule is in the local space of polyData
	bool generateFullContactManifold(const Gu::CapsuleV& capsule, Gu::PolygonalData& polyData, Gu::SupportLocal* map, const Ps::aos::PsMatTransformV& aToB, Gu::PersistentContact* manifoldContacts, PxU32& numContacts,
		const Ps::aos::FloatVArg contactDist, Ps::aos::Vec3V& normal, const bool doOverlapTest)
	{
		using namespace Ps::aos;

		const PxU32 originalContacts = numContacts;
	
		if(doOverlapTest)
		{	
			FloatV minOverlap;
			//overwrite the normal with the sperating axis
			if(!testSATCapsulePoly(capsule, polyData, map, contactDist, minOverlap, normal))
				return false;
		}

		generatedFaceContacts(capsule, polyData, map, aToB, manifoldContacts, numContacts, contactDist, normal);
		const PxU32 faceContacts = numContacts - originalContacts;
		if(faceContacts < 2)
		{
			const Gu::HullPolygonData& referencePolygon = polyData.mPolygons[getPolygonIndex(polyData, map, V3Neg(normal))];
			generatedContactsEEContacts(capsule, polyData,referencePolygon, map, aToB,  manifoldContacts, numContacts, contactDist, normal);
		}

		return true;
	}

	//sphere is in the local space of polyData, we treate sphere as capsule
	bool generateSphereFullContactManifold(const Gu::CapsuleV& capsule, Gu::PolygonalData& polyData, Gu::SupportLocal* map, Gu::PersistentContact* manifoldContacts, PxU32& numContacts,
		const Ps::aos::FloatVArg contactDist, Ps::aos::Vec3V& normal, const bool doOverlapTest)
	{
		using namespace Ps::aos;

		if(doOverlapTest)
		{	
			FloatV minOverlap;
			//overwrite the normal with the sperating axis
			if(!testPolyDataAxis(capsule, polyData, map, contactDist, minOverlap, normal))
				return false;
		}

		const FloatV zero = FZero();
		FloatV tEnter=zero, tExit=zero;
		const FloatV inflatedRadius = FAdd(capsule.radius, contactDist);
		const Vec3V dir = V3Neg(normal);
		const Vec3V vertexSpaceDir = M33MulV3(map->shape2Vertex, dir);
		const Vec3V p0 = M33MulV3(map->shape2Vertex, capsule.p0);
		if(intersectSegmentPolyhedron(p0, vertexSpaceDir, polyData, tEnter, tExit) && FAllGrtrOrEq(inflatedRadius, tEnter))
		{
			//ML: for sphere, the contact point A is always zeroV in its local space
			manifoldContacts[numContacts].mLocalPointA = V3Zero();
			manifoldContacts[numContacts].mLocalPointB = V3ScaleAdd(dir, tEnter, capsule.p0);
			manifoldContacts[numContacts++].mLocalNormalPen = V4SetW(Vec4V_From_Vec3V(normal), tEnter);
		}

		return true;

	}


	//capsule is in the shape space of polyData 
	bool computeMTD(const Gu::CapsuleV& capsule, Gu::PolygonalData& polyData, Gu::SupportLocal* map, Ps::aos::FloatV& penDepth, Ps::aos::Vec3V& normal)
	{
		using namespace Ps::aos;
		
		const FloatV contactDist = FZero();
		Vec3V seperatingAxis;
		FloatV minOverlap;
		if(!testSATCapsulePoly(capsule, polyData, map, contactDist, minOverlap, seperatingAxis))
			return false;

		//transform normal in to the worl space
		normal = map->transform.rotate(seperatingAxis);
		penDepth = minOverlap;
		
		return true;
	}


}//Gu
}//physx
