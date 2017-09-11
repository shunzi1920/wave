// Copyright (c) 2015, NVIDIA CORPORATION. All rights reserved.

#include "D3D11RHIPrivate.h"
#include "D3D11NvRHI.h"
#include "AllowWindowsPlatformTypes.h"
	#include <d3d11.h>
	#include <d3dcompiler.h>
#include "HideWindowsPlatformTypes.h"
#include "nvapi.h"


namespace NVRHI
{
	class ConstantBuffer
	{
	public:
		ConstantBufferDesc Desc;
		FRHIUniformBufferLayout Layout;
		FUniformBufferRHIRef UniformBufferRHI;
	};

	class Texture
	{
	public:
		TextureDesc Desc;
		FTextureRHIRef TextureRHI;
		std::map<std::pair<Format::Enum, uint32>, FShaderResourceViewRHIRef> ShaderResourceViews;
		std::map<std::pair<Format::Enum, uint32>, FUnorderedAccessViewRHIRef> UnorderedAccessViews;
	};

	class Buffer
	{
	public:
		BufferDesc Desc;
		uint32 Usage;
		uint32 EffectiveStride;
		FStructuredBufferRHIRef BufferRHI;
		FShaderResourceViewRHIRef ShaderResourceView;
		FUnorderedAccessViewRHIRef UnorderedAccessView;
	};

	struct FormatMapping
	{
		Format::Enum abstractFormat;
		EPixelFormat unrealFormat;
		DXGI_FORMAT resourceFormat;
		DXGI_FORMAT srvFormat;
		DXGI_FORMAT rtvFormat;
		uint32 bytesPerPixel;
		bool isDepthStencil;
	};

	// Format mapping table. The rows must be in the exactly same order as Format enum members are defined.
	const FormatMapping FormatMappings[] = {
		{ Format::UNKNOWN,              PF_Unknown,           DXGI_FORMAT_UNKNOWN,                DXGI_FORMAT_UNKNOWN,                DXGI_FORMAT_UNKNOWN,                0, false },
		{ Format::R8_UINT,              PF_Unknown,           DXGI_FORMAT_R8_TYPELESS,            DXGI_FORMAT_R8_UINT,                DXGI_FORMAT_R8_UINT,                1, false },
		{ Format::R8_UNORM,             PF_Unknown,           DXGI_FORMAT_R8_TYPELESS,            DXGI_FORMAT_R8_UNORM,               DXGI_FORMAT_R8_UNORM,               1, false },
		{ Format::RG8_UINT,             PF_R8G8,              DXGI_FORMAT_R8G8_TYPELESS,          DXGI_FORMAT_R8G8_UINT,              DXGI_FORMAT_R8G8_UINT,              2, false },
		{ Format::RG8_UNORM,            PF_R8G8,              DXGI_FORMAT_R8G8_TYPELESS,          DXGI_FORMAT_R8G8_UNORM,             DXGI_FORMAT_R8G8_UNORM,             2, false },
		{ Format::R16_UINT,             PF_R16_UINT,          DXGI_FORMAT_R16_TYPELESS,           DXGI_FORMAT_R16_UINT,               DXGI_FORMAT_R16_UINT,               2, false },
		{ Format::R16_UNORM,            PF_R16_UINT,          DXGI_FORMAT_R16_TYPELESS,           DXGI_FORMAT_R16_UNORM,              DXGI_FORMAT_R16_UNORM,              2, false },
		{ Format::R16_FLOAT,            PF_R16F,              DXGI_FORMAT_R16_TYPELESS,           DXGI_FORMAT_R16_FLOAT,              DXGI_FORMAT_R16_FLOAT,              2, false },
		{ Format::RGBA8_UNORM,          PF_R8G8B8A8,          DXGI_FORMAT_R8G8B8A8_TYPELESS,      DXGI_FORMAT_R8G8B8A8_UNORM,         DXGI_FORMAT_R8G8B8A8_UNORM,         4, false },
		{ Format::BGRA8_UNORM,          PF_B8G8R8A8,          DXGI_FORMAT_B8G8R8A8_TYPELESS,      DXGI_FORMAT_B8G8R8A8_UNORM,         DXGI_FORMAT_B8G8R8A8_UNORM,         4, false },
		{ Format::SRGBA8_UNORM,         PF_R8G8B8A8,          DXGI_FORMAT_R8G8B8A8_TYPELESS,      DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,    DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,    4, false },
		{ Format::R10G10B10A2_UNORM,    PF_A2B10G10R10,       DXGI_FORMAT_R10G10B10A2_TYPELESS,   DXGI_FORMAT_R10G10B10A2_UNORM,      DXGI_FORMAT_R10G10B10A2_UNORM,      4, false },
		{ Format::RG16_UINT,            PF_G16R16,            DXGI_FORMAT_R16G16_TYPELESS,        DXGI_FORMAT_R16G16_UINT,            DXGI_FORMAT_R16G16_UINT,            4, false },
		{ Format::RG16_FLOAT,           PF_G16R16F,           DXGI_FORMAT_R16G16_TYPELESS,        DXGI_FORMAT_R16G16_FLOAT,           DXGI_FORMAT_R16G16_FLOAT,           4, false },
		{ Format::R32_UINT,             PF_R32_UINT,          DXGI_FORMAT_R32_TYPELESS,           DXGI_FORMAT_R32_UINT,               DXGI_FORMAT_R32_UINT,               4, false },
		{ Format::R32_FLOAT,            PF_R32_FLOAT,         DXGI_FORMAT_R32_TYPELESS,           DXGI_FORMAT_R32_FLOAT,              DXGI_FORMAT_R32_FLOAT,              4, false },
		{ Format::RGBA16_FLOAT,         PF_FloatRGBA,         DXGI_FORMAT_R16G16B16A16_TYPELESS,  DXGI_FORMAT_R16G16B16A16_FLOAT,     DXGI_FORMAT_R16G16B16A16_FLOAT,     8, false },
		{ Format::RG32_UINT,            PF_Unknown,           DXGI_FORMAT_R32G32_TYPELESS,        DXGI_FORMAT_R32G32_UINT,            DXGI_FORMAT_R32G32_UINT,            8, false },
		{ Format::RG32_FLOAT,           PF_G32R32F,           DXGI_FORMAT_R32G32_TYPELESS,        DXGI_FORMAT_R32G32_FLOAT,           DXGI_FORMAT_R32G32_FLOAT,           8, false },
		{ Format::RGB32_UINT,           PF_Unknown,           DXGI_FORMAT_R32G32B32_TYPELESS,     DXGI_FORMAT_R32G32B32_UINT,         DXGI_FORMAT_R32G32B32_UINT,         12, false },
		{ Format::RGB32_FLOAT,          PF_FloatRGB,          DXGI_FORMAT_R32G32B32_TYPELESS,     DXGI_FORMAT_R32G32B32_FLOAT,        DXGI_FORMAT_R32G32B32_FLOAT,        12, false },
		{ Format::RGBA32_UINT,          PF_R32G32B32A32_UINT, DXGI_FORMAT_R32G32B32A32_TYPELESS,  DXGI_FORMAT_R32G32B32A32_UINT,      DXGI_FORMAT_R32G32B32A32_UINT,      16, false },
		{ Format::RGBA32_FLOAT,         PF_A32B32G32R32F,     DXGI_FORMAT_R32G32B32A32_TYPELESS,  DXGI_FORMAT_R32G32B32A32_FLOAT,     DXGI_FORMAT_R32G32B32A32_FLOAT,     16, false },
		{ Format::D16,                  PF_ShadowDepth,       DXGI_FORMAT_R16_TYPELESS,           DXGI_FORMAT_R16_UNORM,              DXGI_FORMAT_D16_UNORM,              2, true },
		{ Format::D24S8,                PF_DepthStencil,      DXGI_FORMAT_R24G8_TYPELESS,         DXGI_FORMAT_R24_UNORM_X8_TYPELESS,  DXGI_FORMAT_D24_UNORM_S8_UINT,      4, true },
		{ Format::X24G8_UINT,           PF_DepthStencil,      DXGI_FORMAT_R24G8_TYPELESS,         DXGI_FORMAT_X24_TYPELESS_G8_UINT,   DXGI_FORMAT_D24_UNORM_S8_UINT,      4, true },
		{ Format::D32,                  PF_R32_FLOAT,         DXGI_FORMAT_R32_TYPELESS,           DXGI_FORMAT_R32_FLOAT,              DXGI_FORMAT_D32_FLOAT,              4, true },
	};

