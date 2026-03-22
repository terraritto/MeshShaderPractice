#include "ResourceModel.h"
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <queue>
#include "MeshShaderPractice/Base/Util/Logger.h"
#include "MeshShaderPractice/Base/Graphics/GraphicsProxy.h"
#include "MeshShaderPractice/External/MeshOptimizer/src/meshoptimizer.h"

ResourceModel::ResourceModel()
{
}

ResourceModel::~ResourceModel()
{
    Terminate();
}

bool ResourceModel::LoadMesh(std::string& path)
{
    if (path.empty()) { return false; }

    Assimp::Importer importer;
    int flag = 0;
    flag |= aiProcess_Triangulate;              // force triangle.
    flag |= aiProcess_PreTransformVertices;     // node simplify(if use animation, this flag must be off.)
    flag |= aiProcess_CalcTangentSpace;         // enable Tangent Space.
    flag |= aiProcess_GenSmoothNormals;         // smooth normal
    flag |= aiProcess_GenUVCoords;              // Make UV
    flag |= aiProcess_FlipUVs;                  // UV flip by Y-axis
    flag |= aiProcess_RemoveRedundantMaterials; // remove non-referenced material
    flag |= aiProcess_JoinIdenticalVertices;

    const aiScene* scene = importer.ReadFile(path, flag);
    if (scene == nullptr)
    {
        ELOGA("ELLOR : Assimp::Importer::ReadFile Failed.");
        return false;
    }

    ParseMesh(scene);

    return true;
}

void ResourceModel::Terminate()
{
    m_meshes.clear();
}

uint32_t ResourceModel::GetMeshCount() const
{
    return static_cast<uint32_t>(m_meshes.size());
}

std::weak_ptr<ResourceMesh> ResourceModel::GetMesh(uint32_t index) const
{
    return m_meshes[index];
}

const Bounds& ResourceModel::GetBounds() const
{
    return m_bounds;
}

void ResourceModel::ParseMesh(const aiScene* scene)
{
    m_meshes.clear(); m_meshes.resize(scene->mNumMeshes, nullptr);

    // for bounds
    XMFLOAT3 minValue; 
    minValue.x = (std::numeric_limits<float>::max)();
    minValue.y = minValue.x; minValue.z = minValue.x;
    XMFLOAT3 maxValue; 
    maxValue.x = std::numeric_limits<float>::lowest();
    maxValue.y = maxValue.x; maxValue.z = maxValue.x;

    for (size_t k = 0; k < scene->mNumMeshes; ++k)
    {
        const aiMesh* mesh = scene->mMeshes[k];
        std::shared_ptr<ResourceMesh> resourceMesh = std::make_shared<ResourceMesh>();
      
        // alloc
        resourceMesh->m_positions.resize(mesh->mNumVertices);

        // push vertex data
        for (unsigned int i = 0; i < mesh->mNumVertices; ++i)
        {
            auto& position = mesh->mVertices[i];
            resourceMesh->m_positions[i] = XMFLOAT3{ position.x, position.y, position.z };

            // update bounds
            minValue.x = min(minValue.x, position.x);
            minValue.y = min(minValue.y, position.y);
            minValue.z = min(minValue.z, position.z);
            maxValue.x = max(maxValue.x, position.x);
            maxValue.y = max(maxValue.y, position.y);
            maxValue.z = max(maxValue.z, position.z);
        }

        // push index data
        for (unsigned int i = 0; i < mesh->mNumFaces; ++i)
        {
            auto& face = mesh->mFaces[i];
            for (unsigned int j = 0; j < face.mNumIndices; j++)
            {
                resourceMesh->m_indices.push_back(face.mIndices[j]);
            }
        }

        resourceMesh->m_name = mesh->mName.C_Str();

        if (GraphicsProxy::IsUseMeshlet())
        {
            ConstructMeshletFromDirectX(resourceMesh.get());
        }

        m_meshes[k] = resourceMesh;
    }

    // Initialize Bounds.
    m_bounds.SetValue(minValue, maxValue);
}

