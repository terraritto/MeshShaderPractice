#pragma once
#include "MeshShaderPractice/Base/App/AppBase.h"
#include "MeshShaderPractice/Base/Graphics/Buffer/VertexBuffer.h"
#include "MeshShaderPractice/Base/Graphics/Buffer/IndexBuffer.h"
#include "MeshShaderPractice/Base/Graphics/RootSignature.h"
#include "MeshShaderPractice/Base/Graphics/PipelineState.h"

class SimpleMeshShaderApp : public AppBase
{
protected:
	// AppBase override Interface
	virtual bool OnInitialize() override;
	virtual bool OnTerminate() override;
	virtual void OnRender() override;

protected:
	// DX Base
	RootSignature m_rootSignature;
	MeshShaderPipelineState m_pso;
	std::weak_ptr<CommandQueue> m_graphicsQueue;
	WaitPoint m_frameWaitPoint;

	// Vertex
	VertexBuffer m_vertexBuffer;
	IndexBuffer m_indexBuffer;
};