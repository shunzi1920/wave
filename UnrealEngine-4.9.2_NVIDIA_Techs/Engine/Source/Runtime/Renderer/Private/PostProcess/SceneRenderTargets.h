// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	SceneRenderTargets.h: Scene render target definitions.
=============================================================================*/

#pragma once

#include "ShaderParameters.h"
#include "RenderTargetPool.h"
#include "../SystemTextures.h"
#include "RHIStaticStates.h"

// NVCHANGE_BEGIN: Add VXGI
#if WITH_GFSDK_VXGI
#include "GFSDK_VXGI.h"
#define NUM_SHADOW_CASCADE_SURFACES 4
#endif
// NVCHANGE_END: Add VXGI

struct IPooledRenderTarget;

/** Number of cube map shadow depth surfaces that will be created and used for rendering one pass point light shadows. */
static const int32 NumCubeShadowDepthSurfaces = 5;

/** 
 * Allocate enough sets of translucent volume textures to cover all the cascades, 
 * And then one more which will be used as a scratch target when doing ping-pong operations like filtering. 
 */
static const int32 NumTranslucentVolumeRenderTargetSets = TVC_MAX + 1;

/** Forward declaration of console variable controlling translucent volume blur */
extern int32 GUseTranslucencyVolumeBlur;

/** Forward declaration of console variable controlling translucent lighting volume dimensions */
extern int32 GTranslucencyLightingVolumeDim;

/** Function to select the index of the volume texture that we will hold the final translucency lighting volume texture */
inline int SelectTranslucencyVolumeTarget(ETranslucencyVolumeCascade InCascade)
{
	if (GUseTranslucencyVolumeBlur)
	{
		switch (InCascade)
		{
		case TVC_Inner:
			{
				return 2;
			}
		case TVC_Outer:
			{
				return 0;
			}
		default:
			{
				// error
				check(false);
				return 0;
			}
		}
	}
	else
	{
		switch (InCascade)
		{
		case TVC_Inner:
			{
				return 0;
			}
		case TVC_Outer:
			{
				return 1;
			}
		default:
			{
				// error
				check(false);
				return 0;
			}
		}
	}
}

/** Number of surfaces used for translucent shadows. */
static const int32 NumTranslucencyShadowSurfaces = 2;

BEGIN_UNIFORM_BUFFER_STRUCT(FGBufferResourceStruct, )
	DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER_TEXTURE( Texture2D,			GBufferATexture )
	DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER_TEXTURE( Texture2D,			GBufferBTexture )
	DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER_TEXTURE( Texture2D,			GBufferCTexture )
	DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER_TEXTURE( Texture2D,			GBufferDTexture )
	DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER_TEXTURE( Texture2D,			GBufferETexture )
	DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER_TEXTURE( Texture2D,			GBufferVelocityTexture )
	DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER_TEXTURE( Texture2D<float4>,	GBufferATextureNonMS )
	DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER_TEXTURE( Texture2D<float4>,	GBufferBTextureNonMS )
	DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER_TEXTURE( Texture2D<float4>,	GBufferCTextureNonMS )
	DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER_TEXTURE( Texture2D<float4>,	GBufferDTextureNonMS )
	DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER_TEXTURE( Texture2D<float4>,	GBufferETextureNonMS )
	DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER_TEXTURE( Texture2D<float4>,	GBufferVelocityTextureNonMS )
	DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER_TEXTURE( Texture2DMS<float4>,	GBufferATextureMS )
	DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER_TEXTURE( Texture2DMS<float4>,	GBufferBTextureMS )
	DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER_TEXTURE( Texture2DMS<float4>,	GBufferCTextureMS )
	DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER_TEXTURE( Texture2DMS<float4>,	GBufferDTextureMS )
	DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER_TEXTURE( Texture2DMS<float4>,	GBufferETextureMS )
	DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER_TEXTURE( Texture2DMS<float4>,	GBufferVelocityTextureMS )
	DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER_SAMPLER( SamplerState,			GBufferATextureSampler )
	DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER_SAMPLER( SamplerState,			GBufferBTextureSampler )
	DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER_SAMPLER( SamplerState,			GBufferCTextureSampler )
	DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER_SAMPLER( SamplerState,			GBufferDTextureSampler )
	DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER_SAMPLER( SamplerState,			GBufferETextureSampler )
	DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER_SAMPLER( SamplerState,			GBufferVelocityTextureSampler)
END_UNIFORM_BUFFER_STRUCT( FGBufferResourceStruct )

/**
 * Encapsulates the render targets used for scene rendering.
 */
class RENDERER_API FSceneRenderTargets : public FRenderResource
{
public:
	/** Destructor. */
	virtual ~FSceneRenderTargets() {}

	/** Singletons. At the moment parallel tasks get their snapshot from the rhicmdlist */
	static FSceneRenderTargets& Get(FRHICommandList& RHICmdList);
	static FSceneRenderTargets& Get(FRHICommandListImmediate& RHICmdList);
	// this is a placeholder, the context should come from somewhere. This is very unsafe, please don't use it!
	static FSceneRenderTargets& Get_Todo_PassContext();
	// As above but relaxed checks and always gives the global FSceneRenderTargets. The intention here is that it is only used for constants that don't change during a frame. This is very unsafe, please don't use it!
	static FSceneRenderTargets& Get_FrameConstantsOnly();

