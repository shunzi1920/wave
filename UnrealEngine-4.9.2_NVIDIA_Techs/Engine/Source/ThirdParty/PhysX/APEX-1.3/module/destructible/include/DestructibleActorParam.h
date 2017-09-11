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
// Created: 2015.06.02 04:11:51

#ifndef HEADER_DestructibleActorParam_h
#define HEADER_DestructibleActorParam_h

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
namespace destructible
{

#pragma warning(push)
#pragma warning(disable: 4324) // structure was padded due to __declspec(align())

namespace DestructibleActorParamNS
{

struct GroupsMask_Type;
struct LODWeights_Type;
struct ShapeDescFlags_Type;
struct ShapeDescTemplate_Type;
struct ContactPairFlag_Type;
struct ActorDescFlags_Type;
struct ActorDescTemplate_Type;
struct BodyDescFlags_Type;
struct BodyDescTemplate_Type;
struct DestructibleDepthParameters_Type;
struct DestructibleParametersFlag_Type;
struct FractureGlass_Type;
struct FractureVoronoi_Type;
struct FractureAttachment_Type;
struct RuntimeFracture_Type;
struct DestructibleParameters_Type;
struct DamageSpreadFunction_Type;
struct P3FilterData_Type;
struct P3ShapeFlags_Type;
struct P3PairFlag_Type;
struct P3ShapeDescTemplate_Type;
struct P3ActorFlags_Type;
struct P3ActorDescTemplate_Type;
struct P3BodyDescFlags_Type;
struct P3BodyDescTemplate_Type;
struct StructureSettings_Type;
struct BehaviorGroup_Type;

struct STRING_DynamicArray1D_Type
{
	NxParameterized::DummyStringStruct* buf;
	bool isAllocated;
	physx::PxI32 elementSize;
	physx::PxI32 arraySizes[1];
};

struct DestructibleDepthParameters_DynamicArray1D_Type
{
	DestructibleDepthParameters_Type* buf;
	bool isAllocated;
	physx::PxI32 elementSize;
	physx::PxI32 arraySizes[1];
};

struct BehaviorGroup_DynamicArray1D_Type
{
	BehaviorGroup_Type* buf;
	bool isAllocated;
	physx::PxI32 elementSize;
	physx::PxI32 arraySizes[1];
};

struct DestructibleDepthParameters_Type
{
	bool OVERRIDE_IMPACT_DAMAGE;
	bool OVERRIDE_IMPACT_DAMAGE_VALUE;
	bool IGNORE_POSE_UPDATES;
	bool IGNORE_RAYCAST_CALLBACKS;
	bool IGNORE_CONTACT_CALLBACKS;
	bool USER_FLAG_0;
	bool USER_FLAG_1;
	bool USER_FLAG_2;
	bool USER_FLAG_3;
};
struct P3BodyDescFlags_Type
{
	bool eKINEMATIC;
	bool eENABLE_CCD;
};
struct GroupsMask_Type
{
	bool useGroupsMask;
	physx::PxU32 bits0;
	physx::PxU32 bits1;
	physx::PxU32 bits2;
	physx::PxU32 bits3;
};
struct DestructibleParametersFlag_Type
{
	bool ACCUMULATE_DAMAGE;
	bool DEBRIS_TIMEOUT;
	bool DEBRIS_MAX_SEPARATION;
	bool CRUMBLE_SMALLEST_CHUNKS;
	bool ACCURATE_RAYCASTS;
	bool USE_VALID_BOUNDS;
	bool CRUMBLE_VIA_RUNTIME_FRACTURE;
};
struct ShapeDescFlags_Type
{
	bool NX_TRIGGER_ON_ENTER;
	bool NX_TRIGGER_ON_LEAVE;
	bool NX_TRIGGER_ON_STAY;
	bool NX_SF_VISUALIZATION;
	bool NX_SF_DISABLE_COLLISION;
	bool NX_SF_FEATURE_INDICES;
	bool NX_SF_DISABLE_RAYCASTING;
	bool NX_SF_POINT_CONTACT_FORCE;
	bool NX_SF_FLUID_DRAIN;
	bool NX_SF_FLUID_DISABLE_COLLISION;
	bool NX_SF_FLUID_TWOWAY;
	bool NX_SF_DISABLE_RESPONSE;
	bool NX_SF_DYNAMIC_DYNAMIC_CCD;
	bool NX_SF_DISABLE_SCENE_QUERIES;
	bool NX_SF_CLOTH_DRAIN;
	bool NX_SF_CLOTH_DISABLE_COLLISION;
	bool NX_SF_CLOTH_TWOWAY;
	bool NX_SF_SOFTBODY_DRAIN;
	bool NX_SF_SOFTBODY_DISABLE_COLLISION;
	bool NX_SF_SOFTBODY_TWOWAY;
};
struct ShapeDescTemplate_Type
{
	ShapeDescFlags_Type flags;
	physx::PxU16 collisionGroup;
	GroupsMask_Type groupsMask;
	physx::PxU16 materialIndex;
	physx::PxF32 density;
	physx::PxF32 skinWidth;
	physx::PxU64 userData;
	physx::PxU64 name;
};
struct P3FilterData_Type
{
	physx::PxU32 word0;
	physx::PxU32 word1;
	physx::PxU32 word2;
	physx::PxU32 word3;
};
struct StructureSettings_Type
{
	bool useStressSolver;
	physx::PxF32 stressSolverTimeDelay;
	physx::PxF32 stressSolverMassThreshold;
};
struct FractureGlass_Type
{
	physx::PxU32 numSectors;
	physx::PxF32 sectorRand;
	physx::PxF32 firstSegmentSize;
	physx::PxF32 segmentScale;
	physx::PxF32 segmentRand;
};
struct DamageSpreadFunction_Type
{
	physx::PxF32 minimumRadius;
	physx::PxF32 radiusMultiplier;
	physx::PxF32 falloffExponent;
};
struct FractureVoronoi_Type
{
	physx::PxVec3 dimensions;
	physx::PxU32 numCells;
	physx::PxF32 biasExp;
	physx::PxF32 maxDist;
};
struct BehaviorGroup_Type
{
	NxParameterized::DummyStringStruct name;
	physx::PxF32 damageThreshold;
	physx::PxF32 damageToRadius;
	DamageSpreadFunction_Type damageSpread;
	DamageSpreadFunction_Type damageColorSpread;
	physx::PxVec4 damageColorChange;
	physx::PxF32 materialStrength;
	physx::PxF32 density;
	physx::PxF32 fadeOut;
	physx::PxF32 maxDepenetrationVelocity;
	GroupsMask_Type groupsMask;
	physx::PxU64 userData;
};
struct P3ShapeFlags_Type
{
	bool eSIMULATION_SHAPE;
	bool eSCENE_QUERY_SHAPE;
	bool eTRIGGER_SHAPE;
	bool eVISUALIZATION;
	bool ePARTICLE_DRAIN;
	bool eDEFORMABLE_DRAIN;
};
struct P3ShapeDescTemplate_Type
{
	P3ShapeFlags_Type flags;
	P3FilterData_Type simulationFilterData;
	P3FilterData_Type queryFilterData;
	physx::PxU64 material;
	physx::PxF32 contactOffset;
	physx::PxF32 restOffset;
	physx::PxU64 userData;
	physx::PxU64 name;
};
struct P3ActorFlags_Type
{
	bool eVISUALIZATION;
	bool eDISABLE_GRAVITY;
	bool eSEND_SLEEP_NOTIFIES;
};
struct P3BodyDescTemplate_Type
{
	physx::PxF32 density;
	P3BodyDescFlags_Type flags;
	physx::PxF32 sleepThreshold;
	physx::PxF32 wakeUpCounter;
	physx::PxF32 linearDamping;
	physx::PxF32 angularDamping;
	physx::PxF32 maxAngularVelocity;
	physx::PxU32 solverIterationCount;
	physx::PxU32 velocityIterationCount;
	physx::PxF32 contactReportThreshold;
	physx::PxF32 sleepLinearVelocity;
};
struct ContactPairFlag_Type
{
	bool NX_IGNORE_PAIR;
	bool NX_NOTIFY_ON_START_TOUCH;
	bool NX_NOTIFY_ON_END_TOUCH;
	bool NX_NOTIFY_ON_TOUCH;
	bool NX_NOTIFY_ON_IMPACT;
	bool NX_NOTIFY_ON_ROLL;
	bool NX_NOTIFY_ON_SLIDE;
	bool NX_NOTIFY_FORCES;
	bool NX_NOTIFY_ON_START_TOUCH_FORCE_THRESHOLD;
	bool NX_NOTIFY_ON_END_TOUCH_FORCE_THRESHOLD;
	bool NX_NOTIFY_ON_TOUCH_FORCE_THRESHOLD;
	bool NX_NOTIFY_CONTACT_MODIFICATION;
};
struct LODWeights_Type
{
	physx::PxF32 maxDistance;
	physx::PxF32 distanceWeight;
	physx::PxF32 maxAge;
	physx::PxF32 ageWeight;
	physx::PxF32 bias;
};
struct BodyDescFlags_Type
{
	bool NX_BF_DISABLE_GRAVITY;
	bool NX_BF_FILTER_SLEEP_VEL;
	bool NX_BF_ENERGY_SLEEP_TEST;
	bool NX_BF_VISUALIZATION;
};
struct BodyDescTemplate_Type
{
	BodyDescFlags_Type flags;
	physx::PxF32 wakeUpCounter;
	physx::PxF32 linearDamping;
	physx::PxF32 angularDamping;
	physx::PxF32 maxAngularVelocity;
	physx::PxF32 CCDMotionThreshold;
	physx::PxF32 sleepLinearVelocity;
	physx::PxF32 sleepAngularVelocity;
	physx::PxU32 solverIterationCount;
	physx::PxF32 sleepEnergyThreshold;
	physx::PxF32 sleepDamping;
	physx::PxF32 contactReportThreshold;
};
struct FractureAttachment_Type
{
	bool posX;
	bool negX;
	bool posY;
	bool negY;
	bool posZ;
	bool negZ;
};
struct RuntimeFracture_Type
{
	const char* RuntimeFractureType;
	bool sheetFracture;
	physx::PxU32 depthLimit;
	bool destroyIfAtDepthLimit;
	physx::PxF32 minConvexSize;
	physx::PxF32 impulseScale;
	FractureGlass_Type glass;
	FractureVoronoi_Type voronoi;
	FractureAttachment_Type attachment;
};
struct DestructibleParameters_Type
{
	physx::PxF32 damageCap;
	physx::PxF32 forceToDamage;
	physx::PxF32 impactVelocityThreshold;
	physx::PxU32 minimumFractureDepth;
	physx::PxI32 impactDamageDefaultDepth;
	physx::PxI32 debrisDepth;
	physx::PxU32 essentialDepth;
	physx::PxF32 debrisLifetimeMin;
	physx::PxF32 debrisLifetimeMax;
	physx::PxF32 debrisMaxSeparationMin;
	physx::PxF32 debrisMaxSeparationMax;
	physx::PxF32 debrisDestructionProbability;
	physx::PxBounds3 validBounds;
	physx::PxF32 maxChunkSpeed;
	DestructibleParametersFlag_Type flags;
	physx::PxF32 fractureImpulseScale;
	physx::PxU16 damageDepthLimit;
	physx::PxU16 dynamicChunkDominanceGroup;
	GroupsMask_Type dynamicChunksGroupsMask;
	RuntimeFracture_Type runtimeFracture;
	physx::PxF32 supportStrength;
	physx::PxI8 legacyChunkBoundsTestSetting;
	physx::PxI8 legacyDamageRadiusSpreadSetting;
};
struct ActorDescFlags_Type
{
	bool NX_AF_DISABLE_COLLISION;
	bool NX_AF_DISABLE_RESPONSE;
	bool NX_AF_LOCK_COM;
	bool NX_AF_FLUID_DISABLE_COLLISION;
	bool NX_AF_CONTACT_MODIFICATION;
	bool NX_AF_FORCE_CONE_FRICTION;
	bool NX_AF_USER_ACTOR_PAIR_FILTERING;
};
struct ActorDescTemplate_Type
{
	ActorDescFlags_Type flags;
	physx::PxF32 density;
	physx::PxU16 actorCollisionGroup;
	physx::PxU16 dominanceGroup;
	ContactPairFlag_Type contactReportFlags;
	physx::PxU16 forceFieldMaterial;
	physx::PxU64 userData;
	physx::PxU64 name;
	physx::PxU64 compartment;
};
struct P3PairFlag_Type
{
	bool eSOLVE_CONTACT;
	bool eMODIFY_CONTACTS;
	bool eNOTIFY_TOUCH_FOUND;
	bool eNOTIFY_TOUCH_PERSISTS;
	bool eNOTIFY_TOUCH_LOST;
	bool eNOTIFY_THRESHOLD_FORCE_FOUND;
	bool eNOTIFY_THRESHOLD_FORCE_PERSISTS;
	bool eNOTIFY_THRESHOLD_FORCE_LOST;
	bool eNOTIFY_CONTACT_POINTS;
	bool eNOTIFY_CONTACT_FORCES;
	bool eNOTIFY_CONTACT_FORCE_PER_POINT;
	bool eNOTIFY_CONTACT_FEATURE_INDICES_PER_POINT;
	bool eDETECT_CCD_CONTACT;
	bool eCONTACT_DEFAULT;
	bool eTRIGGER_DEFAULT;
};
struct P3ActorDescTemplate_Type
{
	P3ActorFlags_Type flags;
	physx::PxU8 dominanceGroup;
	physx::PxU8 ownerClient;
	physx::PxU32 clientBehaviorBits;
	P3PairFlag_Type contactReportFlags;
	physx::PxU64 userData;
	physx::PxU64 name;
};

struct ParametersStruct
{

