// Copyright 1998-2013 Epic Games, Inc. All Rights Reserved.

#include "EnginePrivate.h"
#include "PhysXSupport.h"

#include "PhysicsEngine/FlexAsset.h"
#include "PhysicsEngine/FlexAssetSolid.h"
#include "PhysicsEngine/FlexAssetCloth.h"
#include "PhysicsEngine/FlexAssetSoft.h"
#include "PhysicsEngine/FlexAssetPreviewComponent.h"

#include "StaticMeshResources.h"

FFlexPhase::FFlexPhase()
{
	AutoAssignGroup = true;
	Group = 0;
	SelfCollide = false;
	IgnoreRestCollisions = false;
	Fluid = false;
}

UFlexAsset::UFlexAsset(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	Asset = NULL;
}

void UFlexAsset::PostInitProperties()
{
#if WITH_FLEX
	// allocate an extensions object to represent particles and constraints for this asset
	Asset = new FlexExtAsset();
#endif

	Super::PostInitProperties();
}

void UFlexAsset::BeginDestroy()
{
#if WITH_FLEX
	delete Asset;
#endif

	Super::BeginDestroy();
}

#if WITH_EDITOR
void UFlexAsset::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	/*
	FName PropertyName = (PropertyChangedEvent.Property != NULL) ? PropertyChangedEvent.Property->GetFName() : NAME_None;

	if ((PropertyName == FName(TEXT("Mesh"))))
	{
		ReImport();
	}
	*/
}
#endif // WITH_EDITOR

/*=============================================================================
UFlexAssetCloth
=============================================================================*/

UFlexAssetCloth::UFlexAssetCloth(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	ContainerTemplate = NULL;
	AttachToRigids = false;
	Mass = 1.0f;

	RigidStiffness = 0.0f;

	StretchStiffness = 1.0f;
	BendStiffness = 1.0f;
	TetherStiffness = 1.0f;
	TetherGive = 0.1f;
	EnableInflatable = false;
	OverPressure = 1.0f;
	InflatableStiffness = 1.0f;
}