void ResourceModel::ConstructMeshletFromOptimizer(ResourceMesh* mesh)
{
    // weight is constant
    constexpr float CONE_WEIGHT = 0.0f;

    // Calculate max num of meshlet
    const size_t maxMeshlets = 
        meshopt_buildMeshletsBound(
            mesh->GetIndicesNum(), 
            MAX_VERTICES, 
            MAX_TRIANGLES);
    
    // alloc max size
    std::vector<meshopt_Meshlet> meshlets; meshlets.resize(maxMeshlets);
    mesh->m_uniqueVertexIndices.resize(maxMeshlets * MAX_VERTICES);
    std::vector<uint8_t> meshletTriangles;
    meshletTriangles.resize(maxMeshlets * MAX_TRIANGLES * 3);

    // construct meshlet
    size_t meshletCount = meshopt_buildMeshlets
    (
        meshlets.data(),                                            // Output: meshopt_Meshlet
        mesh->m_uniqueVertexIndices.data(),                         // Output: meshlet to mesh index mapping
        meshletTriangles.data(),                                    // Output: triangle indices
        mesh->m_indices.data(),                                     // Input:  pointer indices
        mesh->GetIndicesNum(),                                      // Input:  number of indices
        reinterpret_cast<const float*>(mesh->m_positions.data()),   // Input:  pointer vertices
        mesh->GetVerticesNum(),                                     // Input:  number of vertices
        sizeof(XMFLOAT3),                                           // Input:  stride of vertices
        MAX_VERTICES,                                               // Input:  maximum number of vertices per meshlet
        MAX_TRIANGLES,                                              // Input:  maximum number of triangles per mehslet
        CONE_WEIGHT                                                 // Input:  cone weight
    );

    // shrink vector size using meshlet size.
    // triangle align to equal (x mod 4 = 0).
    meshopt_Meshlet& last = meshlets[meshletCount - 1];
    mesh->m_uniqueVertexIndices.resize(last.vertex_offset + last.vertex_count);
    meshletTriangles.resize(last.triangle_offset + ((last.triangle_count * 3 + 3) & ~3));
    meshlets.resize(meshletCount);

    // repack triangles from uint8 to uint32
    for (meshopt_Meshlet& meshlet : meshlets)
    {
        // Current triangle offset for current meshlet
        uint32_t triangleOffset = static_cast<uint32_t>(mesh->m_primitiveIndices.size());

        // per triangle
        for (uint32_t i = 0; i < meshlet.triangle_count; ++i)
        {
            // pick up offset
            const uint32_t index = i * 3;
            uint32_t i0 = index + meshlet.triangle_offset;
            uint32_t i1 = index + 1 + meshlet.triangle_offset;
            uint32_t i2 = index + 2 + meshlet.triangle_offset;

            uint8_t vi0 = meshletTriangles[i0];
            uint8_t vi1 = meshletTriangles[i1];
            uint8_t vi2 = meshletTriangles[i2];

            // repack!!
            uint32_t packed = 
                ((static_cast<uint32_t>(vi0) & 0xFF) << 0) |
                ((static_cast<uint32_t>(vi1) & 0xFF) << 8) |
                ((static_cast<uint32_t>(vi2) & 0xFF) << 16);

            mesh->m_primitiveIndices.push_back(packed);
        }

        // adjust offset
        meshlet.triangle_offset = triangleOffset;
    }

    // set meshlet
    for (const meshopt_Meshlet& meshlet : meshlets)
    {
        ResourceMeshlet meshletResource;
        meshletResource.m_vertexOffset = meshlet.vertex_offset;
        meshletResource.m_vertexCount = meshlet.vertex_count;
        meshletResource.m_primitiveOffset = meshlet.triangle_offset;
        meshletResource.m_primitiveCount = meshlet.triangle_count;
        mesh->m_meshlets.push_back(meshletResource);
    }
}