	/** Create a snapshot on the scene allocator */
	FSceneRenderTargets* CreateSnapshot(const FViewInfo& InView);
	/** Set a snapshot on the TargetCmdList */
	void SetSnapshotOnCmdList(FRHICommandList& TargetCmdList);	
	/** Destruct all snapshots */
	void DestroyAllSnapshots();

protected:
	/** Constructor */
	FSceneRenderTargets(): 
		bScreenSpaceAOIsValid(false),
		bCustomDepthIsValid(false),
		bPreshadowCacheNewlyAllocated(false),
		GBufferRefCount(0),
		LargestDesiredSizeThisFrame( 0, 0 ),
		LargestDesiredSizeLastFrame( 0, 0 ),
		ThisFrameNumber( 0 ),
		bVelocityPass(false),
		BufferSize(0, 0), 
		SmallColorDepthDownsampleFactor(2),
		bLightAttenuationEnabled(true),
		bUseDownsizedOcclusionQueries(true),
		CurrentGBufferFormat(0),
		CurrentSceneColorFormat(0),
		bAllowStaticLighting(true),
		CurrentMaxShadowResolution(0),
		CurrentRSMResolution(0),
		CurrentTranslucencyLightingVolumeDim(64),
		CurrentMobile32bpp(0),
		bCurrentLightPropagationVolume(false),
		CurrentFeatureLevel(ERHIFeatureLevel::Num),
		CurrentShadingPath(EShadingPath::Num),
		bAllocateVelocityGBuffer(false),
		bSnapshot(false)
		// NVCHANGE_BEGIN: Add VXGI
#if WITH_GFSDK_VXGI
		, VxgiCurrentInputBufferIndex(0)
#endif
		// NVCHANGE_END: Add VXGI
		{
		}
	/** Constructor that creates snapshot */
	FSceneRenderTargets(const FViewInfo& InView, const FSceneRenderTargets& SnapshotSource);
public:

	enum class EShadingPath
	{
		Forward,
		Deferred,

		Num,
	};

	/**
	 * Checks that scene render targets are ready for rendering a view family of the given dimensions.
	 * If the allocated render targets are too small, they are reallocated.
	 */
	void Allocate(const FSceneViewFamily& ViewFamily);

	/**
	 * Forward shading can't know how big the optimal atlased shadow buffer will be, so provide a set it up per frame.
	 */
	void AllocateForwardShadingShadowDepthTarget(const FIntPoint& ShadowBufferResolution);

	/**
	 *
	 */
	void SetBufferSize(int32 InBufferSizeX, int32 InBufferSizeY);

	void BeginRenderingGBuffer(FRHICommandList& RHICmdList, ERenderTargetLoadAction ColorLoadAction, ERenderTargetLoadAction DepthLoadAction, const FLinearColor& ClearColor = FLinearColor(0, 0, 0, 1));
	void FinishRenderingGBuffer(FRHICommandListImmediate& RHICmdList);

	/**
	 * Sets the scene color target and restores its contents if necessary
	 */
	void BeginRenderingSceneColor(FRHICommandList& RHICmdList, ESimpleRenderTargetMode RenderTargetMode = ESimpleRenderTargetMode::EUninitializedColorExistingDepth, FExclusiveDepthStencil DepthStencilAccess = FExclusiveDepthStencil::DepthWrite_StencilWrite);
	
	/**
	 * Called when finished rendering to the scene color surface
	 * @param bKeepChanges - if true then the SceneColorSurface is resolved to the SceneColorTexture
	 */
	void FinishRenderingSceneColor(FRHICommandListImmediate& RHICmdList, bool bKeepChanges = true, const FResolveRect& ResolveRect = FResolveRect());

	// @return true: call FinishRenderingCustomDepth after rendering, false: don't render it, feature is disabled
	bool BeginRenderingCustomDepth(FRHICommandListImmediate& RHICmdList, bool bPrimitives);
	// only call if BeginRenderingCustomDepth() returned true
	void FinishRenderingCustomDepth(FRHICommandListImmediate& RHICmdList, const FResolveRect& ResolveRect = FResolveRect());

	/**
	 * Resolve a previously rendered scene color surface.
	 */
	void ResolveSceneColor(FRHICommandList& RHICmdList, const FResolveRect& ResolveRect = FResolveRect());

	/** Resolves the GBuffer targets so that their resolved textures can be sampled. */
	void ResolveGBufferSurfaces(FRHICommandList& RHICmdList, const FResolveRect& ResolveRect = FResolveRect());

	void BeginRenderingShadowDepth(FRHICommandList& RHICmdList, bool bClear);

	/** Binds the appropriate shadow depth cube map for rendering. */
	void BeginRenderingCubeShadowDepth(FRHICommandList& RHICmdList, int32 ShadowResolution);

	/**
	 * Called when finished rendering to the subject shadow depths so the surface can be copied to texture
	 * @param ResolveParams - optional resolve params
	 */
	void FinishRenderingShadowDepth(FRHICommandList& RHICmdList, const FResolveRect& ResolveRect = FResolveRect());

