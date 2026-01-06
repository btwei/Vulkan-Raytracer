#ifndef VKRT_GLTFIMPORTER_HPP
#define VKRT_GLTFIMPORTER_HPP

#include "AssetManager.hpp"

namespace vkrt {

ImportResult importGLTF(const std::filesystem::path& filepath, AssetManager* assetManager);

} // namespace vkrt

#endif // VKRT_GLTFIMPORTER_HPP