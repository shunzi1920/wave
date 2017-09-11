// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	D3D12Shaders.cpp: D3D shader RHI implementation.
	=============================================================================*/

#include "D3D12RHIPrivate.h"

FVertexShaderRHIRef FD3D12DynamicRHI::RHICreateVertexShader(const TArray<uint8>& Code)
{
	check(Code.Num());
	FD3D12VertexShader* Shader = new FD3D12VertexShader;

	FMemoryReader Ar(Code, true);
	Ar << Shader->ShaderResourceTable;
	int32 Offset = Ar.Tell();
	const uint8* CodePtr = Code.GetData() + Offset;
	const SIZE_T CodeSize = Code.Num() - Offset - 5;

	// bGlobalUniformBufferUsed and resource counts are packed in the last few bytes, see CompileD3D11Shader
	Shader->bShaderNeedsGlobalConstantBuffer = Code[Code.Num() - 5] != 0;
	Shader->SamplerCount                     = Code[Code.Num() - 4];
	Shader->SRVCount                         = Code[Code.Num() - 3];
	Shader->UAVCount                         = Code[Code.Num() - 1];

	//TODO Special case for Constant Buffer count as it currently needs all buffers set if it needs any set.
	Shader->CBCount = (Code[Code.Num() - 2]) ? MAX_CBS : 0;

	Shader->Code = Code;
	Shader->Offset = Offset;

	D3D12_SHADER_BYTECODE ShaderBytecode;
	ShaderBytecode.pShaderBytecode = Shader->Code.GetData() + Offset;
	ShaderBytecode.BytecodeLength = CodeSize;
	Shader->ShaderBytecode.SetShaderBytecode(ShaderBytecode);

	return Shader;
}

FPixelShaderRHIRef FD3D12DynamicRHI::RHICreatePixelShader(const TArray<uint8>& Code)
{
	check(Code.Num());

	FD3D12PixelShader* Shader = new FD3D12PixelShader;

	FMemoryReader Ar(Code, true);
	Ar << Shader->ShaderResourceTable;
	int32 Offset = Ar.Tell();
	const uint8* CodePtr = Code.GetData() + Offset;
	const SIZE_T CodeSize = Code.Num() - Offset - 5;

	// bGlobalUniformBufferUsed and resource counts are packed in the last few bytes, see CompileD3D11Shader
	Shader->bShaderNeedsGlobalConstantBuffer = Code[Code.Num() - 5] != 0;
	Shader->SamplerCount                     = Code[Code.Num() - 4];
	Shader->SRVCount                         = Code[Code.Num() - 3];
	Shader->UAVCount                         = Code[Code.Num() - 1];

	//TODO Special case for Constant Buffer count as it currently needs all buffers set if it needs any set.
	Shader->CBCount = (Code[Code.Num() - 2]) ? MAX_CBS : 0;

	Shader->Code = Code;

	D3D12_SHADER_BYTECODE ShaderBytecode;
	ShaderBytecode.pShaderBytecode = Shader->Code.GetData() + Offset;
	ShaderBytecode.BytecodeLength = CodeSize;
	Shader->ShaderBytecode.SetShaderBytecode(ShaderBytecode);

	return Shader;
}

