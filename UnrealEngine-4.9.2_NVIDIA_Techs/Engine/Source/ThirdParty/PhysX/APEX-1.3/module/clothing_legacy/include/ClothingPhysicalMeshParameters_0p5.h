/*
 * Copyright (c) 2008-2015, NVIDIA CORPORATION.  All rights reserved.
 *
 * NVIDIA CORPORATION and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA CORPORATION is strictly prohibited.
 */


// This file was generated by NxParameterized/scripts/GenParameterized.pl
// Created: 2015.06.02 04:12:11

#ifndef HEADER_ClothingPhysicalMeshParameters_0p5_h
#define HEADER_ClothingPhysicalMeshParameters_0p5_h

#include "NxParametersTypes.h"

#ifndef NX_PARAMETERIZED_ONLY_LAYOUTS
#include "NxParameterized.h"
#include "NxParameters.h"
#include "NxParameterizedTraits.h"
#include "NxTraitsInternal.h"
#endif

namespace physx
{
namespace apex
{

#pragma warning(push)
#pragma warning(disable: 4324) // structure was padded due to __declspec(align())

namespace ClothingPhysicalMeshParameters_0p5NS
{

struct PhysicalSubmesh_Type;
struct PhysicalLod_Type;
struct ConstrainCoefficient_Type;
struct PhysicalMesh_Type;
struct SkinClothMapB_Type;
struct SkinClothMapC_Type;
struct TetraLink_Type;

struct VEC3_DynamicArray1D_Type
{
	physx::PxVec3* buf;
	bool isAllocated;
	physx::PxI32 elementSize;
	physx::PxI32 arraySizes[1];
};

struct ConstrainCoefficient_DynamicArray1D_Type
{
	ConstrainCoefficient_Type* buf;
	bool isAllocated;
	physx::PxI32 elementSize;
	physx::PxI32 arraySizes[1];
};

struct U16_DynamicArray1D_Type
{
	physx::PxU16* buf;
	bool isAllocated;
	physx::PxI32 elementSize;
	physx::PxI32 arraySizes[1];
};

struct F32_DynamicArray1D_Type
{
	physx::PxF32* buf;
	bool isAllocated;
	physx::PxI32 elementSize;
	physx::PxI32 arraySizes[1];
};

struct U32_DynamicArray1D_Type
{
	physx::PxU32* buf;
	bool isAllocated;
	physx::PxI32 elementSize;
	physx::PxI32 arraySizes[1];
};

struct PhysicalSubmesh_DynamicArray1D_Type
{
	PhysicalSubmesh_Type* buf;
	bool isAllocated;
	physx::PxI32 elementSize;
	physx::PxI32 arraySizes[1];
};

struct PhysicalLod_DynamicArray1D_Type
{
	PhysicalLod_Type* buf;
	bool isAllocated;
	physx::PxI32 elementSize;
	physx::PxI32 arraySizes[1];
};

struct SkinClothMapB_DynamicArray1D_Type
{
	SkinClothMapB_Type* buf;
	bool isAllocated;
	physx::PxI32 elementSize;
	physx::PxI32 arraySizes[1];
};

struct SkinClothMapC_DynamicArray1D_Type
{
	SkinClothMapC_Type* buf;
	bool isAllocated;
	physx::PxI32 elementSize;
	physx::PxI32 arraySizes[1];
};

struct SkinClothMapC_Type
{
	physx::PxVec3 vertexBary;
	physx::PxU32 faceIndex0;
	physx::PxVec3 normalBary;
	physx::PxU32 vertexIndexPlusOffset;
};
struct PhysicalSubmesh_Type
{
	physx::PxU32 numIndices;
	physx::PxU32 numVertices;
	physx::PxU32 numMaxDistance0Vertices;
};
struct PhysicalLod_Type
{
	physx::PxU32 costWithoutIterations;
	physx::PxU32 submeshId;
	physx::PxF32 solverIterationScale;
	physx::PxF32 maxDistanceReduction;
};
struct ConstrainCoefficient_Type
{
	physx::PxF32 maxDistance;
	physx::PxF32 collisionSphereRadius;
	physx::PxF32 collisionSphereDistance;
};
struct PhysicalMesh_Type
{
	physx::PxU32 numVertices;
	physx::PxU32 numIndices;
	physx::PxU32 numBonesPerVertex;
	VEC3_DynamicArray1D_Type vertices;
	VEC3_DynamicArray1D_Type normals;
	VEC3_DynamicArray1D_Type skinningNormals;
	ConstrainCoefficient_DynamicArray1D_Type constrainCoefficients;
	U16_DynamicArray1D_Type boneIndices;
	F32_DynamicArray1D_Type boneWeights;
	U32_DynamicArray1D_Type indices;
	physx::PxF32 maximumMaxDistance;
	physx::PxF32 shortestEdgeLength;
	physx::PxF32 averageEdgeLength;
	bool isTetrahedralMesh;
	bool flipNormals;
};
struct SkinClothMapB_Type
{
	physx::PxVec3 vtxTetraBary;
	physx::PxU32 vertexIndexPlusOffset;
	physx::PxVec3 nrmTetraBary;
	physx::PxU32 faceIndex0;
	physx::PxU32 tetraIndex;
	physx::PxU32 submeshIndex;
};
struct TetraLink_Type
{
	physx::PxVec3 vertexBary;
	physx::PxU32 tetraIndex0;
	physx::PxVec3 normalBary;
	physx::PxU32 _dummyForAlignment;
};

struct ParametersStruct
{