void UFlexAssetCloth::ReImport(const UStaticMesh* Mesh)
{
#if WITH_FLEX
	Particles.Empty();

	SpringIndices.Empty();
	SpringCoefficients.Empty();
	SpringRestLengths.Empty();

	ShapeCenters.Empty();
	ShapeIndices.Empty();
	ShapeOffsets.Empty();
	ShapeCoefficients.Empty();

	Triangles.Empty();

	// create particles from mesh LOD0	
	if (Mesh->RenderData->LODResources.Num() == 0)
		return;

	FStaticMeshLODResources* Res = &Mesh->RenderData->LODResources[0];

	// flatten vertex struct to positions 
	TArray<FVector> Positions;

	int32 NumVertices = Res->PositionVertexBuffer.GetNumVertices();
	int32 NumColors = Res->ColorVertexBuffer.GetNumVertices();

	float InvMass = (Mass > 0.0f) ? (1.0f / Mass) : 0.0f;

	for (int i = 0; i < NumVertices; ++i)
	{
		FVector Pos = Res->PositionVertexBuffer.VertexPosition(i);
		Positions.Add(Pos);
	}

	TArray<uint32> VertexIndices;
	Res->IndexBuffer.GetCopy(VertexIndices);

	TArray<int> UniqueVerts;
	UniqueVerts.SetNum(NumVertices);

	// mapping from vertex index to particle index
	VertexToParticleMap.SetNum(NumVertices);

	// render mesh has vertex duplicates (for texture mapping etc), weld mesh and generate particles for unique verts
	int ParticleCount = flexExtCreateWeldedMeshIndices((float*)&Positions[0], NumVertices, &UniqueVerts[0], &VertexToParticleMap[0], THRESH_POINTS_ARE_SAME);

	FVector Center(0.0f);

	for (int i = 0; i < ParticleCount; ++i)
	{
		float MassScale = 1.0f;

		int VertIndex = UniqueVerts[i];
		if (VertIndex < NumColors)
		{
			// if there is a color channel set invmass according to color (zero = heavier)
			FColor Col = Res->ColorVertexBuffer.VertexColor(VertIndex);
			MassScale = Col.R / 255.0f;
		}

		FVector Pos = Positions[UniqueVerts[i]];

		Particles.Add(FVector4(Pos, InvMass*MassScale));

		Center += Pos;
	}

	Center /= float(ParticleCount);

	// remap render index buffer from vertices to particles
	TArray<int> ParticleIndices;
	for (int i = 0; i < VertexIndices.Num(); ++i)
		ParticleIndices.Add(VertexToParticleMap[VertexIndices[i]]);

	// create cloth from unique particles
	FlexExtAsset* Asset = flexExtCreateClothFromMesh((float*)&Particles[0], Particles.Num(), (int*)&ParticleIndices[0], ParticleIndices.Num() / 3, StretchStiffness, BendStiffness, TetherStiffness, TetherGive, OverPressure);

	if (Asset)
	{
		RigidCenter = Center;

		// copy out spring data
		for (int i = 0; i < Asset->mNumSprings; ++i)
		{
			SpringIndices.Add(Asset->mSpringIndices[i * 2 + 0]);
			SpringIndices.Add(Asset->mSpringIndices[i * 2 + 1]);
			SpringCoefficients.Add(Asset->mSpringCoefficients[i]);
			SpringRestLengths.Add(Asset->mSpringRestLengths[i]);
		}

		// faces for cloth
		for (int i = 0; i < Asset->mNumTriangles * 3; ++i)
			Triangles.Add(Asset->mTriangleIndices[i]);

		// save inflatable properties
		InflatableVolume = Asset->mInflatableVolume;
		InflatableStiffness = Asset->mInflatableStiffness;

		// discard flex asset, we will recreate it from our internal data
		flexExtDestroyAsset(Asset);
	}

	UE_LOG(LogFlex, Display, TEXT("Created a FlexAsset with %d Particles, %d Springs, %d Triangles\n"), Particles.Num(), SpringRestLengths.Num(), Triangles.Num() / 3);

#endif //WITH_FLEX
}

FlexExtAsset* UFlexAssetCloth::GetFlexAsset()
{
#if WITH_FLEX
	// reset Asset
	FMemory::Memset(Asset, 0, sizeof(FlexExtAsset));

	// re-update the Asset each time it is requested, could cache this
	Asset->mNumParticles = Particles.Num();

	// particles
	if (Asset->mNumParticles)
	{
		Asset->mParticles = (float*)&Particles[0];
	}

	// distance constraints
	Asset->mNumSprings = SpringCoefficients.Num();
	if (Asset->mNumSprings)
	{
		Asset->mSpringIndices = (int*)&SpringIndices[0];
		Asset->mSpringCoefficients = (float*)&SpringCoefficients[0];
		Asset->mSpringRestLengths = (float*)&SpringRestLengths[0];
	}

	// triangles
	Asset->mNumTriangles = Triangles.Num() / 3;
	if (Asset->mNumTriangles)
	{
		Asset->mTriangleIndices = (int*)&Triangles[0];
	}

	// inflatables
	Asset->mInflatable = EnableInflatable;
	Asset->mInflatablePressure = OverPressure;
	Asset->mInflatableVolume = InflatableVolume;
	Asset->mInflatableStiffness = InflatableStiffness;

	if (RigidStiffness > 0.0f && ShapeCenters.Num() == 0)
	{
		// construct a single rigid shape constraint  
		ShapeCenters.Add(RigidCenter);
		ShapeCoefficients.Add(RigidStiffness);
	
		for (int i=0; i < Particles.Num(); ++i)
			ShapeIndices.Add(i);

		ShapeOffsets.Add(Particles.Num());
	}

	if (ShapeCenters.Num())
	{
		// soft bodies
		Asset->mNumShapes = ShapeCenters.Num();
		Asset->mNumShapeIndices = ShapeIndices.Num();
		Asset->mShapeOffsets = &ShapeOffsets[0];
		Asset->mShapeIndices = &ShapeIndices[0];
		Asset->mShapeCoefficients = &ShapeCoefficients[0];
		Asset->mShapeCenters = (float*)&ShapeCenters[0];
	}

	#endif //WITH_FLEX
	return Asset;
}