	// NVCHANGE_BEGIN: Add VXGI
#if WITH_GFSDK_VXGI
	bool IsValidCascadeIndex(int32 CascadeIndex) { return CascadeIndex < ARRAY_COUNT(CascadedShadowDepthZ); }
	void BeginRenderingCascadedShadowDepth(FRHICommandList& RHICmdList, bool bClear, int32 CascadeIndex);
	void FinishRenderingCascadedShadowDepth(FRHICommandList& RHICmdList, int32 CascadeIndex);
#endif
	// NVCHANGE_END: Add VXGI

	void BeginRenderingReflectiveShadowMap(FRHICommandList& RHICmdList, class FLightPropagationVolume* Lpv);
	void FinishRenderingReflectiveShadowMap(FRHICommandList& RHICmdList, const FResolveRect& ResolveRect = FResolveRect());

	/** Resolves the appropriate shadow depth cube map and restores default state. */
	void FinishRenderingCubeShadowDepth(FRHICommandList& RHICmdList, int32 ShadowResolution);
	
	void BeginRenderingTranslucency(FRHICommandList& RHICmdList, const class FViewInfo& View);
	void FinishRenderingTranslucency(FRHICommandListImmediate& RHICmdList, const class FViewInfo& View);

	bool BeginRenderingSeparateTranslucency(FRHICommandList& RHICmdList, const FViewInfo& View, bool bFirstTimeThisFrame);
	void FinishRenderingSeparateTranslucency(FRHICommandList& RHICmdList, const FViewInfo& View);
	void FreeSeparateTranslucency();

	void ResolveSceneDepthTexture(FRHICommandList& RHICmdList);
	void ResolveSceneDepthToAuxiliaryTexture(FRHICommandList& RHICmdList);

	void BeginRenderingPrePass(FRHICommandList& RHICmdList, bool bPerformClear);
	void FinishRenderingPrePass(FRHICommandListImmediate& RHICmdList);

	void BeginRenderingSceneAlphaCopy(FRHICommandListImmediate& RHICmdList);
	void FinishRenderingSceneAlphaCopy(FRHICommandListImmediate& RHICmdList);

	void BeginRenderingLightAttenuation(FRHICommandList& RHICmdList, bool bClearToWhite=false);
	void FinishRenderingLightAttenuation(FRHICommandList& RHICmdList);

	/**
	 * Cleans up editor primitive targets that we no longer need
	 */
	void CleanUpEditorPrimitiveTargets();

	/**
	 * Affects the render quality of the editor 3d objects. MSAA is needed if >1
	 * @return clamped to reasonable numbers
	 */
	int32 GetEditorMSAACompositingSampleCount() const;

	bool IsStaticLightingAllowed() const { return bAllowStaticLighting; }

	/**
	 * Gets the editor primitives color target/shader resource.  This may recreate the target
	 * if the msaa settings dont match
	 */
	const FTexture2DRHIRef& GetEditorPrimitivesColor();

	/**
	 * Gets the editor primitives depth target/shader resource.  This may recreate the target
	 * if the msaa settings dont match
	 */
	const FTexture2DRHIRef& GetEditorPrimitivesDepth();


	// FRenderResource interface.
	virtual void ReleaseDynamicRHI() override;

	// Texture Accessors -----------

	const FTextureRHIRef& GetSceneColorTexture() const;
	const FTexture2DRHIRef& GetSceneAlphaCopyTexture() const { return (const FTexture2DRHIRef&)SceneAlphaCopy->GetRenderTargetItem().ShaderResourceTexture; }
	bool HasSceneAlphaCopyTexture() const { return SceneAlphaCopy.GetReference() != 0; }
	const FTexture2DRHIRef& GetSceneDepthTexture() const { return (const FTexture2DRHIRef&)SceneDepthZ->GetRenderTargetItem().ShaderResourceTexture; }
	const FTexture2DRHIRef& GetAuxiliarySceneDepthTexture() const 
	{ 
		check(!GSupportsDepthFetchDuringDepthTest);
		return (const FTexture2DRHIRef&)AuxiliarySceneDepthZ->GetRenderTargetItem().ShaderResourceTexture; 
	}

	// NVCHANGE_BEGIN: Add VXGI
#if WITH_GFSDK_VXGI
	const FTexture2DRHIRef& GetShadowDepthZTexture(int32 CascadeIndex = 0, bool bInPreshadowCache = false) const
	{
		if (bInPreshadowCache)
		{
			return (const FTexture2DRHIRef&)PreShadowCacheDepthZ->GetRenderTargetItem().ShaderResourceTexture;
		}
		else
		{
			check(CascadeIndex < ARRAY_COUNT(CascadedShadowDepthZ));
			return (const FTexture2DRHIRef&)CascadedShadowDepthZ[CascadeIndex]->GetRenderTargetItem().ShaderResourceTexture;
		}
	}
#else
	// NVCHANGE_END: Add VXGI

	const FTexture2DRHIRef& GetShadowDepthZTexture(bool bInPreshadowCache = false) const 
	{ 
		if (bInPreshadowCache)
		{
			return (const FTexture2DRHIRef&)PreShadowCacheDepthZ->GetRenderTargetItem().ShaderResourceTexture; 
		}
		else
		{
			return (const FTexture2DRHIRef&)ShadowDepthZ->GetRenderTargetItem().ShaderResourceTexture; 
		}
	}
	// NVCHANGE_BEGIN: Add VXGI
#endif
	// NVCHANGE_END: Add VXGI

