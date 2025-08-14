#pragma once
#include <vulkan/vulkan.h>
#include <vector>

class Subpass final
{
public:
	~Subpass() = default;

	VkSubpassDescription* GetDescriptionPtr() { return &m_Description; }

private:
	Subpass() = default;
	friend class SubpassBuilder;
	VkSubpassDescription m_Description;

	std::vector<VkAttachmentReference> m_ColorAttachments;
	std::vector<VkAttachmentReference> m_InputAttachments;
	std::vector<VkAttachmentReference> m_PreserveAttachments;
	VkAttachmentReference m_DepthAttachment;
};

class SubpassBuilder final
{
public:
	SubpassBuilder() = default;
	~SubpassBuilder() = default;
	
	SubpassBuilder(const SubpassBuilder&) 				= delete;
	SubpassBuilder(SubpassBuilder&&) noexcept 			= delete;
	SubpassBuilder& operator=(const SubpassBuilder&) 	 	= delete;
	SubpassBuilder& operator=(SubpassBuilder&&) noexcept 	= delete;

	SubpassBuilder& AddColorAttachmentRef(uint32_t index, VkImageLayout referenceLayout)
	{
		VkAttachmentReference reference{};
		reference.attachment	= index;
		reference.layout		= referenceLayout;

		m_ColorAttachments.emplace_back(reference);
		return *this;
	}

	SubpassBuilder& AddInputAttachmentRef(uint32_t index, VkImageLayout referenceLayout)
	{
		VkAttachmentReference reference{};
		reference.attachment	= index;
		reference.layout		= referenceLayout;

		m_InputAttachments.emplace_back(reference);
		return *this;
	}

	SubpassBuilder& AddPreserveAttachmentRef(uint32_t index, VkImageLayout referenceLayout)
	{
		VkAttachmentReference reference{};
		reference.attachment	= index;
		reference.layout		= referenceLayout;

		m_PreserveAttachments.emplace_back(reference);
		return *this;
	}

	SubpassBuilder& SetDepthStencilAttachmentRef(uint32_t index, VkImageLayout referenceLayout)
	{
		VkAttachmentReference reference{};
		reference.attachment = index;
		reference.layout = referenceLayout;

		m_DepthAttachment = reference;
		m_HasDepth = true;
		return *this;
	}

	SubpassBuilder& SetFlags(VkSubpassDescriptionFlags flags)
	{
		m_Flags = flags;
		return *this;
	}

	// cares that builder can be reused for building multiple subpasses
	Subpass Build(VkPipelineBindPoint pipelineBindPoint)
	{
		Subpass subpass{};

		subpass.m_ColorAttachments		= m_ColorAttachments;
		subpass.m_InputAttachments		= m_InputAttachments;
		subpass.m_PreserveAttachments	= m_PreserveAttachments;
		subpass.m_DepthAttachment		= m_DepthAttachment;

		VkSubpassDescription& description = subpass.m_Description;
		description.flags = m_Flags;
		description.inputAttachmentCount	= subpass.m_InputAttachments.size();
		description.pInputAttachments		= subpass.m_InputAttachments.data();
		description.colorAttachmentCount	= subpass.m_ColorAttachments.size();
		description.pColorAttachments		= subpass.m_ColorAttachments.data();
		description.preserveAttachmentCount = subpass.m_PreserveAttachments.size();
		description.pResolveAttachments		= subpass.m_PreserveAttachments.data();
		description.pDepthStencilAttachment = (m_HasDepth) ? &subpass.m_DepthAttachment : nullptr;

		m_ColorAttachments.clear();
		m_InputAttachments.clear();
		m_PreserveAttachments.clear();
		m_HasDepth = false;

		return subpass;
	}

private:

	std::vector<VkAttachmentReference> m_ColorAttachments;
	std::vector<VkAttachmentReference> m_InputAttachments;
	std::vector<VkAttachmentReference> m_PreserveAttachments;
	VkAttachmentReference m_DepthAttachment;
	bool m_HasDepth{};

	VkSubpassDescriptionFlags m_Flags;
};