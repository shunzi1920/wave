// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.
#pragma once

#include "DynamicMeshBuilder.h"

#include "Box.h"
#include "GeometryCacheMeshData.generated.h"

/** Stores per-batch data used for rendering */
USTRUCT()
struct FGeometryCacheMeshBatchInfo
{
	GENERATED_USTRUCT_BODY()

	/** Starting index into IndexBuffer to draw from */
	uint32 StartIndex;
	/** Total number of Triangles to draw */
	uint32 NumTriangles;
	/** Index to Material used to draw this batch */
	uint32 MaterialIndex;

	friend FArchive& operator<<(FArchive& Ar, FGeometryCacheMeshBatchInfo& Mesh)
	{
		Ar << Mesh.StartIndex;
		Ar << Mesh.NumTriangles;
		Ar << Mesh.MaterialIndex;
		return Ar;
	}
};

/** Stores per Track/Mesh data used for rendering*/
USTRUCT()
struct FGeometryCacheMeshData
{
	GENERATED_USTRUCT_BODY()

	FGeometryCacheMeshData() {}
	~FGeometryCacheMeshData()
	{
		Vertices.Empty();
		BatchesInfo.Empty();
		BoundingBox.Init();
	}
	
	/** Draw-able vertices */
	TArray<FDynamicMeshVertex> Vertices;
	/** Array of per-batch info structs*/
	TArray<FGeometryCacheMeshBatchInfo> BatchesInfo;
	/** Bounding box for this sample in the track */
	FBox BoundingBox;
	
	/** Serialization for FVertexAnimationSample. */
	friend FArchive& operator<<(FArchive& Ar, FGeometryCacheMeshData& Mesh)
	{
		int32 NumVertices = 0;
		
		if (Ar.IsSaving())
		{
			NumVertices = Mesh.Vertices.Num();
		}

		Ar << NumVertices;
		if (Ar.IsLoading())
		{
			Mesh.Vertices.AddUninitialized(NumVertices);
		}

		for (int32 VertexIndex = 0; VertexIndex < NumVertices; ++VertexIndex)
		{
			FDynamicMeshVertex& Vertex = Mesh.Vertices[VertexIndex];
			Ar << Vertex.Position;
			Ar << Vertex.TextureCoordinate;
			Ar << Vertex.TangentX;
			Ar << Vertex.TangentZ;
			Ar << Vertex.Color;
		}

		Ar << Mesh.BoundingBox;
		Ar << Mesh.BatchesInfo;

		return Ar;
	}

	SIZE_T GetResourceSize() const
	{
		// Calculate resource size according to what is actually serialized
		SIZE_T Size = 0;
		Size += Vertices.Num() * sizeof(FDynamicMeshVertex);
		Size += BatchesInfo.Num() * sizeof(FGeometryCacheMeshBatchInfo);
		Size += sizeof(Vertices);
		Size += sizeof(BatchesInfo);
		Size += sizeof(BoundingBox);

		return Size;
	}
};