	const FTexture2DRHIRef* GetActualDepthTexture() const;
	const FTexture2DRHIRef& GetReflectiveShadowMapDepthTexture() const { return (const FTexture2DRHIRef&)ReflectiveShadowMapDepth->GetRenderTargetItem().ShaderResourceTexture; }
	const FTexture2DRHIRef& GetReflectiveShadowMapNormalTexture() const { return (const FTexture2DRHIRef&)ReflectiveShadowMapNormal->GetRenderTargetItem().ShaderResourceTexture; }
	const FTexture2DRHIRef& GetReflectiveShadowMapDiffuseTexture() const { return (const FTexture2DRHIRef&)ReflectiveShadowMapDiffuse->GetRenderTargetItem().ShaderResourceTexture; }

	const FTextureCubeRHIRef& GetCubeShadowDepthZTexture(int32 ShadowResolution) const 
	{ 
		return (const FTextureCubeRHIRef&)CubeShadowDepthZ[GetCubeShadowDepthZIndex(ShadowResolution)]->GetRenderTargetItem().ShaderResourceTexture; 
	}
	const FTexture2DRHIRef& GetGBufferATexture() const { return (const FTexture2DRHIRef&)GBufferA->GetRenderTargetItem().ShaderResourceTexture; }

	/** 
	* Allows substitution of a 1x1 white texture in place of the light attenuation buffer when it is not needed;
	* this improves shader performance and removes the need for redundant Clears
	*/
	void SetLightAttenuationMode(bool bEnabled) { bLightAttenuationEnabled = bEnabled; }
	const FTextureRHIRef& GetEffectiveLightAttenuationTexture(bool bReceiveDynamicShadows) const 
	{
		if( bLightAttenuationEnabled && bReceiveDynamicShadows )
		{
			return GetLightAttenuationTexture();
		}
		else
		{
			return GWhiteTexture->TextureRHI;
		}
	}
	const FTextureRHIRef& GetLightAttenuationTexture() const
	{
		return *(FTextureRHIRef*)&GetLightAttenuation()->GetRenderTargetItem().ShaderResourceTexture;
	}

	const FTextureRHIRef& GetSceneColorSurface() const;
	const FTexture2DRHIRef& GetSceneAlphaCopySurface() const						{ return (const FTexture2DRHIRef&)SceneAlphaCopy->GetRenderTargetItem().TargetableTexture; }
	const FTexture2DRHIRef& GetSceneDepthSurface() const							{ return (const FTexture2DRHIRef&)SceneDepthZ->GetRenderTargetItem().TargetableTexture; }
	const FTexture2DRHIRef& GetSmallDepthSurface() const							{ return (const FTexture2DRHIRef&)SmallDepthZ->GetRenderTargetItem().TargetableTexture; }
	// NVCHANGE_BEGIN: Add VXGI
#if WITH_GFSDK_VXGI
	const FTexture2DRHIRef& GetShadowDepthZSurface(int32 CascadeIndex = 0) const
	{
		check(CascadeIndex < ARRAY_COUNT(CascadedShadowDepthZ));
		return (const FTexture2DRHIRef&)CascadedShadowDepthZ[CascadeIndex]->GetRenderTargetItem().TargetableTexture;
	}
#else
	// NVCHANGE_END: Add VXGI
	const FTexture2DRHIRef& GetShadowDepthZSurface() const						
	{ 
		return (const FTexture2DRHIRef&)ShadowDepthZ->GetRenderTargetItem().TargetableTexture; 
	}
	// NVCHANGE_BEGIN: Add VXGI
#endif
	// NVCHANGE_END: Add VXGI
	const FTexture2DRHIRef& GetOptionalShadowDepthColorSurface() const 
	{ 
		return (const FTexture2DRHIRef&)OptionalShadowDepthColor->GetRenderTargetItem().TargetableTexture; 
	}

	const FTexture2DRHIRef& GetReflectiveShadowMapNormalSurface() const { return (const FTexture2DRHIRef&)ReflectiveShadowMapNormal->GetRenderTargetItem().TargetableTexture; }
	const FTexture2DRHIRef& GetReflectiveShadowMapDiffuseSurface() const { return (const FTexture2DRHIRef&)ReflectiveShadowMapDiffuse->GetRenderTargetItem().TargetableTexture; }
	const FTexture2DRHIRef& GetReflectiveShadowMapDepthSurface() const { return (const FTexture2DRHIRef&)ReflectiveShadowMapDepth->GetRenderTargetItem().TargetableTexture; }

	const FTextureCubeRHIRef& GetCubeShadowDepthZSurface(int32 ShadowResolution) const						
	{ 
		return (const FTextureCubeRHIRef&)CubeShadowDepthZ[GetCubeShadowDepthZIndex(ShadowResolution)]->GetRenderTargetItem().TargetableTexture; 
	}
	const FTexture2DRHIRef& GetLightAttenuationSurface() const					{ return (const FTexture2DRHIRef&)GetLightAttenuation()->GetRenderTargetItem().TargetableTexture; }
	const FTexture2DRHIRef& GetAuxiliarySceneDepthSurface() const 
	{	
		check(!GSupportsDepthFetchDuringDepthTest); 
		return (const FTexture2DRHIRef&)AuxiliarySceneDepthZ->GetRenderTargetItem().TargetableTexture; 
	}

