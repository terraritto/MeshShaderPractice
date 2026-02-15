#include "RootSignature.h"
#include <cassert>
#include "MeshShaderPractice/Base/Util/Logger.h"

RANGE_CBV::RANGE_CBV(UINT baseRegister, UINT registerSpace)
{
	RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
	NumDescriptors = 1;
	BaseShaderRegister = baseRegister;
	RegisterSpace = registerSpace;
	OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
}

RANGE_SRV::RANGE_SRV(UINT baseRegister, UINT registerSpace)
{
	RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	NumDescriptors = 1;
	BaseShaderRegister = baseRegister;
	RegisterSpace = registerSpace;
	OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
}

RANGE_UAV::RANGE_UAV(UINT baseRegister, UINT registerSpace)
{
	RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
	NumDescriptors = 1;
	BaseShaderRegister = baseRegister;
	RegisterSpace = registerSpace;
	OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
}

RANGE_SMP::RANGE_SMP(UINT baseRegister, UINT registerSpace)
{
	RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER;
	NumDescriptors = 1;
	BaseShaderRegister = baseRegister;
	RegisterSpace = registerSpace;
	OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
}

PARAM_TABLE::PARAM_TABLE(D3D12_SHADER_VISIBILITY visibility, UINT count, const D3D12_DESCRIPTOR_RANGE* ranges)
{
	ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	DescriptorTable.NumDescriptorRanges = count;
	DescriptorTable.pDescriptorRanges = ranges;
	ShaderVisibility = visibility;
}

PARAM_CONSTANT::PARAM_CONSTANT(D3D12_SHADER_VISIBILITY visibility, UINT count, UINT baseRegister, UINT registerSpace)
{
	ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
	Constants.Num32BitValues = count;
	Constants.ShaderRegister = baseRegister;
	Constants.RegisterSpace = registerSpace;
	ShaderVisibility = visibility;
}

PARAM_CBV::PARAM_CBV(D3D12_SHADER_VISIBILITY visibility, UINT baseRegister, UINT registerSpace)
{
	ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	Descriptor.ShaderRegister = baseRegister;
	Descriptor.RegisterSpace = registerSpace;
	ShaderVisibility = visibility;
}

PARAM_SRV::PARAM_SRV(D3D12_SHADER_VISIBILITY visibility, UINT baseRegister, UINT registerSpace)
{
	ParameterType = D3D12_ROOT_PARAMETER_TYPE_SRV;
	Descriptor.ShaderRegister = baseRegister;
	Descriptor.RegisterSpace = registerSpace;
	ShaderVisibility = visibility;
}

PARAM_UAV::PARAM_UAV(D3D12_SHADER_VISIBILITY visibility, UINT baseRegister, UINT registerSpace)
{
	ParameterType = D3D12_ROOT_PARAMETER_TYPE_UAV;
	Descriptor.ShaderRegister = baseRegister;
	Descriptor.RegisterSpace = registerSpace;
	ShaderVisibility = visibility;
}

RootSignature::RootSignature()
{
}

RootSignature::~RootSignature()
{
	Terminate();
}

bool RootSignature::Initialize(ID3D12Device* device, const D3D12_ROOT_SIGNATURE_DESC* desc)
{
	ComPtr<ID3DBlob> blob;
	ComPtr<ID3DBlob> errorBlob;

	// Serialize
	HRESULT hr = D3D12SerializeRootSignature
	(
		desc,
		D3D_ROOT_SIGNATURE_VERSION_1_0,
		blob.GetAddressOf(),
		errorBlob.GetAddressOf()
	);
	if (FAILED(hr))
	{
		const char* msg = "";
		if (errorBlob != nullptr)
		{
			msg = static_cast<const char*>(errorBlob->GetBufferPointer());
		}

		ELOGA("Error: D3D12SerializeRootSignature() Failed. errcode=0x%x msg=%s", hr, msg);
		return false;
	}

	// Create
	hr = device->CreateRootSignature
	(
		0,
		blob->GetBufferPointer(),
		blob->GetBufferSize(),
		IID_PPV_ARGS(m_rootSignature.GetAddressOf())
	);
	if (FAILED(hr))
	{
		ELOGA("Error : ID3D12Device::CreateRootSignature() Failed. errcode = 0x%x", hr);
		return false;
	}

	return true;
}

void RootSignature::Terminate()
{
	m_rootSignature.Reset();
}

ID3D12RootSignature* RootSignature::Get() const
{
	return m_rootSignature.Get();
}