	NxParameterized::DummyStringStruct crumbleEmitterName;
	physx::PxF32 crumbleParticleSpacing;
	NxParameterized::DummyStringStruct dustEmitterName;
	physx::PxF32 dustParticleSpacing;
	physx::PxMat34Legacy globalPose;
	physx::PxVec3 scale;
	bool dynamic;
	physx::PxU32 supportDepth;
	bool formExtendedStructures;
	bool performDetailedOverlapTestForExtendedStructures;
	bool keepPreviousFrameBoneBuffer;
	bool doNotCreateRenderable;
	bool useAssetDefinedSupport;
	bool useWorldSupport;
	bool renderStaticChunksSeparately;
	bool keepVisibleBonesPacked;
	bool createChunkEvents;
	STRING_DynamicArray1D_Type overrideSkinnedMaterialNames;
	STRING_DynamicArray1D_Type overrideStaticMaterialNames;
	physx::PxF32 sleepVelocityFrameDecayConstant;
	bool useHardSleeping;
	DestructibleParameters_Type destructibleParameters;
	DestructibleDepthParameters_DynamicArray1D_Type depthParameters;
	ShapeDescTemplate_Type shapeDescTemplate;
	ActorDescTemplate_Type actorDescTemplate;
	BodyDescTemplate_Type bodyDescTemplate;
	P3ShapeDescTemplate_Type p3ShapeDescTemplate;
	P3ActorDescTemplate_Type p3ActorDescTemplate;
	P3BodyDescTemplate_Type p3BodyDescTemplate;
	StructureSettings_Type structureSettings;
	BehaviorGroup_Type defaultBehaviorGroup;
	BehaviorGroup_DynamicArray1D_Type behaviorGroups;
	bool deleteChunksLeavingUserDefinedBB;
	bool deleteChunksEnteringUserDefinedBB;

};

static const physx::PxU32 checksum[] = { 0x57ae1ad0, 0xccc3c9a4, 0xa3716200, 0xfd2df802, };

} // namespace DestructibleActorParamNS

#ifndef NX_PARAMETERIZED_ONLY_LAYOUTS
class DestructibleActorParam : public NxParameterized::NxParameters, public DestructibleActorParamNS::ParametersStruct
{
public:
	DestructibleActorParam(NxParameterized::Traits* traits, void* buf = 0, PxI32* refCount = 0);