FHullShaderRHIRef FD3D12DynamicRHI::RHICreateHullShader(const TArray<uint8>& Code)
{
	check(Code.Num());

	FD3D12HullShader* Shader = new FD3D12HullShader;

	FMemoryReader Ar(Code, true);
	Ar << Shader->ShaderResourceTable;
	int32 Offset = Ar.Tell();
	const uint8* CodePtr = Code.GetData() + Offset;
	const SIZE_T CodeSize = Code.Num() - Offset - 5;

	// bGlobalUniformBufferUsed and resource counts are packed in the last few bytes, see CompileD3D11Shader
	Shader->bShaderNeedsGlobalConstantBuffer = Code[Code.Num() - 5] != 0;
	Shader->SamplerCount                     = Code[Code.Num() - 4];
	Shader->SRVCount                         = Code[Code.Num() - 3];
	Shader->UAVCount                         = Code[Code.Num() - 1];

	//TODO Special case for Constant Buffer count as it currently needs all buffers set if it needs any set.
	Shader->CBCount = (Code[Code.Num() - 2]) ? MAX_CBS : 0;

	Shader->Code = Code;

	D3D12_SHADER_BYTECODE ShaderBytecode;
	ShaderBytecode.pShaderBytecode = Shader->Code.GetData() + Offset;
	ShaderBytecode.BytecodeLength = CodeSize;
	Shader->ShaderBytecode.SetShaderBytecode(ShaderBytecode);

	return Shader;
}

FDomainShaderRHIRef FD3D12DynamicRHI::RHICreateDomainShader(const TArray<uint8>& Code)
{
	check(Code.Num());

	FD3D12DomainShader* Shader = new FD3D12DomainShader;

	FMemoryReader Ar(Code, true);
	Ar << Shader->ShaderResourceTable;
	int32 Offset = Ar.Tell();
	const uint8* CodePtr = Code.GetData() + Offset;
	const SIZE_T CodeSize = Code.Num() - Offset - 5;

	// bGlobalUniformBufferUsed and resource counts are packed in the last few bytes, see CompileD3D11Shader
	Shader->bShaderNeedsGlobalConstantBuffer = Code[Code.Num() - 5] != 0;
	Shader->SamplerCount                     = Code[Code.Num() - 4];
	Shader->SRVCount                         = Code[Code.Num() - 3];
	Shader->UAVCount                         = Code[Code.Num() - 1];

	//TODO Special case for Constant Buffer count as it currently needs all buffers set if it needs any set.
	Shader->CBCount = (Code[Code.Num() - 2] > 0) ? MAX_CBS : 0;

	Shader->Code = Code;

	D3D12_SHADER_BYTECODE ShaderBytecode;
	ShaderBytecode.pShaderBytecode = Shader->Code.GetData() + Offset;
	ShaderBytecode.BytecodeLength = CodeSize;
	Shader->ShaderBytecode.SetShaderBytecode(ShaderBytecode);

	return Shader;
}

FGeometryShaderRHIRef FD3D12DynamicRHI::RHICreateGeometryShader(const TArray<uint8>& Code)
{
	check(Code.Num());

	FD3D12GeometryShader* Shader = new FD3D12GeometryShader;

	FMemoryReader Ar(Code, true);
	Ar << Shader->ShaderResourceTable;
	int32 Offset = Ar.Tell();
	const uint8* CodePtr = Code.GetData() + Offset;
	const SIZE_T CodeSize = Code.Num() - Offset - 5;

	// bGlobalUniformBufferUsed and resource counts are packed in the last few bytes, see CompileD3D11Shader
	Shader->bShaderNeedsGlobalConstantBuffer = Code[Code.Num() - 5] != 0;
	Shader->SamplerCount                     = Code[Code.Num() - 4];
	Shader->SRVCount                         = Code[Code.Num() - 3];
	Shader->UAVCount                         = Code[Code.Num() - 1];

	//TODO Special case for Constant Buffer count as it currently needs all buffers set if it needs any set.
	Shader->CBCount = (Code[Code.Num() - 2] > 0) ? MAX_CBS : 0;

	Shader->Code = Code;

	D3D12_SHADER_BYTECODE ShaderBytecode;
	ShaderBytecode.pShaderBytecode = Shader->Code.GetData() + Offset;
	ShaderBytecode.BytecodeLength = CodeSize;
	Shader->ShaderBytecode.SetShaderBytecode(ShaderBytecode);

	return Shader;
}

