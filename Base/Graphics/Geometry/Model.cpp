#include "Model.h"
#include "MeshShaderPractice/Base/Util/Logger.h"

Model::Model()
{
}

Model::~Model()
{
    Terminate();
}

bool Model::Initialize(ID3D12GraphicsCommandList* command, std::string& path)
{
    m_model.reset();
    m_model = std::make_shared<ResourceModel>();

    // Load Model
    if (!m_model->LoadMesh(path))
    {
        ELOGA("Error : Model Initialize Failed.");
        return false;
    }

    m_meshes.resize(m_model->GetMeshCount());
    for (auto i = 0u; i < m_model->GetMeshCount(); ++i)
    {
        std::shared_ptr<ResourceMesh> resource = m_model->GetMesh(i).lock();
        std::shared_ptr<Mesh> mesh = std::make_shared<Mesh>();
        if (!mesh->Initialize(command, *resource.get()))
        {
            ELOGA("Error : Mesh Initialize Failed. index = %zu", i);
            return false;
        }
        m_meshes[i] = mesh;
    }

    return true;
}

void Model::Terminate()
{
    for (size_t i = 0;i < m_meshes.size(); ++i)
    {
        m_meshes[i]->Terminate();
    }
    m_meshes.clear();
    m_model.reset();
}

uint32_t Model::GetMeshCount() const
{
    return static_cast<uint32_t>(m_meshes.size());
}

const std::weak_ptr<Mesh> Model::GetMesh(uint32_t index) const
{
    assert(index < m_meshes.size());
    return m_meshes[index];
}

const std::weak_ptr<ResourceMesh> Model::GetResourceMesh(uint32_t index) const
{
    return m_model->GetMesh(index);
}

const Bounds& Model::GetBounds() const
{
    return m_model->GetBounds();
}
