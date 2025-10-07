#include "mesh_loader.h"

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

#include <filesystem>
#include <unordered_map>

#include "gfx/common.h"
#include "gfx/mesh.h"

namespace vfs {
std::vector<gfx::CPUMesh> LoadObjMesh(const std::string& path) {
    std::vector<gfx::CPUMesh> meshes;

    auto obj_file_path = std::filesystem::path(path);

    tinyobj::ObjReaderConfig reader_config;
    // reader_config.mtl_search_path = obj_file_path.parent_path();
    reader_config.triangulate = true;  // true by default but putting here so that it is clear

    tinyobj::ObjReader obj_reader;
    if (!obj_reader.ParseFromFile(obj_file_path, reader_config)) {
        if (!obj_reader.Error().empty()) {
            fmt::println("Error while parsing OBJ file [{}]: {}", path, obj_reader.Error());
        }
        fmt::println("Failed to load OBJ file");
        return {};
    }

    if (!obj_reader.Warning().empty()) {
        fmt::println("Warning while parsing OBJ file [{}]: {}", path, obj_reader.Warning());
    }

    const auto& attrib = obj_reader.GetAttrib();

    // TODO: Ignoring materials for now, consider them later
    // const auto& materials = obj_reader.GetMaterials();

    using h = std::hash<int>;
    auto hash = [](const tinyobj::index_t& n) {
        return ((17 * 31 + h()(n.vertex_index)) * 31 + h()(n.normal_index)) * 31 +
               h()(n.texcoord_index);
    };
    auto equal = [](const tinyobj::index_t& l, const tinyobj::index_t& r) {
        return l.vertex_index == r.vertex_index && l.normal_index == r.normal_index &&
               l.texcoord_index == r.texcoord_index;
    };

    for (const auto& shape : obj_reader.GetShapes()) {
        gfx::CPUMesh mesh;
        mesh.name = shape.name;

        // TODO: Here we assume that we have only triangles. Later change this to consider quads
        // or polygons.
        std::unordered_map<tinyobj::index_t, int, decltype(hash), decltype(equal)> used_indices;
        for (u32 i = 0; i < shape.mesh.indices.size(); i++) {
            auto idx = shape.mesh.indices[i];

            if (used_indices.contains(idx)) {
                mesh.indices.push_back(used_indices[idx]);
                continue;
            }

            mesh.indices.push_back(mesh.vertices.size());
            used_indices[idx] = mesh.indices.back();

            auto vert = gfx::Vertex{};

            if (idx.texcoord_index >= 0) {
                vert.uv = {attrib.texcoords[2 * size_t(idx.texcoord_index) + 0],
                           attrib.texcoords[2 * size_t(idx.texcoord_index) + 1]};
            }

            vert.pos = {attrib.vertices[3 * size_t(idx.vertex_index) + 0],
                        attrib.vertices[3 * size_t(idx.vertex_index) + 1],
                        attrib.vertices[3 * size_t(idx.vertex_index) + 2]};

            if (idx.normal_index >= 0) {
                vert.normal = {attrib.normals[3 * size_t(idx.normal_index) + 0],
                               attrib.normals[3 * size_t(idx.normal_index) + 1],
                               attrib.normals[3 * size_t(idx.normal_index) + 2]};
            }

            mesh.vertices.push_back(vert);
        }

        meshes.push_back(std::move(mesh));
    }

    return meshes;
}

std::vector<gfx::CPUMesh> LoadGltfMesh(const std::string& path) {}

}  // namespace vfs
