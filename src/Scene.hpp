#ifndef MXC_SCENE_HPP
#define MXC_SCENE_HPP

#include "StaticVector.hpp"

#include <ranges>
#include <vector>
#include <span>
#include <string_view>
#include <cassert>

namespace std {
// forward declaration to do some template requires clauses
template< class T, class Allocator = std::allocator<T>> class forward_list;
}

namespace mxc
{

// Supported layouts:
//    Position: The vertex position is a required attribute for most shaders, including PBR shaders.
//    Normal: The normal vector is used to calculate lighting and shading, and is typically required for PBR shaders.
//    Tangent and Bitangent: The tangent and bitangent vectors are used to calculate the direction of the surface's texture coordinates in tangent space, which is necessary for normal mapping.
//    Texture Coordinates: Texture coordinates are used to sample textures and apply them to the surface.
//    Vertex Color: Vertex colors can be used to blend between different textures or apply additional color information to the surface.
//    Material ID: A material ID can be used to identify which material should be applied to the surface.
//    Instance ID: An instance ID can be used to identify which instance of a mesh is being rendered.
//    Bone Weights and Indices: If the mesh is skinned, bone weights and indices are used to determine how each vertex is influenced by the bones in the skeleton.
//    Custom Attributes: Finally, custom vertex attributes can be used to pass additional information to the shader, such as per-vertex roughness or metallic values.
enum class VAttribute : uint8_t
{
    POSITION_XY = 0,
    POSITION_XYZ,
    NORMAL,
    TEXCOORD,
    COLOR_R8G8B8A8,
    TANGENT,
    BITANGENT,
    MATERIALID,
    INSTANCEID,
    VLAYOUT_SIZE
};


// TODO add lights, materials, textures
template <typename Allocator, typename Range> requires (std::ranges::contiguous_range<Range> && allocatorAligned<Allocator>)
class SceneData
{
public: // typedefs
    using PointerType = FancyPointerType;
    using VLayout = StaticVector<VAttribute, static_cast<uint32_t>(VLayout::VLAYOUT_SIZE)>;

public: // constructors
    SceneData() = delete;
    constexpr SceneData(VLayout const& vLayout) 
        : m_alloc(Allocator()), m_meshes(m_alloc), m_vLayout(vLayout), m_vData(m_alloc), m_vSize(computeVertexSize(vLayout)) {}

    // Disable copying and moving
    SceneData(const SceneData&) = delete;
    SceneData& operator=(const SceneData&) = delete;
    SceneData(SceneData&&) = delete;
    SceneData& operator=(SceneData&&) = delete;

public: // member functions
    // called insert_back instead of push_back to signal that the underlying implementation is calling insert at end()
    // done like this to ensure the most compatibility across containers (except forward list)
    template <typename Range2> requires ((std::ranges::forward_range<Range2> && std::is_same_v<Range2::value_type, Range::value_type>) 
                                         || std::is_same_v<Range2, std::initializer_list<Range::value_type>>)
    constexpr auto insertMesh(Range2 const& meshVertices, std::span<uint32_t> meshIndices) -> void
    {
        auto const newMesh_vbegin = m_vData.insert(m_vData.end(), meshVertices.begin(), meshVertices.end());
        auto const newMesh_iBegin = m_meshVertex_indices.insert(m_meshVertex_indices.end(), meshIndices.begin(), meshIndices.end());
        m_meshes.emplace_back(newMesh_begin, m_vData.end(), newMesh_iBegin, m_meshVertex_indices.end());
    }

    constexpr auto eraseMesh(uint32_t meshIndex) -> void
    {
        assert(meshIndex < m_meshes.size());
        m_vData.erase(m_meshes[i].firstVertex, m_meshes[i].pastLastVertex);
        m_meshVertex_indices(m_meshes[i].firstVertexIndex, m_meshes[i].pastLastVertexIndex);
        m_meshes.erase(m_meshes.begin() + meshIndex);
    }

private: // utility functions
    constexpr auto computeVertexSize(VLayout const& vLayout) const -> uint8_t;

private: // defined types
    struct Mesh
    {
        Range::iterator firstVertex;
        Range::iterator pastLastVertex;
        std::vector<uint32_t, Allocator>::iterator firstVertexIndex;
        std::vector<uint32_t, Allocator>::iterator pastLastVertexIndex;
    };

private: // data
    Allocator m_alloc;
    std::vector<Mesh, Allocator> m_meshes;
    std::vector<uint32_t, Allocator> m_meshVertex_indices;

    VLayout m_vLayout;
    Range m_vData;
    uint8_t m_vSize;
};

template <typename Allocator, typename Range> requires (std::ranges::forward_range<Range> && allocatorAligned<Allocator>)
constexpr auto SceneData<Allocator, Range>::computeVertexSize(VLayout const& vLayout) const -> uint8_t
{
    uint8_t result = 0;
    for (uint32_t i = 0; i != vLayout.size(); ++i)
    {
        // could be stored in a lookup table
        switch (vLayout[i])
        {
    		case VAttribute::POSITION_XY: result += 8; break;
    		case VAttribute::POSITION_XYZ: result += 12; break;
    		case VAttribute::NORMAL: result += 12; break;
    		case VAttribute::TEXCOORD: result += 8; break;
    		case VAttribute::COLOR_R8G8B8A8: result += 16; break;
    		case VAttribute::TANGENT: result += 12; break;
    		case VAttribute::BITANGENT: result += 12; break;
    		case VAttribute::MATERIALID: result += 4; break;
    		case VAttribute::INSTANCEID: result += 4; break;
    		case VAttribute::VLAYOUT_SIZE: result += 0; break;
        }
    }

    assert(result);
    return result;
}

template <typename Range> requires std::ranges::contiguous_range<Range>
struct SceneUpdate
{
    std::span<uint32_t> meshesToRemove_indices;
    std::span<Range> meshesToAdd_vertices;
    std::span<std::span<uint32_t>> meshesToAdd_indices;
};

}

#endif // MXC_SCENE_HPP