FGeometryShaderRHIRef FD3D12DynamicRHI::RHICreateGeometryShaderWithStreamOutput(const TArray<uint8>& Code, const FStreamOutElementList& ElementList, uint32 NumStrides, const uint32* Strides, int32 RasterizedStream)
{
	check(Code.Num());

	FD3D12GeometryShader* Shader = new FD3D12GeometryShader;

	FMemoryReader Ar(Code, true);
	Ar << Shader->ShaderResourceTable;
	int32 Offset = Ar.Tell();
	const uint8* CodePtr = Code.GetData() + Offset;
	const SIZE_T CodeSize = Code.Num() - Offset - 5;

	Shader->StreamOutput.RasterizedStream = RasterizedStream;
	if (RasterizedStream == -1)
	{
		Shader->StreamOutput.RasterizedStream = D3D12_SO_NO_RASTERIZED_STREAM;
	}

	Shader->StreamOutput.NumEntries = ElementList.Num();
	Shader->pStreamOutEntries = new D3D12_SO_DECLARATION_ENTRY[ElementList.Num()];
	Shader->StreamOutput.pSODeclaration = Shader->pStreamOutEntries;
	if (Shader->pStreamOutEntries == nullptr)
	{
		return nullptr;	// Out of memory
	}
	for (int32 EntryIndex = 0; EntryIndex < ElementList.Num(); EntryIndex++)
	{
		Shader->pStreamOutEntries[EntryIndex].Stream = ElementList[EntryIndex].Stream;
		Shader->pStreamOutEntries[EntryIndex].SemanticName = ElementList[EntryIndex].SemanticName;
		Shader->pStreamOutEntries[EntryIndex].SemanticIndex = ElementList[EntryIndex].SemanticIndex;
		Shader->pStreamOutEntries[EntryIndex].StartComponent = ElementList[EntryIndex].StartComponent;
		Shader->pStreamOutEntries[EntryIndex].ComponentCount = ElementList[EntryIndex].ComponentCount;
		Shader->pStreamOutEntries[EntryIndex].OutputSlot = ElementList[EntryIndex].OutputSlot;
	}


	// bGlobalUniformBufferUsed and resource counts are packed in the last few bytes, see CompileD3D11Shader
	Shader->bShaderNeedsGlobalConstantBuffer = Code[Code.Num() - 5] != 0;
	Shader->SamplerCount                     = Code[Code.Num() - 4];
	Shader->SRVCount                         = Code[Code.Num() - 3];
	Shader->UAVCount                         = Code[Code.Num() - 1];

	//TODO Special case for Constant Buffer count as it currently needs all buffers set if it needs any set.
	Shader->CBCount = (Code[Code.Num() - 2] > 0) ? MAX_CBS : 0;

	// Indicate this shader uses stream output
	Shader->bShaderNeedsStreamOutput = true;

	// Copy the strides
	Shader->StreamOutput.NumStrides = NumStrides;
	Shader->pStreamOutStrides = new uint32[NumStrides];
	Shader->StreamOutput.pBufferStrides = Shader->pStreamOutStrides;
	if (Shader->pStreamOutStrides == nullptr)
	{
		return nullptr;	// Out of memory
	}
	FMemory::Memcpy(Shader->pStreamOutStrides, Strides, sizeof(uint32) * NumStrides);

	Shader->Code = Code;

	D3D12_SHADER_BYTECODE ShaderBytecode;
	ShaderBytecode.pShaderBytecode = Shader->Code.GetData() + Offset;
	ShaderBytecode.BytecodeLength = CodeSize;
	Shader->ShaderBytecode.SetShaderBytecode(ShaderBytecode);

	return Shader;
}