#ifdef DIRECTX_MESH_API
void ResourceModel::ConstructMeshletFromDirectX(ResourceMesh* mesh)
{
    // use data
    std::vector<DirectX::Meshlet> meshlets;
    std::vector<DirectX::MeshletTriangle> meshletTriangles;
    std::vector<uint8_t> uniqueVertexIB;

    // construct meshlet
    HRESULT hr = DirectX::ComputeMeshlets
    (
        mesh->m_indices.data(),             // Input: pointer indices
        mesh->GetIndicesNum() / 3,          // Input: number of triangles
        mesh->m_positions.data(),           // Input: pointer vertex positions
        mesh->m_positions.size(),           // Input: number of vertex
        nullptr,                            // Input: adjacency
        meshlets,                           // Output: DirectX::Meshlet
        uniqueVertexIB,                     // Output: meshlet to mesh index mapping
        meshletTriangles,                   // Output: DirectX::MeshletTriangle
        MAX_VERTICES,                       // Input: maximum number of vertices per meshlet
        MAX_TRIANGLES                       // Input: maximum number of triangles per meshlet
        );

    if (FAILED(hr))
    {
        ELOGA("Error : DirectX::ComputeMeshlets() Failed.");
        return;
    }

    size_t meshletCount = meshlets.size();

    // shrink vector size using meshlet size.
    // triangle align to equal (x mod 4 = 0).
    DirectX::Meshlet& last = meshlets[meshletCount - 1];
    mesh->m_uniqueVertexIndices.resize(last.VertOffset + last.VertCount);
    meshletTriangles.resize(last.PrimOffset + ((last.PrimCount * 3 + 3) & ~3));
    meshlets.resize(meshletCount);

    // repack triangles from uint8 to uint32
    for (DirectX::Meshlet& meshlet : meshlets)
    {
        // Current triangle offset for current meshlet
        uint32_t triangleOffset = static_cast<uint32_t>(mesh->m_primitiveIndices.size());

        // per triangle
        for (uint32_t i = 0; i < meshlet.PrimCount; ++i)
        {
            // pick up offset
            const uint32_t index = i + meshlet.PrimOffset;

            uint8_t vi0 = meshletTriangles[index].i0;
            uint8_t vi1 = meshletTriangles[index].i1;
            uint8_t vi2 = meshletTriangles[index].i2;

            // repack!!
            uint32_t packed =
                ((static_cast<uint32_t>(vi0) & 0xFF) << 0) |
                ((static_cast<uint32_t>(vi1) & 0xFF) << 8) |
                ((static_cast<uint32_t>(vi2) & 0xFF) << 16);

            mesh->m_primitiveIndices.push_back(packed);
        }

        // adjust offset
        meshlet.PrimOffset = triangleOffset;
    }

    // remap unique vertex indices data.
    memcpy(mesh->m_uniqueVertexIndices.data(), uniqueVertexIB.data(), sizeof(uint8_t) * uniqueVertexIB.size());

    // set meshlet
    for (const DirectX::Meshlet& meshlet : meshlets)
    {
        ResourceMeshlet meshletResource;
        meshletResource.m_vertexOffset = meshlet.VertOffset;
        meshletResource.m_vertexCount = meshlet.VertCount;
        meshletResource.m_primitiveOffset = meshlet.PrimOffset;
        meshletResource.m_primitiveCount = meshlet.PrimCount;
        mesh->m_meshlets.push_back(meshletResource);
    }
}
#endif

