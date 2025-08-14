#include "Scene.h"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include "DataTypes.h"
#include <stdexcept>
#include <iostream>
#include "../inc/Device.h"
#include <limits>

void Scene::Load(Device* device, CommandPool* commandPool, const char* filepath)
{
	Assimp::Importer importer;
	const aiScene* scene = importer.ReadFile(filepath, aiProcess_Triangulate |
											 aiProcess_FlipUVs | aiProcess_JoinIdenticalVertices |
											 aiProcess_ImproveCacheLocality | aiProcess_GenUVCoords | 
											 aiProcess_GenNormals | aiProcess_CalcTangentSpace);

	if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
	{
		throw std::runtime_error("failed to load model " + std::string(importer.GetErrorString()));
	}

	ProcessNode(device, commandPool, scene->mRootNode, scene);
	glm::mat4 model{ GetModelMatrix() };
	m_AABBMin = model * glm::vec4(m_AABBMin, 1.f);
	m_AABBMax = model * glm::vec4(m_AABBMax, 1.f);
} 

void Scene::CalculateLightViewProj(datatype::DirectionalLight& light)
{
	const glm::vec3 sceneCenter = (m_AABBMin + m_AABBMax) * .5f;
	const glm::vec3 lightDirection = glm::normalize(light.Direction);

	const std::vector<glm::vec3> corners
	{
		{ m_AABBMin.x, m_AABBMin.y, m_AABBMin.z },
		{ m_AABBMax.x, m_AABBMin.y, m_AABBMin.z },
		{ m_AABBMin.x, m_AABBMax.y, m_AABBMin.z },
		{ m_AABBMax.x, m_AABBMax.y, m_AABBMin.z },
		{ m_AABBMin.x, m_AABBMin.y, m_AABBMax.z },
		{ m_AABBMax.x, m_AABBMin.y, m_AABBMax.z },
		{ m_AABBMin.x, m_AABBMax.y, m_AABBMax.z },
		{ m_AABBMax.x, m_AABBMax.y, m_AABBMax.z }
	};

	float minProj = FLT_MAX;
	float maxProj = FLT_MIN;
	for (const glm::vec3& corner : corners)
	{
		const float proj = glm::dot(corner, lightDirection);
		minProj = std::min(minProj, proj);
		maxProj = std::max(maxProj, proj);
	} 

	const float distance = maxProj - glm::dot(sceneCenter, lightDirection);
	const glm::vec3 lightPos = sceneCenter - lightDirection * distance;

	const glm::vec3 up = glm::abs(glm::dot(lightDirection, glm::vec3(.0f, .0f, 1.f))) > .99f
		? glm::vec3(.0f, 1.f, .0f) 
		: glm::vec3(.0f, .0f, 1.f);
	 
	light.View = glm::lookAt(lightPos, sceneCenter, up);

	glm::vec3 minLightSpace{ FLT_MAX };
	glm::vec3 maxLightSpace{ FLT_MIN }; 
	for (const glm::vec3& corner : corners)
	{
		const glm::vec3 transformedCorner = glm::vec3{ light.View * glm::vec4(corner, 1.f) };
		minLightSpace = glm::min(minLightSpace, transformedCorner);
		maxLightSpace = glm::max(maxLightSpace, transformedCorner);
	}

	const float near = .0f;
	const float far = maxLightSpace.z - minLightSpace.z;
	light.Projection = glm::ortho(minLightSpace.x, maxLightSpace.x, minLightSpace.y, maxLightSpace.y, near, far);
	light.Projection[1][1] *= -1;
}

std::vector<Mesh>& Scene::GetMeshes()
{ 
	return m_Meshes;
}

std::vector<Image>& Scene::GetTextures()
{
	return m_Textures;
}

