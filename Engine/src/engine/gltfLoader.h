#pragma once
#include <string>
#include <filesystem>
#include <vector>
#include "type.h"

class KEngine;

struct GeoSurface
{
	uint32_t startIndex;
	uint32_t indexCount;
};

struct MeshAssert
{
	std::string name;
	std::vector<GeoSurface> surfaces;
	MeshBuffer meshBuffer;
};

std::vector<std::shared_ptr<MeshAssert>> LoadGltfMeshAsserts(KEngine* engine, const std::filesystem::path& path);