	const FTexture2DRHIRef& GetDirectionalOcclusionTexture() const 
	{	
		return (const FTexture2DRHIRef&)DirectionalOcclusion->GetRenderTargetItem().TargetableTexture; 
	}

	IPooledRenderTarget* GetGBufferVelocityRT();

	int32 GetGBufferEIndex() const
	{
		return bAllowStaticLighting ? 5 : -1;
	}

	int32 GetGBufferVelocityIndex() const
	{
		if (bAllocateVelocityGBuffer)
		{
			return bAllowStaticLighting ? 6 : 5;
		}

		return -1;
	}

	// @return can be 0 if the feature is disabled
	IPooledRenderTarget* RequestCustomDepth(bool bPrimitives);

	bool IsCustomDepthPassWritingStencil() const;

	// ---

	/** */
	bool UseDownsizedOcclusionQueries() const { return bUseDownsizedOcclusionQueries; }

	// ---

	/** Get the current translucent ambient lighting volume texture. Can vary depending on whether volume filtering is enabled */
	IPooledRenderTarget* GetTranslucencyVolumeAmbient(ETranslucencyVolumeCascade Cascade) { return TranslucencyLightingVolumeAmbient[SelectTranslucencyVolumeTarget(Cascade)].GetReference(); }

	/** Get the current translucent directional lighting volume texture. Can vary depending on whether volume filtering is enabled */
	IPooledRenderTarget* GetTranslucencyVolumeDirectional(ETranslucencyVolumeCascade Cascade) { return TranslucencyLightingVolumeDirectional[SelectTranslucencyVolumeTarget(Cascade)].GetReference(); }

	// ---
	/** Get the uniform buffer containing GBuffer resources. */
	FUniformBufferRHIParamRef GetGBufferResourcesUniformBuffer() const 
	{ 
		// if this triggers you need to make sure the GBuffer is not getting released before (using AdjustGBufferRefCount(1) and AdjustGBufferRefCount(-1))
		check(IsValidRef(GBufferResourcesUniformBuffer));

		return GBufferResourcesUniformBuffer; 
	}
	/** */
	FIntPoint GetBufferSizeXY() const { return BufferSize; }
	/** */
	uint32 GetSmallColorDepthDownsampleFactor() const { return SmallColorDepthDownsampleFactor; }
	/** Returns an index in the range [0, NumCubeShadowDepthSurfaces) given an input resolution. */
	int32 GetCubeShadowDepthZIndex(int32 ShadowResolution) const;
	/** Returns the appropriate resolution for a given cube shadow index. */
	int32 GetCubeShadowDepthZResolution(int32 ShadowIndex) const;
	/** Returns the size of the shadow depth buffer, taking into account platform limitations and game specific resolution limits. */
	FIntPoint GetShadowDepthTextureResolution() const;
	// @return >= 1x1 <= GMaxShadowDepthBufferSizeX x GMaxShadowDepthBufferSizeY
	FIntPoint GetPreShadowCacheTextureResolution() const;
	FIntPoint GetTranslucentShadowDepthTextureResolution() const;
	int32 GetTranslucentShadowDownsampleFactor() const { return 2; }

	/** Returns the size of the RSM buffer, taking into account platform limitations and game specific resolution limits. */
	int32 GetReflectiveShadowMapResolution() const;

	int32 GetNumGBufferTargets() const;

	// ---

	// needs to be called between AllocSceneColor() and ReleaseSceneColor()
	const TRefCountPtr<IPooledRenderTarget>& GetSceneColor() const;

	TRefCountPtr<IPooledRenderTarget>& GetSceneColor();

	// changes depending at which part of the frame this is called
	bool IsSceneColorAllocated() const;

	void SetSceneColor(IPooledRenderTarget* In);

	// ---

	void SetLightAttenuation(IPooledRenderTarget* In);

	// needs to be called between AllocSceneColor() and SetSceneColor(0)
	const TRefCountPtr<IPooledRenderTarget>& GetLightAttenuation() const;

	TRefCountPtr<IPooledRenderTarget>& GetLightAttenuation();

	// ---

	// allows to release the GBuffer once post process materials no longer need it
	// @param 1: add a reference, -1: remove a reference
	void AdjustGBufferRefCount(int Delta);

	//
	void PreallocGBufferTargets(bool bShouldRenderVelocities);
	void AllocGBufferTargets();

	void AllocLightAttenuation();

	void AllocateReflectionTargets();

	TRefCountPtr<IPooledRenderTarget>& GetReflectionBrightnessTarget();

	/**
	 * Takes the requested buffer size and quantizes it to an appropriate size for the rest of the
	 * rendering pipeline. Currently ensures that sizes are multiples of 8 so that they can safely
	 * be halved in size several times.
	 */
	static void QuantizeBufferSize(int32& InOutBufferSizeX, int32& InOutBufferSizeY);

	bool IsSeparateTranslucencyActive(const FViewInfo& View) const;