	const FormatMapping& GetFormatMapping(Format::Enum abstractFormat)
	{
		const FormatMapping& mapping = FormatMappings[abstractFormat];
		check(mapping.abstractFormat == abstractFormat);
		return mapping;
	}

	bool GetSSE42Support()
	{
		int cpui[4];
		__cpuidex(cpui, 1, 0);
		return !!(cpui[2] & 0x100000);
	}

	static const bool CpuSupportsSSE42 = GetSSE42Support();

	class CrcHash
	{
	private:
		uint32 crc;
	public:
		CrcHash() 
			: crc(0) 
		{ 
		}

		uint32 Get() 
		{
			return crc;
		}

		template<size_t size> __forceinline void AddBytesSSE42(void* p)
		{
			static_assert(size % 4 == 0, "Size of hashable types must be multiple of 4");

			uint32* data = (uint32*)p;

			const size_t numIterations = size / sizeof(uint32);
			for (size_t i = 0; i < numIterations; i++)
			{
				crc = _mm_crc32_u32(crc, data[i]);
			}
		}

		__forceinline void AddBytes(char* p, uint32 size)
		{
			crc = FCrc::MemCrc32(p, size, crc);
		}

		template<typename T> void Add(const T& value)
		{
			if (CpuSupportsSSE42)
				AddBytesSSE42<sizeof(value)>((void*)&value);
			else
				AddBytes((char*)&value, sizeof(value));
		}
	};

	FRendererInterfaceD3D11::FRendererInterfaceD3D11(ID3D11Device* Device)
		: m_Device(Device)
		, m_TreatErrorsAsFatal(true)
		, m_RHICmdList(&FRHICommandListExecutor::GetImmediateCommandList())
	{

	}

	void FRendererInterfaceD3D11::setTreatErrorsAsFatal(bool v)
	{
		m_TreatErrorsAsFatal = v;
	}

	void FRendererInterfaceD3D11::signalError(const char* file, int line, const char* errorDesc)
	{
		if (m_TreatErrorsAsFatal)
		{
			UE_LOG(LogD3D11RHI, Fatal, TEXT("VXGI Error: %s (%s, %i)"), ANSI_TO_TCHAR(errorDesc), ANSI_TO_TCHAR(file), line);
		}
		else
		{
			UE_LOG(LogD3D11RHI, Error, TEXT("VXGI Error: %s (%s, %i)"), ANSI_TO_TCHAR(errorDesc), ANSI_TO_TCHAR(file), line);
		}
	}

	class FTextureInitData : public FResourceBulkDataInterface
	{
	public:
		const void* Data;
		uint32 Size;
		bool Disposed;
		FTextureInitData() : Disposed(false), Data(0), Size(0) {}
		virtual const void* GetResourceBulkData() const { return Data; }
		virtual uint32 GetResourceBulkDataSize() const { return Size; }
		virtual void Discard() { Disposed = true; }
	};


	TextureHandle FRendererInterfaceD3D11::createTexture(const TextureDesc& d, const void* data)
	{
		TextureHandle texture = new Texture();
		texture->Desc = d;

		const FormatMapping& mapping = GetFormatMapping(d.format);

		uint32 flags = TexCreate_None;

		flags |= TexCreate_ShaderResource;
		if (d.isRenderTarget)
			flags |= mapping.isDepthStencil ? TexCreate_DepthStencilTargetable : TexCreate_RenderTargetable;
		if (d.isUAV)
			flags |= TexCreate_UAV;

		FTextureInitData InitData;
		InitData.Data = data;
		InitData.Size = d.width * d.height * mapping.bytesPerPixel * FMath::Max(1u, d.depthOrArraySize);

		FRHIResourceCreateInfo CreateInfo;
		if (data)
		{
			CreateInfo.BulkData = &InitData;
		}

		if (d.depthOrArraySize == 0)
		{
			texture->TextureRHI = GDynamicRHI->RHICreateTexture2D(d.width, d.height, mapping.unrealFormat, d.mipLevels, d.sampleCount, flags, CreateInfo);
			check(texture->TextureRHI.IsValid());
		}
		else if (d.isArray)
		{
			check(d.sampleCount == 1);
			texture->TextureRHI = GDynamicRHI->RHICreateTexture2DArray(d.width, d.height, d.depthOrArraySize, mapping.unrealFormat, d.mipLevels, flags, CreateInfo);
			check(texture->TextureRHI.IsValid());
		}
		else if (d.isCubeMap)
		{
			check(d.sampleCount == 1);
			check(d.width == d.height);
			check(d.depthOrArraySize == 6);
			texture->TextureRHI = GDynamicRHI->RHICreateTextureCube(d.width, mapping.unrealFormat, d.mipLevels, flags, CreateInfo);
			check(texture->TextureRHI.IsValid());
		}
		else
		{
			check(d.sampleCount == 1);
			texture->TextureRHI = GDynamicRHI->RHICreateTexture3D(d.width, d.height, d.depthOrArraySize, mapping.unrealFormat, d.mipLevels, flags, CreateInfo);
			check(texture->TextureRHI.IsValid());
		}

		return texture;
	}

	TextureDesc FRendererInterfaceD3D11::describeTexture(TextureHandle t) 
	{ 
		return t->Desc;
	}

	void FRendererInterfaceD3D11::clearTextureFloat(TextureHandle t, const Color& clearColor) 
	{ 
		if (!t->Desc.isUAV)
			// TODO: add clear of non-UAV textures
			return;

		uint32 Color[4] = { clearColor.r, clearColor.g, clearColor.b, clearColor.a };

		for (uint32 mipLevel = 0; mipLevel < t->Desc.mipLevels; mipLevel++)
		{
			auto UAV = getTextureUAV(t, mipLevel, Format::UNKNOWN);
			m_RHICmdList->ClearUAV(UAV, Color);
		}
	}

	void FRendererInterfaceD3D11::clearTextureUInt(TextureHandle t, uint32 clearColor)
	{
		clearTextureFloat(t, Color(float(clearColor)));
	}

	void FRendererInterfaceD3D11::writeTexture(TextureHandle t, uint32 subresource, const void* data, uint32 rowPitch, uint32 depthPitch) 
	{ 
		checkNoEntry();
	}

	void FRendererInterfaceD3D11::destroyTexture(TextureHandle t)
	{
		if (!t) return;
		delete t;
	}

	class FBufferInitData : public FResourceArrayInterface
	{
	public:
		const void* Data;
		uint32 Size;
		FBufferInitData() : Data(nullptr), Size(0) { }
		virtual const void* GetResourceData() const { return Data; }
		virtual uint32 GetResourceDataSize() const { return Size; }
		virtual void Discard() { }
		virtual bool IsStatic() const { return true; }
		virtual bool GetAllowCPUAccess() const { return false; }
		virtual void SetAllowCPUAccess(bool bInNeedsCPUAccess) { }
	};

	BufferHandle FRendererInterfaceD3D11::createBuffer(const BufferDesc& d, const void* data) 
	{ 
		BufferHandle buffer = new Buffer();
		buffer->Desc = d;
		buffer->EffectiveStride = (d.structStride == 0) ? 4 : d.structStride;

		buffer->Usage = BUF_ShaderResource;
		
		if (d.canHaveUAVs) 
			buffer->Usage |= BUF_UnorderedAccess; 
		else 
			buffer->Usage |= BUF_Dynamic;

		// Force non-structured buffers to have BUF_DrawIndirect flag because otherwise UE will create them as structured.
		if (d.isDrawIndirectArgs || d.structStride == 0) 
			buffer->Usage |= BUF_DrawIndirect;

		FBufferInitData InitData;
		InitData.Data = data;
		InitData.Size = d.byteSize;

		FRHIResourceCreateInfo CreateInfo;
		if (data)
		{
			CreateInfo.ResourceArray = &InitData;
		}

		if (data || !(buffer->Usage & BUF_Dynamic))
			buffer->BufferRHI = GDynamicRHI->RHICreateStructuredBuffer(buffer->EffectiveStride, d.byteSize, buffer->Usage, CreateInfo);
		else
			buffer->BufferRHI = nullptr;

		return buffer;
	}

