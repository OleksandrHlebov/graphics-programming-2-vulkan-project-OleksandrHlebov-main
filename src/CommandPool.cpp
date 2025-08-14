#include "CommandPool.h"
#include "Device.h"
#include <stdexcept>
#include <vector>
#include <cassert>
#include <iostream>

void CommandBuffer::Start()
{
	vkResetCommandBuffer(m_Buffer, 0);
	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

	if (vkBeginCommandBuffer(m_Buffer, &beginInfo) != VK_SUCCESS)
		throw std::runtime_error("failed to submit");
}

void CommandBuffer::BeginLabel(const char* name, float color[4])
{
#ifndef NDEBUG
	VkDebugUtilsLabelEXT label{};
	label.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT;
	label.pLabelName = name;
	label.color[0] = color[0];
	label.color[1] = color[1];
	label.color[2] = color[2];
	label.color[3] = color[3];

	vkCmdBeginDebugUtilsLabelEXT(m_Buffer, &label);
#endif
}

void CommandBuffer::EndLabel()
{
#ifndef NDEBUG
	vkCmdEndDebugUtilsLabelEXT(m_Buffer);
#endif
}

void CommandBuffer::InsertLabel(const char* name, float color[4])
{
#ifndef NDEBUG
	VkDebugUtilsLabelEXT label{};
	label.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT;
	label.pLabelName = name;
	label.color[0] = color[0];
	label.color[1] = color[1];
	label.color[2] = color[2];
	label.color[3] = color[3];

	vkCmdInsertDebugUtilsLabelEXT(m_Buffer, &label);
#endif
}

void CommandBuffer::End(Device* device)
{
	if (vkEndCommandBuffer(m_Buffer) != VK_SUCCESS)
		throw std::runtime_error("failed to end command buffer");
}

CommandBuffer::CommandBuffer(VkCommandPool pool, VkCommandBuffer buffer) : m_Buffer{ buffer }, m_CommandPool{ pool }
{
}

CommandBuffer::CommandBuffer(VkCommandPool pool) : m_CommandPool{ pool }
{
}

void SingleTimeCommand::Start()
{
	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	vkBeginCommandBuffer(m_Buffer, &beginInfo);
}

void SingleTimeCommand::End(Device* device)
{
	CommandBuffer::End(device);

	VkCommandBufferSubmitInfo commandBufferSubmitInfo{};
	commandBufferSubmitInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO;
	commandBufferSubmitInfo.commandBuffer = m_Buffer;

	VkSubmitInfo2 submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2;
	submitInfo.commandBufferInfoCount = 1;
	submitInfo.pCommandBufferInfos = &commandBufferSubmitInfo;

	vkQueueSubmit2(*device->GetGraphicsQueuePtr(), 1, &submitInfo, m_SubmitFence);
	vkWaitForFences(*device->GetDevicePtr(), 1, &m_SubmitFence, VK_TRUE, UINT64_MAX);
	//std::cout << "Resetting single time submit fence\n";
	vkResetFences(*device->GetDevicePtr(), 1, &m_SubmitFence);

	//std::cout << "Freeing single time command\n";
	vkFreeCommandBuffers(*device->GetDevicePtr(), m_CommandPool, 1, &m_Buffer);
}

SingleTimeCommand::SingleTimeCommand(VkDevice device, VkCommandPool commandPool, VkFence& fence) : CommandBuffer(commandPool), m_SubmitFence{ fence }
{
}

CommandPool::CommandPool(VkDevice device, VkCommandPoolCreateFlags flags, uint32_t queueFamilyIndex)
{
	VkFenceCreateInfo fenceInfo{};
	fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	if (vkCreateFence(device, &fenceInfo, nullptr, &m_SingleTimeCommandFence) != VK_SUCCESS)
		throw std::runtime_error("failed to create command pool fence");

	VkCommandPoolCreateInfo poolInfo{};
	poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	poolInfo.flags = flags;
	poolInfo.queueFamilyIndex = queueFamilyIndex;

	if (vkCreateCommandPool(device, &poolInfo, nullptr, &m_Pool) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create command pool");
	}

	CommandBuffer::vkCmdBeginDebugUtilsLabelEXT		= (PFN_vkCmdBeginDebugUtilsLabelEXT)	vkGetDeviceProcAddr(device, "vkCmdBeginDebugUtilsLabelEXT");
	CommandBuffer::vkCmdEndDebugUtilsLabelEXT		= (PFN_vkCmdEndDebugUtilsLabelEXT)		vkGetDeviceProcAddr(device, "vkCmdEndDebugUtilsLabelEXT");
	CommandBuffer::vkCmdInsertDebugUtilsLabelEXT	= (PFN_vkCmdInsertDebugUtilsLabelEXT)	vkGetDeviceProcAddr(device, "vkCmdInsertDebugUtilsLabelEXT");
}

void CommandPool::AllocateCommandBuffers(std::vector<CommandBuffer>& where, VkDevice device, uint32_t count, VkCommandBufferLevel level)
{
	assert(count != 0);
	std::vector<VkCommandBuffer> vkBuffers(count);

	VkCommandBufferAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool = m_Pool;
	allocInfo.level = level;
	allocInfo.commandBufferCount = count;

	if (vkAllocateCommandBuffers(device, &allocInfo, vkBuffers.data()) != VK_SUCCESS)
		throw std::runtime_error("failed to allocate command buffers");

	for (VkCommandBuffer& buffer : vkBuffers)
		where.emplace_back(CommandBuffer(m_Pool, buffer));
}

void CommandPool::AllocateCommandBuffer(std::unique_ptr<CommandBuffer>& commandBuffer, VkDevice device, VkCommandBufferLevel level)
{
	commandBuffer.reset(new CommandBuffer(m_Pool));

	VkCommandBufferAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool = m_Pool;
	allocInfo.level = level;
	allocInfo.commandBufferCount = 1;

	if (vkAllocateCommandBuffers(device, &allocInfo, commandBuffer->GetBufferPtr()) != VK_SUCCESS)
		throw std::runtime_error("failed to allocate command buffers");
}

void CommandPool::AllocateCommandBuffer(CommandBuffer& commandBuffer, VkDevice device, VkCommandBufferLevel level)
{
	VkCommandBufferAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool = m_Pool;
	allocInfo.level = level;
	allocInfo.commandBufferCount = 1;

	if (vkAllocateCommandBuffers(device, &allocInfo, commandBuffer.GetBufferPtr()) != VK_SUCCESS)
		throw std::runtime_error("failed to allocate command buffers");
}

CommandBuffer CommandPool::AllocateCommandBuffer(VkDevice device, VkCommandBufferLevel level)
{
	CommandBuffer command{ m_Pool };
	AllocateCommandBuffer(command, device, level);
	return command;
}

SingleTimeCommand CommandPool::AllocateSingleTimeCommand(VkDevice device)
{
	SingleTimeCommand command(device, m_Pool, m_SingleTimeCommandFence);

	AllocateCommandBuffer(command, device, VK_COMMAND_BUFFER_LEVEL_PRIMARY);

	return command;
}

void CommandPool::Destroy(VkDevice device)
{
	vkDestroyCommandPool(device, m_Pool, nullptr);
	vkDestroyFence(device, m_SingleTimeCommandFence, nullptr);
}