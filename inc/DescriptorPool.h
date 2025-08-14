#pragma once
#include <vulkan/vulkan.h>
#include <vector>
#include <memory>

class Device;

class DescriptorPool final
{
public:
	~DescriptorPool() = default;

	VkDescriptorPool* GetDescriptorPoolPtr() { return &m_Pool; }

	void Destroy(VkDevice device)
	{
		vkDestroyDescriptorPool(device, m_Pool, nullptr);
   	}
	  
private:
	friend class DescriptorPoolBuilder;
	DescriptorPool() = default;
	 
	VkDescriptorPool m_Pool; 
}; 

class DescriptorPoolBuilder final
{
public:
	DescriptorPoolBuilder() = default;
	~DescriptorPoolBuilder() = default;
	
	DescriptorPoolBuilder(const DescriptorPoolBuilder&) 				= delete;
	DescriptorPoolBuilder(DescriptorPoolBuilder&&) noexcept 			= delete;
	DescriptorPoolBuilder& operator=(const DescriptorPoolBuilder&) 	 	= delete;
	DescriptorPoolBuilder& operator=(DescriptorPoolBuilder&&) noexcept 	= delete;

	DescriptorPoolBuilder& AddPoolSize(VkDescriptorType type, uint32_t count)
	{
		m_PoolSizes.emplace_back(type, count);
		return *this;
	}

	DescriptorPoolBuilder& SetFlags(VkDescriptorPoolCreateFlags flags)
	{
		m_Flags = flags;
		return *this;
	}

	void Build(std::unique_ptr<DescriptorPool>& descriptorPool, Device* device, uint32_t maxSets);
	DescriptorPool Build(Device* device, uint32_t maxSets);

private:
	void Build(DescriptorPool& pool, Device* device, uint32_t maxSets);

	std::vector<VkDescriptorPoolSize> m_PoolSizes;
	VkDescriptorPoolCreateFlags m_Flags;
};  