	void FRendererInterfaceD3D11::writeBuffer(BufferHandle b, const void* data, size_t dataSize) 
	{ 
		check(data);
		check(dataSize == b->Desc.byteSize);
		check(b->Usage & BUF_Dynamic);

		FBufferInitData InitData;
		InitData.Data = data;
		InitData.Size = dataSize;

		FRHIResourceCreateInfo CreateInfo;
		CreateInfo.ResourceArray = &InitData;

		b->BufferRHI.SafeRelease();
		b->UnorderedAccessView.SafeRelease();
		b->ShaderResourceView.SafeRelease();

		b->BufferRHI = GDynamicRHI->RHICreateStructuredBuffer(b->EffectiveStride, dataSize, b->Usage, CreateInfo);
	}

	void FRendererInterfaceD3D11::clearBufferUInt(BufferHandle b, uint32 clearValue) 
	{ 
		FRHIUnorderedAccessView* UAV = getBufferUAV(b, Format::UNKNOWN);
		uint32 ClearValues[4] = { clearValue, clearValue, clearValue, clearValue };
		m_RHICmdList->ClearUAV(UAV, ClearValues);
	}

	void FRendererInterfaceD3D11::copyToBuffer(BufferHandle dest, uint32 destOffsetBytes, BufferHandle src, uint32 srcOffsetBytes, size_t dataSizeBytes) 
	{ 
		m_RHICmdList->CopyStructuredBufferData(dest->BufferRHI, destOffsetBytes, src->BufferRHI, srcOffsetBytes, dataSizeBytes);
	}

	void FRendererInterfaceD3D11::destroyBuffer(BufferHandle b) 
	{ 
		if (!b) return;
		delete b;
	}

	void FRendererInterfaceD3D11::readBuffer(BufferHandle b, void* data, size_t* dataSize) { }

	ConstantBufferHandle FRendererInterfaceD3D11::createConstantBuffer(const ConstantBufferDesc& d, const void* data) 
	{ 
		ConstantBuffer* pCB = new ConstantBuffer();
		pCB->Desc = d;
		pCB->Layout.ConstantBufferSize = d.byteSize;

		if (data)
			writeConstantBuffer(pCB, data, d.byteSize);

		return pCB;
	}

	void FRendererInterfaceD3D11::writeConstantBuffer(ConstantBufferHandle b, const void* data, size_t dataSize) 
	{ 
		check(dataSize == b->Desc.byteSize);
		b->UniformBufferRHI.SafeRelease();
		b->UniformBufferRHI = RHICreateUniformBuffer(data, b->Layout, UniformBuffer_SingleFrame);
	}

	void FRendererInterfaceD3D11::destroyConstantBuffer(ConstantBufferHandle b) 
	{ 
		if (!b) return;
		b->UniformBufferRHI.SafeRelease();
		delete b;
	}

	ShaderHandle FRendererInterfaceD3D11::createShader(const ShaderDesc& d, const void* binary, const size_t binarySize) 
	{ 
		if (d.preCreationCommand)
			d.preCreationCommand->executeAndDispose();

		FD3D11ShaderResourceTable ShaderResourceTable;
		ShaderHandle ret = NULL;
		TArray<uint8> Code;

		FMemoryWriter Ar(Code, true, true);
		Ar << ShaderResourceTable;
		int64 Offset = Ar.Tell();

		Code.AddZeroed(binarySize + 5);
		FMemory::Memcpy(Code.GetData() + Offset, binary, binarySize);

		switch (d.shaderType)
		{
		case ShaderType::SHADER_VERTEX:
		{
			FVertexShaderRHIRef shader = GDynamicRHI->RHICreateVertexShader(Code);
			check(shader);
			//Add a ref for VXGI
			shader->AddRef();
			ret = (ShaderHandle)shader.GetReference();
		}
		break;
		case ShaderType::SHADER_HULL:
		{
			FHullShaderRHIRef shader = GDynamicRHI->RHICreateHullShader(Code);
			check(shader);
			//Add a ref for VXGI
			shader->AddRef();
			ret = (ShaderHandle)shader.GetReference();
		}
		break;
		case ShaderType::SHADER_DOMAIN:
		{
			FDomainShaderRHIRef shader = GDynamicRHI->RHICreateDomainShader(Code);
			check(shader);
			//Add a ref for VXGI
			shader->AddRef();
			ret = (ShaderHandle)shader.GetReference();
		}
		break;
		case ShaderType::SHADER_GEOMETRY:
		{
			FGeometryShaderRHIRef shader = GDynamicRHI->RHICreateGeometryShader(Code);
			check(shader);
			//Add a ref for VXGI
			shader->AddRef();
			ret = (ShaderHandle)shader.GetReference();
		}
		break;
		case ShaderType::SHADER_PIXEL:
		{
			FPixelShaderRHIRef shader = GDynamicRHI->RHICreatePixelShader(Code);
			check(shader);
			//Add a ref for VXGI
			shader->AddRef();
			ret = (ShaderHandle)shader.GetReference();
		}
		break;
		case ShaderType::SHADER_COMPUTE:
		{
			FComputeShaderRHIRef shader = GDynamicRHI->RHICreateComputeShader(Code);
			check(shader);
			//Add a ref for VXGI
			shader->AddRef();
			ret = (ShaderHandle)shader.GetReference();
		}
		break;
		}

		if (d.postCreationCommand)
			d.postCreationCommand->executeAndDispose();

		return (ShaderHandle)ret;
	}

	void FRendererInterfaceD3D11::setPixelShaderResourceAttributes(NVRHI::ShaderHandle PixelShader, const TArray<uint8>& ShaderResourceTable, bool bUsesGlobalCB)
	{
		FD3D11PixelShader* PixelShaderRHI = (FD3D11PixelShader*)PixelShader;

		//Overwrite PixelShader->ShaderResourceTable
		FMemoryReader Ar( ShaderResourceTable, true );
		Ar << PixelShaderRHI->ShaderResourceTable;

		PixelShaderRHI->bShaderNeedsGlobalConstantBuffer = bUsesGlobalCB;
	}

	ShaderHandle FRendererInterfaceD3D11::createShaderFromAPIInterface(ShaderType::Enum shaderType, const void* apiInterface) 
	{ 
		check(shaderType == ShaderType::SHADER_GEOMETRY);

		FD3D11GeometryShader* Shader = new FD3D11GeometryShader;

		Shader->Resource = (ID3D11GeometryShader*)apiInterface;

		FD3D11ShaderResourceTable BlankTable;
		Shader->ShaderResourceTable = BlankTable;
		Shader->bShaderNeedsGlobalConstantBuffer = false;

		//Add a ref for VXGI
		FGeometryShaderRHIRef ShaderRHIRef = Shader;
		ShaderRHIRef->AddRef();
		return (ShaderHandle)ShaderRHIRef.GetReference();
	}

	void FRendererInterfaceD3D11::destroyShader(ShaderHandle s)
	{ 
		FRHIResource* shader = (FRHIResource*)s;
		if (!shader)
			return;

		shader->Release();
	}

	ESamplerAddressMode convertSamplerAddressMode(SamplerDesc::WrapMode mode)
	{
		switch (mode)
		{
		case SamplerDesc::WRAP_MODE_CLAMP: return AM_Clamp; 
		case SamplerDesc::WRAP_MODE_WRAP: return AM_Wrap; 
		case SamplerDesc::WRAP_MODE_BORDER: return AM_Border; 
		}

		return AM_Wrap;
	}