/*=============================================================================
UFlexAssetSolid
=============================================================================*/

UFlexAssetSolid::UFlexAssetSolid(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	ContainerTemplate = NULL;
	AttachToRigids = false;
	Mass = 1.0f;

	Stiffness = 1.0f;
	SamplingDistance = 10.0f;
}

void UFlexAssetSolid::ReImport(const UStaticMesh* Mesh)
{
#if WITH_FLEX
	Particles.Empty();
	ShapeCenters.Empty();
	ShapeIndices.Empty();
	ShapeOffsets.Empty();
	ShapeCoefficients.Empty();
	
	// create particles from mesh LOD0	
	if (Mesh->RenderData->LODResources.Num() == 0)
		return;

	FStaticMeshLODResources* Res = &Mesh->RenderData->LODResources[0];

	// flatten vertex struct to positions 
	TArray<FVector> Positions;

	int32 NumVertices = Res->PositionVertexBuffer.GetNumVertices();

	float InvMass = (Mass > 0.0f) ? (1.0f / Mass) : 0.0f;

	for (int i = 0; i < NumVertices; ++i)
	{
		FVector Pos = Res->PositionVertexBuffer.VertexPosition(i);
		Positions.Add(Pos);
	}

	TArray<uint32> VertexIndices;
	Res->IndexBuffer.GetCopy(VertexIndices);

	UE_LOG(LogFlex, Display, TEXT("Voxelizing Flex rigid body\n"));

	FlexExtAsset* Asset = flexExtCreateRigidFromMesh((const float*)&Positions[0], Positions.Num(), (const int*)&VertexIndices[0], VertexIndices.Num(), SamplingDistance, 0.0f);

	if (Asset)
	{
		for (int i = 0; i < Asset->mNumParticles; ++i)
		{
			FVector4 Particle = ((FVector4*)Asset->mParticles)[i];
			Particle.W = InvMass;

			Particles.Add(Particle);
		}

		for (int i=0; i < Asset->mNumShapes; ++i)
		{
			ShapeCenters.Add(((FVector*)Asset->mShapeCenters)[i]);
			ShapeOffsets.Add(Asset->mShapeOffsets[i]);
			ShapeCoefficients.Add(Asset->mShapeCoefficients[i]);
		}

		for (int i=0; i < Asset->mNumShapeIndices; ++i)
			ShapeIndices.Add(Asset->mShapeIndices[i]);

		flexExtDestroyAsset(Asset);
	}
	else
	{
		UE_LOG(LogFlex, Warning, TEXT("Failed to voxelize Flex rigid, check mesh is closed and objectSize/SamplingDistance < 64\n"));
	}

	UE_LOG(LogFlex, Display, TEXT("Created a FlexAsset with %d Particles, %d Springs, %d Triangles\n"), Particles.Num(), 0, 0);
#endif //WITH_FLEX
}

void UFlexAssetSolid::PostLoad()
{
	Super::PostLoad();
}

FlexExtAsset* UFlexAssetSolid::GetFlexAsset()
{
#if WITH_FLEX
	// reset Asset
	FMemory::Memset(Asset, 0, sizeof(FlexExtAsset));

	// re-update the Asset each time it is requested, could cache this
	Asset->mNumParticles = Particles.Num();

	// particles
	if (Asset->mNumParticles)
	{
		Asset->mParticles = (float*)&Particles[0];
	}

	if (ShapeCenters.Num() == 0)
	{
		// construct a single rigid shape constraint  
		ShapeCenters.Add(RigidCenter);
		ShapeCoefficients.Add(Stiffness);
	
		for (int i=0; i < Particles.Num(); ++i)
			ShapeIndices.Add(i);

		ShapeOffsets.Add(Particles.Num());
	}

	// shapes 
	Asset->mNumShapes = ShapeCenters.Num();
	Asset->mNumShapeIndices = ShapeIndices.Num();
	Asset->mShapeOffsets = &ShapeOffsets[0];
	Asset->mShapeIndices = &ShapeIndices[0];
	Asset->mShapeCoefficients = &ShapeCoefficients[0];
	Asset->mShapeCenters = (float*)&ShapeCenters[0];

#endif //WITH_FLEX
	return Asset;
}

