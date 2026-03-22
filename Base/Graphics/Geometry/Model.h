#pragma once
#include "MeshShaderPractice/Base/Graphics/Resource/ResourceModel.h"
#include "MeshShaderPractice/Base/Graphics/Geometry/Mesh.h"

class Model
{
public:
	Model();
	~Model();

	bool Initialize(ID3D12GraphicsCommandList* command, std::string& path);
	void Terminate();

	// Getter
	uint32_t GetMeshCount() const;
	const std::weak_ptr<Mesh> GetMesh(uint32_t index) const;
	const std::weak_ptr<ResourceMesh> GetResourceMesh(uint32_t index) const;
	const Bounds& GetBounds() const;

private:
	std::shared_ptr<ResourceModel> m_model;
	std::vector<std::shared_ptr<Mesh>> m_meshes;
};