FComputeShaderRHIRef FD3D12DynamicRHI::RHICreateComputeShader(const TArray<uint8>& Code)
{
	check(Code.Num());

	FD3D12ComputeShader* Shader = new FD3D12ComputeShader;

	FMemoryReader Ar(Code, true);
	Ar << Shader->ShaderResourceTable;
	int32 Offset = Ar.Tell();
	const uint8* CodePtr = Code.GetData() + Offset;
	const SIZE_T CodeSize = Code.Num() - Offset - 5;

	// bGlobalUniformBufferUsed and resource counts are packed in the last few bytes, see CompileD3D11Shader
	Shader->bShaderNeedsGlobalConstantBuffer = Code[Code.Num() - 5] != 0;
	Shader->SamplerCount                     = Code[Code.Num() - 4];
	Shader->SRVCount                         = Code[Code.Num() - 3];
	Shader->UAVCount                         = Code[Code.Num() - 1];

	//TODO Special case for Constant Buffer count as it currently needs all buffers set if it needs any set.
	Shader->CBCount = (Code[Code.Num() - 2] > 0) ? MAX_CBS : 0;

	Shader->Code = Code;

	D3D12_SHADER_BYTECODE ShaderBytecode;
	ShaderBytecode.pShaderBytecode = Shader->Code.GetData() + Offset;
	ShaderBytecode.BytecodeLength = CodeSize;
	Shader->ShaderBytecode.SetShaderBytecode(ShaderBytecode);

	return Shader;
}

void FD3D12CommandContext::RHISetMultipleViewports(uint32 Count, const FViewportBounds* Data)
{
	// structures are chosen to be directly mappable
	D3D12_VIEWPORT* D3DData = (D3D12_VIEWPORT*)Data;

	StateCache.SetViewports(Count, D3DData);
}

static volatile uint64 BoundShaderStateID = 0;

FD3D12BoundShaderState::FD3D12BoundShaderState(
	FVertexDeclarationRHIParamRef InVertexDeclarationRHI,
	FVertexShaderRHIParamRef InVertexShaderRHI,
	FPixelShaderRHIParamRef InPixelShaderRHI,
	FHullShaderRHIParamRef InHullShaderRHI,
	FDomainShaderRHIParamRef InDomainShaderRHI,
	FGeometryShaderRHIParamRef InGeometryShaderRHI
	) :
	CacheLink(InVertexDeclarationRHI, InVertexShaderRHI, InPixelShaderRHI, InHullShaderRHI, InDomainShaderRHI, InGeometryShaderRHI, this),
    UniqueID(InterlockedIncrement64(reinterpret_cast<volatile int64*>(&BoundShaderStateID)))
{
	INC_DWORD_STAT(STAT_D3D12NumBoundShaderState);

    // Warning: Input layout desc contains padding which must be zero-initialized to prevent PSO cache misses
    FMemory::Memzero(&InputLayout, sizeof(InputLayout));

	FD3D12VertexDeclaration*  InVertexDeclaration = FD3D12DynamicRHI::ResourceCast(InVertexDeclarationRHI);
	FD3D12VertexShader*  InVertexShader = FD3D12DynamicRHI::ResourceCast(InVertexShaderRHI);
	FD3D12PixelShader*  InPixelShader = FD3D12DynamicRHI::ResourceCast(InPixelShaderRHI);
	FD3D12HullShader*  InHullShader = FD3D12DynamicRHI::ResourceCast(InHullShaderRHI);
	FD3D12DomainShader*  InDomainShader = FD3D12DynamicRHI::ResourceCast(InDomainShaderRHI);
	FD3D12GeometryShader*  InGeometryShader = FD3D12DynamicRHI::ResourceCast(InGeometryShaderRHI);

	// Create an input layout for this combination of vertex declaration and vertex shader.
	InputLayout.NumElements = (InVertexDeclaration ? InVertexDeclaration->VertexElements.Num() : 0);
	InputLayout.pInputElementDescs = (InVertexDeclaration ? InVertexDeclaration->VertexElements.GetData() : nullptr);

	bShaderNeedsGlobalConstantBuffer[SF_Vertex] = InVertexShader->bShaderNeedsGlobalConstantBuffer;
	bShaderNeedsGlobalConstantBuffer[SF_Hull] = InHullShader ? InHullShader->bShaderNeedsGlobalConstantBuffer : false;
	bShaderNeedsGlobalConstantBuffer[SF_Domain] = InDomainShader ? InDomainShader->bShaderNeedsGlobalConstantBuffer : false;
	bShaderNeedsGlobalConstantBuffer[SF_Pixel] = InPixelShader ? InPixelShader->bShaderNeedsGlobalConstantBuffer : false;
	bShaderNeedsGlobalConstantBuffer[SF_Geometry] = InGeometryShader ? InGeometryShader->bShaderNeedsGlobalConstantBuffer : false;

	static_assert(ARRAY_COUNT(bShaderNeedsGlobalConstantBuffer) == SF_NumFrequencies, "EShaderFrequency size should match with array count of bShaderNeedsGlobalConstantBuffer.");

#if D3D12_SUPPORTS_PARALLEL_RHI_EXECUTE
	CacheLink.AddToCache();
#endif
}