	SamplerHandle FRendererInterfaceD3D11::createSampler(const SamplerDesc& d) 
	{ 
		FSamplerStateInitializerRHI Desc;
		
		if (d.minFilter || d.magFilter)
			if (d.mipFilter)
				Desc.Filter = d.anisotropy > 1 ? SF_AnisotropicLinear : SF_Trilinear;
			else
				Desc.Filter = d.anisotropy > 1 ? SF_AnisotropicLinear : SF_Bilinear;
		else
			Desc.Filter = SF_Point;

		Desc.AddressU = convertSamplerAddressMode(d.wrapMode[0]);
		Desc.AddressV = convertSamplerAddressMode(d.wrapMode[1]);
		Desc.AddressW = convertSamplerAddressMode(d.wrapMode[2]);

		Desc.MipBias = d.mipBias;
		Desc.MaxAnisotropy = d.anisotropy;
		Desc.BorderColor = FColor(
			uint8(d.borderColor.r * 255.f), 
			uint8(d.borderColor.g * 255.f), 
			uint8(d.borderColor.b * 255.f), 
			uint8(d.borderColor.a * 255.f)).DWColor();

		Desc.SamplerComparisonFunction = d.shadowCompare ? SCF_Less : SCF_Never;
		Desc.MinMipLevel = 0;
		Desc.MaxMipLevel = FLT_MAX;

		FSamplerStateRHIRef Sampler = GDynamicRHI->RHICreateSamplerState(Desc);
		Sampler->AddRef();
		return (SamplerHandle)Sampler.GetReference();
	}

	void FRendererInterfaceD3D11::destroySampler(SamplerHandle s) 
	{ 
		if (!s) return;
		((FSamplerStateRHIParamRef)s)->Release();
	}

	InputLayoutHandle FRendererInterfaceD3D11::createInputLayout(const VertexAttributeDesc* d, uint32 attributeCount, const void* vertexShaderBinary, const size_t binarySize) 
	{ 
		checkNoEntry();
		return nullptr;
	}

	void FRendererInterfaceD3D11::destroyInputLayout(InputLayoutHandle i) { }

	PerformanceQueryHandle FRendererInterfaceD3D11::createPerformanceQuery(const char* name) 
	{ 
		checkNoEntry();
		return nullptr;
	}

	void FRendererInterfaceD3D11::destroyPerformanceQuery(PerformanceQueryHandle query) { }
	void FRendererInterfaceD3D11::beginPerformanceQuery(PerformanceQueryHandle query, bool onlyAnnotation) { }
	void FRendererInterfaceD3D11::endPerformanceQuery(PerformanceQueryHandle query) { }

	float FRendererInterfaceD3D11::getPerformanceQueryTimeMS(PerformanceQueryHandle query) 
	{ 
		return 0.f;
	}

	GraphicsAPI::Enum FRendererInterfaceD3D11::getGraphicsAPI() 
	{ 
		return GraphicsAPI::D3D11;
	}

	void* FRendererInterfaceD3D11::getAPISpecificInterface(APISpecificInterface::Enum interfaceType) 
	{ 
		if (interfaceType == APISpecificInterface::D3D11DEVICE)
			return GDynamicRHI->RHIGetNativeDevice();

		return nullptr;
	}

	bool FRendererInterfaceD3D11::isOpenGLExtensionSupported(const char* name) 
	{ 
		return false;
	}

	void* FRendererInterfaceD3D11::getOpenGLProcAddress(const char* procname) 
	{ 
		return nullptr;
	}

	void convertPrimTypeAndCount(PrimitiveType::Enum primType, uint32 vertexCount, uint32& unrealPrimType, uint32& primCount)
	{
		unrealPrimType = PT_TriangleList;
		primCount = 0;

		switch (primType)
		{
		case PrimitiveType::POINT_LIST:            
			unrealPrimType = PT_PointList;               
			primCount = vertexCount;
			break;

		case PrimitiveType::TRIANGLE_STRIP:        
			unrealPrimType = PT_TriangleStrip;      
			primCount = vertexCount - 2;
			break;

		case PrimitiveType::TRIANGLE_LIST:         
			unrealPrimType = PT_TriangleList;            
			primCount = vertexCount / 3;
			break;

		case PrimitiveType::PATCH_1_CONTROL_POINT: 
			unrealPrimType = PT_1_ControlPointPatchList; 
			primCount = vertexCount;
			break;

		case PrimitiveType::PATCH_3_CONTROL_POINT: 
			unrealPrimType = PT_3_ControlPointPatchList; 
			primCount = vertexCount / 3;
			break;

		default: check(0); // unknown prim type
		}
	}

	void FRendererInterfaceD3D11::draw(const DrawCallState& state, const DrawArguments* args, uint32 numDrawCalls) 
	{ 
		applyState(state, true);

		for (uint32 n = 0; n < numDrawCalls; n++)
		{
			uint32 PrimitiveType = 0;
			uint32 PrimitiveCount = 0;

			convertPrimTypeAndCount(state.primType, args[n].vertexCount, PrimitiveType, PrimitiveCount);

			m_RHICmdList->DrawPrimitive(PrimitiveType, args[n].startVertexLocation, PrimitiveCount, args[n].instanceCount);
		}
	}

	void FRendererInterfaceD3D11::drawIndexed(const DrawCallState& state, const DrawArguments* args, uint32 numDrawCalls) 
	{ 
		// Not used by VXGI
		check(false);
	}

	void FRendererInterfaceD3D11::drawIndirect(const DrawCallState& state, BufferHandle indirectParams, uint32 offsetBytes) 
	{ 
		// Only used by VXGI for sample debug rendering and adaptive diffuse tracing, which are not integrated into UE
		check(false);
	}

	void FRendererInterfaceD3D11::dispatch(const DispatchState& state, uint32 groupsX, uint32 groupsY, uint32 groupsZ) 
	{ 
		applyState(state);

		m_RHICmdList->DispatchComputeShader(groupsX, groupsY, groupsZ);

		clearUAVs(state);
	}

	void FRendererInterfaceD3D11::dispatchIndirect(const DispatchState& state, BufferHandle indirectParams, uint32 offsetBytes) 
	{ 
		check(indirectParams);
		
		applyState(state);

		m_RHICmdList->DispatchIndirectComputeShaderStructured(indirectParams->BufferRHI.GetReference(), offsetBytes);

		clearUAVs(state);
	}

	void FRendererInterfaceD3D11::executeRenderThreadCommand(IRenderThreadCommand* onCommand) 
	{ 
		// TODO: deferred execution
		onCommand->executeAndDispose();
	}

	uint32 FRendererInterfaceD3D11::getAFRGroupOfCurrentFrameForSLI(uint32 numAFRGroups) 
	{ 
		return 0;
	}

	void FRendererInterfaceD3D11::setEnableUavBarriersForTexture(TextureHandle, bool)
	{
	}

	void FRendererInterfaceD3D11::setEnableUavBarriersForBuffer(BufferHandle, bool)
	{
	}

	TextureHandle FRendererInterfaceD3D11::getTextureFromRHI(FRHITexture* TextureRHI)
	{
		if (!TextureRHI) return nullptr;

		TextureHandle texture = m_UnmanagedTextures[TextureRHI];
		if (texture) return texture;

		auto Texture2D = (FD3D11Texture2D*)TextureRHI->GetTexture2D();
		auto Texture2DArray = (FD3D11Texture2DArray*)TextureRHI->GetTexture2DArray();
		auto TextureCube = (FD3D11TextureCube*)TextureRHI->GetTextureCube();

		ID3D11Texture2D* pResource =
			Texture2D ? Texture2D->GetResource() :
			Texture2DArray ? Texture2DArray->GetResource() :
			TextureCube ? TextureCube->GetResource() :
			nullptr;

		check(pResource);

		D3D11_TEXTURE2D_DESC desc;
		pResource->GetDesc(&desc);

		texture = new Texture();
		texture->TextureRHI = TextureRHI;
		texture->Desc.width = uint32(desc.Width);
		texture->Desc.height = desc.Height;
		texture->Desc.depthOrArraySize = Texture2D ? 0 : desc.ArraySize;
		texture->Desc.mipLevels = desc.MipLevels;
		texture->Desc.sampleCount = desc.SampleDesc.Count;
		texture->Desc.sampleQuality = desc.SampleDesc.Quality;
		texture->Desc.isArray = desc.ArraySize > 1;
		texture->Desc.isCubeMap = (desc.MiscFlags & D3D11_RESOURCE_MISC_TEXTURECUBE) != 0;
		texture->Desc.isRenderTarget = (desc.BindFlags & (D3D11_BIND_RENDER_TARGET | D3D11_BIND_DEPTH_STENCIL)) != 0;
		texture->Desc.isUAV = (desc.BindFlags & D3D11_BIND_UNORDERED_ACCESS) != 0;

		for (auto& mapping : FormatMappings)
			if (mapping.resourceFormat == desc.Format || mapping.srvFormat == desc.Format || mapping.rtvFormat == desc.Format)
			{
				texture->Desc.format = mapping.abstractFormat;
				break;
			}

        check(texture->Desc.format != Format::UNKNOWN);

		m_UnmanagedTextures[TextureRHI] = texture;
		return texture;
	}

