#pragma once
#include "DataTypes.h"
#include "Image.h"
#include <vector>
#include "Buffer.h"
#include "Device.h"

class CommandPool;
class Mesh final
{
public:
	~Mesh() = default;
	
	Buffer* GetVertexBuffer() { return &m_VertexBuffer; }
	Buffer* GetIndexBuffer() { return &m_IndexBuffer; }

	datatype::TextureIndices* GetTextureIndices() { return &m_TextureIndices; }

	void Destroy(Device* device)
	{
		m_VertexBuffer.Destroy(device);
		m_IndexBuffer.Destroy(device);
	}

private:
	friend class Scene;
	Mesh(Device* device, CommandPool* commandPool, const std::vector<datatype::Vertex>& vertices, const std::vector<uint32_t>& indices, const datatype::TextureIndices& textureIndices) :
		  m_TextureIndices{ textureIndices }
		, m_Vertices{ vertices }
		, m_Indices{ indices }
	{
		VkDeviceSize vertBufferSize = sizeof(vertices[0]) * vertices.size();
		VkDeviceSize indBufferSize = sizeof(indices[0]) * indices.size();
		BufferBuilder builder{};
		builder
			.BindData((void*)m_Vertices.data(), commandPool)
			.CreateBufferWithData(m_VertexBuffer, device, commandPool, vertBufferSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
		device->SetObjectName(VK_OBJECT_TYPE_BUFFER, (uint64_t)*m_VertexBuffer.GetBufferPtr(), "Vertex buffer");
		builder
			.BindData((void*)m_Indices.data(), commandPool)
			.CreateBufferWithData(m_IndexBuffer, device, commandPool, indBufferSize, VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
		device->SetObjectName(VK_OBJECT_TYPE_BUFFER, (uint64_t)*m_IndexBuffer.GetBufferPtr(), "Index buffer");
	}
	datatype::TextureIndices m_TextureIndices;
	std::vector<datatype::Vertex> m_Vertices;
	Buffer m_VertexBuffer;
	std::vector<uint32_t> m_Indices;
	Buffer m_IndexBuffer;
};