FD3D12BoundShaderState::~FD3D12BoundShaderState()
{
	DEC_DWORD_STAT(STAT_D3D12NumBoundShaderState);
#if D3D12_SUPPORTS_PARALLEL_RHI_EXECUTE
	CacheLink.RemoveFromCache();
#endif
}

/**
* Creates a bound shader state instance which encapsulates a decl, vertex shader, and pixel shader
* @param VertexDeclaration - existing vertex decl
* @param StreamStrides - optional stream strides
* @param VertexShader - existing vertex shader
* @param HullShader - existing hull shader
* @param DomainShader - existing domain shader
* @param PixelShader - existing pixel shader
* @param GeometryShader - existing geometry shader
*/
FBoundShaderStateRHIRef FD3D12DynamicRHI::RHICreateBoundShaderState(
	FVertexDeclarationRHIParamRef VertexDeclarationRHI,
	FVertexShaderRHIParamRef VertexShaderRHI,
	FHullShaderRHIParamRef HullShaderRHI,
	FDomainShaderRHIParamRef DomainShaderRHI,
	FPixelShaderRHIParamRef PixelShaderRHI,
	FGeometryShaderRHIParamRef GeometryShaderRHI
	)
{
	SCOPE_CYCLE_COUNTER(STAT_D3D12CreateBoundShaderStateTime);

    checkf(GIsRHIInitialized && GetRHIDevice()->GetCommandListManager().IsReady(), (TEXT("Bound shader state RHI resource was created without initializing Direct3D first")));

#if D3D12_SUPPORTS_PARALLEL_RHI_EXECUTE
	// Check for an existing bound shader state which matches the parameters
	FBoundShaderStateRHIRef CachedBoundShaderState = GetCachedBoundShaderState_Threadsafe(
		VertexDeclarationRHI,
		VertexShaderRHI,
		PixelShaderRHI,
		HullShaderRHI,
		DomainShaderRHI,
		GeometryShaderRHI
		);
	if(CachedBoundShaderState.GetReference())
	{
		// If we've already created a bound shader state with these parameters, reuse it.
		return CachedBoundShaderState;
	}
#else
	check(IsInRenderingThread() || IsInRHIThread());
	// Check for an existing bound shader state which matches the parameters
	FCachedBoundShaderStateLink* CachedBoundShaderStateLink = GetCachedBoundShaderState(
		VertexDeclarationRHI,
		VertexShaderRHI,
		PixelShaderRHI,
		HullShaderRHI,
		DomainShaderRHI,
		GeometryShaderRHI
		);
	if(CachedBoundShaderStateLink)
	{
		// If we've already created a bound shader state with these parameters, reuse it.
		return CachedBoundShaderStateLink->BoundShaderState;
	}
#endif
	else
	{
		SCOPE_CYCLE_COUNTER(STAT_D3D12NewBoundShaderStateTime);
		return new FD3D12BoundShaderState(VertexDeclarationRHI, VertexShaderRHI, PixelShaderRHI, HullShaderRHI, DomainShaderRHI, GeometryShaderRHI);
	}
}
