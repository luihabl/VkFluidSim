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

    for (const auto& shape : obj_reader.GetShapes()) {
        gfx::CPUMesh mesh;
        mesh.name = shape.name;

        // TODO: Here we assume that we have only triangles. Later change this to consider quads
        // or polygons.
        std::unordered_map<int, int> used_indices;
        for (u32 i = 0; i < shape.mesh.indices.size(); i++) {
            auto idx = shape.mesh.indices[i];

            if (used_indices.contains(idx.vertex_index)) {
                mesh.indices.push_back(used_indices[idx.vertex_index]);
                continue;
            }

            mesh.indices.push_back(mesh.vertices.size());
            used_indices[idx.vertex_index] = mesh.indices.back();

            auto vert = gfx::Vertex{};

            if (idx.texcoord_index >= 0) {
                vert.uv = {attrib.texcoords[2 * size_t(idx.texcoord_index) + 0],
                           attrib.texcoords[2 * size_t(idx.texcoord_index) + 1]};
            }

            vert.pos = {attrib.vertices[3 * size_t(idx.vertex_index) + 0],
                        attrib.vertices[3 * size_t(idx.vertex_index) + 1],
                        attrib.vertices[3 * size_t(idx.vertex_index) + 2]};

            mesh.vertices.push_back(vert);
        }

        meshes.push_back(std::move(mesh));
    }

    return meshes;
}
}  // namespace vfs
