#pragma once
#include <d3d12.h>
#include "MeshShaderPractice/Base/Util.h"

struct RANGE_CBV : public D3D12_DESCRIPTOR_RANGE
{
	RANGE_CBV(UINT baseRegister, UINT registerSpace = 0);
};

struct RANGE_SRV : public D3D12_DESCRIPTOR_RANGE
{
	RANGE_SRV(UINT baseRegister, UINT registerSpace = 0);
};

struct RANGE_UAV : public D3D12_DESCRIPTOR_RANGE
{
	RANGE_UAV(UINT baseRegister, UINT registerSpace = 0);
};

struct RANGE_SMP : public D3D12_DESCRIPTOR_RANGE
{
	RANGE_SMP(UINT baseRegister, UINT registerSpace = 0);
};

struct PARAM_TABLE : public D3D12_ROOT_PARAMETER
{
	PARAM_TABLE
	(
		D3D12_SHADER_VISIBILITY visibility,
		UINT count,
		const D3D12_DESCRIPTOR_RANGE* ranges
	);
};

struct PARAM_CONSTANT : public D3D12_ROOT_PARAMETER
{
	PARAM_CONSTANT
	(
		D3D12_SHADER_VISIBILITY visibility,
		UINT count,
		UINT baseRegister,
		UINT registerSpace = 0
	);
};

struct PARAM_CBV : public D3D12_ROOT_PARAMETER
{
	PARAM_CBV
	(
		D3D12_SHADER_VISIBILITY visibility,
		UINT baseRegister,
		UINT registerSpace = 0
	);
};

struct PARAM_SRV : public D3D12_ROOT_PARAMETER
{
	PARAM_SRV
	(
		D3D12_SHADER_VISIBILITY visibility,
		UINT baseRegister,
		UINT registerSpace = 0
	);
};

struct PARAM_UAV : public D3D12_ROOT_PARAMETER
{
	PARAM_UAV
	(
		D3D12_SHADER_VISIBILITY visibility,
		UINT baseRegister,
		UINT registerSpace = 0
	);
};

class RootSignature
{
public:
	RootSignature();
	~RootSignature();

	bool Initialize(ID3D12Device* device, const D3D12_ROOT_SIGNATURE_DESC* desc);
	void Terminate();

	// Getter
	ID3D12RootSignature* Get() const;

private:
	ComPtr<ID3D12RootSignature> m_rootSignature;
};