#pragma region GreedyImpl
void ResourceModel::ConstructMeshletGreedy(ResourceMesh* mesh)
{
    // Calculate Structure
    struct Vertex;
    struct Triangle
    {
        std::array<Vertex*, 3> m_vertices;
        XMFLOAT3 m_centroid; //for sort
        bool m_isVisited = false;
    };

    struct Vertex
    {
        XMFLOAT3 m_position;
        uint32_t m_index;
        std::vector<Triangle*> m_neighbors;
        bool m_isVisited = false;
    };

    // sort function
    int sortIndex = 0;
    auto CompareTriangles = [sortIndex](const Triangle* t1, const Triangle* t2)
        {
            if (sortIndex == 1) { return t1->m_centroid.y < t2->m_centroid.y; }
            else if (sortIndex == 2) { return t1->m_centroid.z < t2->m_centroid.z; }
            return t1->m_centroid.x < t2->m_centroid.x;
        };
    auto CompareVertices = [sortIndex](const Vertex* v1, const Vertex* v2)
        {
            if (sortIndex == 1) { return v1->m_position.y < v2->m_position.y; }
            else if (sortIndex == 2) { return v1->m_position.z < v2->m_position.z; }
            return v1->m_position.x < v2->m_position.x;
        };

    // BBox
    XMFLOAT3 minValue; minValue.x = (std::numeric_limits<float>::max)();
    minValue.y = minValue.x; minValue.z = minValue.x;
    XMFLOAT3 maxValue; maxValue.x = std::numeric_limits<float>::lowest();
    maxValue.y = maxValue.x; maxValue.z = maxValue.x;

    // Vertices
    std::vector<Vertex> pureVertices; pureVertices.resize(mesh->GetVerticesNum());
    std::vector<Vertex*> vertices; vertices.resize(mesh->GetVerticesNum());

    // Construct Vertices and make BBox
    for (auto i = 0u; i < vertices.size(); ++i)
    {
        pureVertices[i].m_position = mesh->m_positions[i];
        pureVertices[i].m_index = i;
        vertices[i] = &pureVertices.at(i);
        
        minValue.x = min(minValue.x, vertices[i]->m_position.x);
        maxValue.x = max(maxValue.x, vertices[i]->m_position.x);
        minValue.y = min(minValue.y, vertices[i]->m_position.y);
        maxValue.y = max(maxValue.y, vertices[i]->m_position.y);
        minValue.z = min(minValue.z, vertices[i]->m_position.z);
        maxValue.z = max(maxValue.z, vertices[i]->m_position.z);
    }

    // construct triangles
    std::vector<Triangle> pureTriangles; pureTriangles.resize(mesh->GetIndicesNum() / 3);
    std::vector<Triangle*> triangles; triangles.resize(mesh->GetIndicesNum() / 3);
    {
        for (auto i = 0u; i < (mesh->GetIndicesNum() / 3); ++i)
        {
            uint32_t index = i * 3;
            Triangle* triangle = &pureTriangles.at(i);

            // register vertices
            triangle->m_vertices[0] = &pureVertices.at(mesh->m_indices[index]);
            triangle->m_vertices[1] = &pureVertices.at(mesh->m_indices[index + 1]);
            triangle->m_vertices[2] = &pureVertices.at(mesh->m_indices[index + 2]);

            // Register neighbor
            Vertex* vert1 = triangle->m_vertices[0];
            Vertex* vert2 = triangle->m_vertices[1];
            Vertex* vert3 = triangle->m_vertices[2];

            vert1->m_neighbors.push_back(triangle);
            vert2->m_neighbors.push_back(triangle);
            vert3->m_neighbors.push_back(triangle);
            
            // calculate simple average pos
            triangle->m_centroid.x = (vert1->m_position.x + vert2->m_position.x + vert3->m_position.x) / 3.0f;
            triangle->m_centroid.y = (vert1->m_position.y + vert2->m_position.y + vert3->m_position.y) / 3.0f;
            triangle->m_centroid.z = (vert1->m_position.z + vert2->m_position.z + vert3->m_position.z) / 3.0f;

            triangles[i] = triangle;
        }
    }

    // calculate length per axis
    XMFLOAT3 axis;
    axis.x = abs(maxValue.x - minValue.x);
    axis.y = abs(maxValue.y - minValue.y);
    axis.z = abs(maxValue.z - minValue.z);

    // sort by longest axis length!!
    if (axis.x > axis.y && axis.x > axis.z)
    {
        sortIndex = 0; // X
    }
    else if (axis.y > axis.z && axis.y > axis.x)
    {
        sortIndex = 1; // Y
    }
    else
    {
        sortIndex = 2; // Z
    }
    std::sort(vertices.begin(), vertices.end(), CompareVertices);
    std::sort(triangles.begin(), triangles.end(), CompareTriangles);

    // construct meshlet
    {
        std::queue<Vertex*> verticesQueue;
        std::vector<Vertex*> tempVertices;
        std::vector<Triangle*> tempTriangles;

        auto Construct = [&]()
            {
                // calculate prev start point.
                ResourceMeshlet meshlet;
                meshlet.m_vertexOffset = mesh->m_uniqueVertexIndices.size();
                meshlet.m_primitiveOffset = mesh->m_primitiveIndices.size();

                // vertex or triangle is filled.
                // so start to construct meshlet data.

                // Add Vertex
                for (const Vertex* vertex : tempVertices)
                {
                    mesh->m_uniqueVertexIndices.push_back(vertex->m_index);
                }

                // add Primitive
                for (const Triangle* triangle : tempTriangles)
                {
                    auto vertexIter = std::find(tempVertices.begin(), tempVertices.end(), triangle->m_vertices[0]);
                    uint32_t vi0 = std::distance(tempVertices.begin(), vertexIter);
                    vertexIter = std::find(tempVertices.begin(), tempVertices.end(), triangle->m_vertices[1]);
                    uint32_t vi1 = std::distance(tempVertices.begin(), vertexIter);
                    vertexIter = std::find(tempVertices.begin(), tempVertices.end(), triangle->m_vertices[2]);
                    uint32_t vi2 = std::distance(tempVertices.begin(), vertexIter);

                    uint32_t packed =
                        ((static_cast<uint32_t>(vi0) & 0xFF) << 0) |
                        ((static_cast<uint32_t>(vi1) & 0xFF) << 8) |
                        ((static_cast<uint32_t>(vi2) & 0xFF) << 16);

                    mesh->m_primitiveIndices.push_back(packed);
                }

                // calculate size and store it.
                meshlet.m_vertexCount = tempVertices.size();
                meshlet.m_primitiveCount = tempTriangles.size();
                mesh->m_meshlets.push_back(meshlet);
            };

        for (Vertex* vertex : vertices)
        {
            if (vertex->m_isVisited) { continue; }

            // push vertex
            verticesQueue.push(vertex);

            while (!verticesQueue.empty())
            {
                // Get head data
                Vertex* currentVertex = verticesQueue.front();
                verticesQueue.pop();

                if (currentVertex->m_isVisited) { continue; }

                // Search Triangle
                for (Triangle* triangle : currentVertex->m_neighbors)
                {
                    Vertex* vert1 = triangle->m_vertices[0];
                    Vertex* vert2 = triangle->m_vertices[1];
                    Vertex* vert3 = triangle->m_vertices[2];

                    // if not registered, register triangle.
                    if (triangle->m_isVisited)
                    {
                        // if registered, skip.
                        continue;
                    }

                    // if not searched, push queue.
                    if (vert1->m_isVisited == false) { verticesQueue.push(vert1); }
                    if (vert2->m_isVisited == false) { verticesQueue.push(vert2); }
                    if (vert3->m_isVisited == false) { verticesQueue.push(vert3); }

                    // hou much add vertex to temp vertex list?
                    bool isVert1 = std::find(tempVertices.begin(), tempVertices.end(), vert1) == tempVertices.end();
                    bool isVert2 = std::find(tempVertices.begin(), tempVertices.end(), vert2) == tempVertices.end();
                    bool isVert3 = std::find(tempVertices.begin(), tempVertices.end(), vert3) == tempVertices.end();
                    int mustAddVertexCount = 0;
                    if (isVert1) { mustAddVertexCount++; }
                    if (isVert2) { mustAddVertexCount++; }
                    if (isVert3) { mustAddVertexCount++; }

                    // is full triangle?
                    bool isAddVertex = (tempVertices.size() + mustAddVertexCount) < MAX_VERTICES;
                    bool isAddTriangle = (tempTriangles.size() + 1) < MAX_TRIANGLES;

                    if (!isAddVertex || !isAddTriangle)
                    {
                        Construct();

                        // init for next loop.
                        std::queue<Vertex*>().swap(verticesQueue);
                        verticesQueue.push(currentVertex);
                        tempVertices.clear();
                        tempTriangles.clear();

                        break;
                    }
                    else
                    {
                        // Add Vertex
                        if (isVert1) { tempVertices.push_back(vert1); }
                        if (isVert2) { tempVertices.push_back(vert2); }
                        if (isVert3) { tempVertices.push_back(vert3); }

                        // Add Triangle and flag on
                        tempTriangles.push_back(triangle);
                        triangle->m_isVisited = true;
                    }
                }

                // if all triangle is registered, vertex don't have to search.
                // so flag on.
                currentVertex->m_isVisited = true;
            }
        }

        // when loop finished and triangle remained, make meshlet.
        if (tempTriangles.size() > 0)
        {
            Construct();
        }
    }

    size_t meshletCount = mesh->m_meshlets.size();

    // shrink vector size using meshlet size.
    // triangle align to equal (x mod 4 = 0).
    ResourceMeshlet& last = mesh->m_meshlets[meshletCount - 1];
    mesh->m_primitiveIndices.resize(last.m_primitiveOffset + ((last.m_primitiveCount * 3 + 3) & ~3));
}
#pragma endregion

