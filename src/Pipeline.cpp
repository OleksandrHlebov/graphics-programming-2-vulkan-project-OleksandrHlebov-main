#include "Pipeline.h"
#include <stdexcept>
#include "../inc/DataTypes.h"
#include "../inc/Device.h"

PipelineBuilder::PipelineBuilder()
{
	m_Rasterizer.sType						= VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	m_Rasterizer.depthClampEnable			= VK_FALSE;
	m_Rasterizer.rasterizerDiscardEnable	= VK_FALSE;
	m_Rasterizer.polygonMode				= VK_POLYGON_MODE_FILL;
	m_Rasterizer.lineWidth					= 1.f;
	m_Rasterizer.cullMode					= VK_CULL_MODE_BACK_BIT;
	m_Rasterizer.frontFace					= VK_FRONT_FACE_COUNTER_CLOCKWISE;
	m_Rasterizer.depthBiasEnable			= VK_FALSE;

	m_Multisampling.sType					= VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	m_Multisampling.sampleShadingEnable		= VK_FALSE;
	m_Multisampling.rasterizationSamples	= VK_SAMPLE_COUNT_1_BIT;

	m_DepthStencil.sType					= VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;

	m_ColorBlending.sType			= VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	m_ColorBlending.logicOpEnable	= VK_FALSE;
	m_ColorBlending.logicOp			= VK_LOGIC_OP_NO_OP;
}

PipelineBuilder& PipelineBuilder::EnableDynamicRendering(const std::vector<VkFormat>& colorAttachmentFormats, VkFormat depthAttachmentFormat, VkFormat stencilAttachmentFormat)
{
	m_RenderingInfo.sType					= VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
	m_RenderingInfo.colorAttachmentCount	= colorAttachmentFormats.size();
	m_RenderingInfo.pColorAttachmentFormats = colorAttachmentFormats.data();
	m_RenderingInfo.depthAttachmentFormat	= depthAttachmentFormat;
	m_RenderingInfo.stencilAttachmentFormat = stencilAttachmentFormat;
	m_RenderingInfo.viewMask = 0;
	return *this;
}

PipelineBuilder& PipelineBuilder::AddShaderStage(const ShaderStage& shaderStage)
{
	m_ShaderStages.emplace_back(shaderStage);
	return *this;
}

PipelineBuilder& PipelineBuilder::AddDynamicState(VkDynamicState state)
{
	m_DynamicStates.emplace_back(state);
	return *this;
}

PipelineBuilder& PipelineBuilder::SetPrimitiveTopology(VkPrimitiveTopology topology)
{
	m_PrimitiveTopology = topology;
	return *this;
}

PipelineBuilder& PipelineBuilder::EnablePrimitiveRestart()
{
	m_EnablePrimitiveRestart = VK_TRUE;
	return *this;
}

PipelineBuilder& PipelineBuilder::EnableDepthClamp()
{
	m_Rasterizer.depthClampEnable = VK_TRUE;
	return *this;
}

PipelineBuilder& PipelineBuilder::EnableRasterizerDiscard()
{
	m_Rasterizer.rasterizerDiscardEnable = VK_TRUE;
	return *this;
}

PipelineBuilder& PipelineBuilder::EnableDepthTest(VkCompareOp op)
{
	m_DepthStencil.depthTestEnable = VK_TRUE;
	m_DepthStencil.depthCompareOp = op;
	return *this;
}

PipelineBuilder& PipelineBuilder::EnableDepthWrite()
{
	m_DepthStencil.depthWriteEnable = VK_TRUE;
	return *this;
}

PipelineBuilder& PipelineBuilder::EnableDepthBoundsTest()
{
	m_DepthStencil.depthBoundsTestEnable = VK_TRUE;
	return *this;
}

PipelineBuilder& PipelineBuilder::SetPolygonMode(VkPolygonMode mode)
{
	m_Rasterizer.polygonMode = mode;
	return *this;
}

PipelineBuilder& PipelineBuilder::SetCullMode(VkCullModeFlags mode)
{
	m_Rasterizer.cullMode = mode;
	return *this;
}

PipelineBuilder& PipelineBuilder::SetVertexDescription(VkVertexInputBindingDescription bindingDesc, VkVertexInputAttributeDescription* attributeDesc, uint32_t attributes)
{
	m_VertexBindingDescription = bindingDesc;
	m_VertexAttributeDescription = attributeDesc;
	m_VertexAttributeCount = attributes;
	return *this;
}

PipelineBuilder& PipelineBuilder::SetFrontFace(VkFrontFace frontFace)
{
	m_Rasterizer.frontFace = frontFace;
	return *this;
}

PipelineBuilder& PipelineBuilder::SetDepthBias(float constantFactor, float clamp, float slopeFactor)
{
	m_Rasterizer.depthBiasEnable = VK_TRUE;
	m_Rasterizer.depthBiasConstantFactor = constantFactor;
	m_Rasterizer.depthBiasClamp = clamp;
	m_Rasterizer.depthBiasSlopeFactor = slopeFactor;
	return *this;
}