	FRHITexture* FRendererInterfaceD3D11::getRHITexture(TextureHandle texture)
	{
		if (!texture) return nullptr;

		return texture->TextureRHI;
	}

	void FRendererInterfaceD3D11::forgetAboutTexture(FRHITexture* texture)
	{
		m_UnmanagedTextures.erase(texture);
	}

	FRHIShaderResourceView* FRendererInterfaceD3D11::getTextureSRV(TextureHandle t, uint32 mipLevel, Format::Enum format)
	{
		if (format == Format::UNKNOWN)
			format = t->Desc.format;

		auto key = std::make_pair(format, mipLevel);
		auto found = t->ShaderResourceViews.find(key);
		if (found != t->ShaderResourceViews.end())
			return found->second;

		D3D11_SHADER_RESOURCE_VIEW_DESC SRVDesc = {};

		SRVDesc.Format = GetFormatMapping(format).srvFormat;

		uint32 firstMip = mipLevel >= t->Desc.mipLevels ? 0 : mipLevel;
		uint32 mipLevels = mipLevel >= t->Desc.mipLevels ? t->Desc.mipLevels : 1;

		if (t->Desc.isArray || t->Desc.isCubeMap)
		{
			if (t->Desc.sampleCount > 1)
			{
				SRVDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DMSARRAY;
				SRVDesc.Texture2DMSArray.ArraySize = t->Desc.depthOrArraySize;
			}
			else if (t->Desc.isCubeMap)
			{
				if (t->Desc.depthOrArraySize > 6)
				{
					SRVDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBEARRAY;
					SRVDesc.TextureCubeArray.NumCubes = t->Desc.depthOrArraySize / 6;
					SRVDesc.TextureCubeArray.MostDetailedMip = firstMip;
					SRVDesc.TextureCubeArray.MipLevels = mipLevels;
				}
				else
				{
					SRVDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBE;
					SRVDesc.TextureCube.MostDetailedMip = firstMip;
					SRVDesc.TextureCube.MipLevels = mipLevels;
				}
			}
			else
			{
				SRVDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DARRAY;
				SRVDesc.Texture2DArray.ArraySize = t->Desc.depthOrArraySize;
				SRVDesc.Texture2DArray.MostDetailedMip = firstMip;
				SRVDesc.Texture2DArray.MipLevels = mipLevels;
			}
		}
		else if (t->Desc.depthOrArraySize > 0)
		{
			SRVDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE3D;
			SRVDesc.Texture3D.MostDetailedMip = firstMip;
			SRVDesc.Texture3D.MipLevels = mipLevels;
		}
		else
		{
			if (t->Desc.sampleCount > 1)
			{
				SRVDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DMS;
			}
			else
			{
				SRVDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
				SRVDesc.Texture2D.MostDetailedMip = firstMip;
				SRVDesc.Texture2D.MipLevels = mipLevels;
			}
		}

		FD3D11TextureBase* Texture = GetD3D11TextureFromRHITexture(t->TextureRHI);
		TRefCountPtr<ID3D11ShaderResourceView> D3DView;
		m_Device->CreateShaderResourceView(Texture->GetResource(), &SRVDesc, D3DView.GetInitReference());
		FRHIShaderResourceView* View = new FD3D11ShaderResourceView(D3DView, Texture);
		t->ShaderResourceViews[key] = View;
		return View;
	}

	FRHIUnorderedAccessView* FRendererInterfaceD3D11::getTextureUAV(TextureHandle t, uint32 mipLevel, Format::Enum format)
	{
		if (format == Format::UNKNOWN)
			format = t->Desc.format;

		auto key = std::make_pair(format, mipLevel);
		auto found = t->UnorderedAccessViews.find(key);
		if (found != t->UnorderedAccessViews.end())
			return found->second;

		D3D11_UNORDERED_ACCESS_VIEW_DESC UAVDesc = {};

		const FormatMapping& mapping = GetFormatMapping(format);
		UAVDesc.Format = mapping.srvFormat;

		if (t->Desc.isArray || t->Desc.isCubeMap)
		{
			UAVDesc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2DARRAY;
			UAVDesc.Texture2DArray.ArraySize = t->Desc.depthOrArraySize;
			UAVDesc.Texture2DArray.MipSlice = mipLevel;
		}
		else if (t->Desc.depthOrArraySize > 0)
		{
			UAVDesc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE3D;
			UAVDesc.Texture3D.WSize = t->Desc.depthOrArraySize;
			UAVDesc.Texture3D.MipSlice = mipLevel;
		}
		else
		{
			UAVDesc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2D;
			UAVDesc.Texture2D.MipSlice = mipLevel;
		}

		FD3D11TextureBase* Texture = GetD3D11TextureFromRHITexture(t->TextureRHI);
		TRefCountPtr<ID3D11UnorderedAccessView> D3DView;
		m_Device->CreateUnorderedAccessView(Texture->GetResource(), &UAVDesc, D3DView.GetInitReference());
		FRHIUnorderedAccessView* View = new FD3D11UnorderedAccessView(D3DView, Texture);
		t->UnorderedAccessViews[key] = View;
		return View;
	}

	FRHIShaderResourceView* FRendererInterfaceD3D11::getBufferSRV(BufferHandle b, Format::Enum format)
	{
		if (b->ShaderResourceView)
			return b->ShaderResourceView;

		if (!b->BufferRHI)
			return nullptr;

		D3D11_SHADER_RESOURCE_VIEW_DESC SRVDesc = {};

		SRVDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;

		uint32 EffectiveStride = 0;

		if (b->Desc.structStride != 0)
		{
			EffectiveStride = b->Desc.structStride;
			SRVDesc.Format = DXGI_FORMAT_UNKNOWN;
			SRVDesc.Buffer.ElementWidth = b->Desc.structStride;
		}
		else
		{
			const FormatMapping& mapping = GetFormatMapping(format == Format::UNKNOWN ? Format::R32_UINT : format);

			EffectiveStride = mapping.bytesPerPixel;
			SRVDesc.Format = mapping.srvFormat;
		}

		FD3D11StructuredBuffer*  StructuredBuffer = FD3D11DynamicRHI::ResourceCast(b->BufferRHI.GetReference());
		SRVDesc.Buffer.FirstElement = 0;
		SRVDesc.Buffer.NumElements  = b->Desc.byteSize / EffectiveStride;

		TRefCountPtr<ID3D11ShaderResourceView> D3DView;
		m_Device->CreateShaderResourceView(StructuredBuffer->Resource, &SRVDesc, D3DView.GetInitReference());
		FRHIShaderResourceView* View = new FD3D11ShaderResourceView(D3DView, StructuredBuffer);
		b->ShaderResourceView = View;
		return View;
	}

	FRHIUnorderedAccessView* FRendererInterfaceD3D11::getBufferUAV(BufferHandle b, Format::Enum format) 
	{
		if (b->UnorderedAccessView)
			return b->UnorderedAccessView;

		D3D11_UNORDERED_ACCESS_VIEW_DESC UAVDesc = {};

		UAVDesc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;

		uint32 EffectiveStride = 0;

		if (b->Desc.structStride != 0)
		{
			EffectiveStride = b->Desc.structStride;
			UAVDesc.Format = DXGI_FORMAT_UNKNOWN;
		}
		else
		{
			const FormatMapping& mapping = GetFormatMapping(format == Format::UNKNOWN ? Format::R32_UINT : format);

			EffectiveStride = mapping.bytesPerPixel;
			UAVDesc.Format = mapping.srvFormat;
		}

		FD3D11StructuredBuffer*  StructuredBuffer = FD3D11DynamicRHI::ResourceCast(b->BufferRHI.GetReference());
		UAVDesc.Buffer.FirstElement = 0;
		UAVDesc.Buffer.NumElements  = b->Desc.byteSize / EffectiveStride;

		TRefCountPtr<ID3D11UnorderedAccessView> D3DView;
		m_Device->CreateUnorderedAccessView(StructuredBuffer->Resource, &UAVDesc, D3DView.GetInitReference());
		FRHIUnorderedAccessView* View = new FD3D11UnorderedAccessView(D3DView, StructuredBuffer);
		b->UnorderedAccessView = View;
		return View;
	}

