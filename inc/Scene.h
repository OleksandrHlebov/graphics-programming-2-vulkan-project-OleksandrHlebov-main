#pragma once
#include "Mesh.h"
#include "DataTypes.h"
#include "DeletionQueue.h"
#include <vector>
#include <memory>

struct aiNode;
struct aiScene;

class Buffer;

class Scene final
{
public:

	void Load(Device* device, CommandPool* commandPool, const char* filepath);

	void Flush()
	{
		m_DeletionQueue.Flush();
	}

	void CalculateLightViewProj(datatype::DirectionalLight& light);

	std::vector<Mesh>& GetMeshes();

	std::vector<Image>& GetTextures();

	glm::mat4 GetModelMatrix() 
	{
		return glm::rotate(glm::mat4(1.f), glm::radians(90.f), glm::vec3(1.f, .0f, .0f));
	}

private:
	void ProcessNode(Device* device, CommandPool* commandPool, aiNode* node, const aiScene* scene);

	std::unordered_map<std::string, uint32_t> m_LoadedTextures;
	std::vector<Image> m_Textures;
	std::vector<Mesh> m_Meshes;

	glm::vec3 m_AABBMin{ FLT_MAX };
	glm::vec3 m_AABBMax{ FLT_MIN };

	DeletionQueue m_DeletionQueue;
};