	bool IsVelocityPass() const
	{
		return bVelocityPass;
	}
	void SetVelocityPass(bool bInVelocityPass)
	{
		bVelocityPass = bInVelocityPass;
	}


private: // Get...() methods instead of direct access

	// 0 before BeginRenderingSceneColor and after tone mapping in deferred shading
	// Permanently allocated for forward shading
	TRefCountPtr<IPooledRenderTarget> SceneColor[(int32)EShadingPath::Num];

public:

	// also used as LDR scene color
	TRefCountPtr<IPooledRenderTarget> LightAttenuation;

	TRefCountPtr<IPooledRenderTarget> HairMask;
	TRefCountPtr<IPooledRenderTarget> HairLightAttenuation;
	TRefCountPtr<IPooledRenderTarget> HairDepthZ;

	// Reflection Environment: Bringing back light accumulation buffer to apply indirect reflections
	TRefCountPtr<IPooledRenderTarget> LightAccumulation;

	// Reflection Environment: Bringing back light accumulation buffer to apply indirect reflections
	TRefCountPtr<IPooledRenderTarget> DirectionalOcclusion;
	//
	TRefCountPtr<IPooledRenderTarget> SceneDepthZ;
	// Mobile without frame buffer fetch (to get depth from alpha).
	TRefCountPtr<IPooledRenderTarget> SceneAlphaCopy;
	// Auxiliary scene depth target. The scene depth is resolved to this surface when targeting SM4. 
	TRefCountPtr<IPooledRenderTarget> AuxiliarySceneDepthZ;
	// Quarter-sized version of the scene depths
	TRefCountPtr<IPooledRenderTarget> SmallDepthZ;

	// GBuffer: Geometry Buffer rendered in base pass for deferred shading, only available between AllocGBufferTargets() and FreeGBufferTargets()
	TRefCountPtr<IPooledRenderTarget> GBufferA;
	TRefCountPtr<IPooledRenderTarget> GBufferB;
	TRefCountPtr<IPooledRenderTarget> GBufferC;
	TRefCountPtr<IPooledRenderTarget> GBufferD;
	TRefCountPtr<IPooledRenderTarget> GBufferE;

	TRefCountPtr<IPooledRenderTarget> GBufferVelocity;

	// DBuffer: For decals before base pass (only temporarily available after early z pass and until base pass)
	TRefCountPtr<IPooledRenderTarget> DBufferA;
	TRefCountPtr<IPooledRenderTarget> DBufferB;
	TRefCountPtr<IPooledRenderTarget> DBufferC;

	// for AmbientOcclusion, only valid for a short time during the frame to allow reuse
	TRefCountPtr<IPooledRenderTarget> ScreenSpaceAO;
	// used by the CustomDepth material feature, is allocated on demand or if r.CustomDepth is 2
	TRefCountPtr<IPooledRenderTarget> CustomDepth;
	// used by the CustomDepth material feature for stencil
	TRefCountPtr<FRHIShaderResourceView> CustomStencilSRV;

	// NVCHANGE_BEGIN: Add VXGI
#if WITH_GFSDK_VXGI
	TRefCountPtr<IPooledRenderTarget> CascadedShadowDepthZ[NUM_SHADOW_CASCADE_SURFACES];
#else
	// NVCHANGE_END: Add VXGI

	// Render target for per-object shadow depths.
	TRefCountPtr<IPooledRenderTarget> ShadowDepthZ;

	// NVCHANGE_BEGIN: Add VXGI
#endif
	// NVCHANGE_END: Add VXGI

	// optional in case this RHI requires a color render target
	TRefCountPtr<IPooledRenderTarget> OptionalShadowDepthColor;
	// Cache of preshadow depths
	//@todo - this should go in FScene
	TRefCountPtr<IPooledRenderTarget> PreShadowCacheDepthZ;
	// Stores accumulated density for shadows from translucency
	TRefCountPtr<IPooledRenderTarget> TranslucencyShadowTransmission[NumTranslucencyShadowSurfaces];

	// NVCHANGE_BEGIN: Add VXGI
#if WITH_GFSDK_VXGI
private:
	bool VxgiCurrentInputBufferIndex;
	TRefCountPtr<IPooledRenderTarget> VxgiSceneDepthZArray[2];
	TRefCountPtr<IPooledRenderTarget> VxgiNormalAndRoughnessArray[2];

	const FTexture2DRHIRef& GetVxgiSceneDepthTexture(bool BufferIndex) const { return (const FTexture2DRHIRef&)VxgiSceneDepthZArray[BufferIndex]->GetRenderTargetItem().ShaderResourceTexture; }
	const FTexture2DRHIRef& GetVxgiNormalAndRoughnessTexture(bool BufferIndex) const { return (const FTexture2DRHIRef&)VxgiNormalAndRoughnessArray[BufferIndex]->GetRenderTargetItem().ShaderResourceTexture; }