	FRasterizerStateRHIParamRef FRendererInterfaceD3D11::getRasterizerState(const RasterState& rasterState)
	{
		CrcHash hasher;
		hasher.Add(rasterState);
		uint32 hash = hasher.Get();

		auto it = m_RasterizerStates.find(hash);
		if (it != m_RasterizerStates.end())
			return it->second;

		D3D11_RASTERIZER_DESC RasterizerDesc = { };

		switch (rasterState.fillMode)
		{
		case RasterState::FILL_SOLID:
			RasterizerDesc.FillMode = D3D11_FILL_SOLID;
			break;
		case RasterState::FILL_LINE:
			RasterizerDesc.FillMode = D3D11_FILL_WIREFRAME;
			break;
		}

		switch (rasterState.cullMode)
		{
		case RasterState::CULL_BACK:
			RasterizerDesc.CullMode = D3D11_CULL_BACK;
			break;
		case RasterState::CULL_FRONT:
			RasterizerDesc.CullMode = D3D11_CULL_FRONT;
			break;
		case RasterState::CULL_NONE:
			RasterizerDesc.CullMode = D3D11_CULL_NONE;
			break;
		}

		RasterizerDesc.FrontCounterClockwise = rasterState.frontCounterClockwise;
		RasterizerDesc.DepthBias = rasterState.depthBias;
		RasterizerDesc.DepthBiasClamp = rasterState.depthBiasClamp;
		RasterizerDesc.SlopeScaledDepthBias = rasterState.slopeScaledDepthBias;
		RasterizerDesc.DepthClipEnable = rasterState.depthClipEnable;
		RasterizerDesc.MultisampleEnable = rasterState.multisampleEnable;
		RasterizerDesc.AntialiasedLineEnable = rasterState.antialiasedLineEnable;
		RasterizerDesc.ScissorEnable = false; // rasterState.scissorEnable;
		
		FD3D11RasterizerState* RasterizerState = new FD3D11RasterizerState();

		bool extendedState = rasterState.conservativeRasterEnable || rasterState.forcedSampleCount || rasterState.programmableSamplePositionsEnable;
		
		if (extendedState)
		{
			NvAPI_D3D11_RASTERIZER_DESC_EX descEx;
			memset(&descEx, 0, sizeof(descEx));
			memcpy(&descEx, &RasterizerDesc, sizeof(RasterizerDesc));

			descEx.ConservativeRasterEnable = rasterState.conservativeRasterEnable;
			descEx.ProgrammableSamplePositionsEnable = rasterState.programmableSamplePositionsEnable;
			descEx.SampleCount = rasterState.forcedSampleCount;
			descEx.ForcedSampleCount = rasterState.forcedSampleCount;
			memcpy(descEx.SamplePositionsX, rasterState.samplePositionsX, sizeof(rasterState.samplePositionsX));
			memcpy(descEx.SamplePositionsY, rasterState.samplePositionsY, sizeof(rasterState.samplePositionsY));

			NvAPI_D3D11_CreateRasterizerState(m_Device, &descEx, RasterizerState->Resource.GetInitReference());
		}
		else
		{
			m_Device->CreateRasterizerState(&RasterizerDesc, RasterizerState->Resource.GetInitReference());
		}

		check(RasterizerState->Resource.IsValid());

		m_RasterizerStates[hash] = RasterizerState;
		return RasterizerState;
	}

	D3D11_STENCIL_OP convertStencilOp(DepthStencilState::StencilOp value)
	{
		switch (value)
		{
		case DepthStencilState::STENCIL_OP_KEEP:
			return D3D11_STENCIL_OP_KEEP;
		case DepthStencilState::STENCIL_OP_ZERO:
			return D3D11_STENCIL_OP_ZERO;
		case DepthStencilState::STENCIL_OP_REPLACE:
			return D3D11_STENCIL_OP_REPLACE;
		case DepthStencilState::STENCIL_OP_INCR_SAT:
			return D3D11_STENCIL_OP_INCR_SAT;
		case DepthStencilState::STENCIL_OP_DECR_SAT:
			return D3D11_STENCIL_OP_DECR_SAT;
		case DepthStencilState::STENCIL_OP_INVERT:
			return D3D11_STENCIL_OP_INVERT;
		case DepthStencilState::STENCIL_OP_INCR:
			return D3D11_STENCIL_OP_INCR;
		case DepthStencilState::STENCIL_OP_DECR:
			return D3D11_STENCIL_OP_DECR;
		default:
			return D3D11_STENCIL_OP_KEEP;
		}
	}

	D3D11_COMPARISON_FUNC convertComparisonFunc(DepthStencilState::ComparisonFunc value)
	{
		switch (value)
		{
		case DepthStencilState::COMPARISON_NEVER:
			return D3D11_COMPARISON_NEVER;
		case DepthStencilState::COMPARISON_LESS:
			return D3D11_COMPARISON_LESS;
		case DepthStencilState::COMPARISON_EQUAL:
			return D3D11_COMPARISON_EQUAL;
		case DepthStencilState::COMPARISON_LESS_EQUAL:
			return D3D11_COMPARISON_LESS_EQUAL;
		case DepthStencilState::COMPARISON_GREATER:
			return D3D11_COMPARISON_GREATER;
		case DepthStencilState::COMPARISON_NOT_EQUAL:
			return D3D11_COMPARISON_NOT_EQUAL;
		case DepthStencilState::COMPARISON_GREATER_EQUAL:
			return D3D11_COMPARISON_GREATER_EQUAL;
		case DepthStencilState::COMPARISON_ALWAYS:
			return D3D11_COMPARISON_ALWAYS;
		default:
			return D3D11_COMPARISON_NEVER;
		}
	}

	FDepthStencilStateRHIParamRef FRendererInterfaceD3D11::getDepthStencilState(const DepthStencilState& depthStencilState, bool depthTargetPresent)
	{
		CrcHash hasher;
		hasher.Add(depthStencilState);
		uint32 hash = hasher.Get();

		auto it = m_DepthStencilStates.find(hash);
		if (it != m_DepthStencilStates.end())
			return it->second;

		D3D11_DEPTH_STENCIL_DESC DepthStencilDesc = { };

		DepthStencilDesc.DepthEnable = depthStencilState.depthEnable;
		DepthStencilDesc.DepthWriteMask = depthStencilState.depthWriteMask == DepthStencilState::DEPTH_WRITE_MASK_ALL ? D3D11_DEPTH_WRITE_MASK_ALL : D3D11_DEPTH_WRITE_MASK_ZERO;
		DepthStencilDesc.DepthFunc = convertComparisonFunc(depthStencilState.depthFunc);
		DepthStencilDesc.StencilEnable = depthStencilState.stencilEnable;
		DepthStencilDesc.StencilReadMask = (UINT8)depthStencilState.stencilReadMask;
		DepthStencilDesc.StencilWriteMask = (UINT8)depthStencilState.stencilWriteMask;
		DepthStencilDesc.FrontFace.StencilFailOp = convertStencilOp(depthStencilState.frontFace.stencilFailOp);
		DepthStencilDesc.FrontFace.StencilDepthFailOp = convertStencilOp(depthStencilState.frontFace.stencilDepthFailOp);
		DepthStencilDesc.FrontFace.StencilPassOp = convertStencilOp(depthStencilState.frontFace.stencilPassOp);
		DepthStencilDesc.FrontFace.StencilFunc = convertComparisonFunc(depthStencilState.frontFace.stencilFunc);
		DepthStencilDesc.BackFace.StencilFailOp = convertStencilOp(depthStencilState.backFace.stencilFailOp);
		DepthStencilDesc.BackFace.StencilDepthFailOp = convertStencilOp(depthStencilState.backFace.stencilDepthFailOp);
		DepthStencilDesc.BackFace.StencilPassOp = convertStencilOp(depthStencilState.backFace.stencilPassOp);
		DepthStencilDesc.BackFace.StencilFunc = convertComparisonFunc(depthStencilState.backFace.stencilFunc);

		if ((depthStencilState.depthEnable || depthStencilState.stencilEnable) && !depthTargetPresent)
		{
			DepthStencilDesc.DepthEnable = false;
			DepthStencilDesc.StencilEnable = false;
		}

		FD3D11DepthStencilState* DepthStencilState = new FD3D11DepthStencilState();
		
		m_Device->CreateDepthStencilState(&DepthStencilDesc, DepthStencilState->Resource.GetInitReference());

		check(DepthStencilState->Resource.IsValid());

		m_DepthStencilStates[hash] = DepthStencilState;
		return DepthStencilState;
	}

