#include "RenderPass.h"
#include <stdexcept>

void RenderPassBuilder::Build(std::unique_ptr<RenderPass>& renderPass, VkDevice device)
{
	renderPass.reset(new RenderPass(std::move(Build(device))));
}

RenderPass RenderPassBuilder::Build(VkDevice device)
{
	RenderPass renderPass{};

	std::vector<VkSubpassDescription> subpasses;
	for (auto& subpass : m_Subpasses)
		subpasses.emplace_back(*subpass.GetDescriptionPtr());

	VkRenderPassCreateInfo renderPassInfo{};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassInfo.flags = m_Flags;
	renderPassInfo.attachmentCount = m_Attachments.size();
	renderPassInfo.pAttachments = m_Attachments.data();
	renderPassInfo.subpassCount = subpasses.size();
	renderPassInfo.pSubpasses = subpasses.data();
	renderPassInfo.dependencyCount = m_Dependencies.size();
	renderPassInfo.pDependencies = m_Dependencies.data();

	if (vkCreateRenderPass(device, &renderPassInfo, nullptr, renderPass.GetRenderPassPtr()))
		throw std::runtime_error("failed to create render pass");

	return renderPass;
}