/*=============================================================================
UFlexAssetSoft
=============================================================================*/

void FFlexSoftSkinningIndicesVertexBuffer::Init(const TArray<int32>& ClusterIndices)
{
	Vertices.SetNum(ClusterIndices.Num());

	// convert to 16bit indices (this means max of 65k bones per soft.. should be enough for anybody)
	for (int i=0; i < ClusterIndices.Num(); ++i)
		Vertices[i] = int16(ClusterIndices[i]);
}

void FFlexSoftSkinningIndicesVertexBuffer::InitRHI()
{
	if (Vertices.Num() > 0)
	{
		FRHIResourceCreateInfo CreateInfo;
		VertexBufferRHI = RHICreateVertexBuffer(Vertices.Num() * sizeof(int16), BUF_Static, CreateInfo);

		// Copy the vertex data into the vertex buffer.
		void* VertexBufferData = RHILockVertexBuffer(VertexBufferRHI,0,Vertices.Num() * sizeof(int16), RLM_WriteOnly);
		FMemory::Memcpy(VertexBufferData,Vertices.GetData(),Vertices.Num() * sizeof(int16));
		RHIUnlockVertexBuffer(VertexBufferRHI);
	}
}

void FFlexSoftSkinningWeightsVertexBuffer::Init(const TArray<float>& ClusterWeights)
{
	Vertices = ClusterWeights;
}

void FFlexSoftSkinningWeightsVertexBuffer::InitRHI()
{
	if (Vertices.Num() > 0)
	{
		FRHIResourceCreateInfo CreateInfo;
		VertexBufferRHI = RHICreateVertexBuffer(Vertices.Num() * sizeof(float), BUF_Static, CreateInfo);

		// Copy the vertex data into the vertex buffer.
		void* VertexBufferData = RHILockVertexBuffer(VertexBufferRHI,0,Vertices.Num() * sizeof(float), RLM_WriteOnly);
		FMemory::Memcpy(VertexBufferData,Vertices.GetData(),Vertices.Num() * sizeof(float));
		RHIUnlockVertexBuffer(VertexBufferRHI);
	}
}

UFlexAssetSoft::UFlexAssetSoft(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	Phase.IgnoreRestCollisions = true;

	ContainerTemplate = NULL;
	AttachToRigids = false;
	Mass = 1.0f;

	ParticleSpacing = 10.0f;
	VolumeSampling = 4.0f;
	SurfaceSampling = 1.0f;
	
	ClusterSpacing = 20.0f;
	ClusterRadius = 30.0f;
	ClusterStiffness = 0.5f;
	
	LinkRadius = 0.0f;
	LinkStiffness = 1.0f;

	SkinningFalloff = 2.0f;
	SkinningMaxDistance = 100.0f;
}

