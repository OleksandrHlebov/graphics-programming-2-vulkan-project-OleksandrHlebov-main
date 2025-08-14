#pragma once
#include <vulkan/vulkan.h>
#include <vector>
#include <memory>
#include "DescriptorSetLayout.h"

class PipelineLayout final
{
public:
	~PipelineLayout() = default;

	VkPipelineLayout* GetPipelineLayoutPtr() { return &m_Layout; }

private:
	friend class PipelineLayoutBuilder;
	PipelineLayout() = default;

	VkPipelineLayout m_Layout;

};

class PipelineLayoutBuilder final
{
public:
	PipelineLayoutBuilder()		= default;
	~PipelineLayoutBuilder()	= default;
	
	PipelineLayoutBuilder(const PipelineLayoutBuilder&) 				= delete;
	PipelineLayoutBuilder(PipelineLayoutBuilder&&) noexcept 			= delete;
	PipelineLayoutBuilder& operator=(const PipelineLayoutBuilder&) 	 	= delete;
	PipelineLayoutBuilder& operator=(PipelineLayoutBuilder&&) noexcept 	= delete;

	PipelineLayoutBuilder& AddPushConstant(VkShaderStageFlags flags, uint32_t offset, uint32_t size)
	{
		m_PushConstantRanges.emplace_back(flags, offset, size);
		return *this;
	}

	PipelineLayoutBuilder& AddDescriptorSetLayout(DescriptorSetLayout* layout)
	{
		m_SetLayouts.emplace_back(*layout->GetLayoutPtr());
		return *this;
	}

	void Build(std::unique_ptr<PipelineLayout>& layout, VkDevice device)
	{
		layout.reset(new PipelineLayout(std::move(Build(device))));
	}

	PipelineLayout Build(VkDevice device)
	{
		PipelineLayout layout{};
		VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
		pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutInfo.setLayoutCount = m_SetLayouts.size();
		pipelineLayoutInfo.pSetLayouts = m_SetLayouts.data();
		pipelineLayoutInfo.pushConstantRangeCount = m_PushConstantRanges.size();
		pipelineLayoutInfo.pPushConstantRanges = m_PushConstantRanges.data();

		if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &layout.m_Layout) != VK_SUCCESS)
			throw std::runtime_error("failed to create pipeline layout");

		return layout;
	}

private:
	std::vector<VkPushConstantRange>	m_PushConstantRanges;
	std::vector<VkDescriptorSetLayout>	m_SetLayouts;
};