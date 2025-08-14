#include "Buffer.h"
#include "Device.h"
#include <stdexcept>
#include <cassert>
#include "CommandPool.h"
#include "../inc/Image.h"

void Buffer::UpdateMappedData(void* newData, size_t size, size_t offset)
{
	memcpy(static_cast<char*>(m_Data) + offset, newData, size);
}

void Buffer::CopyTo(Buffer* buffer, CommandBuffer* command, Device* device, CommandPool* commandPool)
{
	assert(m_Size == buffer->m_Size);

	VkBufferCopy copyRegion{};
	copyRegion.srcOffset = 0;
	copyRegion.dstOffset = 0;
	copyRegion.size = m_Size;
	vkCmdCopyBuffer(*command->GetBufferPtr(), m_Buffer, buffer->m_Buffer, 1, &copyRegion);
}

void Buffer::CopyTo(Image* image, CommandBuffer* command, Device* device, CommandPool* commandPool)
{
	VkBufferImageCopy region{};
	region.bufferOffset = 0;
	region.bufferRowLength = 0;
	region.bufferImageHeight = 0;

	region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	region.imageSubresource.layerCount = 1;
	VkExtent2D extent = image->GetExtent();
	region.imageOffset = { 0, 0, 0 };
	region.imageExtent = { extent.width, extent.height, 1 };

	vkCmdCopyBufferToImage(*command->GetBufferPtr(), m_Buffer, image->m_Image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
}

void Buffer::Destroy(Device* device)
{
	vkDestroyBuffer(*device->GetDevicePtr(), m_Buffer, nullptr);
	vkFreeMemory(*device->GetDevicePtr(), m_Memory, nullptr);
}

BufferBuilder& BufferBuilder::MapMemory()
{
	m_MapMemory = true;
	return *this;
}

BufferBuilder& BufferBuilder::BindData(void* data, CommandPool* commandPool)
{
	m_CommandPool = commandPool;
	m_BoundData = data;
	return *this;
}

void BufferBuilder::Build(std::unique_ptr<Buffer>& buffer, Device* device, CommandPool* commandPool, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties)
{
	buffer.reset(new Buffer());
	if (!m_BoundData)
		CreateBuffer(*buffer, device, commandPool, size, usage, properties, m_MapMemory);
	else
		CreateBufferWithData(*buffer, device, commandPool, size, usage, properties);
}

void BufferBuilder::Build(std::vector<Buffer>& buffers, Device* device, CommandPool* commandPool, uint32_t amount, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties)
{
	buffers.resize(amount, Buffer());
	if (!m_BoundData)
		for (Buffer& buffer : buffers)
			CreateBuffer(buffer, device, commandPool, size, usage, properties, m_MapMemory);
	else
		for (Buffer& buffer : buffers)
			CreateBufferWithData(buffer, device, commandPool, size, usage, properties);
}

Buffer BufferBuilder::Build(Device* device, CommandPool* commandPool, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties)
{
	Buffer buffer{};
	if (!m_BoundData)
		CreateBuffer(buffer, device, commandPool, size, usage, properties, m_MapMemory);
	else
		CreateBufferWithData(buffer, device, commandPool, size, usage, properties);
	return buffer;
}

void BufferBuilder::CreateBuffer(Buffer& buffer, Device* device, CommandPool* commandPool, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, bool mapMemory)
{
	VkBufferCreateInfo bufferInfo{};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.size = size;
	bufferInfo.usage = usage;
	bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	buffer.m_Size = size;

	if (vkCreateBuffer(*device->GetDevicePtr(), &bufferInfo, nullptr, &buffer.m_Buffer) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create vertex buffer");
	}

	VkMemoryRequirements memRequirements;
	vkGetBufferMemoryRequirements(*device->GetDevicePtr(), buffer.m_Buffer, &memRequirements);
	 
	VkMemoryAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memRequirements.size;
	allocInfo.memoryTypeIndex = device->FindMemoryType(memRequirements.memoryTypeBits, properties);

	if (vkAllocateMemory(*device->GetDevicePtr(), &allocInfo, nullptr, &buffer.m_Memory) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to allocate vertex buffer memory");
	}
	vkBindBufferMemory(*device->GetDevicePtr(), buffer.m_Buffer, buffer.m_Memory, 0);

	if (mapMemory)
	{
		assert(properties & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);

		vkMapMemory(*device->GetDevicePtr(), buffer.m_Memory, 0, size, 0, &buffer.m_Data);
	}  
}

void BufferBuilder::CreateBufferWithData(Buffer& buffer, Device* device, CommandPool* commandPool, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties)
{
	buffer.m_Size = size;

	Buffer stagingBuffer{};
	CreateBuffer(stagingBuffer, device, commandPool, size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, true);

	stagingBuffer.UpdateMappedData(m_BoundData, size, 0);

	CreateBuffer(buffer, device, commandPool, size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | usage, properties, m_MapMemory);

	SingleTimeCommand command = commandPool->AllocateSingleTimeCommand(*device->GetDevicePtr());

	command.Start();

	stagingBuffer.CopyTo(&buffer, &command, device, m_CommandPool);

	command.End(device);

	stagingBuffer.Destroy(device);
}