	virtual ~DestructibleActorParam();

	virtual void destroy();

	static const char* staticClassName(void)
	{
		return("DestructibleActorParam");
	}

	const char* className(void) const
	{
		return(staticClassName());
	}

	static const physx::PxU32 ClassVersion = ((physx::PxU32)0 << 16) + (physx::PxU32)30;

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
		bits = 8 * sizeof(DestructibleActorParamNS::checksum);
		return DestructibleActorParamNS::checksum;
	}

	static void freeParameterDefinitionTable(NxParameterized::Traits* traits);

	const physx::PxU32* checksum(physx::PxU32& bits) const
	{
		return staticChecksum(bits);
	}

	const DestructibleActorParamNS::ParametersStruct& parameters(void) const
	{
		DestructibleActorParam* tmpThis = const_cast<DestructibleActorParam*>(this);
		return *(static_cast<DestructibleActorParamNS::ParametersStruct*>(tmpThis));
	}

	DestructibleActorParamNS::ParametersStruct& parameters(void)
	{
		return *(static_cast<DestructibleActorParamNS::ParametersStruct*>(this));
	}

	virtual NxParameterized::ErrorType getParameterHandle(const char* long_name, NxParameterized::Handle& handle) const;
	virtual NxParameterized::ErrorType getParameterHandle(const char* long_name, NxParameterized::Handle& handle);