void UFlexAssetSoft::ReImport(const UStaticMesh* Mesh)
{

#if WITH_FLEX
	Particles.Empty();
	
	ShapeCenters.Empty();
	ShapeIndices.Empty();
	ShapeOffsets.Empty();
	ShapeCoefficients.Empty();

	SpringIndices.Empty();
	SpringCoefficients.Empty();
	SpringRestLengths.Empty();

	// create particles from mesh LOD0	
	if (Mesh->RenderData->LODResources.Num() == 0)
		return;

	FStaticMeshLODResources* Res = &Mesh->RenderData->LODResources[0];

	// flatten vertex struct to positions 
	TArray<FVector> Positions;
	int32 NumVertices = Res->PositionVertexBuffer.GetNumVertices();

	for (int i = 0; i < NumVertices; ++i)
		Positions.Add(Res->PositionVertexBuffer.VertexPosition(i));

	TArray<uint32> VertexIndices;
	Res->IndexBuffer.GetCopy(VertexIndices);

	UE_LOG(LogFlex, Display, TEXT("Voxelizing Flex rigid body\n"));

	FlexExtAsset* Asset = flexExtCreateSoftFromMesh(
		(const float*)&Positions[0], 
		Positions.Num(), 
		(const int*)&VertexIndices[0], 
		VertexIndices.Num(), 
		ParticleSpacing, 
		VolumeSampling, 
		SurfaceSampling, 
		ClusterSpacing, 
		ClusterRadius, 
		ClusterStiffness, 
		LinkRadius, 
		LinkStiffness);
	
	if (Asset)
	{
		SkinningWeights.SetNum(Positions.Num()*4);
		SkinningIndices.SetNum(Positions.Num()*4);

		// create skinning
		flexExtCreateSoftMeshSkinning(
			(const float*)&Positions[0], 
			Positions.Num(), 
			Asset->mShapeCenters, 
			Asset->mNumShapes, 
			SkinningFalloff, 
			SkinningMaxDistance, 
			&SkinningWeights[0],
			&SkinningIndices[0]);

		const float InvMass = (Mass > 0.0f) ? (1.0f / Mass) : 0.0f;

		// create particles
		for (int i = 0; i < Asset->mNumParticles; ++i)
		{
			FVector4 Particle = ((FVector4*)Asset->mParticles)[i];
			Particle.W = InvMass;

			Particles.Add(Particle);
		}

		// create shapes
		if (Asset->mNumShapes)
		{
			ShapeCenters.Append((FVector*)Asset->mShapeCenters, Asset->mNumShapes);
			ShapeCoefficients.Append(Asset->mShapeCoefficients, Asset->mNumShapes);
			ShapeOffsets.Append(Asset->mShapeOffsets, Asset->mNumShapes);
			ShapeIndices.Append(Asset->mShapeIndices, Asset->mNumShapeIndices);
		}
		
		// create links
		if (Asset->mNumSprings)
		{
			SpringIndices.Append(Asset->mSpringIndices, Asset->mNumSprings*2);
			SpringRestLengths.Append(Asset->mSpringRestLengths, Asset->mNumSprings);
			SpringCoefficients.Append(Asset->mSpringCoefficients, Asset->mNumSprings);
		}

		flexExtDestroyAsset(Asset);
	}
	else
	{
		UE_LOG(LogFlex, Warning, TEXT("Failed to voxelize Flex rigid, check mesh is closed and objectSize/SamplingDistance < 64\n"));
	}

	UE_LOG(LogFlex, Display, TEXT("Created a Flex soft body with %d Particles, %d Springs, %d Clusters\n"), Particles.Num(), SpringCoefficients.Num(), ShapeCenters.Num());

	// update vertex buffer data
	WeightsVertexBuffer.Init(SkinningWeights);
	IndicesVertexBuffer.Init(SkinningIndices);

	// initialize / update resources
	if (WeightsVertexBuffer.IsInitialized())
		BeginUpdateResourceRHI(&WeightsVertexBuffer);
	else
		BeginInitResource(&WeightsVertexBuffer);

	if (IndicesVertexBuffer.IsInitialized())
		BeginUpdateResourceRHI(&IndicesVertexBuffer);
	else
		BeginInitResource(&IndicesVertexBuffer);

#endif //WITH_FLEX
}

void UFlexAssetSoft::PostLoad()
{
	Super::PostLoad();

	// update vertex buffer data
	WeightsVertexBuffer.Init(SkinningWeights);
	IndicesVertexBuffer.Init(SkinningIndices);

	BeginInitResource(&WeightsVertexBuffer);
	BeginInitResource(&IndicesVertexBuffer);
}

void UFlexAssetSoft::BeginDestroy()
{
	Super::BeginDestroy();
	
	BeginReleaseResource(&WeightsVertexBuffer);
	BeginReleaseResource(&IndicesVertexBuffer);
}

