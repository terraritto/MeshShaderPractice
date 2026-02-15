#pragma once
#include <vector>
#include <string>
#include <d3dcompiler.h>
#include <d3d12shader.h>
#include <DirectXMath.h>
#include <dxcapi.h>
#include <wrl.h>
#include <d3d12.h>
#include <dxgi1_6.h>

// linker
#pragma comment( lib, "d3d12.lib" )
#pragma comment( lib, "dxgi.lib" )
#pragma comment( lib, "dxguid.lib" )
#pragma comment( lib, "winmm.lib" )
#pragma comment( lib, "comctl32.lib" )
#pragma comment( lib, "dxcompiler.lib")
#pragma comment( lib, "d3dcompiler.lib" )

// using
template<class T>
using ComPtr = Microsoft::WRL::ComPtr<T>;

using XMVECTOR = DirectX::XMVECTOR;
using XMFLOAT2 = DirectX::XMFLOAT2;
using XMFLOAT3 = DirectX::XMFLOAT3;
using XMFLOAT4 = DirectX::XMFLOAT4;
using XMFLOAT3X4 = DirectX::XMFLOAT3X4;
using XMFLOAT4X4 = DirectX::XMFLOAT4X4;
using XMMATIX = DirectX::XMMATRIX;

// SRGB -> Non-SRGB
DXGI_FORMAT GetNoSRGBFormat(const DXGI_FORMAT value);

// Non-SRGB -> SRGB
DXGI_FORMAT GetSRGBFormat(const DXGI_FORMAT value);

// Depth -> Resource
DXGI_FORMAT GetResourceFormat(const DXGI_FORMAT value, bool isStencil);

// SRGB?
bool IsSRGBFormat(DXGI_FORMAT value);

// for mesh shader
/*
#pragma warning(push)
#pragma warning(disable : 4324)
template<typename InnerStructType, D3D12_PIPELINE_STATE_SUBOBJECT_TYPE Type, typename DefaultArg = InnerStructType>
class alignas(void*) PSSubObject
{
public:
    PSSubObject() noexcept
        : m_Type(Type)
        , m_Inner(DefaultArg())
    {
    }

    PSSubObject(InnerStructType const& value) noexcept
        : m_Type(Type)
        , m_Inner(value)
    {
    }

    PSSubObject& operator = (InnerStructType const& value) noexcept
    {
        m_Type = Type;
        m_Inner = value;
        return *this;
    }

    operator InnerStructType const& () const noexcept
    {
        return m_Inner;
    }

    operator InnerStructType& () noexcept
    {
        return m_Inner;
    }

    InnerStructType* operator&() noexcept
    {
        return &m_Inner;
    }

    InnerStructType const* operator&() const noexcept
    {
        return &m_Inner;
    }

private:
    D3D12_PIPELINE_STATE_SUBOBJECT_TYPE m_Type;
    InnerStructType                     m_Inner;
};
#pragma warning(pop)

using PSS_ROOT_SIGNATURE = PSSubObject< ID3D12RootSignature*, D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_ROOT_SIGNATURE >;
using PSS_AS = PSSubObject< D3D12_SHADER_BYTECODE, D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_AS >;
using PSS_MS = PSSubObject< D3D12_SHADER_BYTECODE, D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_MS >;
using PSS_PS = PSSubObject< D3D12_SHADER_BYTECODE, D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_PS >;
using PSS_BLEND = PSSubObject< D3D12_BLEND_DESC, D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_BLEND >;
using PSS_SAMPLE_MASK = PSSubObject< UINT, D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_SAMPLE_MASK >;
using PSS_RASTERIZER = PSSubObject< D3D12_RASTERIZER_DESC, D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_RASTERIZER >;
using PSS_DEPTH_STENCIL = PSSubObject< D3D12_DEPTH_STENCIL_DESC, D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_DEPTH_STENCIL >;
using PSS_RTV_FORMATS = PSSubObject< D3D12_RT_FORMAT_ARRAY, D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_RENDER_TARGET_FORMATS >;
using PSS_DSV_FORMAT = PSSubObject< DXGI_FORMAT, D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_DEPTH_STENCIL_FORMAT >;
using PSS_SAMPLE_DESC = PSSubObject< DXGI_SAMPLE_DESC, D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_SAMPLE_DESC >;
using PSS_NODE_MASK = PSSubObject< UINT, D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_NODE_MASK >;
using PSS_CACHED_PSO = PSSubObject< D3D12_CACHED_PIPELINE_STATE, D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_CACHED_PSO >;
using PSS_FLAGS = PSSubObject< D3D12_PIPELINE_STATE_FLAGS, D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_FLAGS >;

// MeshShader—p‚ÌPS
struct GEOMETRY_PIPELINE_STATE_DESC
{
    PSS_ROOT_SIGNATURE      RootSignature;
    PSS_AS                  AS;
    PSS_MS                  MS;
    PSS_PS                  PS;
    PSS_BLEND               BlendState;
    PSS_SAMPLE_MASK         SampleMask;
    PSS_RASTERIZER          RasterizerState;
    PSS_DEPTH_STENCIL       DepthStencilState;
    PSS_RTV_FORMATS         RTVFormats;
    PSS_DSV_FORMAT          DSVFormat;
    PSS_SAMPLE_DESC         SampleDesc;
    PSS_NODE_MASK           NodeMask;
    PSS_FLAGS               Flags;
};
*/