	const NVRHI::TextureHandle GetVxgiSceneDepthTextureHandle(bool BufferIndex) const { return GDynamicRHI->GetVXGITextureFromRHI((FRHITexture*)GetVxgiSceneDepthTexture(BufferIndex)); }
	const NVRHI::TextureHandle GetVxgiNormalAndRoughnessTextureHandle(bool BufferIndex) const { return GDynamicRHI->GetVXGITextureFromRHI((FRHITexture*)GetVxgiNormalAndRoughnessTexture(BufferIndex)); }

public:
	TRefCountPtr<IPooledRenderTarget> VxgiNormalAndRoughness;
	TArray<FTexture2DRHIRef> VxgiOutputDiffuse;
	TArray<FTexture2DRHIRef> VxgiOutputSpec;

	void SwapVxgiInputBuffers()
	{
		VxgiCurrentInputBufferIndex = !VxgiCurrentInputBufferIndex;
		VxgiNormalAndRoughness = VxgiNormalAndRoughnessArray[VxgiCurrentInputBufferIndex];
		SceneDepthZ = VxgiSceneDepthZArray[VxgiCurrentInputBufferIndex];
	}

	FTextureRHIParamRef GetVxgiOutputDiffuse(int32 ViewIndex) const { return VxgiOutputDiffuse.IsValidIndex(ViewIndex) && IsValidRef(VxgiOutputDiffuse[ViewIndex]) ? (FTextureRHIParamRef)VxgiOutputDiffuse[ViewIndex] : GSystemTextures.BlackDummy->GetRenderTargetItem().ShaderResourceTexture; }
	FTextureRHIParamRef GetVxgiOutputSpecular(int32 ViewIndex) const { return VxgiOutputSpec.IsValidIndex(ViewIndex) && IsValidRef(VxgiOutputSpec[ViewIndex]) ? (FTextureRHIParamRef)VxgiOutputSpec[ViewIndex] : GSystemTextures.BlackDummy->GetRenderTargetItem().ShaderResourceTexture; }

	const NVRHI::TextureHandle GetCurrentVxgiSceneDepthTextureHandle() const { return GetVxgiSceneDepthTextureHandle(VxgiCurrentInputBufferIndex); }
	const NVRHI::TextureHandle GetCurrentVxgiNormalAndRoughnessTextureHandle() const { return GetVxgiNormalAndRoughnessTextureHandle(VxgiCurrentInputBufferIndex); }

	const NVRHI::TextureHandle GetPreviousVxgiSceneDepthTextureHandle() const { return GetVxgiSceneDepthTextureHandle(!VxgiCurrentInputBufferIndex); }
	const NVRHI::TextureHandle GetPreviousVxgiNormalAndRoughnessTextureHandle() const { return GetVxgiNormalAndRoughnessTextureHandle(!VxgiCurrentInputBufferIndex); }
#endif
	// NVCHANGE_END: Add VXGI

	TRefCountPtr<IPooledRenderTarget> ReflectiveShadowMapNormal;
	TRefCountPtr<IPooledRenderTarget> ReflectiveShadowMapDiffuse;
	TRefCountPtr<IPooledRenderTarget> ReflectiveShadowMapDepth;

	// Render target for one pass point light shadows, 0:at the highest resolution 4:at the lowest resolution
	TRefCountPtr<IPooledRenderTarget> CubeShadowDepthZ[NumCubeShadowDepthSurfaces];

	/** 2 scratch cubemaps used for filtering reflections. */
	TRefCountPtr<IPooledRenderTarget> ReflectionColorScratchCubemap[2];

	/** Temporary storage during SH irradiance map generation. */
	TRefCountPtr<IPooledRenderTarget> DiffuseIrradianceScratchCubemap[2];

	/** Temporary storage during SH irradiance map generation. */
	TRefCountPtr<IPooledRenderTarget> SkySHIrradianceMap;

	/** Temporary storage, used during reflection capture filtering. 
	  * 0 - R32 version for > ES2
	  * 1 - RGBAF version for ES2
	  */
	TRefCountPtr<IPooledRenderTarget> ReflectionBrightness[2];

	/** Volume textures used for lighting translucency. */
	TRefCountPtr<IPooledRenderTarget> TranslucencyLightingVolumeAmbient[NumTranslucentVolumeRenderTargetSets];
	TRefCountPtr<IPooledRenderTarget> TranslucencyLightingVolumeDirectional[NumTranslucentVolumeRenderTargetSets];

	/** Color and opacity for editor primitives (i.e editor gizmos). */
	TRefCountPtr<IPooledRenderTarget> EditorPrimitivesColor;

	/** Depth for editor primitives */
	TRefCountPtr<IPooledRenderTarget> EditorPrimitivesDepth;

	/** ONLY for snapshots!!! this is a copy of the SeparateTranslucencyRT from the view state. */
	TRefCountPtr<IPooledRenderTarget> SeparateTranslucencyRT;

	// todo: free ScreenSpaceAO so pool can reuse
	bool bScreenSpaceAOIsValid;

	// todo: free ScreenSpaceAO so pool can reuse
	bool bCustomDepthIsValid;

	/** Whether the preshadow cache render target has been newly allocated and cached shadows need to be re-rendered. */
	bool bPreshadowCacheNewlyAllocated;

private:
	/** used by AdjustGBufferRefCount */
	int32 GBufferRefCount;

	/** as we might get multiple BufferSize requests each frame for SceneCaptures and we want to avoid reallocations we can only go as low as the largest request */
	FIntPoint LargestDesiredSizeThisFrame;
	FIntPoint LargestDesiredSizeLastFrame;
	/** to detect when LargestDesiredSizeThisFrame is outdated */
	uint32 ThisFrameNumber;