#pragma region BoundingSphereImpl
void ResourceModel::ConstructMeshletBoundingSphere(ResourceMesh* mesh)
{
    // Calculate Structure
    struct Vertex;
    struct Triangle
    {
        std::array<Vertex*, 3> m_vertices;
        XMFLOAT3 m_centroid; //for sort
        bool m_isVisited = false;
    };

    struct Vertex
    {
        XMFLOAT3 m_position;
        uint32_t m_index;
        std::vector<Triangle*> m_neighbors;
        bool m_isVisited = false;
    };

    auto LengthVec3 = [](XMFLOAT3& lhs, XMFLOAT3 rhs)
        {
            float x = lhs.x - rhs.x;
            float y = lhs.y - rhs.y;
            float z = lhs.z - rhs.z;
            return sqrt(x * x + y * y + z * z);
        };

    // sort function
    int sortIndex = 0;
    auto CompareTriangles = [sortIndex](const Triangle* t1, const Triangle* t2)
        {
            if (sortIndex == 1) { return t1->m_centroid.y < t2->m_centroid.y; }
            else if (sortIndex == 2) { return t1->m_centroid.z < t2->m_centroid.z; }
            return t1->m_centroid.x < t2->m_centroid.x;
        };
    auto CompareVertices = [sortIndex](const Vertex* v1, const Vertex* v2)
        {
            if (sortIndex == 1) { return v1->m_position.y < v2->m_position.y; }
            else if (sortIndex == 2) { return v1->m_position.z < v2->m_position.z; }
            return v1->m_position.x < v2->m_position.x;
        };

    // BBox
    XMFLOAT3 minValue; minValue.x = (std::numeric_limits<float>::max)();
    minValue.y = minValue.x; minValue.z = minValue.x;
    XMFLOAT3 maxValue; maxValue.x = std::numeric_limits<float>::lowest();
    maxValue.y = maxValue.x; maxValue.z = maxValue.x;

    // Vertices
    std::vector<Vertex> pureVertices; pureVertices.resize(mesh->GetVerticesNum());
    std::vector<Vertex*> vertices; vertices.resize(mesh->GetVerticesNum());

    // Construct Vertices and make BBox
    for (auto i = 0u; i < vertices.size(); ++i)
    {
        pureVertices[i].m_position = mesh->m_positions[i];
        pureVertices[i].m_index = i;
        vertices[i] = &pureVertices.at(i);

        minValue.x = min(minValue.x, vertices[i]->m_position.x);
        maxValue.x = max(maxValue.x, vertices[i]->m_position.x);
        minValue.y = min(minValue.y, vertices[i]->m_position.y);
        maxValue.y = max(maxValue.y, vertices[i]->m_position.y);
        minValue.z = min(minValue.z, vertices[i]->m_position.z);
        maxValue.z = max(maxValue.z, vertices[i]->m_position.z);
    }

    // construct triangles
    std::vector<Triangle> pureTriangles; pureTriangles.resize(mesh->GetIndicesNum() / 3);
    std::vector<Triangle*> triangles; triangles.resize(mesh->GetIndicesNum() / 3);
    {
        for (auto i = 0u; i < (mesh->GetIndicesNum() / 3); ++i)
        {
            uint32_t index = i * 3;
            Triangle* triangle = &pureTriangles.at(i);

            // register vertices
            triangle->m_vertices[0] = &pureVertices.at(mesh->m_indices[index]);
            triangle->m_vertices[1] = &pureVertices.at(mesh->m_indices[index + 1]);
            triangle->m_vertices[2] = &pureVertices.at(mesh->m_indices[index + 2]);

            // Register neighbor
            Vertex* vert1 = triangle->m_vertices[0];
            Vertex* vert2 = triangle->m_vertices[1];
            Vertex* vert3 = triangle->m_vertices[2];

            vert1->m_neighbors.push_back(triangle);
            vert2->m_neighbors.push_back(triangle);
            vert3->m_neighbors.push_back(triangle);

            // calculate simple average pos
            triangle->m_centroid.x = (vert1->m_position.x + vert2->m_position.x + vert3->m_position.x) / 3.0f;
            triangle->m_centroid.y = (vert1->m_position.y + vert2->m_position.y + vert3->m_position.y) / 3.0f;
            triangle->m_centroid.z = (vert1->m_position.z + vert2->m_position.z + vert3->m_position.z) / 3.0f;

            triangles[i] = triangle;
        }
    }

    // calculate length per axis
    XMFLOAT3 axis;
    axis.x = abs(maxValue.x - minValue.x);
    axis.y = abs(maxValue.y - minValue.y);
    axis.z = abs(maxValue.z - minValue.z);

    // sort by longest axis length!!
    if (axis.x > axis.y && axis.x > axis.z)
    {
        sortIndex = 0; // X
    }
    else if (axis.y > axis.z && axis.y > axis.x)
    {
        sortIndex = 1; // Y
    }
    else
    {
        sortIndex = 2; // Z
    }
    std::sort(vertices.begin(), vertices.end(), CompareVertices);
    std::sort(triangles.begin(), triangles.end(), CompareTriangles);

    // construct meshlet
    {
        std::vector<Vertex*> tempVertices;
        std::vector<Triangle*> tempTriangles;

        auto Construct = [&]()
            {
                // calculate prev start point.
                ResourceMeshlet meshlet;
                meshlet.m_vertexOffset = mesh->m_uniqueVertexIndices.size();
                meshlet.m_primitiveOffset = mesh->m_primitiveIndices.size();

                // vertex or triangle is filled.
                // so start to construct meshlet data.

                // Add Vertex
                for (const Vertex* vertex : tempVertices)
                {
                    mesh->m_uniqueVertexIndices.push_back(vertex->m_index);
                }

                // add Primitive
                for (const Triangle* triangle : tempTriangles)
                {
                    auto vertexIter = std::find(tempVertices.begin(), tempVertices.end(), triangle->m_vertices[0]);
                    uint32_t vi0 = std::distance(tempVertices.begin(), vertexIter);
                    vertexIter = std::find(tempVertices.begin(), tempVertices.end(), triangle->m_vertices[1]);
                    uint32_t vi1 = std::distance(tempVertices.begin(), vertexIter);
                    vertexIter = std::find(tempVertices.begin(), tempVertices.end(), triangle->m_vertices[2]);
                    uint32_t vi2 = std::distance(tempVertices.begin(), vertexIter);

                    uint32_t packed =
                        ((static_cast<uint32_t>(vi0) & 0xFF) << 0) |
                        ((static_cast<uint32_t>(vi1) & 0xFF) << 8) |
                        ((static_cast<uint32_t>(vi2) & 0xFF) << 16);

                    mesh->m_primitiveIndices.push_back(packed);
                }

                // calculate size and store it.
                meshlet.m_vertexCount = tempVertices.size();
                meshlet.m_primitiveCount = tempTriangles.size();
                mesh->m_meshlets.push_back(meshlet);
            };

        float radius = 0.0f;
        XMFLOAT3 center = XMFLOAT3(0.0f, 0.0f, 0.0f);

        for (auto i = 0u; i < vertices.size(); /* no cond */)
        {
            Vertex* vertex = vertices[i];

            Triangle* bestTriangle = nullptr;
            Vertex* newVertex = nullptr;
            float newRadius = (std::numeric_limits<float>::max)();
            float bestNewRadius = newRadius - 1.0f;
            int bestVertexScore = 0;

            for (Vertex* vertex : tempVertices)
            {
                for (Triangle* triangle : vertex->m_neighbors)
                {
                    if (triangle->m_isVisited) { continue; }

                    // Get info about triangle
                    int vertexScore = 0;
                    for (int k = 0; k < 3; ++k)
                    {
                        Vertex* target = triangle->m_vertices[k];
                        if (std::find(tempVertices.begin(), tempVertices.end(), target) == tempVertices.end())
                        {
                            // don't add meshlet yet.
                            newVertex = target;
                            continue;
                        }

                        // already add meshlet.
                        ++vertexScore;
                    }

                    // Search Triangle Neighbors
                    int used = 0;
                    int neighbors = 0;
                    for (Vertex* vertex : triangle->m_vertices)
                    {
                        for (Triangle* neighborTriangle : vertex->m_neighbors)
                        {
                            // not add self.
                            if (neighborTriangle == triangle) { continue; }

                            neighbors++;
                            if (neighborTriangle->m_isVisited) { used++; }
                        }
                    }

                    if (used == neighbors) { vertexScore++; }

                    // Update Radius
                    if (vertexScore == 3 || vertexScore == 4)
                    {
                        // Score == 3 means that 3 vertex is added to meshlet.
                        // Score == 4 means that 3 vertex is added to meshlet,
                        // but triangle isn't added meshlet. it's corner case.
                        newRadius = radius;
                    }
                    else if (vertexScore == 1)
                    {
                        continue;
                    }
                    else
                    {
                        newRadius = 0.5 * (radius + LengthVec3(center, newVertex->m_position));
                    }

                    // Update best
                    if (bestVertexScore < vertexScore || newRadius < bestNewRadius)
                    {
                        bestVertexScore = vertexScore;
                        bestNewRadius = newRadius;
                        bestTriangle = triangle;
                    }
                }
            }

            if (bestTriangle == nullptr)
            {
                for (Triangle* triangle : vertex->m_neighbors)
                {
                    if (triangle->m_isVisited) { continue; }

                    // Register Triangle
                    bestTriangle = triangle;
                    
                    // Update Parameter
                    center = triangle->m_centroid;
                    bestNewRadius = LengthVec3(center, triangle->m_vertices[0]->m_position);
                    bestNewRadius = max(bestNewRadius, LengthVec3(center, triangle->m_vertices[1]->m_position));
                    bestNewRadius = max(bestNewRadius, LengthVec3(center, triangle->m_vertices[2]->m_position));

                    // find only a triangle.
                    break;
                }
            }

            // if not found next triangle, you can translate next loop!
            if (bestTriangle == nullptr)
            {
                ++i;
                continue;
            }

            // best Triangle vertex
            Vertex* vert1 = bestTriangle->m_vertices[0];
            Vertex* vert2 = bestTriangle->m_vertices[1];
            Vertex* vert3 = bestTriangle->m_vertices[2];

            // hou much add vertex to temp vertex list?
            bool isVert1 = std::find(tempVertices.begin(), tempVertices.end(), vert1) == tempVertices.end();
            bool isVert2 = std::find(tempVertices.begin(), tempVertices.end(), vert2) == tempVertices.end();
            bool isVert3 = std::find(tempVertices.begin(), tempVertices.end(), vert3) == tempVertices.end();
            
            // count and change new vertex.
            int mustAddVertexCount = 0;
            if (isVert1) { newVertex = vert1; mustAddVertexCount++; }
            if (isVert2) { newVertex = vert2; mustAddVertexCount++; }
            if (isVert3) { newVertex = vert3; mustAddVertexCount++; }

            // if all vertex added but triangle isn't add pattern.
            // this pattern value is 0, it skip radius update.
            if (mustAddVertexCount != 0)
            {
                // Update
                radius = bestNewRadius;
                XMFLOAT3 tempCenter;
                XMFLOAT3 pos = newVertex->m_position;
                tempCenter.x = pos.x + (radius / (FLT_EPSILON + LengthVec3(center, pos))) * (center.x - pos.x);
                tempCenter.y = pos.y + (radius / (FLT_EPSILON + LengthVec3(center, pos))) * (center.y - pos.y);
                tempCenter.z = pos.z + (radius / (FLT_EPSILON + LengthVec3(center, pos))) * (center.z - pos.z);
                std::swap(tempCenter, center);
            }

            // is full triangle?
            bool isAddVertex = (tempVertices.size() + mustAddVertexCount) < MAX_VERTICES;
            bool isAddTriangle = (tempTriangles.size() + 1) < MAX_TRIANGLES;
            if (!isAddVertex || !isAddTriangle)
            {
                Construct();

                // init for next loop.
                tempVertices.clear();
                tempTriangles.clear();
                continue;
            }

            // register triangle and vertices.
            bestTriangle->m_isVisited = true;
            tempTriangles.push_back(bestTriangle);
            if (isVert1) { tempVertices.push_back(vert1); vert1->m_isVisited = true; }
            if (isVert2) { tempVertices.push_back(vert2); vert2->m_isVisited = true; }
            if (isVert3) { tempVertices.push_back(vert3); vert3->m_isVisited = true; }
        }

        // when loop finished and triangle remained, make meshlet.
        if (tempTriangles.size() > 0)
        {
            Construct();
        }
    }

    size_t meshletCount = mesh->m_meshlets.size();

    // shrink vector size using meshlet size.
    // triangle align to equal (x mod 4 = 0).
    ResourceMeshlet& last = mesh->m_meshlets[meshletCount - 1];
    mesh->m_primitiveIndices.resize(last.m_primitiveOffset + ((last.m_primitiveCount * 3 + 3) & ~3));
}
#pragma endregion
