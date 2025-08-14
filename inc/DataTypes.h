#pragma once
#include <vulkan/vulkan.h>
#include <array>

#include <glm/gtx/hash.hpp>

namespace datatype
{ 
	struct TextureIndices
	{
		uint32_t albedo;
		uint32_t roughness;
		uint32_t metalness;
		uint32_t normal;
	}; 
	  
	struct ModelViewProjection
	{
		glm::mat4 model;
		glm::mat4 view;
		glm::mat4 projection;
	}; 

	struct PointLight
	{
		glm::vec3 Position;
		alignas(16)glm::vec3 Color;
		float Lumen;
	}; 
	
	struct DirectionalLight
	{ 
		glm::vec3 Direction;
		alignas(16)glm::vec3 Color;
		float Lux;
		glm::mat4 View;
		glm::mat4 Projection;
	};
	 
	struct Vertex  
	{
		glm::vec3 position;
		glm::vec3 colour;
		glm::vec2 uv;
		glm::vec3 normal;
		glm::vec3 tangent;
		glm::vec3 bitangent;

		static VkVertexInputBindingDescription GetBindingDescription() {
			VkVertexInputBindingDescription bindingDescription{};
			bindingDescription.binding = 0;
			bindingDescription.stride = sizeof(Vertex);
			bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

			return bindingDescription;
		}

		static std::array<VkVertexInputAttributeDescription, 6> GetAttributeDescriptions() {
			VkVertexInputAttributeDescription posAttributeDescription{};
			posAttributeDescription.binding = 0;
			posAttributeDescription.location = 0;
			posAttributeDescription.format = VK_FORMAT_R32G32B32_SFLOAT;
			posAttributeDescription.offset = offsetof(Vertex, position);

			VkVertexInputAttributeDescription colorAttributeDescription{};
			colorAttributeDescription.binding = 0;
			colorAttributeDescription.location = 1;
			colorAttributeDescription.format = VK_FORMAT_R32G32B32_SFLOAT;
			colorAttributeDescription.offset = offsetof(Vertex, colour);

			VkVertexInputAttributeDescription uvAttributeDescription{};
			uvAttributeDescription.binding = 0;
			uvAttributeDescription.location = 2;
			uvAttributeDescription.format = VK_FORMAT_R32G32_SFLOAT;
			uvAttributeDescription.offset = offsetof(Vertex, uv);

			VkVertexInputAttributeDescription normalAttributeDescription{};
			normalAttributeDescription.binding = 0;
			normalAttributeDescription.location = 3;
			normalAttributeDescription.format = VK_FORMAT_R32G32B32_SFLOAT;
			normalAttributeDescription.offset = offsetof(Vertex, normal);

			VkVertexInputAttributeDescription tangentAttributeDescription{};
			tangentAttributeDescription.binding = 0;
			tangentAttributeDescription.location = 4;
			tangentAttributeDescription.format = VK_FORMAT_R32G32B32_SFLOAT;
			tangentAttributeDescription.offset = offsetof(Vertex, tangent);

			VkVertexInputAttributeDescription bitangentAttributeDescription{};
			bitangentAttributeDescription.binding = 0;
			bitangentAttributeDescription.location = 5;
			bitangentAttributeDescription.format = VK_FORMAT_R32G32B32_SFLOAT;
			bitangentAttributeDescription.offset = offsetof(Vertex, bitangent);

			return
			{
				posAttributeDescription,
				colorAttributeDescription,
				uvAttributeDescription,
				normalAttributeDescription,
				tangentAttributeDescription,
				bitangentAttributeDescription
			};
		}

		bool operator==(const Vertex& other) const {
			return position == other.position && colour == other.colour && uv == other.uv;
		}
	};

}

namespace std {
	using namespace datatype;
	template<> struct hash<Vertex> {
		size_t operator()(Vertex const& vertex) const {
			return ((hash<glm::vec3>()(vertex.position) ^
					 (hash<glm::vec3>()(vertex.colour) << 1)) >> 1) ^
				(hash<glm::vec2>()(vertex.uv) << 1);
		}
	};
}