	D3D11_BLEND convertBlendValue(BlendState::BlendValue value)
	{
		switch (value)
		{
		case BlendState::BLEND_ZERO:
			return D3D11_BLEND_ZERO;
		case BlendState::BLEND_ONE:
			return D3D11_BLEND_ONE;
		case BlendState::BLEND_SRC_COLOR:
			return D3D11_BLEND_SRC_COLOR;
		case BlendState::BLEND_INV_SRC_COLOR:
			return D3D11_BLEND_INV_SRC_COLOR;
		case BlendState::BLEND_SRC_ALPHA:
			return D3D11_BLEND_SRC_ALPHA;
		case BlendState::BLEND_INV_SRC_ALPHA:
			return D3D11_BLEND_INV_SRC_ALPHA;
		case BlendState::BLEND_DEST_ALPHA:
			return D3D11_BLEND_DEST_ALPHA;
		case BlendState::BLEND_INV_DEST_ALPHA:
			return D3D11_BLEND_INV_DEST_ALPHA;
		case BlendState::BLEND_DEST_COLOR:
			return D3D11_BLEND_DEST_COLOR;
		case BlendState::BLEND_INV_DEST_COLOR:
			return D3D11_BLEND_INV_DEST_COLOR;
		case BlendState::BLEND_SRC_ALPHA_SAT:
			return D3D11_BLEND_SRC_ALPHA_SAT;
		case BlendState::BLEND_BLEND_FACTOR:
			return D3D11_BLEND_BLEND_FACTOR;
		case BlendState::BLEND_INV_BLEND_FACTOR:
			return D3D11_BLEND_INV_BLEND_FACTOR;
		case BlendState::BLEND_SRC1_COLOR:
			return D3D11_BLEND_SRC1_COLOR;
		case BlendState::BLEND_INV_SRC1_COLOR:
			return D3D11_BLEND_INV_SRC1_COLOR;
		case BlendState::BLEND_SRC1_ALPHA:
			return D3D11_BLEND_SRC1_ALPHA;
		case BlendState::BLEND_INV_SRC1_ALPHA:
			return D3D11_BLEND_INV_SRC1_ALPHA;
		default:
			return D3D11_BLEND_ZERO;
		}
	}

	D3D11_BLEND_OP convertBlendOp(BlendState::BlendOp value)
	{
		switch (value)
		{
		case BlendState::BLEND_OP_ADD:
			return D3D11_BLEND_OP_ADD;
		case BlendState::BLEND_OP_SUBTRACT:
			return D3D11_BLEND_OP_SUBTRACT;
		case BlendState::BLEND_OP_REV_SUBTRACT:
			return D3D11_BLEND_OP_REV_SUBTRACT;
		case BlendState::BLEND_OP_MIN:
			return D3D11_BLEND_OP_MIN;
		case BlendState::BLEND_OP_MAX:
			return D3D11_BLEND_OP_MAX;
		default:
			return D3D11_BLEND_OP_ADD;
		}
	}

	FBlendStateRHIParamRef FRendererInterfaceD3D11::getBlendState(const BlendState& blendState)
	{
		CrcHash hasher;
		hasher.Add(blendState);
		uint32 hash = hasher.Get();

		auto it = m_BlendStates.find(hash);
		if (it != m_BlendStates.end())
			return it->second;

		D3D11_BLEND_DESC BlendDesc = { };

		BlendDesc.AlphaToCoverageEnable = blendState.alphaToCoverage;
		BlendDesc.IndependentBlendEnable = true;

		for (uint32_t i = 0; i < ARRAYSIZE(blendState.blendEnable); i++)
		{
			BlendDesc.RenderTarget[i].BlendEnable = blendState.blendEnable[i];
			BlendDesc.RenderTarget[i].SrcBlend = convertBlendValue(blendState.srcBlend[i]);
			BlendDesc.RenderTarget[i].DestBlend = convertBlendValue(blendState.destBlend[i]);
			BlendDesc.RenderTarget[i].BlendOp = convertBlendOp(blendState.blendOp[i]);
			BlendDesc.RenderTarget[i].SrcBlendAlpha = convertBlendValue(blendState.srcBlendAlpha[i]);
			BlendDesc.RenderTarget[i].DestBlendAlpha = convertBlendValue(blendState.destBlendAlpha[i]);
			BlendDesc.RenderTarget[i].BlendOpAlpha = convertBlendOp(blendState.blendOpAlpha[i]);
			BlendDesc.RenderTarget[i].RenderTargetWriteMask = 
				(blendState.colorWriteEnable[i] & BlendState::COLOR_MASK_RED   ? D3D11_COLOR_WRITE_ENABLE_RED   : 0) |
				(blendState.colorWriteEnable[i] & BlendState::COLOR_MASK_GREEN ? D3D11_COLOR_WRITE_ENABLE_GREEN : 0) |
				(blendState.colorWriteEnable[i] & BlendState::COLOR_MASK_BLUE  ? D3D11_COLOR_WRITE_ENABLE_BLUE  : 0) |
				(blendState.colorWriteEnable[i] & BlendState::COLOR_MASK_ALPHA ? D3D11_COLOR_WRITE_ENABLE_ALPHA : 0);
		}

		FD3D11BlendState* BlendState = new FD3D11BlendState();
		
		m_Device->CreateBlendState(&BlendDesc, BlendState->Resource.GetInitReference());

		check(BlendState->Resource.IsValid());

		m_BlendStates[hash] = BlendState;
		return BlendState;
	}

	template<typename ShaderType>
	void FRendererInterfaceD3D11::applyShaderState(PipelineStageBindings bindings)
	{
		if (!bindings.shader)
			return;

		ShaderType shader = (ShaderType)bindings.shader;

		for (uint32 n = 0; n < bindings.constantBufferBindingCount; n++)
		{
			const auto& binding = bindings.constantBuffers[n];
			check(binding.buffer->UniformBufferRHI);
			m_RHICmdList->SetShaderUniformBuffer(shader, binding.slot, binding.buffer->UniformBufferRHI);
		}

		for (uint32 n = 0; n < bindings.textureBindingCount; n++)
		{
			const auto& binding = bindings.textures[n];

			// UAVs are handled elsewhere
			if (!binding.isWritable)
			{ 
				m_RHICmdList->SetShaderResourceViewParameter(shader, binding.slot, getTextureSRV(binding.texture, binding.mipLevel, binding.format));
			}
		}

		for (uint32 n = 0; n < bindings.bufferBindingCount; n++)
		{
			const auto& binding = bindings.buffers[n];

			// UAVs are handled elsewhere
			if (!binding.isWritable)
			{
				m_RHICmdList->SetShaderResourceViewParameter(shader, binding.slot, getBufferSRV(binding.buffer, binding.format));
			}
		}

		for (uint32 n = 0; n < bindings.textureSamplerBindingCount; n++)
		{
			const auto& binding = bindings.textureSamplers[n];

			m_RHICmdList->SetShaderSampler(shader, binding.slot, (FSamplerStateRHIParamRef)binding.sampler);
		}
	}
	
