#pragma once
#include <vulkan/vulkan.h>
#include <vector>
#include <memory>

class DescriptorSetLayout final
{
public:
	~DescriptorSetLayout()	= default;

	VkDescriptorSetLayout* GetLayoutPtr() { return &m_Layout; }

private:
	friend class DescriptorSetLayoutBuilder;
	DescriptorSetLayout()	= default;

	VkDescriptorSetLayout m_Layout;
};

class DescriptorSetLayoutBuilder final
{
public:
	DescriptorSetLayoutBuilder() = default;
	~DescriptorSetLayoutBuilder() = default;
	
	DescriptorSetLayoutBuilder(const DescriptorSetLayoutBuilder&) 					= delete;
	DescriptorSetLayoutBuilder(DescriptorSetLayoutBuilder&&) noexcept 				= delete;
	DescriptorSetLayoutBuilder& operator=(const DescriptorSetLayoutBuilder&) 	 	= delete;
	DescriptorSetLayoutBuilder& operator=(DescriptorSetLayoutBuilder&&) noexcept 	= delete;

	DescriptorSetLayoutBuilder& AddBinding(uint32_t index, VkDescriptorType type, VkShaderStageFlags stageFlag, uint32_t count = 1)
	{
		VkDescriptorSetLayoutBinding binding{};
		binding.binding = index;
		binding.descriptorCount = count;
		binding.descriptorType = type;
		binding.stageFlags = stageFlag;
		m_Bindings.emplace_back(binding);
		return *this;
	}

	void Build(std::unique_ptr<DescriptorSetLayout>& layout, VkDevice device)
	{
		layout.reset(new DescriptorSetLayout());
		VkDescriptorSetLayoutCreateInfo layoutInfo{};
		layoutInfo.sType		= VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		layoutInfo.flags		= m_Flags;
		layoutInfo.bindingCount = m_Bindings.size();
		layoutInfo.pBindings	= m_Bindings.data();

		if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &layout->m_Layout) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create descriptor set layout");
		}
	}

	DescriptorSetLayout Build(VkDevice device)
	{
		DescriptorSetLayout layout{};
		VkDescriptorSetLayoutCreateInfo layoutInfo{};
		layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		layoutInfo.flags = m_Flags;
		layoutInfo.bindingCount = m_Bindings.size();
		layoutInfo.pBindings = m_Bindings.data();

		if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &layout.m_Layout) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create descriptor set layout");
		}
		return layout;
	}

private:
	VkDescriptorSetLayoutCreateFlags m_Flags;

	std::vector<VkDescriptorSetLayoutBinding> m_Bindings{};
};