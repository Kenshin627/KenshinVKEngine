#include <fastgltf/core.hpp>
#include <glm.hpp>
#include <fastgltf/glm_element_traits.hpp>
#include <fastgltf/tools.hpp>
#include "gltfLoader.h"
#include "core.h"
#include "../engine/kEngine.h"

std::vector<std::shared_ptr<MeshAssert>> LoadGltfMeshAsserts(KEngine* engine, const std::filesystem::path& path)
{
	std::vector<std::shared_ptr<MeshAssert>> res;
	auto dataBuffer = fastgltf::GltfDataBuffer::FromPath(path);
	if (dataBuffer.error() != fastgltf::Error::None)
	{
		KS_CORE_ASSERT(false, "Failed to load glTF file: {}", static_cast<int>(dataBuffer.error()));
		return {};
	}

	fastgltf::Parser parser;
	fastgltf::Asset assert;
	fastgltf::Options opts = fastgltf::Options::LoadExternalBuffers;
	auto parseRes = parser.loadGltfBinary(dataBuffer.get(), path.parent_path(), opts);
	if (parseRes.error() != fastgltf::Error::None)
	{
		KS_CORE_ASSERT(false, "Failed to parse glTF file: {}", static_cast<int>(parseRes.error()));
		return {};
	}
	assert = std::move(parseRes.get());
	std::vector<std::shared_ptr<MeshAssert>> meshes;
	std::vector<uint32_t> indices;
	std::vector<Vertex> vertices;
	for (auto& mesh : assert.meshes)
	{
		auto meshAssert = std::make_shared<MeshAssert>();
		meshAssert->name = mesh.name.empty() ? "unknown name" : mesh.name;
		indices.clear();
		vertices.clear();
		for (auto& primative : mesh.primitives)
		{
			auto& indexAccessor = assert.accessors[primative.indicesAccessor.value()];
			GeoSurface newSurface;
			newSurface.startIndex = static_cast<uint32_t>(indices.size());
			newSurface.indexCount = static_cast<uint32_t>(indexAccessor.count);
			size_t initialVtx = vertices.size();
			indices.reserve(indices.size() + indexAccessor.count);

			//load indices
			{
				fastgltf::iterateAccessor<uint32_t>(assert, indexAccessor, [&](std::uint32_t idx) {
					indices.push_back(idx + initialVtx);
				});
			}

			//load vertices
			{
				auto& posAccessor = assert.accessors[primative.findAttribute("POSITION")->accessorIndex];
				vertices.resize(vertices.size() + posAccessor.count);
				fastgltf::iterateAccessorWithIndex<glm::vec3>(assert, posAccessor, [&](glm::vec3 pos, size_t index) {
					Vertex vertex{};
					vertex.position = { pos , 1.0 };
					vertex.color = { 1.0, 0.0, 0.0, 1.0 };
					vertices[initialVtx + index] = vertex;
				});
			}

			meshAssert->surfaces.push_back(newSurface);
		}
		meshAssert->meshBuffer = engine->loadMeshBuffer(vertices, indices);
		res.push_back(meshAssert);
	}
	return res;
}