FlexExtAsset* UFlexAssetSoft::GetFlexAsset()
{
#if WITH_FLEX

	// reset Asset
	FMemory::Memset(Asset, 0, sizeof(FlexExtAsset));

	// particles
	Asset->mNumParticles = Particles.Num();	
	if (Asset->mNumParticles)
		Asset->mParticles = (float*)&Particles[0];

	// distance constraints
	Asset->mNumSprings = SpringCoefficients.Num();
	if (Asset->mNumSprings)
	{
		Asset->mSpringIndices = (int*)&SpringIndices[0];
		Asset->mSpringCoefficients = (float*)&SpringCoefficients[0];
		Asset->mSpringRestLengths = (float*)&SpringRestLengths[0];
	}

	// soft bodies
	Asset->mNumShapes = ShapeCenters.Num();
	if (Asset->mNumShapes)
	{
		Asset->mNumShapeIndices = ShapeIndices.Num();
		Asset->mShapeOffsets = &ShapeOffsets[0];
		Asset->mShapeIndices = &ShapeIndices[0];
		Asset->mShapeCoefficients = &ShapeCoefficients[0];
		Asset->mShapeCenters = (float*)&ShapeCenters[0];
	}

#endif //WITH_FLEX

	return Asset;
}


/*=============================================================================
UFlexAssetPreviewComponent and FFlexAssetPreviewSceneProxy for rendering particles in
static mesh editor.
=============================================================================*/

class FFlexAssetPreviewSceneProxy : public FPrimitiveSceneProxy
{
	struct Line
	{
		FVector Start;
		FVector End;
		FColor Color;
	};

public:

	void AddSolidSphere(FVector Position, float Radius, FColor Color, int32 NumSides, int32 NumRings)
	{
		// The first/last arc are on top of each other.
		int32 NumVerts = (NumSides + 1) * (NumRings + 1);
		FDynamicMeshVertex* Verts = (FDynamicMeshVertex*)FMemory::Malloc(NumVerts * sizeof(FDynamicMeshVertex));

		// Calculate verts for one arc
		FDynamicMeshVertex* ArcVerts = (FDynamicMeshVertex*)FMemory::Malloc((NumRings + 1) * sizeof(FDynamicMeshVertex));

		for (int32 i = 0; i < NumRings + 1; i++)
		{
			FDynamicMeshVertex* ArcVert = &ArcVerts[i];

			float angle = ((float)i / NumRings) * PI;

			// Note- unit sphere, so position always has mag of one. We can just use it for normal!			
			ArcVert->Position.X = 0.0f;
			ArcVert->Position.Y = FMath::Sin(angle);
			ArcVert->Position.Z = FMath::Cos(angle);

			ArcVert->SetTangents(
				FVector(1, 0, 0),
				FVector(0.0f, -ArcVert->Position.Z, ArcVert->Position.Y),
				ArcVert->Position
				);

			ArcVert->TextureCoordinate.X = 0.0f;
			ArcVert->TextureCoordinate.Y = ((float)i / NumRings);
			ArcVert->Color = Color;
		}

		// Then rotate this arc NumSides+1 times.
		for (int32 s = 0; s < NumSides + 1; s++)
		{
			FRotator ArcRotator(0, 360.f * (float)s / NumSides, 0);
			FRotationMatrix ArcRot(ArcRotator);
			float XTexCoord = ((float)s / NumSides);

			for (int32 v = 0; v < NumRings + 1; v++)
			{
				int32 VIx = (NumRings + 1)*s + v;

				Verts[VIx].Position = ArcRot.TransformPosition(ArcVerts[v].Position);

				Verts[VIx].SetTangents(
					ArcRot.TransformVector(ArcVerts[v].TangentX),
					ArcRot.TransformVector(ArcVerts[v].GetTangentY()),
					ArcRot.TransformVector(ArcVerts[v].TangentZ)
					);

				Verts[VIx].TextureCoordinate.X = XTexCoord;
				Verts[VIx].TextureCoordinate.Y = ArcVerts[v].TextureCoordinate.Y;
				Verts[VIx].Color = Color;
			}
		}

		// Add all of the vertices we generated to the mesh builder.
		int32 VertexOffset = Vertices.Num();
		Vertices.AddUninitialized(NumVerts);
		for (int32 VertIdx = 0; VertIdx < NumVerts; VertIdx++)
		{
			FDynamicMeshVertex Vertex = Verts[VertIdx];
			Vertex.Position = Vertex.Position*Radius + Position;
			Vertices[VertexOffset + VertIdx] = Vertex;
		}

		// Add all of the triangles we generated to the mesh builder.
		int32 TriangleOffset = Triangles.Num();
		Triangles.AddUninitialized(NumSides*NumRings * 2 * 3);
		int32 Index = 0;
		for (int32 s = 0; s < NumSides; s++)
		{
			int32 a0start = (s + 0) * (NumRings + 1);
			int32 a1start = (s + 1) * (NumRings + 1);

			for (int32 r = 0; r < NumRings; r++)
			{
				Triangles[TriangleOffset + Index++] = a0start + r + 0 + VertexOffset;
				Triangles[TriangleOffset + Index++] = a1start + r + 0 + VertexOffset;
				Triangles[TriangleOffset + Index++] = a0start + r + 1 + VertexOffset;

				Triangles[TriangleOffset + Index++] = a1start + r + 0 + VertexOffset;
				Triangles[TriangleOffset + Index++] = a1start + r + 1 + VertexOffset;
				Triangles[TriangleOffset + Index++] = a0start + r + 1 + VertexOffset;
			}
		}

		// Free our local copy of verts and arc verts
		FMemory::Free(Verts);
		FMemory::Free(ArcVerts);
	}