	void initDefaults(void);

	NxParameterized::ErrorType copy(const NxParameterized::Interface &other);
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

class DestructibleActorParamFactory : public NxParameterized::Factory
{
	static const char* const vptr;

public:
	virtual NxParameterized::Interface* create(NxParameterized::Traits* paramTraits)
	{
		// placement new on this class using mParameterizedTraits

		void* newPtr = paramTraits->alloc(sizeof(DestructibleActorParam), DestructibleActorParam::ClassAlignment);
		if (!NxParameterized::IsAligned(newPtr, DestructibleActorParam::ClassAlignment))
		{
			NX_PARAM_TRAITS_WARNING(paramTraits, "Unaligned memory allocation for class DestructibleActorParam");
			paramTraits->free(newPtr);
			return 0;
		}

		memset(newPtr, 0, sizeof(DestructibleActorParam)); // always initialize memory allocated to zero for default values
		return NX_PARAM_PLACEMENT_NEW(newPtr, DestructibleActorParam)(paramTraits);
	}

	virtual NxParameterized::Interface* finish(NxParameterized::Traits* paramTraits, void* bufObj, void* bufStart, physx::PxI32* refCount)
	{
		if (!NxParameterized::IsAligned(bufObj, DestructibleActorParam::ClassAlignment)
		        || !NxParameterized::IsAligned(bufStart, DestructibleActorParam::ClassAlignment))
		{
			NX_PARAM_TRAITS_WARNING(paramTraits, "Unaligned memory allocation for class DestructibleActorParam");
			return 0;
		}

		// Init NxParameters-part
		// We used to call empty constructor of DestructibleActorParam here
		// but it may call default constructors of members and spoil the data
		NX_PARAM_PLACEMENT_NEW(bufObj, NxParameterized::NxParameters)(paramTraits, bufStart, refCount);

		// Init vtable (everything else is already initialized)
		*(const char**)bufObj = vptr;

		return (DestructibleActorParam*)bufObj;
	}

	virtual const char* getClassName()
	{
		return (DestructibleActorParam::staticClassName());
	}

	virtual physx::PxU32 getVersion()
	{
		return (DestructibleActorParam::staticVersion());
	}

	virtual physx::PxU32 getAlignment()
	{
		return (DestructibleActorParam::ClassAlignment);
	}

	virtual const physx::PxU32* getChecksum(physx::PxU32& bits)
	{
		return (DestructibleActorParam::staticChecksum(bits));
	}
};
#endif // NX_PARAMETERIZED_ONLY_LAYOUTS

} // namespace destructible
} // namespace apex
} // namespace physx

#pragma warning(pop)

#endif