	PhysicalMesh_Type physicalMesh;
	PhysicalSubmesh_DynamicArray1D_Type submeshes;
	PhysicalLod_DynamicArray1D_Type physicalLods;
	SkinClothMapB_DynamicArray1D_Type transitionUpB;
	SkinClothMapC_DynamicArray1D_Type transitionUpC;
	physx::PxF32 transitionUpThickness;
	physx::PxF32 transitionUpOffset;
	SkinClothMapB_DynamicArray1D_Type transitionDownB;
	SkinClothMapC_DynamicArray1D_Type transitionDownC;
	physx::PxF32 transitionDownThickness;
	physx::PxF32 transitionDownOffset;
	physx::PxU32 referenceCount;

};

static const physx::PxU32 checksum[] = { 0x6baed1a8, 0x1687e4a1, 0x0569f5e0, 0x32d0cc3a, };

} // namespace ClothingPhysicalMeshParameters_0p5NS

#ifndef NX_PARAMETERIZED_ONLY_LAYOUTS
class ClothingPhysicalMeshParameters_0p5 : public NxParameterized::NxParameters, public ClothingPhysicalMeshParameters_0p5NS::ParametersStruct
{
public:
	ClothingPhysicalMeshParameters_0p5(NxParameterized::Traits* traits, void* buf = 0, PxI32* refCount = 0);

	virtual ~ClothingPhysicalMeshParameters_0p5();

	virtual void destroy();

	static const char* staticClassName(void)
	{
		return("ClothingPhysicalMeshParameters");
	}

	const char* className(void) const
	{
		return(staticClassName());
	}

	static const physx::PxU32 ClassVersion = ((physx::PxU32)0 << 16) + (physx::PxU32)5;

	static physx::PxU32 staticVersion(void)
	{
		return ClassVersion;
	}

	physx::PxU32 version(void) const
	{
		return(staticVersion());
	}

	static const physx::PxU32 ClassAlignment = 8;

	static const physx::PxU32* staticChecksum(physx::PxU32& bits)
	{
		bits = 8 * sizeof(ClothingPhysicalMeshParameters_0p5NS::checksum);
		return ClothingPhysicalMeshParameters_0p5NS::checksum;
	}

	static void freeParameterDefinitionTable(NxParameterized::Traits* traits);

	const physx::PxU32* checksum(physx::PxU32& bits) const
	{
		return staticChecksum(bits);
	}

	const ClothingPhysicalMeshParameters_0p5NS::ParametersStruct& parameters(void) const
	{
		ClothingPhysicalMeshParameters_0p5* tmpThis = const_cast<ClothingPhysicalMeshParameters_0p5*>(this);
		return *(static_cast<ClothingPhysicalMeshParameters_0p5NS::ParametersStruct*>(tmpThis));
	}