void Scene::ProcessNode(Device* device, CommandPool* commandPool, aiNode* node, const aiScene* scene)
{
	for (uint32_t meshIndex{}; meshIndex < node->mNumMeshes; ++meshIndex)
	{
		aiMesh* mesh = scene->mMeshes[node->mMeshes[meshIndex]];
		std::vector<datatype::Vertex> tempVertices;
		std::vector<uint32_t> tempIndices;
		std::optional<Image> tempTexture;

		for (uint32_t vertexIndex{}; vertexIndex < mesh->mNumVertices; ++vertexIndex)
		{
			aiMatrix4x4 transform = scene->mRootNode->mTransformation;
			aiVector3D aiVertex{ transform * mesh->mVertices[vertexIndex] };

			aiVector3D translation{};
			aiQuaternion rotationQuat{};

			transform.DecomposeNoScaling(rotationQuat, translation);
			
			const aiMatrix3x3 rotation{ rotationQuat.GetMatrix() };

			datatype::Vertex vertex{};
			vertex.position.x = aiVertex.x;
			vertex.position.y = aiVertex.y;
			vertex.position.z = aiVertex.z;

			m_AABBMin.x = std::min(m_AABBMin.x, vertex.position.x);
			m_AABBMin.y = std::min(m_AABBMin.y, vertex.position.y);
			m_AABBMin.z = std::min(m_AABBMin.z, vertex.position.z);

			m_AABBMax.x = std::max(m_AABBMax.x, vertex.position.x);
			m_AABBMax.y = std::max(m_AABBMax.y, vertex.position.y);
			m_AABBMax.z = std::max(m_AABBMax.z, vertex.position.z);

			aiVector3D normal{ rotation * mesh->mNormals[vertexIndex] };
			vertex.normal.x = normal.x;
			vertex.normal.y = normal.y;
			vertex.normal.z = normal.z;

			aiVector3D tangent{ rotation * mesh->mTangents[vertexIndex] };
			vertex.tangent.x = tangent.x;
			vertex.tangent.y = tangent.y;
			vertex.tangent.z = tangent.z;

			aiVector3D bitangent{ rotation * mesh->mBitangents[vertexIndex] };
			vertex.bitangent.x = bitangent.x;
			vertex.bitangent.y = bitangent.y;
			vertex.bitangent.z = bitangent.z;

			if (mesh->mTextureCoords[0])
			{
				aiVector3D& uv = mesh->mTextureCoords[0][vertexIndex];
				vertex.uv.x = uv.x;
				vertex.uv.y = uv.y;
			}
			else
				vertex.uv = { .0f, .0f };

			tempVertices.push_back(vertex);
		}

		for (uint32_t faceIndex{}; faceIndex < mesh->mNumFaces; ++faceIndex)
		{
			aiFace face = mesh->mFaces[faceIndex];
			for (uint32_t indexIndex{}; indexIndex < face.mNumIndices; ++indexIndex)
			{
				tempIndices.push_back(face.mIndices[indexIndex]);
			}
		}

		// TODO: materials
		datatype::TextureIndices textureIndices{  };

		if (mesh->mMaterialIndex >= 0) 
		{
			aiString str;
			aiMaterial* material{};
			material = scene->mMaterials[mesh->mMaterialIndex];
			if (material->GetTextureCount(aiTextureType_BASE_COLOR))
				material->GetTexture(aiTextureType_BASE_COLOR, 0, &str);
			else
				str = aiString{ "textures/200px-Debugempty.png" };

			if (!m_LoadedTextures.contains(std::string(str.C_Str())))
			{
				ImageBuilder builder{};
				tempTexture = builder
					.SetFilePath("resources/" + std::string(str.C_Str()))
					.Build(device, commandPool, VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

				textureIndices.albedo = m_LoadedTextures[std::string(str.C_Str())] = m_Textures.size();
				m_Textures.push_back(tempTexture.value());
				m_DeletionQueue.Push([&, device, textureIndices]() { m_Textures[textureIndices.albedo].Destroy(*device->GetDevicePtr()); });
			}
			else
				textureIndices.albedo = m_LoadedTextures[std::string(str.C_Str())];

			if (material->GetTextureCount(aiTextureType_DIFFUSE_ROUGHNESS))
				material->GetTexture(aiTextureType_DIFFUSE_ROUGHNESS, 0, &str);
			else
				str = aiString{ "textures/200px-Debugempty.png" };

			if (!m_LoadedTextures.contains(std::string(str.C_Str())))
			{
				ImageBuilder builder{};
				tempTexture = builder
					.SetFilePath("resources/" + std::string(str.C_Str()))
					.Build(device, commandPool, VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

				textureIndices.roughness = m_LoadedTextures[std::string(str.C_Str())] = m_Textures.size();
				m_Textures.push_back(tempTexture.value());
				m_DeletionQueue.Push([&, device, textureIndices]() { m_Textures[textureIndices.roughness].Destroy(*device->GetDevicePtr()); });
			}
			else
				textureIndices.roughness = m_LoadedTextures[std::string(str.C_Str())];

			if (material->GetTextureCount(aiTextureType_METALNESS))
				material->GetTexture(aiTextureType_METALNESS, 0, &str);
			else
				str = aiString{ "textures/200px-Debugempty.png" };

			if (!m_LoadedTextures.contains(std::string(str.C_Str())))
			{
				ImageBuilder builder{};
				tempTexture = builder
					.SetFilePath("resources/" + std::string(str.C_Str()))
					.Build(device, commandPool, VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

				textureIndices.metalness = m_LoadedTextures[std::string(str.C_Str())] = m_Textures.size();
				m_Textures.push_back(tempTexture.value());
				m_DeletionQueue.Push([&, device, textureIndices]() { m_Textures[textureIndices.metalness].Destroy(*device->GetDevicePtr()); });
			}
			else
				textureIndices.metalness = m_LoadedTextures[std::string(str.C_Str())];

			if (material->GetTextureCount(aiTextureType_NORMALS))
				material->GetTexture(aiTextureType_NORMALS, 0, &str);
			else
				str = aiString{ "textures/200px-Debugempty.png" };

			if (!m_LoadedTextures.contains(std::string(str.C_Str())))
			{
				ImageBuilder builder{};
				tempTexture = builder
					.SetFilePath("resources/" + std::string(str.C_Str()))
					.SetFormat(VK_FORMAT_R8G8B8A8_UNORM)
					.Build(device, commandPool, VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

				textureIndices.normal = m_LoadedTextures[std::string(str.C_Str())] = m_Textures.size();
				m_Textures.push_back(tempTexture.value());
				m_DeletionQueue.Push([&, device, textureIndices]() { m_Textures[textureIndices.normal].Destroy(*device->GetDevicePtr()); });
			}
			else
				textureIndices.normal = m_LoadedTextures[std::string(str.C_Str())];
		}

		uint32_t index{ static_cast<uint32_t>(m_Meshes.size()) };
		m_Meshes.push_back(Mesh(device, commandPool, tempVertices, tempIndices, textureIndices));
		m_DeletionQueue.Push([&, device, index]() { m_Meshes[index].Destroy(device); });
	}
	for (uint32_t i = 0; i < node->mNumChildren; i++)
	{
		ProcessNode(device, commandPool, node->mChildren[i], scene);
	}
}
