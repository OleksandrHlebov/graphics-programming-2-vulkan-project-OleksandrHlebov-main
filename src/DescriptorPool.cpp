#include "DescriptorPool.h"
#include "Device.h"
#include <stdexcept>

void DescriptorPoolBuilder::Build(std::unique_ptr<DescriptorPool>& descriptorPool, Device* device, uint32_t maxSets)
{
	descriptorPool.reset(new DescriptorPool());

	Build(*descriptorPool, device, maxSets);
}

DescriptorPool DescriptorPoolBuilder::Build(Device* device, uint32_t maxSets)
{
	DescriptorPool descriptorPool{};

	Build(descriptorPool, device, maxSets);

	return descriptorPool;
}

void DescriptorPoolBuilder::Build(DescriptorPool& descriptorPool, Device* device, uint32_t maxSets)
{
	VkDescriptorPoolCreateInfo poolInfo{};
	poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolInfo.flags = m_Flags;
	poolInfo.poolSizeCount = m_PoolSizes.size();
	poolInfo.pPoolSizes = m_PoolSizes.data();
	poolInfo.maxSets = maxSets;

	if (vkCreateDescriptorPool(*device->GetDevicePtr(), &poolInfo, nullptr, &descriptorPool.m_Pool) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create descriptor pool");
	}
}
