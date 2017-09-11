// NVCHANGE_BEGIN: JCAO - Add Field Sampler Asset

#include "EnginePrivate.h"
#include "../PhysicsEngine/PhysXSupport.h"

UVelocitySourceAsset::UVelocitySourceAsset(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// VelocitySource Parameters
	AverageVelocity = 5.0f;
	StandardVelocity = 5.0f;

	Radius = 100.0f;
	Extents = FVector(100.0f, 100.0f, 100.0f);

	GeometryType = EGeometryType_Sphere;

}

#if WITH_APEX_TURBULENCE
#define ITERATE_ASSET_PARAMS() \
	PROCESS_ASSET_PARAM(F32,  physx::PxF32,  "averageVelocity", AverageVelocity) \
	PROCESS_ASSET_PARAM(F32,  physx::PxF32,  "stdVelocity", StandardVelocity) \


const char* UVelocitySourceAsset::GetApexAssetObjTypeName()
{
	return NX_VELOCITY_SOURCE_AUTHORING_TYPE_NAME;
}

void UVelocitySourceAsset::SetupApexAssetParams(NxParameterized::Interface* Params)
{
	if (Params != NULL)
	{
		NxParameterized::Handle h(*Params, "geometryType");
		if (GeometryType == EGeometryType_Sphere)
		{
			h.initParamRef("HeatSourceGeomSphereParams", true);
			verify(NxParameterized::setParamF32(*Params, "geometryType.radius", Radius));
		}
		else if (GeometryType == EGeometryType_Box)
		{
			h.initParamRef("HeatSourceGeomBoxParams", true);
			verify(NxParameterized::setParamVec3(*Params, "geometryType.extents", U2PVector(Extents)));
		}

#define PROCESS_ASSET_PARAM(_typeName, _type, _paramName, _value) \
		verify(NxParameterized::setParam##_typeName(*Params, _paramName, _value));

		ITERATE_ASSET_PARAMS()
#undef PROCESS_ASSET_PARAM
	}
}
#endif // WITH_APEX_TURBULENCE

// NVCHANGE_END: JCAO - Add Field Sampler Asset