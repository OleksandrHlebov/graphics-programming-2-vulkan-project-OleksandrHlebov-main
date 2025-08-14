#pragma once
#include <vulkan/vulkan.h>
#include <vector>
#include <memory>
#include "Subpass.h"

class RenderPass final
{
public:
	~RenderPass() = default;

	VkRenderPass* GetRenderPassPtr() { return &m_RenderPass; }

private:
	friend class RenderPassBuilder;
	RenderPass() = default;

	VkRenderPass m_RenderPass;
};

class RenderPassBuilder final
{
public:
	RenderPassBuilder() = default;
	~RenderPassBuilder() = default;
	
	RenderPassBuilder(const RenderPassBuilder&) 				= delete;
	RenderPassBuilder(RenderPassBuilder&&) noexcept 			= delete;
	RenderPassBuilder& operator=(const RenderPassBuilder&) 	 	= delete;
	RenderPassBuilder& operator=(RenderPassBuilder&&) noexcept 	= delete;

	RenderPassBuilder& AddDependency(const VkSubpassDependency& dependency)
	{
		m_Dependencies.emplace_back(dependency);
		return *this;
	}

	RenderPassBuilder& AddAttachment(const VkAttachmentDescription& attachment)
	{
		m_Attachments.emplace_back(attachment);
		return *this;
	}

	RenderPassBuilder& AddSubpass(Subpass subpass)
	{
		m_Subpasses.push_back(std::move(subpass));
		return *this;
	}

	RenderPassBuilder& SetFlags(VkRenderPassCreateFlags flags)
	{
		m_Flags = flags;
		return *this;
	}

	void Build(std::unique_ptr<RenderPass>& renderPass, VkDevice device);
	RenderPass Build(VkDevice device);

private:
	std::vector<VkSubpassDependency> m_Dependencies;
	std::vector<VkAttachmentDescription> m_Attachments;
	std::vector<Subpass> m_Subpasses;
	VkRenderPassCreateFlags m_Flags;
};