PipelineBuilder& PipelineBuilder::SetCustomRasterizer(const VkPipelineRasterizationStateCreateInfo& rasterizerInfo)
{
	m_Rasterizer = rasterizerInfo;
	m_Rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	return *this;
}

PipelineBuilder& PipelineBuilder::SetCustomMultisampling(const VkPipelineMultisampleStateCreateInfo& multisamplingInfo)
{
	m_Multisampling = multisamplingInfo;
	m_Multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	return *this;
}

PipelineBuilder& PipelineBuilder::SetCustomDepthStencil(const VkPipelineDepthStencilStateCreateInfo& depthStencilInfo)
{
	m_DepthStencil = depthStencilInfo;
	m_DepthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	return *this;
}

PipelineBuilder& PipelineBuilder::AddColorBlendAttachment(const VkPipelineColorBlendAttachmentState& attachment)
{
	m_ColorBlendAttachments.emplace_back(attachment);
	return *this;
}

PipelineBuilder& PipelineBuilder::SetColorBlendingOp(VkLogicOp op)
{
	m_ColorBlending.logicOpEnable = VK_TRUE;
	m_ColorBlending.logicOp		= op;
	return *this;
}

void PipelineBuilder::Build(std::unique_ptr<Pipeline>& pipeline, Device* device, const VkExtent2D& viewportExtent, VkPipelineLayout layout, VkRenderPass renderPass)
{
	pipeline.reset(new Pipeline(std::move(Build(device, viewportExtent, layout, renderPass))));
}

Pipeline PipelineBuilder::Build(Device* device, const VkExtent2D& viewportExtent, VkPipelineLayout layout, VkRenderPass renderPass)
{
	Pipeline pipeline;
	VkPipelineDynamicStateCreateInfo dynamicState{};
	dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamicState.dynamicStateCount = static_cast<uint32_t>(m_DynamicStates.size());
	dynamicState.pDynamicStates = m_DynamicStates.data();

	VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
	vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	if (m_VertexAttributeCount)
	{
		vertexInputInfo.vertexBindingDescriptionCount = 1;
		vertexInputInfo.pVertexBindingDescriptions = &m_VertexBindingDescription;
		vertexInputInfo.vertexAttributeDescriptionCount = m_VertexAttributeCount;
		vertexInputInfo.pVertexAttributeDescriptions = m_VertexAttributeDescription;
	}

	VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
	inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssembly.topology = m_PrimitiveTopology;
	inputAssembly.primitiveRestartEnable = m_EnablePrimitiveRestart;

	VkViewport viewport{};
	viewport.x = .0f;
	viewport.y = .0f;
	viewport.width = (float)viewportExtent.width;
	viewport.height = (float)viewportExtent.height;
	viewport.minDepth = .0f;
	viewport.maxDepth = 1.f;

	VkRect2D scissor{};
	scissor.offset = {};
	scissor.extent = viewportExtent;

	VkPipelineViewportStateCreateInfo viewportState{};
	viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportState.viewportCount = 1;
	viewportState.pViewports = &viewport;
	viewportState.scissorCount = 1;
	viewportState.pScissors = &scissor;

	m_ColorBlending.attachmentCount = m_ColorBlendAttachments.size();
	m_ColorBlending.pAttachments = m_ColorBlendAttachments.data();

	std::vector<VkPipelineShaderStageCreateInfo> shaderStageInfos{};
	for (ShaderStage& shaderStage : m_ShaderStages)
		shaderStageInfos.push_back(shaderStage.GetInfo());

	VkGraphicsPipelineCreateInfo pipelineInfo{};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineInfo.pNext = (m_RenderingInfo.sType == VK_STRUCTURE_TYPE_MAX_ENUM) ? nullptr : &m_RenderingInfo;
	pipelineInfo.stageCount = shaderStageInfos.size();
	pipelineInfo.pStages = shaderStageInfos.data();
	pipelineInfo.pVertexInputState = &vertexInputInfo;
	pipelineInfo.pInputAssemblyState = &inputAssembly;
	pipelineInfo.pViewportState = &viewportState;
	pipelineInfo.pRasterizationState = &m_Rasterizer;
	pipelineInfo.pMultisampleState = &m_Multisampling;
	pipelineInfo.pDepthStencilState = &m_DepthStencil;
	pipelineInfo.pColorBlendState = &m_ColorBlending;
	pipelineInfo.pDynamicState = &dynamicState;
	pipelineInfo.layout = layout;
	pipelineInfo.renderPass = (m_RenderingInfo.sType == VK_STRUCTURE_TYPE_MAX_ENUM) ? renderPass : VK_NULL_HANDLE;
	pipelineInfo.subpass = 0;

  	if (vkCreateGraphicsPipelines(*device->GetDevicePtr(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline.m_Pipeline) != VK_SUCCESS)
		throw std::runtime_error("failed to create graphics pipeline");

	return pipeline;
}