	void FRendererInterfaceD3D11::applyState(DrawCallState state, bool applyShaders)
	{
		check(state.inputLayout == nullptr);

		if (applyShaders)
		{
			auto BoundShaderState = GDynamicRHI->RHICreateBoundShaderState(
				nullptr, // VertexDeclaration - nullptr because VXGI never creates input layouts
				(FVertexShaderRHIParamRef)state.VS.shader,
				(FHullShaderRHIParamRef)state.HS.shader,
				(FDomainShaderRHIParamRef)state.DS.shader,
				(FPixelShaderRHIParamRef)state.PS.shader,	// Pixel, then geometry - there's no bug here
				(FGeometryShaderRHIParamRef)state.GS.shader
				);

			m_RHICmdList->SetBoundShaderState(BoundShaderState);
		}

		FRHIRenderTargetView RTVs[8] = { };
		FRHIDepthRenderTargetView DSV;
		FRHIDepthRenderTargetView* pDSV = nullptr;
		FUnorderedAccessViewRHIParamRef pUAVs[8] = { };

		for (uint32 RTVIndex = 0; RTVIndex < state.renderState.targetCount; RTVIndex++)
		{ 
			RTVs[RTVIndex] = FRHIRenderTargetView(
				state.renderState.targets[RTVIndex]->TextureRHI,
				state.renderState.targetMipSlices[RTVIndex],
				state.renderState.targetIndicies[RTVIndex]
			);
		}

		if (state.renderState.depthTarget)
		{
			check(state.renderState.depthIndex == 0);
			check(state.renderState.depthMipSlice == 0);

			DSV = FRHIDepthRenderTargetView(state.renderState.depthTarget->TextureRHI);
			pDSV = &DSV;
		}

		uint32 NumUAVs = 0;

		for (uint32 n = 0; n < state.PS.textureBindingCount; n++)
		{
			const auto& binding = state.PS.textures[n];
			
			if (binding.isWritable)
			{
				check(binding.slot >= state.renderState.targetCount);
				check(binding.slot < 8);

				uint32 UAVIndex = binding.slot - state.renderState.targetCount;
				pUAVs[UAVIndex] = getTextureUAV(binding.texture, binding.mipLevel, binding.format);
				NumUAVs = FMath::Max(NumUAVs, UAVIndex + 1);
			}
		}

		for (uint32 n = 0; n < state.PS.bufferBindingCount; n++)
		{
			const auto& binding = state.PS.buffers[n];

			if (binding.isWritable)
			{
				check(binding.slot >= state.renderState.targetCount);
				check(binding.slot < 8);

				uint32 UAVIndex = binding.slot - state.renderState.targetCount;
				pUAVs[UAVIndex] = getBufferUAV(binding.buffer, binding.format);
				NumUAVs = FMath::Max(NumUAVs, UAVIndex + 1);
			}
		}

		m_RHICmdList->SetRenderTargets(state.renderState.targetCount, RTVs, pDSV, NumUAVs, pUAVs);

		if (state.renderState.clearColorTarget || state.renderState.clearDepthTarget || state.renderState.clearStencilTarget)
		{
			FLinearColor ClearColors[8];
			for (uint32 rt = 0; rt < state.renderState.targetCount; rt++)
				ClearColors[rt] = FLinearColor(state.renderState.clearColor.r, state.renderState.clearColor.g, state.renderState.clearColor.b, state.renderState.clearColor.a);

			m_RHICmdList->ClearMRT(state.renderState.clearColorTarget, state.renderState.targetCount, ClearColors, state.renderState.clearDepthTarget, state.renderState.clearDepth, state.renderState.clearStencilTarget, state.renderState.clearStencil, FIntRect());
		}

		// Shader resources have to be set after render targets to avoid errors like this one:
		//		Resource being set to PS shader resource slot 2 is still bound on output! Forcing to NULL

		applyShaderState<FVertexShaderRHIParamRef>(state.VS);
		applyShaderState<FHullShaderRHIParamRef>(state.HS);
		applyShaderState<FDomainShaderRHIParamRef>(state.DS);
		applyShaderState<FGeometryShaderRHIParamRef>(state.GS);
		applyShaderState<FPixelShaderRHIParamRef>(state.PS);

		m_RHICmdList->SetRasterizerState(getRasterizerState(state.renderState.rasterState));
		m_RHICmdList->SetDepthStencilState(getDepthStencilState(state.renderState.depthStencilState, state.renderState.depthTarget != nullptr), state.renderState.depthStencilState.stencilRefValue);
		FLinearColor BlendFactors(state.renderState.blendState.blendFactor.r, state.renderState.blendState.blendFactor.g, state.renderState.blendState.blendFactor.b, state.renderState.blendState.blendFactor.a);
		m_RHICmdList->SetBlendState(getBlendState(state.renderState.blendState), BlendFactors);

		FViewportBounds Viewports[16];
		FScissorRect ScissorRects[16];

		for (uint32 vp = 0; vp < state.renderState.viewportCount; vp++)
		{
			Viewports[vp].TopLeftX = state.renderState.viewports[vp].minX;
			Viewports[vp].TopLeftY = state.renderState.viewports[vp].minY;
			Viewports[vp].Width = state.renderState.viewports[vp].maxX - state.renderState.viewports[vp].minX;
			Viewports[vp].Height = state.renderState.viewports[vp].maxY - state.renderState.viewports[vp].minY;
			Viewports[vp].MinDepth = state.renderState.viewports[vp].minZ;
			Viewports[vp].MaxDepth = state.renderState.viewports[vp].maxZ;

			if (state.renderState.rasterState.scissorEnable)
			{
				ScissorRects[vp].Left = state.renderState.scissorRects[vp].minX;
				ScissorRects[vp].Top = state.renderState.scissorRects[vp].minY;
				ScissorRects[vp].Right = state.renderState.scissorRects[vp].maxX;
				ScissorRects[vp].Bottom = state.renderState.scissorRects[vp].maxY;
			}
			else
			{
				ScissorRects[vp].Left = 0;
				ScissorRects[vp].Top = 0;
				ScissorRects[vp].Right = GetMax2DTextureDimension();
				ScissorRects[vp].Bottom = GetMax2DTextureDimension();
			}
		}
		
		m_RHICmdList->SetViewportsAndScissorRects(state.renderState.viewportCount, Viewports, ScissorRects);
	}

	void FRendererInterfaceD3D11::applyState(DispatchState state)
	{
		FComputeShaderRHIParamRef ComputeShader = (FComputeShaderRHIParamRef)state.shader;

		m_RHICmdList->SetComputeShader(ComputeShader);
		m_RHICmdList->SetRenderTargets(0, nullptr, nullptr, 0, nullptr);

		for (uint32 n = 0; n < state.textureBindingCount; n++)
		{
			const auto& binding = state.textures[n];

			if (binding.isWritable)
			{
				check(binding.slot < 8);

				m_RHICmdList->SetUAVParameter(ComputeShader, binding.slot, getTextureUAV(binding.texture, binding.mipLevel, binding.format));
			}
		}

		for (uint32 n = 0; n < state.bufferBindingCount; n++)
		{
			const auto& binding = state.buffers[n];

			if (binding.isWritable)
			{
				check(binding.slot < 8);

				m_RHICmdList->SetUAVParameter(ComputeShader, binding.slot, getBufferUAV(binding.buffer, binding.format));
			}
		}

		applyShaderState<FComputeShaderRHIParamRef>(state);
	}

	void FRendererInterfaceD3D11::clearUAVs(DispatchState state)
	{
		FComputeShaderRHIParamRef ComputeShader = (FComputeShaderRHIParamRef)state.shader;

		for (uint32 n = 0; n < state.textureBindingCount; n++)
		{
			const auto& binding = state.textures[n];

			if (binding.isWritable)
			{
				check(binding.slot < 8);

				m_RHICmdList->SetUAVParameter(ComputeShader, binding.slot, nullptr);
			}
		}

		for (uint32 n = 0; n < state.bufferBindingCount; n++)
		{
			const auto& binding = state.buffers[n];

			if (binding.isWritable)
			{
				check(binding.slot < 8);

				m_RHICmdList->SetUAVParameter(ComputeShader, binding.slot, nullptr);
			}
		}
	}

	void FRendererInterfaceD3D11::setRHICommandList(FRHICommandList& RHICmdList)
	{
		m_RHICmdList = &RHICmdList;
	}

	void FRendererInterfaceD3D11::beginSection(const char* pSectionName)
	{
#if WITH_DX_PERF
		D3DPERF_BeginEvent(FColor(0, 0, 0).DWColor(), ANSI_TO_TCHAR(pSectionName));
#endif
	}

	void FRendererInterfaceD3D11::endSection()
	{
#if WITH_DX_PERF
		D3DPERF_EndEvent();
#endif
	}
}