	bool bVelocityPass;
	/** CAUTION: When adding new data, make sure you copy it in the snapshot constructor! **/

	/**
	 * Initializes the editor primitive color render target
	 */
	void InitEditorPrimitivesColor();

	/**
	 * Initializes the editor primitive depth buffer
	 */
	void InitEditorPrimitivesDepth();

	/** Allocates render targets for use with the forward shading path. */
	void AllocateForwardShadingPathRenderTargets();	

	/** Allocates render targets for use with the deferred shading path. */
	void AllocateDeferredShadingPathRenderTargets();

	/** Allocates render targets for use with the current shading path. */
	void AllocateRenderTargets();

	/** Allocates common depth render targets that are used by both forward and deferred rendering paths */
	void AllocateCommonDepthTargets();

	/** Determine the appropriate render target dimensions. */
	FIntPoint ComputeDesiredSize(const FSceneViewFamily& ViewFamily);

	void AllocSceneColor();

	// internal method, used by AdjustGBufferRefCount()
	void ReleaseGBufferTargets();

	// release all allocated targets to the pool
	void ReleaseAllTargets();

	EPixelFormat GetSceneColorFormat() const;

	/** Get the current scene color target based on our current shading path. Will return a null ptr if there is no valid scene color target  */
	const TRefCountPtr<IPooledRenderTarget>& GetSceneColorForCurrentShadingPath() const { check(CurrentShadingPath < EShadingPath::Num); return SceneColor[(int32)CurrentShadingPath]; }
	TRefCountPtr<IPooledRenderTarget>& GetSceneColorForCurrentShadingPath() { check(CurrentShadingPath < EShadingPath::Num); return SceneColor[(int32)CurrentShadingPath]; }

	/** Determine whether the render targets for a particular shading path have been allocated */
	bool AreShadingPathRenderTargetsAllocated(EShadingPath InShadingPath) const;

	/** Determine whether the render targets for any shading path have been allocated */
	bool AreAnyShadingPathRenderTargetsAllocated() const { return AreShadingPathRenderTargetsAllocated(EShadingPath::Deferred) || AreShadingPathRenderTargetsAllocated(EShadingPath::Forward); }

	/** Gets all GBuffers to use.  Returns the number actually used. */
	int32 GetGBufferRenderTargets(ERenderTargetLoadAction ColorLoadAction, FRHIRenderTargetView OutRenderTargets[MaxSimultaneousRenderTargets], int32& OutVelocityRTIndex);

private:
	/** Uniform buffer containing GBuffer resources. */
	FUniformBufferRHIRef GBufferResourcesUniformBuffer;
	/** size of the back buffer, in editor this has to be >= than the biggest view port */
	FIntPoint BufferSize;
	/** e.g. 2 */
	uint32 SmallColorDepthDownsampleFactor;
	/** if true we use the light attenuation buffer otherwise the 1x1 white texture is used */
	bool bLightAttenuationEnabled;
	/** Whether to use SmallDepthZ for occlusion queries. */
	bool bUseDownsizedOcclusionQueries;
	/** To detect a change of the CVar r.GBufferFormat */
	int CurrentGBufferFormat;
	/** To detect a change of the CVar r.SceneColorFormat */
	int CurrentSceneColorFormat;
	/** Whether render targets were allocated with static lighting allowed. */
	bool bAllowStaticLighting;
	/** To detect a change of the CVar r.Shadow.MaxResolution */
	int32 CurrentMaxShadowResolution;
	/** To detect a change of the CVar r.Shadow.RsmResolution*/
	int32 CurrentRSMResolution;
	/** To detect a change of the CVar r.TranslucencyLightingVolumeDim */
	int32 CurrentTranslucencyLightingVolumeDim;
	/** To detect a change of the CVar r.MobileHDR / r.MobileHDR32bppMode */
	int32 CurrentMobile32bpp;
	/** To detect a change of the CVar r.MobileMSAA */
	int32 CurrentMobileMSAA;
	/** To detect a change of the CVar r.Shadow.MinResolution */
	int32 CurrentMinShadowResolution;
	/** To detect a change of the CVar r.LightPropagationVolume */
	bool bCurrentLightPropagationVolume;
	/** Feature level we were initialized for */
	ERHIFeatureLevel::Type CurrentFeatureLevel;
	/** Shading path that we are currently drawing through. Set when calling Allocate at the start of a scene render. */
	EShadingPath CurrentShadingPath;

	// Set this per frame since there might be cases where we don't need an extra GBuffer
	bool bAllocateVelocityGBuffer;

	/** Helpers to track gbuffer state on platforms that need to propagate clear information across parallel rendering boundaries. */
	bool bGBuffersFastCleared;	

	/** Helpers to track scenedepth state on platforms that need to propagate clear information across parallel rendering boundaries. */
	bool bSceneDepthCleared;
	
	/** true is this is a snapshot on the scene allocator */
	bool bSnapshot;
	/** All outstanding snapshots */
	TArray<FSceneRenderTargets*> Snapshots;

	/** CAUTION: When adding new data, make sure you copy it in the snapshot constructor! **/

};

