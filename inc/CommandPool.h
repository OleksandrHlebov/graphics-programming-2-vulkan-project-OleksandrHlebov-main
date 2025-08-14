#pragma once
#include <vulkan/vulkan.h>
#include <vector>
#include <memory>

class Device;
class CommandBuffer
{
public:
	~CommandBuffer() = default;

	VkCommandBuffer* GetBufferPtr() { return &m_Buffer; }

	virtual void Start();

	void BeginLabel(const char* name, float color[4]);
	void EndLabel();
	void InsertLabel(const char* name, float color[4]);

	virtual void End(Device* device);

protected:
	friend class CommandPool;
	CommandBuffer(VkCommandPool pool, VkCommandBuffer buffer);
	CommandBuffer(VkCommandPool pool);

	VkCommandBuffer m_Buffer;
	VkCommandPool m_CommandPool;

	inline static PFN_vkCmdBeginDebugUtilsLabelEXT		vkCmdBeginDebugUtilsLabelEXT	{ nullptr };
	inline static PFN_vkCmdEndDebugUtilsLabelEXT		vkCmdEndDebugUtilsLabelEXT		{ nullptr };
	inline static PFN_vkCmdInsertDebugUtilsLabelEXT		vkCmdInsertDebugUtilsLabelEXT	{ nullptr };
};

class SingleTimeCommand final : public CommandBuffer
{
public:
	~SingleTimeCommand() = default;

	void Start() override;

	// cannot be reused after
	// also submits queue and waits
	// till commands are executed
	void End(Device* device) override;

private:
	friend class CommandPool;
	SingleTimeCommand(VkDevice device, VkCommandPool commandPool, VkFence& fence);
	VkFence m_SubmitFence;
};


class CommandPool final
{
public:
	CommandPool(VkDevice device, VkCommandPoolCreateFlags flags, uint32_t queueFamilyIndex);
	~CommandPool() = default;
	
	CommandPool(const CommandPool&) 				= delete;
	CommandPool(CommandPool&&) noexcept 			= delete;
	CommandPool& operator=(const CommandPool&) 	 	= delete;
	CommandPool& operator=(CommandPool&&) noexcept 	= delete;
	
	void AllocateCommandBuffers(std::vector<CommandBuffer>& where, VkDevice device, uint32_t count, VkCommandBufferLevel level);
	void AllocateCommandBuffer(std::unique_ptr<CommandBuffer>& commandBuffer, VkDevice device, VkCommandBufferLevel level);
	CommandBuffer AllocateCommandBuffer(VkDevice device, VkCommandBufferLevel level);

	SingleTimeCommand AllocateSingleTimeCommand(VkDevice device);

	void Destroy(VkDevice device);

	VkCommandPool* GetPoolPtr() { return &m_Pool; }

private:
	void AllocateCommandBuffer(CommandBuffer& commandBuffer, VkDevice device, VkCommandBufferLevel level);

	VkCommandPool m_Pool;
	VkFence m_SingleTimeCommandFence;
};