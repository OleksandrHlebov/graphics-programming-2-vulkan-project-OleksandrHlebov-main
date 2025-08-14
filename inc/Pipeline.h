#pragma once
#include <vulkan/vulkan.h>
#include <vector>
#include <memory>
#include "DeletionQueue.h"
#include "ShaderStage.h"

class Pipeline final
{
public:

	~Pipeline() = default;

	VkPipeline* GetPipelinePtr() { return &m_Pipeline; }

private:
	friend class PipelineBuilder;
	Pipeline() = default;

	VkPipeline m_Pipeline;
};

class PipelineBuilder final
{
public:
	PipelineBuilder();
	~PipelineBuilder() = default;
	
	PipelineBuilder(const PipelineBuilder&) 				= delete;
	PipelineBuilder(PipelineBuilder&&) noexcept 			= delete;
	PipelineBuilder& operator=(const PipelineBuilder&) 	 	= delete;
	PipelineBuilder& operator=(PipelineBuilder&&) noexcept 	= delete;

	// requires dynamic rendering feature enabled
	// if enabled render pass is forced to VK_NULL_HANDLE
	PipelineBuilder& EnableDynamicRendering(const std::vector<VkFormat>& colorAttachmentFormats, VkFormat depthAttachmentFormat, VkFormat stencilAttachmentFormat);

	PipelineBuilder& AddShaderStage(const ShaderStage& shaderStage);

	PipelineBuilder& AddDynamicState(VkDynamicState state);

	PipelineBuilder& SetPrimitiveTopology(VkPrimitiveTopology topology);

	PipelineBuilder& EnablePrimitiveRestart();

	PipelineBuilder& EnableDepthClamp();

	PipelineBuilder& EnableRasterizerDiscard();

	PipelineBuilder& EnableDepthTest(VkCompareOp op);

	PipelineBuilder& EnableDepthWrite();

	PipelineBuilder& EnableDepthBoundsTest();

	PipelineBuilder& SetPolygonMode(VkPolygonMode mode);

	PipelineBuilder& SetCullMode(VkCullModeFlags mode);

	PipelineBuilder& SetVertexDescription(VkVertexInputBindingDescription bindingDesc, VkVertexInputAttributeDescription* attributeDesc, uint32_t attributes);

	PipelineBuilder& SetFrontFace(VkFrontFace frontFace);

	PipelineBuilder& SetDepthBias(float constantFactor, float clamp, float slopeFactor);

	PipelineBuilder& SetCustomRasterizer(const VkPipelineRasterizationStateCreateInfo& rasterizerInfo);

	PipelineBuilder& SetCustomMultisampling(const VkPipelineMultisampleStateCreateInfo& multisamplingInfo);

	PipelineBuilder& SetCustomDepthStencil(const VkPipelineDepthStencilStateCreateInfo& depthStencilInfo);

	PipelineBuilder& AddColorBlendAttachment(const VkPipelineColorBlendAttachmentState& attachment);

	PipelineBuilder& SetColorBlendingOp(VkLogicOp op);

	void Build(std::unique_ptr<Pipeline>& pipeline, Device* device, const VkExtent2D& viewportExtent, VkPipelineLayout layout, VkRenderPass renderPass = VK_NULL_HANDLE);
	Pipeline Build(Device* device, const VkExtent2D& viewportExtent, VkPipelineLayout layout, VkRenderPass renderPass = VK_NULL_HANDLE);

private:
	std::vector<ShaderStage> m_ShaderStages;
	std::vector<VkDynamicState>	m_DynamicStates;
	std::vector<VkPipelineColorBlendAttachmentState> m_ColorBlendAttachments;
	VkPrimitiveTopology m_PrimitiveTopology{ VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST };
	VkBool32 m_EnablePrimitiveRestart{ VK_FALSE };
	DeletionQueue m_DeletionQueue{};
	VkPipelineRenderingCreateInfo m_RenderingInfo{ .sType = VK_STRUCTURE_TYPE_MAX_ENUM };
	VkPipelineRasterizationStateCreateInfo m_Rasterizer{};
	VkPipelineMultisampleStateCreateInfo m_Multisampling{};
	VkPipelineDepthStencilStateCreateInfo m_DepthStencil{};
	VkPipelineColorBlendStateCreateInfo m_ColorBlending{};
	VkVertexInputAttributeDescription* m_VertexAttributeDescription{};
	VkVertexInputBindingDescription	  m_VertexBindingDescription{};
	uint32_t m_VertexAttributeCount{};
};