	void AddLine(FVector Start, FVector End, FColor Color)
	{
		Line L;
		L.Start = Start;
		L.End = End;
		L.Color = Color;

		Lines.Add(L);
	}

	void AddBasis(FVector Position, float Length)
	{
		AddLine(Position, Position + FVector(Length, 0.0f, 0.0f), FColor::Red);
		AddLine(Position, Position + FVector(0.0f, Length, 0.0f), FColor::Green);
		AddLine(Position, Position + FVector(0.0f, 0.0f, Length), FColor::Blue);
	}

	FFlexAssetPreviewSceneProxy(const UFlexAssetPreviewComponent* InComponent)
		: FPrimitiveSceneProxy(InComponent)
	{
		bWillEverBeLit = true;
		bNeedsUnbuiltPreviewLighting = true;

		ViewRelevance.bDrawRelevance = true;
		ViewRelevance.bDynamicRelevance = true;
		ViewRelevance.bNormalTranslucencyRelevance = true;

		UFlexAsset* FlexAsset = InComponent->FlexAsset;
		UFlexContainer* Container = FlexAsset ? FlexAsset->ContainerTemplate : NULL;

		if (FlexAsset && Container)
		{
			//UFlexContainer::Radius represents rest spacing, which corresponds to two spheres of radii Radius/2 touching.
			float Radius = Container->Radius*0.5f;
			float InvMass = (FlexAsset->Mass > 0.0f) ? (1.0f / FlexAsset->Mass) : 0.0f;

			for (int32 ParticleIndex = 0; ParticleIndex < FlexAsset->Particles.Num(); ++ParticleIndex)
			{
				FVector4& Particle = FlexAsset->Particles[ParticleIndex];
				FVector Position(Particle.X, Particle.Y, Particle.Z);
				uint8 MassColVal = uint8(Particle.W * 255.0f / InvMass);
				FColor Color = FColor(MassColVal, 0, 0, 255);

				AddSolidSphere(Position, Radius, Color, 7, 7);
			}
		}

		UFlexAssetSoft* FlexSoftAsset = Cast<UFlexAssetSoft>(FlexAsset);

		if (FlexSoftAsset)
		{
			Lines.Empty();
			
			// build cluster rings
			for (int i=0; i < FlexSoftAsset->ShapeCenters.Num(); ++i)
			{
				AddBasis(FlexSoftAsset->ShapeCenters[i], FlexSoftAsset->ClusterRadius);
			}

			// build links
			for (int i=0; i < FlexSoftAsset->SpringCoefficients.Num(); ++i)
			{
				int Particle0 = FlexSoftAsset->SpringIndices[i*2+0];
				int Particle1 = FlexSoftAsset->SpringIndices[i*2+1];

				AddLine(FlexSoftAsset->Particles[Particle0], FlexSoftAsset->Particles[Particle1], FColor::Cyan);
			}
		}
	}

