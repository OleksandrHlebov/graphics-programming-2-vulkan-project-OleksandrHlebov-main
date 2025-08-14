#pragma once
#include <vulkan/vulkan.h>
#include <memory>
#include <vector>

class Device;
class CommandPool;
class Image;
class CommandBuffer;
class Buffer final
{
public:
	~Buffer() = default;

	VkBuffer*		GetBufferPtr() { return &m_Buffer;	}
	VkDeviceMemory* GetMemoryPtr() { return &m_Memory;	}
	void*			GetMappedData(){ return m_Data;		}

	void UpdateMappedData(void* newData, size_t size, size_t offset);

	void CopyTo(Buffer* buffer, CommandBuffer* command, Device* device, CommandPool* commandPool);
	void CopyTo(Image* image, CommandBuffer* command, Device* device, CommandPool* commandPool);

	void Destroy(Device* device);

	VkDeviceSize GetSize() { return m_Size; }
	
private:
	friend class Mesh;
	friend class BufferBuilder;
	Buffer() = default;

	VkBuffer		m_Buffer;
	VkDeviceMemory	m_Memory;
	VkDeviceSize	m_Size;
	void*			m_Data{ nullptr };
};

class BufferBuilder final
{
public:
	BufferBuilder() = default;
	~BufferBuilder() = default;
	
	BufferBuilder(const BufferBuilder&) 				= delete;
	BufferBuilder(BufferBuilder&&) noexcept 			= delete;
	BufferBuilder& operator=(const BufferBuilder&) 	 	= delete;
	BufferBuilder& operator=(BufferBuilder&&) noexcept 	= delete;

	// if used buffer must be built with VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
	BufferBuilder& MapMemory();

	// bind data to stage and copy to created buffer
	BufferBuilder& BindData(void* data, CommandPool* commandPool);

	void Build(std::unique_ptr<Buffer>& buffer, Device* device, CommandPool* commandPool, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties);
	void Build(std::vector<Buffer>& buffers, Device* device, CommandPool* commandPool, uint32_t amount, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties);
	Buffer Build(Device* device, CommandPool* commandPool, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties);

private:
	friend class Mesh;
	void CreateBuffer(Buffer& buffer, Device* device, CommandPool* commandPool, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, bool mapMemory);
	void CreateBufferWithData(Buffer& buffer, Device* device, CommandPool* commandPool, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties);

	CommandPool* m_CommandPool;
	void*	m_BoundData{ nullptr };
	bool	m_MapMemory{ false };
};