	ClothingPhysicalMeshParameters_0p5NS::ParametersStruct& parameters(void)
	{
		return *(static_cast<ClothingPhysicalMeshParameters_0p5NS::ParametersStruct*>(this));
	}

	virtual NxParameterized::ErrorType getParameterHandle(const char* long_name, NxParameterized::Handle& handle) const;
	virtual NxParameterized::ErrorType getParameterHandle(const char* long_name, NxParameterized::Handle& handle);

	void initDefaults(void);

protected:

	virtual const NxParameterized::DefinitionImpl* getParameterDefinitionTree(void);
	virtual const NxParameterized::DefinitionImpl* getParameterDefinitionTree(void) const;


	virtual void getVarPtr(const NxParameterized::Handle& handle, void*& ptr, size_t& offset) const;

private:

	void buildTree(void);
	void initDynamicArrays(void);
	void initStrings(void);
	void initReferences(void);
	void freeDynamicArrays(void);
	void freeStrings(void);
	void freeReferences(void);

	static bool mBuiltFlag;
	static NxParameterized::MutexType mBuiltFlagMutex;
};

class ClothingPhysicalMeshParameters_0p5Factory : public NxParameterized::Factory
{
	static const char* const vptr;

public:
	virtual NxParameterized::Interface* create(NxParameterized::Traits* paramTraits)
	{
		// placement new on this class using mParameterizedTraits

		void* newPtr = paramTraits->alloc(sizeof(ClothingPhysicalMeshParameters_0p5), ClothingPhysicalMeshParameters_0p5::ClassAlignment);
		if (!NxParameterized::IsAligned(newPtr, ClothingPhysicalMeshParameters_0p5::ClassAlignment))
		{
			NX_PARAM_TRAITS_WARNING(paramTraits, "Unaligned memory allocation for class ClothingPhysicalMeshParameters_0p5");
			paramTraits->free(newPtr);
			return 0;
		}

		memset(newPtr, 0, sizeof(ClothingPhysicalMeshParameters_0p5)); // always initialize memory allocated to zero for default values
		return NX_PARAM_PLACEMENT_NEW(newPtr, ClothingPhysicalMeshParameters_0p5)(paramTraits);
	}

	virtual NxParameterized::Interface* finish(NxParameterized::Traits* paramTraits, void* bufObj, void* bufStart, physx::PxI32* refCount)
	{
		if (!NxParameterized::IsAligned(bufObj, ClothingPhysicalMeshParameters_0p5::ClassAlignment)
		        || !NxParameterized::IsAligned(bufStart, ClothingPhysicalMeshParameters_0p5::ClassAlignment))
		{
			NX_PARAM_TRAITS_WARNING(paramTraits, "Unaligned memory allocation for class ClothingPhysicalMeshParameters_0p5");
			return 0;
		}

		// Init NxParameters-part
		// We used to call empty constructor of ClothingPhysicalMeshParameters_0p5 here
		// but it may call default constructors of members and spoil the data
		NX_PARAM_PLACEMENT_NEW(bufObj, NxParameterized::NxParameters)(paramTraits, bufStart, refCount);

		// Init vtable (everything else is already initialized)
		*(const char**)bufObj = vptr;

		return (ClothingPhysicalMeshParameters_0p5*)bufObj;
	}

	virtual const char* getClassName()
	{
		return (ClothingPhysicalMeshParameters_0p5::staticClassName());
	}

	virtual physx::PxU32 getVersion()
	{
		return (ClothingPhysicalMeshParameters_0p5::staticVersion());
	}

	virtual physx::PxU32 getAlignment()
	{
		return (ClothingPhysicalMeshParameters_0p5::ClassAlignment);
	}

	virtual const physx::PxU32* getChecksum(physx::PxU32& bits)
	{
		return (ClothingPhysicalMeshParameters_0p5::staticChecksum(bits));
	}
};
#endif // NX_PARAMETERIZED_ONLY_LAYOUTS

} // namespace apex
} // namespace physx

#pragma warning(pop)

#endif
