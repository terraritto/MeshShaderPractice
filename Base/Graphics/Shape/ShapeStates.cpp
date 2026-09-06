#include "ShapeStates.h"
#include "MeshShaderPractice/Base/Graphics/GraphicsProxy.h"
#include "MeshShaderPractice/Base/Graphics/PresetState.h"

struct alignas(256) CameraParameter
{
	XMMATRIX m_view;
	XMMATRIX m_proj;
};

ShapeStates::ShapeStates()
	: m_bufferIndex(0)
	, m_view()
	, m_proj()
{
}

ShapeStates::~ShapeStates()
{
	Terminate();
}

bool ShapeStates::Initialize(DXGI_FORMAT colorFormat, DXGI_FORMAT depthFormat)
{
	auto device = GraphicsProxy::GetD3D12Device();

	if (device == nullptr) { return false; }
	
	// root signature
	{
		std::vector<D3D12_ROOT_PARAMETER> params;
		params.push_back(PARAM_CBV{ D3D12_SHADER_VISIBILITY_ALL, 0 });
		params.push_back(PARAM_CBV{ D3D12_SHADER_VISIBILITY_ALL, 1 });
		params.push_back(PARAM_CBV{ D3D12_SHADER_VISIBILITY_ALL, 2 });
		params.push_back(PARAM_CONSTANT{ D3D12_SHADER_VISIBILITY_ALL, 4, 3 });
		params.push_back(PARAM_SRV{ D3D12_SHADER_VISIBILITY_ALL, 0 });
		params.push_back(PARAM_SRV{ D3D12_SHADER_VISIBILITY_ALL, 1 });

		D3D12_ROOT_SIGNATURE_DESC desc;
		desc.NumParameters = static_cast<UINT>(params.size());
		desc.pParameters = params.data();
		desc.NumStaticSamplers = _countof(PresetState::m_staticSamplers);
		desc.pStaticSamplers = PresetState::m_staticSamplers;
		desc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

		if (m_rootSignature.Initialize(device, &desc) == false)
		{
			return false;
		}
	}

    // input layout
    D3D12_INPUT_ELEMENT_DESC inputElements[] =
    {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
    };

    // opaque
    {
        // Shader
        m_opaqueState.SetReloadPathPS(L"Resource/Shader/ShapePS.hlsl");
        m_opaqueState.SetReloadPathVS(L"Resource/Shader/ShapeVS.hlsl");

        D3D12_GRAPHICS_PIPELINE_STATE_DESC desc = {};
        desc.pRootSignature = m_rootSignature.Get();
		desc.BlendState = PresetState::m_opaque;
		desc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;
		desc.RasterizerState = PresetState::m_cullNone;
        desc.DepthStencilState = PresetState::m_depthReadWrite;
        desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		desc.InputLayout = { inputElements, _countof(inputElements) };
        desc.NumRenderTargets = 1;
        desc.RTVFormats[0] = colorFormat;
        desc.DSVFormat = depthFormat;
        desc.SampleDesc.Count = 1;
		desc.SampleDesc.Quality = 0;

		if (!m_opaqueState.Initialize(&desc))
		{
			return false;
		}
    }

	// translucent
	{
		// Shader
		m_translucentState.SetReloadPathPS(L"Resource/Shader/ShapePS.hlsl");
		m_translucentState.SetReloadPathVS(L"Resource/Shader/ShapeVS.hlsl");

		D3D12_GRAPHICS_PIPELINE_STATE_DESC desc = {};
		desc.pRootSignature = m_rootSignature.Get();
		desc.BlendState = PresetState::m_alphaBlend;
		desc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;
		desc.RasterizerState = PresetState::m_cullNone;
		desc.DepthStencilState = PresetState::m_depthReadOnly;
		desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		desc.InputLayout = { inputElements, _countof(inputElements) };
		desc.NumRenderTargets = 1;
		desc.RTVFormats[0] = colorFormat;
		desc.DSVFormat = depthFormat;
		desc.SampleDesc.Count = 1;
		desc.SampleDesc.Quality = 0;

		if (!m_translucentState.Initialize(&desc))
		{
			return false;
		}
	}

	// wireframe
	{
		// Shader
		m_wireframeState.SetReloadPathPS(L"Resource/Shader/ShapePS.hlsl");
		m_wireframeState.SetReloadPathVS(L"Resource/Shader/ShapeVS.hlsl");

		D3D12_GRAPHICS_PIPELINE_STATE_DESC desc = {};
		desc.pRootSignature = m_rootSignature.Get();
		desc.BlendState = PresetState::m_opaque;
		desc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;
		desc.RasterizerState = PresetState::m_wireFrame;
		desc.DepthStencilState = PresetState::m_depthReadWrite;
		desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		desc.InputLayout = { inputElements, _countof(inputElements) };
		desc.NumRenderTargets = 1;
		desc.RTVFormats[0] = colorFormat;
		desc.DSVFormat = depthFormat;
		desc.SampleDesc.Count = 1;
		desc.SampleDesc.Quality = 0;

		if (!m_wireframeState.Initialize(&desc))
		{
			return false;
		}
	}

	// camera buffer
	{
		if (!m_cameraBuffer.Initialize(sizeof(CameraParameter) * 2))
		{
			return false;
		}
	}

	return true;
}

void ShapeStates::Terminate()
{
	m_cameraBuffer.Terminate();
	m_wireframeState.Terminate();
	m_translucentState.Terminate();
	m_opaqueState.Terminate();
	m_rootSignature.Terminate();
}

void ShapeStates::SetViewProj(const XMMATRIX& view, const XMMATRIX& proj)
{
	m_view = view;
	m_proj = proj;

	CameraParameter parameter;
	parameter.m_view = m_view;
	parameter.m_proj = m_proj;

	// Calculate memory start point
	auto size = sizeof(CameraParameter);
	auto offset = m_bufferIndex * size;

	// write
	uint8_t* data = nullptr;
	data = m_cameraBuffer.MapAs<uint8_t>();
	if (data == nullptr) { return; }
	memcpy(data + offset, &parameter, sizeof(parameter));

	// swap
	m_bufferIndex = (m_bufferIndex + 1) & 0x1;
}

D3D12_GPU_VIRTUAL_ADDRESS ShapeStates::GetBufferAddress() const
{
	auto size = sizeof(CameraParameter);
	auto offset = m_bufferIndex * size;
	return m_cameraBuffer.GetGpuAddress() + offset;
}

void ShapeStates::ApplyOpaqueState(ID3D12GraphicsCommandList* command)
{
	auto address = GetBufferAddress();
	command->SetGraphicsRootSignature(m_rootSignature.Get());
	command->SetPipelineState(m_opaqueState.Get());
	command->SetGraphicsRootConstantBufferView(0, address);
	command->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
}

void ShapeStates::ApplyTranslucentState(ID3D12GraphicsCommandList* command)
{
	auto address = GetBufferAddress();
	command->SetGraphicsRootSignature(m_rootSignature.Get());
	command->SetPipelineState(m_translucentState.Get());
	command->SetGraphicsRootConstantBufferView(0, address);
	command->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
}

void ShapeStates::ApplyWireframeState(ID3D12GraphicsCommandList* command)
{
	auto address = GetBufferAddress();
	command->SetGraphicsRootSignature(m_rootSignature.Get());
	command->SetPipelineState(m_wireframeState.Get());
	command->SetGraphicsRootConstantBufferView(0, address);
	command->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
}