	//virtual void DrawDynamicElements(FPrimitiveDrawInterface* PDI, const FSceneView* View) override
	virtual void GetDynamicMeshElements(const TArray<const FSceneView*>& Views, const FSceneViewFamily& ViewFamily, uint32 VisibilityMap, class FMeshElementCollector& Collector) const override
	{
		for (int32 ViewIndex = 0; ViewIndex < Views.Num(); ViewIndex++)
		{
			if (VisibilityMap & (1 << ViewIndex))
			{
				const FSceneView* View = Views[ViewIndex];
				FPrimitiveDrawInterface* PDI = Collector.GetPDI(ViewIndex);

				FDynamicMeshBuilder MeshBuilder;
				MeshBuilder.AddVertices(Vertices);
				MeshBuilder.AddTriangles(Triangles);

				UMaterial* Material = GEngine->ShadedLevelColorationLitMaterial;
				if (View->Family->EngineShowFlags.VertexColors)
				{
					Material = GEngine->VertexColorViewModeMaterial_ColorOnly;
				}
				else if (View->Family->EngineShowFlags.Wireframe)
				{
					Material = GEngine->WireframeMaterial;
				}

				MeshBuilder.GetMesh(FMatrix::Identity, Material->GetRenderProxy(false), SDPG_World, false, false, ViewIndex, Collector);

				// draw clusters
				for (int32 i = 0; i < Lines.Num(); i++)
					PDI->DrawLine(Lines[i].Start, Lines[i].End, Lines[i].Color, SDPG_Foreground, 0.0f);
			}
		}
	}
		
	/*{
		FDynamicMeshBuilder MeshBuilder;
		MeshBuilder.AddVertices(Vertices);
		MeshBuilder.AddTriangles(Triangles);

		UMaterial* Material = GEngine->ShadedLevelColorationLitMaterial;
		if (View->Family->EngineShowFlags.VertexColors)
		{
			Material = GEngine->VertexColorViewModeMaterial_ColorOnly;
		}
		else if (View->Family->EngineShowFlags.Wireframe)
		{
			Material = GEngine->ShadedLevelColorationUnlitMaterial;
		}

		FLinearColor BaseColor(1.0f, 1.0f, 1.0f);
		const FColoredMaterialRenderProxy ColoredMaterialInstance(Material->GetRenderProxy(false), BaseColor);

		MeshBuilder.Draw(PDI, FMatrix::Identity, &ColoredMaterialInstance, SDPG_World);
	}*/

	virtual FPrimitiveViewRelevance GetViewRelevance(const FSceneView* View)
	{
		return ViewRelevance;
	}

	virtual uint32 GetMemoryFootprint(void) const { return(sizeof(*this) + GetAllocatedSize()); }

	uint32 GetAllocatedSize(void) const
	{
		//size not correct
		return(FPrimitiveSceneProxy::GetAllocatedSize() + Vertices.GetAllocatedSize() + Triangles.GetAllocatedSize());
	}

private:

	TArray<FDynamicMeshVertex> Vertices;
	TArray<int32> Triangles;

	TArray<Line> Lines;

	FPrimitiveViewRelevance ViewRelevance;
};

UFlexAssetPreviewComponent::UFlexAssetPreviewComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, FlexAsset(NULL)
{}

FPrimitiveSceneProxy* UFlexAssetPreviewComponent::CreateSceneProxy()
{
	return new FFlexAssetPreviewSceneProxy(this);
}

FBoxSphereBounds UFlexAssetPreviewComponent::CalcBounds(const FTransform& LocalToWorld) const
{
	const FVector BoxExtent(HALF_WORLD_MAX);
	return FBoxSphereBounds(FVector::ZeroVector, BoxExtent, BoxExtent.Size());
}
