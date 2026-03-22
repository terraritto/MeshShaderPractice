#pragma once
#include <assimp/scene.h>
#include <memory>
#include <string>
#include <vector>
#include <DirectXMesh.h>
#include "MeshShaderPractice/Base/Graphics/Resource/ResourceMesh.h"
#include "MeshShaderPractice/Base/Graphics/Resource/Bounds.h"

// Hold pure Model resource
class ResourceModel
{
public:
	ResourceModel();
	~ResourceModel();

	bool LoadMesh(std::string& path);
	void Terminate();

	// Getter
	uint32_t GetMeshCount() const;
	std::weak_ptr<ResourceMesh> GetMesh(uint32_t index) const;
	const Bounds& GetBounds() const;

protected:
	void ParseMesh(const aiScene* scene);

	// Meshlet Series
	void ConstructMeshletFromOptimizer(ResourceMesh* mesh);
#ifdef DIRECTX_MESH_API
	void ConstructMeshletFromDirectX(ResourceMesh* mesh);
#endif 
	void ConstructMeshletGreedy(ResourceMesh* mesh);
	void ConstructMeshletBoundingSphere(ResourceMesh* mesh);

protected:
	std::vector<std::shared_ptr<ResourceMesh>> m_meshes;

	Bounds m_bounds;

private:
	// Max num of vertex and triangle
	static constexpr size_t MAX_VERTICES = 64;
	static constexpr size_t MAX_TRIANGLES = 124;
};