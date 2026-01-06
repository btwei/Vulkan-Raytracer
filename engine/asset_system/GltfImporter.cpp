#include "GltfImporter.hpp"

#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <tiny_gltf.h>

#include "MeshAsset.hpp"
#include "ModelAsset.hpp"
#include "TextureAsset.hpp"

namespace vkrt {
    
ImportResult importGLTF(const std::filesystem::path& filepath, AssetManager* assetManager) {
    ImportResult result;
    
    tinygltf::Model model;
    tinygltf::TinyGLTF loader;
    std::string err;
    std::string warn;

    // Load model, return early on failure
    if(filepath.extension() == ".gltf") {
        bool res = loader.LoadASCIIFromFile(&model, &err, &warn, filepath);
        if(!res) return result;
    } else if(filepath.extension() == ".glb") {
        bool res = loader.LoadBinaryFromFile(&model, &err, &warn, filepath);
        if(!res) return result;
    } else {
        return result;
    }

    // Process the model into AssetHandles and into a ImportResult object to return
    {
        // Load all images
        for(const auto& image : model.images) {
            AssetHandle<TextureAsset> textureHandle = assetManager->registerAsset(std::make_shared<TextureAsset>(image.name, image.image.data(), VkExtent3D(image.width, image.height, 1), assetManager->getRenderer()));
            result.textureHandles.push_back(textureHandle);
        }

        // Load all meshes
        for(const auto& mesh : model.meshes) {
            // Process indices and vertices with all attributes specified in the Vertex type
            std::vector<uint32_t> indices;
            std::vector<Vertex> vertices;
            std::vector<MeshAsset::Submesh> submeshes;
            std::unordered_map<Vertex, uint32_t> uniqueVertices;
            for(const auto& primitive : mesh.primitives) {
                // Get indices
                const tinygltf::Accessor& indexAccessor = model.accessors[primitive.indices];
                const tinygltf::BufferView& indexBufferView = model.bufferViews[indexAccessor.bufferView];
                const tinygltf::Buffer& indexBuffer = model.buffers[indexBufferView.buffer];

                // Get vertex positions
                const tinygltf::Accessor& posAccessor = model.accessors[primitive.attributes.at("POSITION")];
                const tinygltf::BufferView& posBufferView = model.bufferViews[posAccessor.bufferView];
                const tinygltf::Buffer& posBuffer = model.buffers[posBufferView.buffer];

                // Get texture coordinates if available
                bool hasTexCoords = primitive.attributes.find("TEXCOORD_0") != primitive.attributes.end();
                const tinygltf::Accessor* texCoordAccessor = nullptr;
                const tinygltf::BufferView* texCoordBufferView = nullptr;
                const tinygltf::Buffer* texCoordBuffer = nullptr;

                if(hasTexCoords) {
                    texCoordAccessor = &model.accessors[primitive.attributes.at("TEXCOORD_0")];
                    texCoordBufferView = &model.bufferViews[texCoordAccessor->bufferView];
                    texCoordBuffer = &model.buffers[texCoordBufferView->buffer];
                }

                // Get vertex color if available
                bool hasColor = primitive.attributes.find("COLOR_0") != primitive.attributes.end();
                const tinygltf::Accessor* colorAccessor = nullptr;
                const tinygltf::BufferView* colorBufferView = nullptr;
                const tinygltf::Buffer* colorBuffer = nullptr;

                if(hasTexCoords) {
                    colorAccessor = &model.accessors[primitive.attributes.at("COLOR_0")];
                    colorBufferView = &model.bufferViews[colorAccessor->bufferView];
                    colorBuffer = &model.buffers[colorBufferView->buffer];
                }

                // Get tangents if available
                bool hasTangents = primitive.attributes.find("TANGENT") != primitive.attributes.end();
                const tinygltf::Accessor* tangentAccessor = nullptr;
                const tinygltf::BufferView* tangentBufferView = nullptr;
                const tinygltf::Buffer* tangentBuffer = nullptr;

                if(hasTangents) {
                    tangentAccessor = &model.accessors[primitive.attributes.at("TANGENT")];
                    tangentBufferView = &model.bufferViews[tangentAccessor->bufferView];
                    tangentBuffer = &model.buffers[tangentBufferView->buffer];
                }

                // Create submesh for the MeshAsset (ranges of indices pointing to the same vertex buffer)
                MeshAsset::Submesh submesh;
                submesh.startIndex = (uint32_t)indices.size();
                submesh.count = indexAccessor.count;

                // Process vertices
                for(size_t i=0; i < posAccessor.count; i++) {
                    Vertex vertex{};

                    const float* pos = reinterpret_cast<const float*>(&posBuffer.data[posBufferView.byteOffset + posAccessor.byteOffset + i * 12]);
                    vertex.position = {pos[0], pos[1], pos[2]};

                    if(hasTexCoords) {
                        const float* texCoord = reinterpret_cast<const float*>(&texCoordBuffer->data[texCoordBufferView->byteOffset + texCoordAccessor->byteOffset + i * 8]);
                        vertex.texCoord0_u = texCoord[0];
                        vertex.texCoord0_v = 1.0f - texCoord[1];
                    } else {
                        vertex.texCoord0_u = 0.0f;
                        vertex.texCoord0_v = 0.0f;
                    }

                    if(hasColor && colorAccessor->type == TINYGLTF_TYPE_VEC3 && colorAccessor->componentType == TINYGLTF_COMPONENT_TYPE_FLOAT) {
                        const float* color = reinterpret_cast<const float*>(&colorBuffer->data[colorBufferView->byteOffset + i*12]);
                        vertex.color = {color[0], color[1], color[2], 1.0f};
                    } else if(hasColor && colorAccessor->type == TINYGLTF_TYPE_VEC4 && colorAccessor->componentType == TINYGLTF_COMPONENT_TYPE_FLOAT) {
                        const float* color = reinterpret_cast<const float*>(&colorBuffer->data[colorBufferView->byteOffset + i*16]);
                        vertex.color = {color[0], color[1], color[2], color[3]};
                    } else {
                        vertex.color = {1.0f, 1.0f, 1.0f, 1.0f};
                    }

                    if(hasTangents) {
                        const float* tangent = reinterpret_cast<const float*>(&tangentBuffer->data[tangentBufferView->byteOffset + i*16]);
                        vertex.tangent = {tangent[0], tangent[1], tangent[2], tangent[3]};
                    } else {
                        // Unimplemented, but ideally we generate tangents by iterating over triangles using two edges
                        vertex.tangent = {0.0f, 0.0f, 0.0f, 0.0f};
                    }

                    // For vertex deduplication
                    if(!uniqueVertices.contains(vertex)) {
                        uniqueVertices[vertex] = static_cast<uint32_t>(vertices.size());
                        vertices.push_back(vertex);
                    }
                }

                // Process indices
                const unsigned char* indexData = &indexBuffer.data[indexBufferView.byteOffset + indexAccessor.byteOffset];

                if(indexAccessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT) {
                    const uint16_t* indices16 = reinterpret_cast<const uint16_t*>(indexData);
                    for(size_t i=0; i < indexAccessor.count; i++) {
                        Vertex vertex = vertices[indices16[i]];
                        indices.push_back(uniqueVertices[vertex]);
                    }
                } else if(indexAccessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT) {
                    const uint32_t* indices32 = reinterpret_cast<const uint32_t*>(indexData);
                    for(size_t i=0; i < indexAccessor.count; i++) {
                        Vertex vertex = vertices[indices32[i]];
                        indices.push_back(uniqueVertices[vertex]);
                    }
                } else if(indexAccessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE) {
                    const uint8_t* indices8 = reinterpret_cast<const uint8_t*>(indexData);
                    for(size_t i = 0; i < indexAccessor.count; i++) {
                        Vertex vertex = vertices[indices8[i]];
                        indices.push_back(uniqueVertices[vertex]);
                    }
                }

                // Create a material handle for the primitive and store it in the result
                MaterialAsset::MaterialInfo info{};
                info.norTexture = assetManager->getHandleById<TextureAsset>("errorCheckerboardTexture");
                info.diffTexture = assetManager->getHandleById<TextureAsset>("whiteTexture");
                info.armTexture = assetManager->getHandleById<TextureAsset>("greyTexture");

                if(primitive.material != -1) {
                    auto material = model.materials[primitive.material];
                    if(material.normalTexture.index != -1) info.norTexture = result.textureHandles[model.textures[material.normalTexture.index].source];
                    if(material.pbrMetallicRoughness.baseColorTexture.index != -1) info.diffTexture = result.textureHandles[model.textures[material.pbrMetallicRoughness.baseColorTexture.index].source];
                    if(material.pbrMetallicRoughness.metallicRoughnessTexture.index != -1) info.armTexture = result.textureHandles[model.textures[material.pbrMetallicRoughness.metallicRoughnessTexture.index].source];
                }

                AssetHandle<MaterialAsset> materialHandle = assetManager->registerAsset(std::make_shared<MaterialAsset>(primitive.material, info, assetManager));
                result.materialHandles.push_back(materialHandle);
            }
            
            AssetHandle<MeshAsset> meshHandle = assetManager->registerAsset(std::make_shared<MeshAsset>(mesh.name, vertices, indices, submeshes, assetManager->getRenderer()));
            result.meshHandles.push_back(meshHandle);

            ModelAsset::ModelInfo info{};
            info.mesh = meshHandle;
            info.materials = result.materialHandles;

            AssetHandle<ModelAsset> modelHandle = assetManager->registerAsset(std::make_shared<ModelAsset>(mesh.name, info, assetManager));
            result.modelHandles.push_back(modelHandle);
        }
    }

    result.success = true;
    return result;
}
    
}