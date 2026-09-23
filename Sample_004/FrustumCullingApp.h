#pragma once
#include "MeshShaderPractice/Base/App/AppBase.h"
#include "MeshShaderPractice/Base/Graphics/Buffer/ConstantBuffer.h"
#include "MeshShaderPractice/Base/Graphics/Shape/BoxShape.h"
#include "MeshShaderPractice/Base/Graphics/Shape/ShapeStates.h"
#include "MeshShaderPractice/Base/Graphics/Shape/SphereShape.h"
#include "MeshShaderPractice/Base/Graphics/Geometry/Model.h"
#include "MeshShaderPractice/Base/Graphics/RootSignature.h"
#include "MeshShaderPractice/Base/Graphics/PS/MeshShaderPipelineState.h"

class FrustumCullingApp : public AppBase
{
protected:
	// AppBase override Interface
	virtual bool OnInitialize() override;
	virtual bool OnTerminate() override;
	virtual void OnUpdate(double deltaTime) override;
	virtual void OnRender() override;

protected:
	// DX Base
	RootSignature m_rootSignature;
	MeshShaderPipelineState m_pso;
	std::weak_ptr<CommandQueue> m_graphicsQueue;
	WaitPoint m_frameWaitPoint;

	// Camera
	std::vector<ConstantBuffer> m_cameraCB;

	// Model
	Model m_model;

	// Transform
	ConstantBuffer m_transformBuffer;

	// instance
	std::vector<XMMATRIX> m_instances;
	StructuredBuffer m_instancesBuffer;

	// debug
	ShapeStates m_shapeStates;
	BoxShape m_frustumShape;
	XMVECTOR m_debugPlanes[6];
	bool m_isDebugFrustum = false;

	// imgui descriptor
	DescriptorHolder m_imguiHolder;

private:
	static constexpr uint32_t InstanceColsNum = 20;
	static constexpr uint32_t InstanceRowsNum = 10;
};