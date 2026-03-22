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
using XMMATRIX = DirectX::XMMATRIX;

static XMFLOAT2 Vec2Zero = XMFLOAT2(0.0f, 0.0f);
static XMVECTOR VecZero = DirectX::XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f);
static XMVECTOR VecUnitX = DirectX::XMVectorSet(1.0f, 0.0f, 0.0f, 1.0f);
static XMVECTOR VecUnitY = DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 1.0f);
static XMVECTOR VecUnitZ = DirectX::XMVectorSet(0.0f, 0.0f, 1.0f, 1.0f);
static XMMATRIX MatIdentity = DirectX::XMMatrixIdentity();

// SRGB -> Non-SRGB
DXGI_FORMAT GetNoSRGBFormat(const DXGI_FORMAT value);

// Non-SRGB -> SRGB
DXGI_FORMAT GetSRGBFormat(const DXGI_FORMAT value);

// Depth -> Resource
DXGI_FORMAT GetResourceFormat(const DXGI_FORMAT value, bool isStencil);

// SRGB?
bool IsSRGBFormat(DXGI_FORMAT value);
