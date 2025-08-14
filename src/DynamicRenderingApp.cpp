#include "DynamicRenderingApp.h"
#include <iostream>

#include "DeletionQueue.h"
#include "Helper.h"

#include "Instance.h"
#include "DebugMessenger.h"
#include "Device.h"
#include "Subpass.h"
#include "RenderPass.h"
#include "DescriptorSetLayout.h"
#include "PipelineLayout.h"
#include "Pipeline.h"
#include "Image.h"
#include "CommandPool.h"
#include "Camera.h"
#include "Buffer.h"
#include "DescriptorPool.h"
#include "DescriptorSet.h"
#include <chrono>

#include <functional>
#include "Sampler.h"

DynamicRenderingApp::DynamicRenderingApp() = default;

DynamicRenderingApp::~DynamicRenderingApp() = default;

void DynamicRenderingApp::Run()
{
	// camera initialized before the mouse callback is set by window
	m_CameraPtr = std::make_unique<Camera>(glm::vec3{ .0f, .0f, 1.f }, 45.f, static_cast<float>(WIDTH) / HEIGHT, .01f, 50.f);
	InitWindow();
	InitVulkan();
	MainLoop();
	End();
}

void DynamicRenderingApp::KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	//DynamicRenderingApp* app = reinterpret_cast<DynamicRenderingApp*>(glfwGetWindowUserPointer(window));
	//app->m_CameraPtr->KeyboardStateChanged(key, scancode, action, mods);
}

void DynamicRenderingApp::MouseMovedCallback(GLFWwindow* window, double xpos, double ypos)
{
	//DynamicRenderingApp* app = reinterpret_cast<DynamicRenderingApp*>(glfwGetWindowUserPointer(window));
	//app->m_CameraPtr->MouseMoved(window, xpos, ypos);
}

void DynamicRenderingApp::RenderToCubeMap(ShaderStage* vertexShader, ShaderStage* pixelShader, Image* inputImage, Sampler* sampler, Image* outputCubeMapImage)
{
	DeletionQueue localDeletionQueue{};
	DescriptorSetLayoutBuilder setLayoutBuilder{};
	DescriptorSetLayout setLayout = setLayoutBuilder
		.AddBinding(0, VK_DESCRIPTOR_TYPE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
		.AddBinding(1, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, VK_SHADER_STAGE_FRAGMENT_BIT)
		.Build(*m_DevicePtr->GetDevicePtr());

	localDeletionQueue.Push([&]() { vkDestroyDescriptorSetLayout(*m_DevicePtr->GetDevicePtr(), *setLayout.GetLayoutPtr(), nullptr); });

	PipelineLayoutBuilder pipelineLayoutBuilder{};
	PipelineLayout pipelineLayout = pipelineLayoutBuilder
		.AddDescriptorSetLayout(&setLayout)
		.AddPushConstant(VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4) * 2)
		.Build(*m_DevicePtr->GetDevicePtr());

	localDeletionQueue.Push([&]() { vkDestroyPipelineLayout(*m_DevicePtr->GetDevicePtr(), *pipelineLayout.GetPipelineLayoutPtr(), nullptr); });

	std::vector<VkFormat> colorAttachmentFormats{ outputCubeMapImage->GetFormat() };

	VkPipelineColorBlendAttachmentState colorBlendAttachment{};
	colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT
		| VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	colorBlendAttachment.blendEnable = VK_FALSE;

	VkExtent2D singleFaceExtent{ outputCubeMapImage->GetExtent() };

	PipelineBuilder pipelineBuilder{};
	Pipeline pipeline = pipelineBuilder
		.AddShaderStage(*vertexShader)
		.AddShaderStage(*pixelShader)
		.AddDynamicState(VK_DYNAMIC_STATE_VIEWPORT)
		.AddDynamicState(VK_DYNAMIC_STATE_SCISSOR)
		.SetCullMode(VK_CULL_MODE_NONE)
		.SetFrontFace(VK_FRONT_FACE_COUNTER_CLOCKWISE)
		.SetPolygonMode(VK_POLYGON_MODE_FILL)
		.AddColorBlendAttachment(colorBlendAttachment)
		.EnableDynamicRendering(colorAttachmentFormats, m_DepthTexturePtr->GetFormat(), VK_FORMAT_UNDEFINED)
		.Build(m_DevicePtr.get(), singleFaceExtent, *pipelineLayout.GetPipelineLayoutPtr());

	localDeletionQueue.Push([&](){ vkDestroyPipeline(*m_DevicePtr->GetDevicePtr(), *pipeline.GetPipelinePtr(), nullptr); });

	DescriptorPoolBuilder poolBuilder{};
	DescriptorPool pool = poolBuilder
		.AddPoolSize(VK_DESCRIPTOR_TYPE_SAMPLER, 1)
		.AddPoolSize(VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1)
		.Build(m_DevicePtr.get(), 1);

	localDeletionQueue.Push([&]() { pool.Destroy(*m_DevicePtr->GetDevicePtr()); });
	
	DescriptorSetBuilder setBuilder{};
	DescriptorSet set = setBuilder.Build(m_DevicePtr.get(), 1, *pool.GetDescriptorPoolPtr(), setLayout.GetLayoutPtr());

	set
		.AddWriteDescriptorSet(*m_TextureSamplerPtr->GetSamplerPtr(), 0, 0)
		.AddWriteDescriptorSet(inputImage, 1, 0)
		.Update(m_DevicePtr.get());

	SingleTimeCommand commandBuffer = m_CommandPoolPtr->AllocateSingleTimeCommand(*m_DevicePtr->GetDevicePtr());
	commandBuffer.Start();

	glm::vec3 eye{ .0f };

	glm::mat4 captureViews[]
	{
		glm::lookAt(eye, eye + glm::vec3( 1.f,  .0f,  .0f), glm::vec3(.0f, .0f, -1.f)),		// +X
		glm::lookAt(eye, eye + glm::vec3(-1.f,  .0f,  .0f), glm::vec3(.0f, .0f, -1.f)),		// -X
		glm::lookAt(eye, eye + glm::vec3( .0f,  .0f, -1.f), glm::vec3(.0f, 1.f, .0f)),		// -Z
		glm::lookAt(eye, eye + glm::vec3( .0f,  .0f,  1.f), glm::vec3(.0f, -1.f, .0f)),		// +Z
		glm::lookAt(eye, eye + glm::vec3( .0f, -1.f,  .0f), glm::vec3(.0f, .0f, -1.f)),		// -Y
		glm::lookAt(eye, eye + glm::vec3( .0f,  1.f,  .0f), glm::vec3(.0f, .0f, 1.f))		// +Y
	};
	glm::mat4 captureProj{ glm::perspective(glm::radians(90.f), 1.f, .1f, 10.f) };
	captureProj[1][1] *= -1.f;

	for (uint32_t index{}; index < 6; ++index)
	{
		{
			Image::Transition transition{};
			{
				transition.newLayout = VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL;
				transition.srcStage = VK_PIPELINE_STAGE_2_NONE;
				transition.dstStage = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
				transition.srcAccess = VK_ACCESS_2_NONE;
				transition.dstAccess = VK_ACCESS_2_COLOR_ATTACHMENT_READ_BIT;
			}
			inputImage->MakeTransition(m_DevicePtr.get(), &commandBuffer, transition);
		}

		{
			Image::Transition transition{};
			{
				transition.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
				transition.srcStage = VK_PIPELINE_STAGE_2_NONE;
				transition.dstStage = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
				transition.srcAccess = VK_ACCESS_2_NONE;
				transition.dstAccess = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
				transition.baseLayerLevel = index;
			}
			outputCubeMapImage->MakeTransition(m_DevicePtr.get(), &commandBuffer, transition);
		}

		{
			VkRenderingAttachmentInfo colorAttachment{};
			{
				VkClearValue clearColor = { { .0f, .0f, .0f, 1.f } };
				colorAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
				colorAttachment.imageView = outputCubeMapImage->GetViews()[index + 1];
				colorAttachment.imageLayout = outputCubeMapImage->GetCurrentLayout();
				colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
				colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
				colorAttachment.clearValue = clearColor;
			}

			VkRenderingAttachmentInfo attachments[]{ colorAttachment };

			VkRenderingInfo renderingInfo{};
			{
				renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
				renderingInfo.renderArea = VkRect2D{ VkOffset2D{}, singleFaceExtent };
				renderingInfo.layerCount = 1;
				renderingInfo.colorAttachmentCount = std::size(attachments);
				renderingInfo.pColorAttachments = attachments;
				renderingInfo.pDepthAttachment = nullptr;
			}

			float colour[4]{ 1.f, .0f, .0f, 1.f };
			commandBuffer.BeginLabel("render to cube map", colour);
			vkCmdBeginRendering(*commandBuffer.GetBufferPtr(), &renderingInfo);
			VkDescriptorSet descSets[]{ *set.GetDescriptorSetPtr() };
			vkCmdBindPipeline(*commandBuffer.GetBufferPtr(), VK_PIPELINE_BIND_POINT_GRAPHICS, *pipeline.GetPipelinePtr());
			vkCmdBindDescriptorSets(*commandBuffer.GetBufferPtr(), VK_PIPELINE_BIND_POINT_GRAPHICS,
									*pipelineLayout.GetPipelineLayoutPtr(), 0, std::size(descSets), descSets, 0, nullptr);

			vkCmdPushConstants(*commandBuffer.GetBufferPtr(), *pipelineLayout.GetPipelineLayoutPtr(), VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4), &captureProj);
			vkCmdPushConstants(*commandBuffer.GetBufferPtr(), *pipelineLayout.GetPipelineLayoutPtr(), VK_SHADER_STAGE_VERTEX_BIT, sizeof(glm::mat4), sizeof(glm::mat4), &captureViews[index]);
			 
			VkViewport viewport{};
			viewport.x = .0f;
			viewport.y = .0f;
			viewport.width = static_cast<float>(singleFaceExtent.width);
			viewport.height = static_cast<float>(singleFaceExtent.height);
			viewport.minDepth = .0f;
			viewport.maxDepth = 1.f;
			vkCmdSetViewport(*commandBuffer.GetBufferPtr(), 0, 1, &viewport);

			VkRect2D scissor{};
			scissor.offset = { 0, 0 };
			scissor.extent = singleFaceExtent;
			vkCmdSetScissor(*commandBuffer.GetBufferPtr(), 0, 1, &scissor);
			vkCmdDraw(*commandBuffer.GetBufferPtr(), 36, 1, 0, 0);
			vkCmdEndRendering(*commandBuffer.GetBufferPtr());
		}
	}
	commandBuffer.EndLabel();
	commandBuffer.End(m_DevicePtr.get());
	outputCubeMapImage->DestroyExtraViews(m_DevicePtr.get());
}

void DynamicRenderingApp::GenerateShadowMap()
{
	{
		ImageBuilder builder{};
		builder
			.SetAspect(m_DepthTexturePtr->GetAspect())
			.SetFormat(m_DepthTexturePtr->GetFormat())
			.SetDimensions(1024, 1024);
		for (int index{}; index < m_DirectionalLights.size(); ++index)
		{
			m_ShadowDepthMaps.emplace_back
			(
				builder.Build(m_DevicePtr.get(), m_CommandPoolPtr.get(), VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)
			);
			m_DevicePtr->SetObjectName(VK_OBJECT_TYPE_IMAGE_VIEW, (uint64_t)*m_ShadowDepthMaps[index].GetFirstViewPtr(), "Shadow depth image view");
			m_DevicePtr->SetObjectName(VK_OBJECT_TYPE_IMAGE, (uint64_t)*m_ShadowDepthMaps[index].GetImagePtr(), "Shadow depth image");
			m_DeletionQueue.Push([&, index]() { m_ShadowDepthMaps[index].Destroy(*m_DevicePtr->GetDevicePtr()); });
		}
	}

	// transition to attachment optimal
	{
		SingleTimeCommand command = m_CommandPoolPtr->AllocateSingleTimeCommand(*m_DevicePtr->GetDevicePtr());

		command.Start();

		Image::Transition transition{};
		transition.newLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		transition.srcAccess = 0;
		transition.dstAccess = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		transition.srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		transition.dstStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
		for (Image& image : m_ShadowDepthMaps)
			image.MakeTransition(m_DevicePtr.get(), &command, transition);

		command.End(m_DevicePtr.get());
	}

	DeletionQueue localDeletionQueue{};

	PipelineLayoutBuilder pipelineLayoutBuilder{};
	PipelineLayout pipelineLayout = pipelineLayoutBuilder
		.AddPushConstant(VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4) + sizeof(uint32_t))
		.AddPushConstant(VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(glm::mat4) + sizeof(uint32_t), sizeof(datatype::TextureIndices))
		.AddDescriptorSetLayout(m_GlobalSetLayoutPtr.get())
		.Build(*m_DevicePtr->GetDevicePtr());

	localDeletionQueue.Push([&]() { vkDestroyPipelineLayout(*m_DevicePtr->GetDevicePtr(), *pipelineLayout.GetPipelineLayoutPtr(), nullptr); });

	uint32_t textureCount{ static_cast<uint32_t>(m_ScenePtr->GetTextures().size()) };
	uint32_t lightCount{ static_cast<uint32_t>(m_DirectionalLights.size()) };

	auto vertShaderCode{ HELP::ReadFile("shaders\\shadow_prepass_vert.spv") };
	auto prepassShaderCode{ HELP::ReadFile("shaders\\shadow_prepass_frag.spv") };

	ShaderStage vertShaderStage{ m_DevicePtr.get(), vertShaderCode, VK_SHADER_STAGE_VERTEX_BIT };
	vertShaderStage.AddSpecialization(sizeof(uint32_t), 1, &lightCount);
	m_DevicePtr->SetObjectName(VK_OBJECT_TYPE_SHADER_MODULE, (uint64_t)vertShaderStage.GetModule(), "shadow vertex shader module");

	ShaderStage prepassShaderStage{ m_DevicePtr.get(), prepassShaderCode, VK_SHADER_STAGE_FRAGMENT_BIT };
	prepassShaderStage.AddSpecialization(sizeof(uint32_t), 1, &textureCount);
	m_DevicePtr->SetObjectName(VK_OBJECT_TYPE_SHADER_MODULE, (uint64_t)prepassShaderStage.GetModule(), "shadow prepass shader module");

	std::vector<VkFormat> colorAttachmentFormats{  };

	auto attributeDesc{ datatype::Vertex::GetAttributeDescriptions() };

	VkPipelineColorBlendAttachmentState colorBlendAttachment{};
	colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT
		| VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	colorBlendAttachment.blendEnable = VK_FALSE;

	PipelineBuilder pipelineBuilder{};
	Pipeline pipeline = pipelineBuilder
		.AddShaderStage(vertShaderStage)
		.AddShaderStage(prepassShaderStage)
		.SetCullMode(VK_CULL_MODE_BACK_BIT)
		.SetFrontFace(VK_FRONT_FACE_COUNTER_CLOCKWISE)
		.SetPolygonMode(VK_POLYGON_MODE_FILL)
		.EnableDepthTest(VK_COMPARE_OP_LESS)
		.EnableDepthWrite()
		.SetDepthBias(1.25f, .0f, 1.75f)
		.SetVertexDescription(datatype::Vertex::GetBindingDescription(), attributeDesc.data(), attributeDesc.size())
		.AddColorBlendAttachment(colorBlendAttachment)
		.EnableDynamicRendering(colorAttachmentFormats, m_ShadowDepthMaps[0].GetFormat(), VK_FORMAT_UNDEFINED)
		.Build(m_DevicePtr.get(), m_ShadowDepthMaps[0].GetExtent(), *pipelineLayout.GetPipelineLayoutPtr(), VK_NULL_HANDLE);
	m_DevicePtr->SetObjectName(VK_OBJECT_TYPE_PIPELINE, (uint64_t)*pipeline.GetPipelinePtr(), "Pipeline (shadow prepass)");

	vertShaderStage.Destroy(m_DevicePtr.get());
	prepassShaderStage.Destroy(m_DevicePtr.get());

	localDeletionQueue.Push([&]() { vkDestroyPipeline(*m_DevicePtr->GetDevicePtr(), *pipeline.GetPipelinePtr(), nullptr); });
	SingleTimeCommand commandBuffer = m_CommandPoolPtr->AllocateSingleTimeCommand(*m_DevicePtr->GetDevicePtr());
	commandBuffer.Start();

	for (int index{}; index < m_ShadowDepthMaps.size(); ++index)
	{
		Image& shadowDepthMap = m_ShadowDepthMaps[index];
		{
			Image::Transition transition{};
			{
				transition.newLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
				transition.srcStage = VK_PIPELINE_STAGE_2_NONE;
				transition.dstStage = VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT;
				transition.srcAccess = VK_ACCESS_2_NONE;
				transition.dstAccess = VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;;
			}
			shadowDepthMap.MakeTransition(m_DevicePtr.get(), &commandBuffer, transition);
		}

		// shadow depth prepass
		{
			VkRenderingAttachmentInfo depthAttachment{};
			{
				VkClearValue depthClearValue{};
				depthClearValue.depthStencil = { 1.f, 0 };

				depthAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
				depthAttachment.imageView = *shadowDepthMap.GetFirstViewPtr();
				depthAttachment.imageLayout = shadowDepthMap.GetCurrentLayout();
				depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
				depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
				depthAttachment.clearValue = depthClearValue;
			}

			VkRenderingInfo prepassRenderingInfo{};
			{
				prepassRenderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
				prepassRenderingInfo.renderArea = VkRect2D{ VkOffset2D{}, shadowDepthMap.GetExtent() };
				prepassRenderingInfo.layerCount = 1;
				prepassRenderingInfo.colorAttachmentCount = 0;
				prepassRenderingInfo.pColorAttachments = nullptr;
				prepassRenderingInfo.pDepthAttachment = &depthAttachment;
			}

			vkCmdBeginRendering(*commandBuffer.GetBufferPtr(), &prepassRenderingInfo);
			 
			{
				VkDescriptorSet descSets[]{ *m_GlobalDescriptorSets[m_CurrentFrame].GetDescriptorSetPtr() };
				vkCmdBindPipeline(*commandBuffer.GetBufferPtr(), VK_PIPELINE_BIND_POINT_GRAPHICS, *pipeline.GetPipelinePtr());
				vkCmdBindDescriptorSets(*commandBuffer.GetBufferPtr(), VK_PIPELINE_BIND_POINT_GRAPHICS,
										*pipelineLayout.GetPipelineLayoutPtr(), 0, std::size(descSets), descSets, 0, nullptr);

				std::vector<Mesh>& meshes{ m_ScenePtr->GetMeshes() };
				glm::mat4 model{ m_ScenePtr->GetModelMatrix() };
				float colour[4]{ 1.f, .0f, .0f, 1.f };
				commandBuffer.BeginLabel("Shadow prepass", colour);
				vkCmdPushConstants(*commandBuffer.GetBufferPtr(), *pipelineLayout.GetPipelineLayoutPtr(), VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4), &model);
				vkCmdPushConstants(*commandBuffer.GetBufferPtr(), *pipelineLayout.GetPipelineLayoutPtr(), VK_SHADER_STAGE_VERTEX_BIT, sizeof(glm::mat4), sizeof(uint32_t), &index);
				for (Mesh& mesh : meshes)
				{
					VkDeviceSize offsets[] = { 0 };

					vkCmdPushConstants(*commandBuffer.GetBufferPtr(), *pipelineLayout.GetPipelineLayoutPtr(), VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(glm::mat4) + sizeof(uint32_t), sizeof(datatype::TextureIndices), mesh.GetTextureIndices());

					vkCmdBindVertexBuffers(*commandBuffer.GetBufferPtr(), 0, 1, mesh.GetVertexBuffer()->GetBufferPtr(), offsets);
					vkCmdBindIndexBuffer(*commandBuffer.GetBufferPtr(), *mesh.GetIndexBuffer()->GetBufferPtr(), 0, VK_INDEX_TYPE_UINT32);

					vkCmdDrawIndexed(*commandBuffer.GetBufferPtr(), static_cast<uint32_t>(mesh.GetIndexBuffer()->GetSize() / sizeof(uint32_t)), 1, 0, 0, 0);
				}
				commandBuffer.EndLabel();
			}

			vkCmdEndRendering(*commandBuffer.GetBufferPtr());
		}

		{
			Image::Transition transition{};
			{
				transition.newLayout = VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL;
				transition.srcStage = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
				transition.dstStage = VK_PIPELINE_STAGE_2_NONE;
				transition.srcAccess = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
				transition.dstAccess = VK_ACCESS_2_NONE;;
			}
			shadowDepthMap.MakeTransition(m_DevicePtr.get(), &commandBuffer, transition);
		}
	}

	commandBuffer.End(m_DevicePtr.get());
}

void DynamicRenderingApp::InitWindow()
{
	if (!glfwInit())
		std::cerr << "Error initializing glfw\n";

	glfwSetErrorCallback(HELP::ErrorCallback);

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE); 
	 
	m_WindowPtr = glfwCreateWindow(WIDTH, HEIGHT, m_AppName.c_str(), nullptr, nullptr);
	glfwSetWindowUserPointer(m_WindowPtr, this);
	glfwSetFramebufferSizeCallback(m_WindowPtr, FramebufferResizeCallback);

	//glfwSetKeyCallback(m_WindowPtr, &DynamicRenderingApp::KeyCallback);
	//glfwSetCursorPosCallback(m_WindowPtr, &DynamicRenderingApp::MouseMovedCallback);

	m_DeletionQueue.Push([&]() { glfwDestroyWindow(m_WindowPtr); });
}

void DynamicRenderingApp::CreateDepthResources()
{
	VkFormat depthFormat = FindDepthFormat();
	bool hasStencil = HELP::HasStencilComponent(depthFormat);

	ImageBuilder builder{};
	builder
		.SetAspect(VK_IMAGE_ASPECT_DEPTH_BIT | (hasStencil * VK_IMAGE_ASPECT_STENCIL_BIT))
		.SetFormat(depthFormat)
		.SetDimensions(m_SwapChainPtr->GetExtentPtr()->width, m_SwapChainPtr->GetExtentPtr()->height)
		.Build(m_DepthTexturePtr, m_DevicePtr.get(), m_CommandPoolPtr.get(), VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	m_DevicePtr->SetObjectName(VK_OBJECT_TYPE_IMAGE_VIEW, (uint64_t)*m_DepthTexturePtr->GetFirstViewPtr(), "Depth image view");
	m_DevicePtr->SetObjectName(VK_OBJECT_TYPE_IMAGE, (uint64_t)*m_DepthTexturePtr->GetImagePtr(), "Depth image");

	// transition to attachment optimal
	{
		SingleTimeCommand command = m_CommandPoolPtr->AllocateSingleTimeCommand(*m_DevicePtr->GetDevicePtr());

		command.Start();

		Image::Transition transition{};
		transition.newLayout = VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL;
		transition.srcAccess = 0;
		transition.dstAccess = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		transition.srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		transition.dstStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
		m_DepthTexturePtr->MakeTransition(m_DevicePtr.get(), &command, transition);

		command.End(m_DevicePtr.get());
	}
}

void DynamicRenderingApp::CreateSurface()
{
	if (glfwCreateWindowSurface(*m_InstancePtr->GetInstancePtr(), m_WindowPtr, nullptr, &m_Surface) != VK_SUCCESS)
		throw std::runtime_error("failed to create window surface");

	m_DeletionQueue.Push(
		[&]()
		{
			vkDestroySurfaceKHR(*m_InstancePtr->GetInstancePtr(), m_Surface, nullptr);
		});
}

void DynamicRenderingApp::InitVulkan()
{
	// Create instance
	{
		std::vector<const char*> extensions = GetRequiredExtensions();
		DebugMessenger::SetMinMessageSeverity(VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT);
		InstanceBuilder builder{};
		if (ENABLE_VALIDATION_LAYERS)
			builder.AddDebugMessenger(*DebugMessenger::GetCreateInfo());
		builder
			.SetAppName(m_AppName)
			.AddMultipleRequiredExtensions(extensions)
			.AddMultipleValidationLayers(m_ValidationLayers)
			.Build(m_InstancePtr);

		m_DeletionQueue.Push([&]() { vkDestroyInstance(*m_InstancePtr->GetInstancePtr(), nullptr); });

		if (ENABLE_VALIDATION_LAYERS)
		{
			m_DebugMessengerPtr = std::make_unique<DebugMessenger>(m_InstancePtr->GetInstancePtr());

			m_DeletionQueue.Push(
				[&]()
				{
					auto function = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(*m_InstancePtr->GetInstancePtr(), "vkDestroyDebugUtilsMessengerEXT");
					if (function != nullptr)
						function(*m_InstancePtr->GetInstancePtr(), *m_DebugMessengerPtr->GetMessengerPtr(), nullptr);
				});
		}
	}

	CreateSurface();

	// create device
	{
		// disabled because assimp sometimes generates uvs with negative values or values bigger than 1
		VkPhysicalDeviceCustomBorderColorFeaturesEXT borderFeatures{};
		//borderFeatures.customBorderColors = VK_TRUE;
		VkPhysicalDeviceFeatures deviceFeatures{};
		deviceFeatures.samplerAnisotropy = VK_TRUE;
		deviceFeatures.depthBiasClamp = VK_TRUE;
		VkPhysicalDeviceVulkan13Features deviceFeatures13{};
		deviceFeatures13.dynamicRendering = VK_TRUE;
		deviceFeatures13.synchronization2 = VK_TRUE;
		VkPhysicalDeviceVulkan12Features deviceFeatures12{};
		deviceFeatures12.shaderSampledImageArrayNonUniformIndexing = VK_TRUE;

		DeviceBuilder builder{};
		builder
			.SetEnabledFeatures(deviceFeatures)
			.SetEnabledFeatures(deviceFeatures13)
			.SetEnabledFeatures(deviceFeatures12)
			.SetEnabledFeatures(borderFeatures)
			.AddMultipleExtensions(m_DeviceExtensions)
			.PreferDedicatedGPU()
			.Build(m_DevicePtr, m_InstancePtr.get(), m_Surface);

		m_DevicePtr->SetObjectName(VK_OBJECT_TYPE_DEVICE, (uint64_t)*m_DevicePtr->GetDevicePtr(), "Device");
		m_DevicePtr->SetObjectName(VK_OBJECT_TYPE_PHYSICAL_DEVICE, (uint64_t)*m_DevicePtr->GetPhysicalDevicePtr(), "Physical Device");
		m_DevicePtr->SetObjectName(VK_OBJECT_TYPE_QUEUE, (uint64_t)*m_DevicePtr->GetGraphicsQueuePtr(), "Graphics queue");
		m_DevicePtr->SetObjectName(VK_OBJECT_TYPE_QUEUE, (uint64_t)*m_DevicePtr->GetPresentQueuePtr(), "Present queue");

		m_DeletionQueue.Push([&]() { m_DevicePtr->Destroy(); });
	}

	// create swapchain
	{
		m_SwapchainBuilder.Build(m_SwapChainPtr, m_DevicePtr.get(), m_Surface, m_WindowPtr, MAX_FRAMES_IN_FLIGHT);
		m_DevicePtr->SetObjectName(VK_OBJECT_TYPE_SWAPCHAIN_KHR, (uint64_t)*m_SwapChainPtr->GetSwapchainPtr(), "Swapchain");
		m_DeletionQueue.Push([&]() { m_SwapChainPtr->Destroy(*m_DevicePtr->GetDevicePtr()); });
	}

	// create command pool
	{
		Device::QueueFamilyIndices queueFamilyIndices = m_DevicePtr->FindQueueFamilies(m_Surface);

		m_CommandPoolPtr = std::make_unique<CommandPool>(*m_DevicePtr->GetDevicePtr(), VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT, queueFamilyIndices.graphicsFamily.value());

		m_DevicePtr->SetObjectName(VK_OBJECT_TYPE_COMMAND_POOL, (uint64_t)*m_CommandPoolPtr->GetPoolPtr(), "Command pool");
		m_DeletionQueue.Push([&]() { m_CommandPoolPtr->Destroy(*m_DevicePtr->GetDevicePtr()); });
	}

	//HELP::LoadScene();
	m_ScenePtr->Load(m_DevicePtr.get(), m_CommandPoolPtr.get(), "resources\\Sponza.gltf");
	m_DeletionQueue.Push([&]() { m_ScenePtr->Flush(); });

	m_PointLights.emplace_back(glm::vec3{ 6.f, .0f, 1.f }, glm::vec3{ .577f, .0f, .0f }, 1521.f);
	m_PointLights.emplace_back(glm::vec3{ 2.f, .0f, 1.f }, glm::vec3{ .34f, .34f, .1f }, 1521.f);
	m_DirectionalLights.emplace_back(glm::normalize(glm::vec3{ .5f, .0f, -.5f }), glm::vec3{ .877f, .877f, .577f }, 100.0f);
	m_DirectionalLights.emplace_back(glm::normalize(glm::vec3{ .999f, .0f, -.577f }), glm::vec3{ .877f, .877f, .577f }, 75.0f);

	for (datatype::DirectionalLight& light : m_DirectionalLights)
		m_ScenePtr->CalculateLightViewProj(light);

	// create descriptor set layout for global values that are mostly unchanged
	{
		DescriptorSetLayoutBuilder builder{};
		builder
			.AddBinding(0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT) // point lights
			.AddBinding(1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_VERTEX_BIT) // directional lights
			.AddBinding(2, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, VK_SHADER_STAGE_FRAGMENT_BIT) // environment
			.AddBinding(3, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, VK_SHADER_STAGE_FRAGMENT_BIT) // ibl
			.AddBinding(4, VK_DESCRIPTOR_TYPE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT) // sampler
			.AddBinding(5, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, VK_SHADER_STAGE_FRAGMENT_BIT, m_ScenePtr->GetTextures().size()) // textures
			.Build(m_GlobalSetLayoutPtr, *m_DevicePtr->GetDevicePtr());
		m_DevicePtr->SetObjectName(VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT, (uint64_t)*m_GlobalSetLayoutPtr->GetLayoutPtr(), "Global descriptor set layout");
		
		m_DeletionQueue.Push([&]() { vkDestroyDescriptorSetLayout(*m_DevicePtr->GetDevicePtr(), *m_GlobalSetLayoutPtr->GetLayoutPtr(), nullptr); });
	} 

	// create descriptor set layout for local data that might change 
	{
		DescriptorSetLayoutBuilder builder{};
		builder
			.AddBinding(0, VK_DESCRIPTOR_TYPE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT) // shadow sampler
			.AddBinding(1, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, VK_SHADER_STAGE_FRAGMENT_BIT, m_DirectionalLights.size()) // shadow map
			.Build(m_LocalSetLayoutPtr, *m_DevicePtr->GetDevicePtr());
		m_DevicePtr->SetObjectName(VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT, (uint64_t)*m_LocalSetLayoutPtr->GetLayoutPtr(), "Local descriptor set layout");

		m_DeletionQueue.Push([&]() { vkDestroyDescriptorSetLayout(*m_DevicePtr->GetDevicePtr(), *m_LocalSetLayoutPtr->GetLayoutPtr(), nullptr); });
	}

	// create GBuffer descriptor set layout that updates every frame
	{
		DescriptorSetLayoutBuilder builder{};
		builder
			.AddBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT) // UBO
			.AddBinding(1, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, VK_SHADER_STAGE_FRAGMENT_BIT) // albedo
			.AddBinding(2, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, VK_SHADER_STAGE_FRAGMENT_BIT) // material
			.AddBinding(3, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, VK_SHADER_STAGE_FRAGMENT_BIT) // depth
			.AddBinding(4, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, VK_SHADER_STAGE_FRAGMENT_BIT) // hdr render
			.Build(m_FrameDescriptorSetLayoutPtr, *m_DevicePtr->GetDevicePtr());
		m_DevicePtr->SetObjectName(VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT, (uint64_t)*m_FrameDescriptorSetLayoutPtr->GetLayoutPtr(), "Frame descriptor set layout");

		m_DeletionQueue.Push([&]() { vkDestroyDescriptorSetLayout(*m_DevicePtr->GetDevicePtr(), *m_FrameDescriptorSetLayoutPtr->GetLayoutPtr(), nullptr); });
	}

	// create prepass pipeline layout
	{
		PipelineLayoutBuilder builder{};
		builder
			.AddPushConstant(VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(datatype::TextureIndices))
			.AddDescriptorSetLayout(m_GlobalSetLayoutPtr.get())
			.AddDescriptorSetLayout(m_FrameDescriptorSetLayoutPtr.get())
			.Build(m_PrepassPipelineLayoutPtr, *m_DevicePtr->GetDevicePtr());

		m_DevicePtr->SetObjectName(VK_OBJECT_TYPE_PIPELINE_LAYOUT, (uint64_t)*m_PrepassPipelineLayoutPtr->GetPipelineLayoutPtr(), "Pipeline layout (prepass)");

		m_DeletionQueue.Push([&]() { vkDestroyPipelineLayout(*m_DevicePtr->GetDevicePtr(), *m_PrepassPipelineLayoutPtr->GetPipelineLayoutPtr(), nullptr); });
	}

	// create pipeline layout for gbuffer generation
	{
		PipelineLayoutBuilder builder{};
		builder
			.AddPushConstant(VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(datatype::TextureIndices))
			.AddDescriptorSetLayout(m_GlobalSetLayoutPtr.get())
			.AddDescriptorSetLayout(m_FrameDescriptorSetLayoutPtr.get())
			.Build(m_GBufferPipelineLayoutPtr, *m_DevicePtr->GetDevicePtr());

		m_DevicePtr->SetObjectName(VK_OBJECT_TYPE_PIPELINE_LAYOUT, (uint64_t)*m_GBufferPipelineLayoutPtr->GetPipelineLayoutPtr(), "Pipeline layout (gbuffer)");

		m_DeletionQueue.Push([&]() { vkDestroyPipelineLayout(*m_DevicePtr->GetDevicePtr(), *m_GBufferPipelineLayoutPtr->GetPipelineLayoutPtr(), nullptr); });
	}

	// create pipeline layout for lighting pass
	{
		PipelineLayoutBuilder builder{};
		builder
			.AddDescriptorSetLayout(m_GlobalSetLayoutPtr.get())
			.AddDescriptorSetLayout(m_LocalSetLayoutPtr.get())
			.AddDescriptorSetLayout(m_FrameDescriptorSetLayoutPtr.get())
			.Build(m_LightingPipelineLayoutPtr, *m_DevicePtr->GetDevicePtr());

		m_DevicePtr->SetObjectName(VK_OBJECT_TYPE_PIPELINE_LAYOUT, (uint64_t)*m_LightingPipelineLayoutPtr->GetPipelineLayoutPtr(), "Pipeline layout (lighting)");

		m_DeletionQueue.Push([&]() { vkDestroyPipelineLayout(*m_DevicePtr->GetDevicePtr(), *m_LightingPipelineLayoutPtr->GetPipelineLayoutPtr(), nullptr); });
	}

	// create depth resources
	{
		VkFormat depthFormat = FindDepthFormat();
		bool hasStencil = HELP::HasStencilComponent(depthFormat);

		ImageBuilder builder{};
		builder
			.SetAspect(VK_IMAGE_ASPECT_DEPTH_BIT | (hasStencil * VK_IMAGE_ASPECT_STENCIL_BIT))
			.SetFormat(depthFormat)
			.SetDimensions(m_SwapChainPtr->GetExtentPtr()->width, m_SwapChainPtr->GetExtentPtr()->height)
			.Build(m_DepthTexturePtr, m_DevicePtr.get(), m_CommandPoolPtr.get(), VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
		m_DevicePtr->SetObjectName(VK_OBJECT_TYPE_IMAGE_VIEW, (uint64_t)*m_DepthTexturePtr->GetFirstViewPtr(), "Depth image view");
		m_DevicePtr->SetObjectName(VK_OBJECT_TYPE_IMAGE, (uint64_t)*m_DepthTexturePtr->GetImagePtr(), "Depth image");

		// transition to attachment optimal
		{
			SingleTimeCommand command = m_CommandPoolPtr->AllocateSingleTimeCommand(*m_DevicePtr->GetDevicePtr());

			command.Start();

			Image::Transition transition{};
			transition.newLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
			transition.srcAccess = 0;
			transition.dstAccess = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
			transition.srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
			transition.dstStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
			m_DepthTexturePtr->MakeTransition(m_DevicePtr.get(), &command, transition);

			command.End(m_DevicePtr.get());
		}
		m_DeletionQueue.Push([&]() { m_DepthTexturePtr->Destroy(*m_DevicePtr->GetDevicePtr()); });
	}

	// create g-buffer
	{
		{
			ImageBuilder builder{};
			builder
				.SetAspect(VK_IMAGE_ASPECT_COLOR_BIT) 
				.SetFormat(VK_FORMAT_R8G8B8A8_SRGB)
				.SetDimensions(m_SwapChainPtr->GetExtentPtr()->width, m_SwapChainPtr->GetExtentPtr()->height)
				.Build(m_AlbedoTexturePtr, m_DevicePtr.get(), m_CommandPoolPtr.get(), VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
			m_DevicePtr->SetObjectName(VK_OBJECT_TYPE_IMAGE_VIEW, (uint64_t)*m_AlbedoTexturePtr->GetFirstViewPtr(), "Albedo image view");
			m_DevicePtr->SetObjectName(VK_OBJECT_TYPE_IMAGE, (uint64_t)*m_AlbedoTexturePtr->GetImagePtr(), "Albedo image");
		}

		{
			ImageBuilder builder{};
			builder
				.SetAspect(VK_IMAGE_ASPECT_COLOR_BIT)
				.SetFormat(VK_FORMAT_R16G16B16A16_UNORM)
				.SetDimensions(m_SwapChainPtr->GetExtentPtr()->width, m_SwapChainPtr->GetExtentPtr()->height)
				.Build(m_MaterialPropsTexturePtr, m_DevicePtr.get(), m_CommandPoolPtr.get(), VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
			m_DevicePtr->SetObjectName(VK_OBJECT_TYPE_IMAGE_VIEW, (uint64_t)*m_MaterialPropsTexturePtr->GetFirstViewPtr(), "Material properties image view");
			m_DevicePtr->SetObjectName(VK_OBJECT_TYPE_IMAGE, (uint64_t)*m_MaterialPropsTexturePtr->GetImagePtr(), "Material properties image");
		}

		{
			ImageBuilder builder{};
			builder
				.SetAspect(VK_IMAGE_ASPECT_COLOR_BIT)
				.SetFormat(VK_FORMAT_R32G32B32A32_SFLOAT)
				.SetDimensions(m_SwapChainPtr->GetExtentPtr()->width, m_SwapChainPtr->GetExtentPtr()->height)
				.Build(m_HDRRenderTargetPtr, m_DevicePtr.get(), m_CommandPoolPtr.get(), VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
			m_DevicePtr->SetObjectName(VK_OBJECT_TYPE_IMAGE_VIEW, (uint64_t)*m_HDRRenderTargetPtr->GetFirstViewPtr(), "HDR Render target view");
			m_DevicePtr->SetObjectName(VK_OBJECT_TYPE_IMAGE, (uint64_t)*m_HDRRenderTargetPtr->GetImagePtr(), "HDR Render target");
		}

		// transition to attachment optimal
		{
			SingleTimeCommand command = m_CommandPoolPtr->AllocateSingleTimeCommand(*m_DevicePtr->GetDevicePtr());

			command.Start();

			Image::Transition transition{};
			transition.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			transition.srcAccess = 0;
			transition.dstAccess = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_2_COLOR_ATTACHMENT_READ_BIT;
			transition.srcStage = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT;
			transition.dstStage = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
			m_AlbedoTexturePtr->MakeTransition(m_DevicePtr.get(), &command, transition);
			m_MaterialPropsTexturePtr->MakeTransition(m_DevicePtr.get(), &command, transition);
			m_HDRRenderTargetPtr->MakeTransition(m_DevicePtr.get(), &command, transition);

			command.End(m_DevicePtr.get());
		}
		m_DeletionQueue.Push(
			[&]()
			{
				m_AlbedoTexturePtr->Destroy(*m_DevicePtr->GetDevicePtr());
				m_MaterialPropsTexturePtr->Destroy(*m_DevicePtr->GetDevicePtr()); 
				m_HDRRenderTargetPtr->Destroy(*m_DevicePtr->GetDevicePtr());
			});
	}

	CreateTextureSampler(); 

	// render to cube map
	{
		auto vertexShaderCode{ HELP::ReadFile("shaders\\cubemap_vert.spv")};
		auto fragShaderCode{ HELP::ReadFile("shaders\\environment_frag.spv") };
		ShaderStage vertShaderStage{ m_DevicePtr.get(), vertexShaderCode, VK_SHADER_STAGE_VERTEX_BIT };
		m_DevicePtr->SetObjectName(VK_OBJECT_TYPE_SHADER_MODULE, (uint64_t)vertShaderStage.GetModule(), "vertex shader module");
		ShaderStage fragShaderStage{ m_DevicePtr.get(), fragShaderCode, VK_SHADER_STAGE_FRAGMENT_BIT };
		m_DevicePtr->SetObjectName(VK_OBJECT_TYPE_SHADER_MODULE, (uint64_t)fragShaderStage.GetModule(), "environment shader module");

		ImageBuilder builder{};
		Image inputImage = builder
			.SetFilePath("resources\\golden_gate_hills_4k.hdr")
			.SetFormat(VK_FORMAT_R32G32B32A32_SFLOAT)
			.Build(m_DevicePtr.get(), m_CommandPoolPtr.get(), VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

		m_DevicePtr->SetObjectName(VK_OBJECT_TYPE_IMAGE, (uint64_t)*inputImage.GetImagePtr(), "ibl source");
		m_DevicePtr->SetObjectName(VK_OBJECT_TYPE_IMAGE_VIEW, (uint64_t)*inputImage.GetFirstViewPtr(), "ibl source view");

		{
			ImageBuilder builder{};
			builder
				.SetAspect(VK_IMAGE_ASPECT_COLOR_BIT)
				.SetFormat(VK_FORMAT_R32G32B32A32_SFLOAT)
				.SetViewType(VK_IMAGE_VIEW_TYPE_CUBE)
				.SetLayers(6)
				.SetFlags(VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT)
				.SetDimensions(inputImage.GetExtent().width / 3, inputImage.GetExtent().width / 3)
				.Build(m_CubeMapPtr, m_DevicePtr.get(), m_CommandPoolPtr.get(), VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

			for (VkImageView& imageView : m_CubeMapPtr->GetViews())
				m_DevicePtr->SetObjectName(VK_OBJECT_TYPE_IMAGE_VIEW, (uint64_t)imageView, "cubemap view");

			m_DevicePtr->SetObjectName(VK_OBJECT_TYPE_IMAGE, (uint64_t)*m_CubeMapPtr->GetImagePtr(), "cubemap");
		}

		// transition to attachment optimal
		{
			SingleTimeCommand command = m_CommandPoolPtr->AllocateSingleTimeCommand(*m_DevicePtr->GetDevicePtr());

			command.Start();

			Image::Transition transition{};
			transition.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			transition.srcAccess = 0;
			transition.dstAccess = VK_ACCESS_2_COLOR_ATTACHMENT_READ_BIT;
			transition.srcStage = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT;
			transition.dstStage = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
			transition.layerCount = 6;
			m_CubeMapPtr->MakeTransition(m_DevicePtr.get(), &command, transition);

			command.End(m_DevicePtr.get());
		}

		m_DeletionQueue.Push(
			[&]()
			{
				m_CubeMapPtr->Destroy(*m_DevicePtr->GetDevicePtr());
			});

		RenderToCubeMap(&vertShaderStage, &fragShaderStage, &inputImage, m_TextureSamplerPtr.get(), m_CubeMapPtr.get());

		inputImage.Destroy(*m_DevicePtr->GetDevicePtr());
		fragShaderStage.Destroy(m_DevicePtr.get());
		vertShaderStage.Destroy(m_DevicePtr.get());
	}

	// render to irradiance map
	{
		auto vertexShaderCode{ HELP::ReadFile("shaders\\cubemap_vert.spv") };
		auto fragShaderCode{ HELP::ReadFile("shaders\\diffuse_irradiance_frag.spv") };
		ShaderStage vertShaderStage{ m_DevicePtr.get(), vertexShaderCode, VK_SHADER_STAGE_VERTEX_BIT };
		m_DevicePtr->SetObjectName(VK_OBJECT_TYPE_SHADER_MODULE, (uint64_t)vertShaderStage.GetModule(), "vertex shader module");
		ShaderStage fragShaderStage{ m_DevicePtr.get(), fragShaderCode, VK_SHADER_STAGE_FRAGMENT_BIT };
		m_DevicePtr->SetObjectName(VK_OBJECT_TYPE_SHADER_MODULE, (uint64_t)fragShaderStage.GetModule(), "irradiance shader module");

		{
			ImageBuilder builder{};
			builder
				.SetAspect(VK_IMAGE_ASPECT_COLOR_BIT)
				.SetFormat(VK_FORMAT_R32G32B32A32_SFLOAT)
				.SetViewType(VK_IMAGE_VIEW_TYPE_CUBE)
				.SetLayers(6)
				.SetFlags(VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT)
				.SetDimensions(64, 64)
				.Build(m_DiffuseIrradiancePtr, m_DevicePtr.get(), m_CommandPoolPtr.get(), VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

			for (VkImageView& imageView : m_DiffuseIrradiancePtr->GetViews())
				m_DevicePtr->SetObjectName(VK_OBJECT_TYPE_IMAGE_VIEW, (uint64_t)imageView, "irradiance view");

			m_DevicePtr->SetObjectName(VK_OBJECT_TYPE_IMAGE, (uint64_t)*m_DiffuseIrradiancePtr->GetImagePtr(), "irradiance");
		}

		{
			SingleTimeCommand command = m_CommandPoolPtr->AllocateSingleTimeCommand(*m_DevicePtr->GetDevicePtr());

			command.Start();

			{
				Image::Transition transition{};
				transition.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
				transition.srcAccess = 0;
				transition.dstAccess = VK_ACCESS_2_COLOR_ATTACHMENT_READ_BIT;
				transition.srcStage = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT;
				transition.dstStage = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
				transition.layerCount = 6;
				m_DiffuseIrradiancePtr->MakeTransition(m_DevicePtr.get(), &command, transition);
			}
			{
				Image::Transition transition{};
				transition.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
				transition.srcAccess = 0;
				transition.dstAccess = VK_ACCESS_2_COLOR_ATTACHMENT_READ_BIT;
				transition.srcStage = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT;
				transition.dstStage = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
				m_CubeMapPtr->MakeTransition(m_DevicePtr.get(), &command, transition);
			}

			command.End(m_DevicePtr.get());
		}

		m_DeletionQueue.Push(
			[&]()
			{
				m_DiffuseIrradiancePtr->Destroy(*m_DevicePtr->GetDevicePtr());
			});

		RenderToCubeMap(&vertShaderStage, &fragShaderStage, m_CubeMapPtr.get(), m_TextureSamplerPtr.get(), m_DiffuseIrradiancePtr.get());

		fragShaderStage.Destroy(m_DevicePtr.get());
		vertShaderStage.Destroy(m_DevicePtr.get());
	}

	// create pipelines
	{
		auto prepassShaderCode{ HELP::ReadFile("shaders\\depth_prepass_frag.spv") };
		auto vertShaderCode{ HELP::ReadFile("shaders\\basic_triangle_shader_vert.spv") };
		auto fragShaderCode{ HELP::ReadFile("shaders\\basic_fragment_shader_frag.spv") };
		auto quadShaderCode{ HELP::ReadFile("shaders\\quad_shader_vert.spv") };
		auto gbufferGenShaderCode{ HELP::ReadFile("shaders\\gbuffer_generation_frag.spv") };
		auto gbufferVertShaderCode{ HELP::ReadFile("shaders\\gbuffer_generation_vert.spv") };
		auto lightingShaderCode{ HELP::ReadFile("shaders\\lighting_frag.spv") };
		auto blitShaderCode{ HELP::ReadFile("shaders\\blit_frag.spv") };

		VkPipelineColorBlendAttachmentState colorBlendAttachment{};
		colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT
			| VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
		colorBlendAttachment.blendEnable = VK_FALSE;

		uint32_t textureCount{ static_cast<uint32_t>(m_ScenePtr->GetTextures().size()) };

		ShaderStage vertShaderStage{ m_DevicePtr.get(), vertShaderCode, VK_SHADER_STAGE_VERTEX_BIT };
		m_DevicePtr->SetObjectName(VK_OBJECT_TYPE_SHADER_MODULE, (uint64_t)vertShaderStage.GetModule(), "vertex shader module");

		ShaderStage prepassShaderStage{ m_DevicePtr.get(), prepassShaderCode, VK_SHADER_STAGE_FRAGMENT_BIT };
		prepassShaderStage.AddSpecialization(sizeof(uint32_t), 1, &textureCount);
		m_DevicePtr->SetObjectName(VK_OBJECT_TYPE_SHADER_MODULE, (uint64_t)prepassShaderStage.GetModule(), "prepass shader module");

		ShaderStage gbufferVertShaderStage{ m_DevicePtr.get(), gbufferVertShaderCode, VK_SHADER_STAGE_VERTEX_BIT };
		m_DevicePtr->SetObjectName(VK_OBJECT_TYPE_SHADER_MODULE, (uint64_t)vertShaderStage.GetModule(), "gbuffer vertex shader module");

		ShaderStage gbufferGenShaderStage{ m_DevicePtr.get(), gbufferGenShaderCode, VK_SHADER_STAGE_FRAGMENT_BIT };
		gbufferGenShaderStage.AddSpecialization(sizeof(uint32_t), 1, &textureCount);
		m_DevicePtr->SetObjectName(VK_OBJECT_TYPE_SHADER_MODULE, (uint64_t)gbufferGenShaderStage.GetModule(), "gbuffer gen shader module");

		ShaderStage quadShaderStage{ m_DevicePtr.get(), quadShaderCode, VK_SHADER_STAGE_VERTEX_BIT };
		m_DevicePtr->SetObjectName(VK_OBJECT_TYPE_SHADER_MODULE, (uint64_t)quadShaderStage.GetModule(), "quad shader module");

		ShaderStage fragShaderStage{ m_DevicePtr.get(), fragShaderCode, VK_SHADER_STAGE_FRAGMENT_BIT };
		m_DevicePtr->SetObjectName(VK_OBJECT_TYPE_SHADER_MODULE, (uint64_t)fragShaderStage.GetModule(), "fragment shader module");

		ShaderStage blitShaderStage{ m_DevicePtr.get(), blitShaderCode, VK_SHADER_STAGE_FRAGMENT_BIT };
		m_DevicePtr->SetObjectName(VK_OBJECT_TYPE_SHADER_MODULE, (uint64_t)blitShaderStage.GetModule(), "blit shader module");

		ShaderStage lightingShaderStage{ m_DevicePtr.get(), lightingShaderCode, VK_SHADER_STAGE_FRAGMENT_BIT };
		uint32_t lightCounts[]{ static_cast<uint32_t>(m_PointLights.size()), static_cast<uint32_t>(m_DirectionalLights.size()) };
		lightingShaderStage.AddSpecialization(sizeof(uint32_t), std::size(lightCounts), static_cast<void*>(lightCounts));
		m_DevicePtr->SetObjectName(VK_OBJECT_TYPE_SHADER_MODULE, (uint64_t)lightingShaderStage.GetModule(), "lighting shader module");

		// create prepass graphics pipeline 
		{
			std::vector<VkFormat> colorAttachmentFormats{  };

			auto attributeDesc{ datatype::Vertex::GetAttributeDescriptions() };

			PipelineBuilder builder{};
			builder
				.AddShaderStage(vertShaderStage)
				.AddShaderStage(prepassShaderStage)
				.AddDynamicState(VK_DYNAMIC_STATE_VIEWPORT)
				.AddDynamicState(VK_DYNAMIC_STATE_SCISSOR)
				.SetCullMode(VK_CULL_MODE_BACK_BIT)
				.SetFrontFace(VK_FRONT_FACE_COUNTER_CLOCKWISE)
				.SetPolygonMode(VK_POLYGON_MODE_FILL)
				.EnableDepthTest(VK_COMPARE_OP_LESS)
				.EnableDepthWrite()
				.SetVertexDescription(datatype::Vertex::GetBindingDescription(), attributeDesc.data(), attributeDesc.size())
				.AddColorBlendAttachment(colorBlendAttachment)
				.EnableDynamicRendering(colorAttachmentFormats, m_DepthTexturePtr->GetFormat(), VK_FORMAT_UNDEFINED)
				.Build(m_PrepassPipelinePtr, m_DevicePtr.get(), *m_SwapChainPtr->GetExtentPtr(), *m_PrepassPipelineLayoutPtr->GetPipelineLayoutPtr(), VK_NULL_HANDLE);
			m_DevicePtr->SetObjectName(VK_OBJECT_TYPE_PIPELINE, (uint64_t)*m_PrepassPipelinePtr->GetPipelinePtr(), "Pipeline (prepass)");

			m_DeletionQueue.Push([&]() { vkDestroyPipeline(*m_DevicePtr->GetDevicePtr(), *m_PrepassPipelinePtr->GetPipelinePtr(), nullptr); });
		}

		// create graphics pipeline for gbuffer generation
		{
			std::vector<VkFormat> colorAttachmentFormats{ m_AlbedoTexturePtr->GetFormat(), m_MaterialPropsTexturePtr->GetFormat() };

			auto attributeDesc{ datatype::Vertex::GetAttributeDescriptions() };

			PipelineBuilder builder{};
			builder
				.AddShaderStage(gbufferVertShaderStage)
				.AddShaderStage(gbufferGenShaderStage)
				.AddDynamicState(VK_DYNAMIC_STATE_VIEWPORT)
				.AddDynamicState(VK_DYNAMIC_STATE_SCISSOR)
				.SetCullMode(VK_CULL_MODE_BACK_BIT)
				.SetFrontFace(VK_FRONT_FACE_COUNTER_CLOCKWISE)
				.SetPolygonMode(VK_POLYGON_MODE_FILL)
				.SetVertexDescription(datatype::Vertex::GetBindingDescription(), attributeDesc.data(), attributeDesc.size())
				.AddColorBlendAttachment(colorBlendAttachment)
				.AddColorBlendAttachment(colorBlendAttachment)
				.EnableDepthTest(VK_COMPARE_OP_EQUAL)
				.EnableDynamicRendering(colorAttachmentFormats, m_DepthTexturePtr->GetFormat(), VK_FORMAT_UNDEFINED)
				.Build(m_GBufferPipelinePtr, m_DevicePtr.get(), *m_SwapChainPtr->GetExtentPtr(), *m_GBufferPipelineLayoutPtr->GetPipelineLayoutPtr(), VK_NULL_HANDLE);
			m_DevicePtr->SetObjectName(VK_OBJECT_TYPE_PIPELINE, (uint64_t)*m_GBufferPipelinePtr->GetPipelinePtr(), "Pipeline (gbuffer)");

			m_DeletionQueue.Push([&]() { vkDestroyPipeline(*m_DevicePtr->GetDevicePtr(), *m_GBufferPipelinePtr->GetPipelinePtr(), nullptr); });
		}

		// create graphics pipeline for lighting
		{
			std::vector<VkFormat> colorAttachmentFormats{ m_HDRRenderTargetPtr->GetFormat() };

			PipelineBuilder builder{};
			builder
				.AddShaderStage(quadShaderStage)
				.AddShaderStage(lightingShaderStage)
				.AddDynamicState(VK_DYNAMIC_STATE_VIEWPORT)
				.AddDynamicState(VK_DYNAMIC_STATE_SCISSOR)
				.SetCullMode(VK_CULL_MODE_FRONT_BIT)
				.SetFrontFace(VK_FRONT_FACE_COUNTER_CLOCKWISE)
				.SetPolygonMode(VK_POLYGON_MODE_FILL)
				.AddColorBlendAttachment(colorBlendAttachment)
				.EnableDynamicRendering(colorAttachmentFormats, m_DepthTexturePtr->GetFormat(), VK_FORMAT_UNDEFINED)
				.Build(m_LightingPipelinePtr, m_DevicePtr.get(), *m_SwapChainPtr->GetExtentPtr(), *m_LightingPipelineLayoutPtr->GetPipelineLayoutPtr(), VK_NULL_HANDLE);
			m_DevicePtr->SetObjectName(VK_OBJECT_TYPE_PIPELINE, (uint64_t)*m_LightingPipelinePtr->GetPipelinePtr(), "Pipeline (lighting)");

			m_DeletionQueue.Push([&]() { vkDestroyPipeline(*m_DevicePtr->GetDevicePtr(), *m_LightingPipelinePtr->GetPipelinePtr(), nullptr); });
		}

		// create graphics pipeline for blit
		{
			std::vector<VkFormat> colorAttachmentFormats{ *m_SwapChainPtr->GetFormatPtr() };

			PipelineBuilder builder{};
			builder
				.AddShaderStage(quadShaderStage)
				.AddShaderStage(blitShaderStage)
				.AddDynamicState(VK_DYNAMIC_STATE_VIEWPORT)
				.AddDynamicState(VK_DYNAMIC_STATE_SCISSOR)
				.SetCullMode(VK_CULL_MODE_FRONT_BIT)
				.SetFrontFace(VK_FRONT_FACE_COUNTER_CLOCKWISE)
				.SetPolygonMode(VK_POLYGON_MODE_FILL)
				.AddColorBlendAttachment(colorBlendAttachment)
				.EnableDynamicRendering(colorAttachmentFormats, m_DepthTexturePtr->GetFormat(), VK_FORMAT_UNDEFINED)
				.Build(m_BlitPipelinePtr, m_DevicePtr.get(), *m_SwapChainPtr->GetExtentPtr(), *m_LightingPipelineLayoutPtr->GetPipelineLayoutPtr(), VK_NULL_HANDLE);
			m_DevicePtr->SetObjectName(VK_OBJECT_TYPE_PIPELINE, (uint64_t)*m_BlitPipelinePtr->GetPipelinePtr(), "Pipeline (blit)");

			m_DeletionQueue.Push([&]() { vkDestroyPipeline(*m_DevicePtr->GetDevicePtr(), *m_BlitPipelinePtr->GetPipelinePtr(), nullptr); });
		}

		prepassShaderStage.Destroy(m_DevicePtr.get());
		vertShaderStage.Destroy(m_DevicePtr.get());
		fragShaderStage.Destroy(m_DevicePtr.get());
		quadShaderStage.Destroy(m_DevicePtr.get());
		gbufferGenShaderStage.Destroy(m_DevicePtr.get());
		gbufferVertShaderStage.Destroy(m_DevicePtr.get());
		lightingShaderStage.Destroy(m_DevicePtr.get());
		blitShaderStage.Destroy(m_DevicePtr.get());
	}

	CreateSyncObjects();

	// create model view projection buffer
	{
		VkDeviceSize bufferSize{ sizeof(datatype::ModelViewProjection) };

		BufferBuilder builder{};
		builder
			.MapMemory()
			.Build(m_MVPUBuffers, m_DevicePtr.get(), m_CommandPoolPtr.get(), MAX_FRAMES_IN_FLIGHT, bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
				   VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

		for (Buffer& buffer : m_MVPUBuffers)
		{
			m_DevicePtr->SetObjectName(VK_OBJECT_TYPE_BUFFER, (uint64_t)*buffer.GetBufferPtr(), "MVP");
		}

		m_DeletionQueue.Push(
			[&]()
			{
				for (Buffer& buffer : m_MVPUBuffers)  
					buffer.Destroy(m_DevicePtr.get());
			});
	}

	// create point light buffers
	{
		VkDeviceSize bufferSize	{ m_PointLights.size() * sizeof(datatype::PointLight) };

		BufferBuilder builder{};
		builder
			.MapMemory()
			.Build(m_PointLightsSSBO, m_DevicePtr.get(), m_CommandPoolPtr.get(), MAX_FRAMES_IN_FLIGHT, bufferSize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
				   VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

		for (Buffer& buffer : m_PointLightsSSBO)
		{
			const size_t pointLightsSize{ m_PointLights.size() * sizeof(datatype::PointLight) };
			buffer.UpdateMappedData(m_PointLights.data(), pointLightsSize, 0);
			m_DevicePtr->SetObjectName(VK_OBJECT_TYPE_BUFFER, (uint64_t)*buffer.GetBufferPtr(), "Lights");
		}

		m_DeletionQueue.Push(
			[&]()
			{
				for (Buffer& buffer : m_PointLightsSSBO)
					buffer.Destroy(m_DevicePtr.get());
			});
	}

	// create point directional buffers
	{
		VkDeviceSize bufferSize{ m_DirectionalLights.size() * sizeof(datatype::DirectionalLight) };

		BufferBuilder builder{};
		builder
			.MapMemory()
			.Build(m_DirectionalLightsSSBO, m_DevicePtr.get(), m_CommandPoolPtr.get(), MAX_FRAMES_IN_FLIGHT, bufferSize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
				   VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

		for (Buffer& buffer : m_DirectionalLightsSSBO)
		{
			const size_t dirLightsSize{ m_DirectionalLights.size() * sizeof(datatype::DirectionalLight) };
			buffer.UpdateMappedData(m_DirectionalLights.data(), dirLightsSize, 0);
			m_DevicePtr->SetObjectName(VK_OBJECT_TYPE_BUFFER, (uint64_t)*buffer.GetBufferPtr(), "Lights");
		}

		m_DeletionQueue.Push(
			[&]()
			{
				for (Buffer& buffer : m_DirectionalLightsSSBO)
					buffer.Destroy(m_DevicePtr.get());
			});
	}

	// create descriptor pool
	{
		DescriptorPoolBuilder builder{};
		builder
			.AddPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, MAX_FRAMES_IN_FLIGHT) // mvp
			.AddPoolSize(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, MAX_FRAMES_IN_FLIGHT) // light data
			.AddPoolSize(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, MAX_FRAMES_IN_FLIGHT) // light data
			.AddPoolSize(VK_DESCRIPTOR_TYPE_SAMPLER, MAX_FRAMES_IN_FLIGHT) // sampler
			.AddPoolSize(VK_DESCRIPTOR_TYPE_SAMPLER, MAX_FRAMES_IN_FLIGHT) // sampler
			.AddPoolSize(VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, MAX_FRAMES_IN_FLIGHT) // textures
			.AddPoolSize(VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, MAX_FRAMES_IN_FLIGHT) // depth
			.AddPoolSize(VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, MAX_FRAMES_IN_FLIGHT) // albedo
			.AddPoolSize(VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, MAX_FRAMES_IN_FLIGHT) // material props
			.AddPoolSize(VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, MAX_FRAMES_IN_FLIGHT) // hdr render
			.AddPoolSize(VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, MAX_FRAMES_IN_FLIGHT) // environment
			.AddPoolSize(VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, MAX_FRAMES_IN_FLIGHT) // ibl
			.AddPoolSize(VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, MAX_FRAMES_IN_FLIGHT) // shadow depth maps
			.Build(m_DescriptorPoolPtr, m_DevicePtr.get(), 3 * MAX_FRAMES_IN_FLIGHT);
		m_DevicePtr->SetObjectName(VK_OBJECT_TYPE_DESCRIPTOR_POOL, (uint64_t)*m_DescriptorPoolPtr->GetDescriptorPoolPtr(), "Descriptor pool");

		m_DeletionQueue.Push([&]() { m_DescriptorPoolPtr->Destroy(*m_DevicePtr->GetDevicePtr()); });
	}

	// create descriptor sets
	{
		{
			std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, *m_GlobalSetLayoutPtr->GetLayoutPtr());
			DescriptorSetBuilder builder{};
			builder
				.Build(m_GlobalDescriptorSets, m_DevicePtr.get(), MAX_FRAMES_IN_FLIGHT, *m_DescriptorPoolPtr->GetDescriptorPoolPtr(), layouts.data());
		}

		std::vector<Image>& textures{ m_ScenePtr->GetTextures() };

		{
			SingleTimeCommand command = m_CommandPoolPtr->AllocateSingleTimeCommand(*m_DevicePtr->GetDevicePtr());

			command.Start();

			Image::Transition transition{};
			transition.newLayout = VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL;
			transition.srcAccess = 0;
			transition.dstAccess = VK_ACCESS_2_SHADER_SAMPLED_READ_BIT;
			transition.srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
			transition.dstStage = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
			m_CubeMapPtr->MakeTransition(m_DevicePtr.get(), &command, transition);
			m_DiffuseIrradiancePtr->MakeTransition(m_DevicePtr.get(), &command, transition);

			command.End(m_DevicePtr.get());
		}

		for (size_t index{}; index < m_GlobalDescriptorSets.size(); ++index)
		{
			m_GlobalDescriptorSets[index]
				.AddWriteDescriptorSet(&m_PointLightsSSBO[index], 0, 0, 0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER)
				.AddWriteDescriptorSet(&m_DirectionalLightsSSBO[index], 0, 1, 0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER)
				.AddWriteDescriptorSet(m_CubeMapPtr.get(), 2, 0)
				.AddWriteDescriptorSet(m_DiffuseIrradiancePtr.get(), 3, 0)
				.AddWriteDescriptorSet(*m_TextureSamplerPtr->GetSamplerPtr(), 4, 0)
				.AddWriteDescriptorSet(textures, 5, 0)
				.Update(m_DevicePtr.get());
			m_DevicePtr->SetObjectName(VK_OBJECT_TYPE_DESCRIPTOR_SET, (uint64_t)*m_GlobalDescriptorSets[index].GetDescriptorSetPtr(), "Global descriptor set");
		}

		{
			std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, *m_LocalSetLayoutPtr->GetLayoutPtr());
			DescriptorSetBuilder builder{};
			builder
				.Build(m_LocalDescriptorSets, m_DevicePtr.get(), MAX_FRAMES_IN_FLIGHT, *m_DescriptorPoolPtr->GetDescriptorPoolPtr(), layouts.data());
		}

		GenerateShadowMap();

		for (size_t index{}; index < m_LocalDescriptorSets.size(); ++index)
		{
			m_LocalDescriptorSets[index]
				.AddWriteDescriptorSet(*m_ShadowSamplerPtr->GetSamplerPtr(), 0, 0)
				.AddWriteDescriptorSet(m_ShadowDepthMaps, 1, 0)
				.Update(m_DevicePtr.get());
				m_DevicePtr->SetObjectName(VK_OBJECT_TYPE_DESCRIPTOR_SET, (uint64_t)*m_LocalDescriptorSets[index].GetDescriptorSetPtr(), "Local descriptor set");
		}
		  
		{
			std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, *m_FrameDescriptorSetLayoutPtr->GetLayoutPtr());
			DescriptorSetBuilder builder{};
			builder
				.Build(m_FrameDescriptorSets, m_DevicePtr.get(), MAX_FRAMES_IN_FLIGHT, *m_DescriptorPoolPtr->GetDescriptorPoolPtr(), layouts.data());
		}

		{
			SingleTimeCommand command = m_CommandPoolPtr->AllocateSingleTimeCommand(*m_DevicePtr->GetDevicePtr());

			command.Start();

			Image::Transition transition{};
			transition.newLayout = VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_OPTIMAL;
			transition.srcAccess = 0;
			transition.dstAccess = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_2_SHADER_SAMPLED_READ_BIT;
			transition.srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
			transition.dstStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
			m_DepthTexturePtr->MakeTransition(m_DevicePtr.get(), &command, transition);

			command.End(m_DevicePtr.get());
		}

		for (size_t index{}; index < m_FrameDescriptorSets.size(); ++index)
		{
			m_FrameDescriptorSets[index]
				.AddWriteDescriptorSet(&m_MVPUBuffers[index], 0, 0, 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER)
				.AddWriteDescriptorSet(m_AlbedoTexturePtr.get(), 1, 0)
				.AddWriteDescriptorSet(m_MaterialPropsTexturePtr.get(), 2, 0)
				.AddWriteDescriptorSet(m_DepthTexturePtr.get(), 3, 0)
				.AddWriteDescriptorSet(m_HDRRenderTargetPtr.get(), 4, 0)
				.Update(m_DevicePtr.get());
				m_DevicePtr->SetObjectName(VK_OBJECT_TYPE_DESCRIPTOR_SET, (uint64_t)*m_FrameDescriptorSets[index].GetDescriptorSetPtr(), "Frame descriptor set");
		}
	}

	m_CommandPoolPtr->AllocateCommandBuffers(m_CommandBuffers, *m_DevicePtr->GetDevicePtr(), MAX_FRAMES_IN_FLIGHT, VK_COMMAND_BUFFER_LEVEL_PRIMARY);
	for (CommandBuffer& commandBuffer : m_CommandBuffers)
		m_DevicePtr->SetObjectName(VK_OBJECT_TYPE_COMMAND_BUFFER, (uint64_t)*commandBuffer.GetBufferPtr(), "Command buffer");
}


std::vector<const char*> DynamicRenderingApp::GetRequiredExtensions()
{
	uint32_t glfwExtensionCount{};
	const char** glfwExtensions;
	glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

	std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

	if (ENABLE_VALIDATION_LAYERS)
		extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

	return extensions;
}

void DynamicRenderingApp::CreateTextureSampler()
{
	{
		VkPhysicalDeviceProperties properties{};
		vkGetPhysicalDeviceProperties(*m_DevicePtr->GetPhysicalDevicePtr(), &properties);

		SamplerBuilder builder{};
		builder
			.SetAddressMode(VK_SAMPLER_ADDRESS_MODE_REPEAT)
			.SetFilter(VK_FILTER_LINEAR)
			.SetMaxAnisotropy(properties.limits.maxSamplerAnisotropy)
			.Build(m_TextureSamplerPtr, m_DevicePtr.get());
		m_DevicePtr->SetObjectName(VK_OBJECT_TYPE_SAMPLER, (uint64_t)*m_TextureSamplerPtr->GetSamplerPtr(), "Sampler");

		m_DeletionQueue.Push([&]() { m_TextureSamplerPtr->Destroy(m_DevicePtr.get()); });
	}
	{
		VkPhysicalDeviceProperties properties{};
		vkGetPhysicalDeviceProperties(*m_DevicePtr->GetPhysicalDevicePtr(), &properties);

		SamplerBuilder builder{};
		builder
			.SetAddressMode(VK_SAMPLER_ADDRESS_MODE_REPEAT)
			.SetFilter(VK_FILTER_LINEAR)
			.SetCompareOp(VK_COMPARE_OP_LESS)
			.Build(m_ShadowSamplerPtr, m_DevicePtr.get());
		m_DevicePtr->SetObjectName(VK_OBJECT_TYPE_SAMPLER, (uint64_t)*m_ShadowSamplerPtr->GetSamplerPtr(), "Shadow sampler");

		m_DeletionQueue.Push([&]() { m_ShadowSamplerPtr->Destroy(m_DevicePtr.get()); });
	}
}

VkFormat DynamicRenderingApp::FindDepthFormat()
{
	return m_DevicePtr->FindSupportedFormats({ VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
											 VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
}

void DynamicRenderingApp::CreateSyncObjects()
{
	m_ImageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
	m_RenderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
	m_InFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

	VkSemaphoreCreateInfo semaphoreInfo{};
	semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	VkFenceCreateInfo fenceInfo{};
	fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
	for (size_t index{}; index < MAX_FRAMES_IN_FLIGHT; ++index)
	{
		if (vkCreateSemaphore(*m_DevicePtr->GetDevicePtr(), &semaphoreInfo, nullptr, &m_ImageAvailableSemaphores[index]) != VK_SUCCESS ||
			vkCreateSemaphore(*m_DevicePtr->GetDevicePtr(), &semaphoreInfo, nullptr, &m_RenderFinishedSemaphores[index]) != VK_SUCCESS ||
			vkCreateFence(*m_DevicePtr->GetDevicePtr(), &fenceInfo, nullptr, &m_InFlightFences[index]) != VK_SUCCESS)
			throw std::runtime_error("failed to create semaphores");

		m_DeletionQueue.Push(
			[&, index]()
			{
				vkDestroySemaphore(*m_DevicePtr->GetDevicePtr(), m_ImageAvailableSemaphores[index], nullptr);

				vkDestroySemaphore(*m_DevicePtr->GetDevicePtr(), m_RenderFinishedSemaphores[index], nullptr);

				vkDestroyFence(*m_DevicePtr->GetDevicePtr(), m_InFlightFences[index], nullptr);
			});
	}

	VkFenceCreateInfo copyFenceInfo{};
	copyFenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
}

void DynamicRenderingApp::FramebufferResizeCallback(GLFWwindow* window, int width, int height)
{
	auto app = reinterpret_cast<DynamicRenderingApp*>(glfwGetWindowUserPointer(window));
	app->m_IsFramebufferResized = true;
}

void DynamicRenderingApp::RecreateSwapChain()
{
	int width{};
	int height{};
	glfwGetFramebufferSize(m_WindowPtr, &width, &height);
	while (width == 0 || height == 0)
	{
		glfwGetFramebufferSize(m_WindowPtr, &width, &height);
		glfwWaitEvents();
	}

	m_CameraPtr->SetAspectRatio(static_cast<float>(width) / height);

	vkDeviceWaitIdle(*m_DevicePtr->GetDevicePtr());

	m_DepthTexturePtr->Destroy(*m_DevicePtr->GetDevicePtr());

	m_SwapChainPtr->Destroy(*m_DevicePtr->GetDevicePtr());

	m_SwapchainBuilder.Build(m_SwapChainPtr, m_DevicePtr.get(), m_Surface, m_WindowPtr, MAX_FRAMES_IN_FLIGHT);

	CreateDepthResources();

	for (size_t index{}; index < m_FrameDescriptorSets.size(); ++index)
	{
		m_FrameDescriptorSets[index]
			.AddWriteDescriptorSet(m_DepthTexturePtr.get(), 3, 0)
			.Update(m_DevicePtr.get());
		m_DevicePtr->SetObjectName(VK_OBJECT_TYPE_DESCRIPTOR_SET, (uint64_t)*m_FrameDescriptorSets[index].GetDescriptorSetPtr(), "Frame descriptor set");
	}
}

void DynamicRenderingApp::RecordCommandBufferWithPrepass(CommandBuffer& commandBuffer, uint32_t imageIndex)
{
	commandBuffer.Start();
	Image& swapchainImage = m_SwapChainPtr->GetImages()[imageIndex];

	{
		Image::Transition transition{};
		{
			transition.newLayout	= VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
			transition.srcStage		= VK_PIPELINE_STAGE_2_NONE;
			transition.dstStage		= VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT;
			transition.srcAccess	= VK_ACCESS_2_NONE;
			transition.dstAccess	= VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;;
		}
		m_DepthTexturePtr->MakeTransition(m_DevicePtr.get(), &commandBuffer, transition);
	}

	// depth prepass
	{
		VkRenderingAttachmentInfo depthAttachment{};
		{
			VkClearValue depthClearValue{};
			depthClearValue.depthStencil = { 1.f, 0 };

			depthAttachment.sType		= VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
			depthAttachment.imageView	= *m_DepthTexturePtr->GetFirstViewPtr();
			depthAttachment.imageLayout = m_DepthTexturePtr->GetCurrentLayout();
			depthAttachment.loadOp		= VK_ATTACHMENT_LOAD_OP_CLEAR;
			depthAttachment.storeOp		= VK_ATTACHMENT_STORE_OP_STORE;
			depthAttachment.clearValue	= depthClearValue;
		}

		VkRenderingInfo prepassRenderingInfo{};
		{
			prepassRenderingInfo.sType					= VK_STRUCTURE_TYPE_RENDERING_INFO;
			prepassRenderingInfo.renderArea				= VkRect2D{ VkOffset2D{}, swapchainImage.GetExtent() };
			prepassRenderingInfo.layerCount				= 1;
			prepassRenderingInfo.colorAttachmentCount	= 0;
			prepassRenderingInfo.pColorAttachments		= nullptr;
			prepassRenderingInfo.pDepthAttachment		= &depthAttachment;
		}

		vkCmdBeginRendering(*commandBuffer.GetBufferPtr(), &prepassRenderingInfo);

		{
			VkDescriptorSet descSets[]{ *m_GlobalDescriptorSets[m_CurrentFrame].GetDescriptorSetPtr(), *m_FrameDescriptorSets[m_CurrentFrame].GetDescriptorSetPtr() };
			vkCmdBindPipeline(*commandBuffer.GetBufferPtr(), VK_PIPELINE_BIND_POINT_GRAPHICS, *m_PrepassPipelinePtr->GetPipelinePtr());
			vkCmdBindDescriptorSets(*commandBuffer.GetBufferPtr(), VK_PIPELINE_BIND_POINT_GRAPHICS, 
									*m_PrepassPipelineLayoutPtr->GetPipelineLayoutPtr(), 0, std::size(descSets), descSets, 0, nullptr);

			VkViewport viewport{};
			viewport.x = .0f;
			viewport.y = .0f;
			viewport.width = static_cast<float>(m_SwapChainPtr->GetExtentPtr()->width);
			viewport.height = static_cast<float>(m_SwapChainPtr->GetExtentPtr()->height);
			viewport.minDepth = .0f;
			viewport.maxDepth = 1.f;
			vkCmdSetViewport(*commandBuffer.GetBufferPtr(), 0, 1, &viewport);

			VkRect2D scissor{};
			scissor.offset = { 0, 0 };
			scissor.extent = *m_SwapChainPtr->GetExtentPtr();
			vkCmdSetScissor(*commandBuffer.GetBufferPtr(), 0, 1, &scissor);

			std::vector<Mesh>& meshes{ m_ScenePtr->GetMeshes() };

			float colour[4]{ 1.f, .0f, .0f, 1.f };
			commandBuffer.BeginLabel("Meshes prepass", colour);
			for (Mesh& mesh : meshes)
			{
				VkDeviceSize offsets[] = { 0 };

				vkCmdPushConstants(*commandBuffer.GetBufferPtr(), *m_PrepassPipelineLayoutPtr->GetPipelineLayoutPtr(), VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(datatype::TextureIndices), mesh.GetTextureIndices());

				vkCmdBindVertexBuffers(*commandBuffer.GetBufferPtr(), 0, 1, mesh.GetVertexBuffer()->GetBufferPtr(), offsets);
				vkCmdBindIndexBuffer(*commandBuffer.GetBufferPtr(), *mesh.GetIndexBuffer()->GetBufferPtr(), 0, VK_INDEX_TYPE_UINT32);

				vkCmdDrawIndexed(*commandBuffer.GetBufferPtr(), static_cast<uint32_t>(mesh.GetIndexBuffer()->GetSize() / sizeof(uint32_t)), 1, 0, 0, 0);
			}
			commandBuffer.EndLabel();
		}

		vkCmdEndRendering(*commandBuffer.GetBufferPtr());
	}

	{
		Image::Transition transition{};
		{
			transition.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
			transition.srcStage = VK_PIPELINE_STAGE_2_NONE;
			transition.dstStage = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
			transition.srcAccess = VK_ACCESS_2_NONE;
			transition.dstAccess = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
		}
		m_AlbedoTexturePtr->MakeTransition(m_DevicePtr.get(), &commandBuffer, transition);
		m_MaterialPropsTexturePtr->MakeTransition(m_DevicePtr.get(), &commandBuffer, transition);
	}

	{
		Image::Transition transition{};
		{
			transition.newLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
			transition.srcStage = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
			transition.dstStage = VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT;
			transition.srcAccess = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
			transition.dstAccess = VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
		}
		m_DepthTexturePtr->MakeTransition(m_DevicePtr.get(), &commandBuffer, transition);
	}

	// gbuffer generation
	{
		VkRenderingAttachmentInfo depthAttachment{};
		{
			VkClearValue depthClearValue{};
			depthClearValue.depthStencil = { 1.f, 0 };

			depthAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO; 
			depthAttachment.imageView = *m_DepthTexturePtr->GetFirstViewPtr();
			depthAttachment.imageLayout = m_DepthTexturePtr->GetCurrentLayout();
			depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
			depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
			depthAttachment.clearValue = depthClearValue;
		}

		VkRenderingAttachmentInfo albedoAttachment{};
		{
			VkClearValue clearValue{ { .0f, .0f, .0f, 1.f } };

			albedoAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
			albedoAttachment.imageView = *m_AlbedoTexturePtr->GetFirstViewPtr();
			albedoAttachment.imageLayout = m_AlbedoTexturePtr->GetCurrentLayout();
			albedoAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
			albedoAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
			albedoAttachment.clearValue = clearValue;
		}

		VkRenderingAttachmentInfo materialPropsAttachment{};
		{
			VkClearValue clearValue{ { .0f, .0f, .0f, 1.f } };

			materialPropsAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
			materialPropsAttachment.imageView = *m_MaterialPropsTexturePtr->GetFirstViewPtr();
			materialPropsAttachment.imageLayout = m_MaterialPropsTexturePtr->GetCurrentLayout();
			materialPropsAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
			materialPropsAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
			materialPropsAttachment.clearValue = clearValue;
		}

		VkRenderingAttachmentInfo attachments[]{ albedoAttachment, materialPropsAttachment };

		VkRenderingInfo prepassRenderingInfo{};
		{
			prepassRenderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
			prepassRenderingInfo.renderArea = VkRect2D{ VkOffset2D{}, swapchainImage.GetExtent() };
			prepassRenderingInfo.layerCount = 1;
			prepassRenderingInfo.colorAttachmentCount = std::size(attachments);
			prepassRenderingInfo.pColorAttachments = attachments;
			prepassRenderingInfo.pDepthAttachment = &depthAttachment;
		}

		vkCmdBeginRendering(*commandBuffer.GetBufferPtr(), &prepassRenderingInfo);

		{
			VkDescriptorSet descSets[]{ *m_GlobalDescriptorSets[m_CurrentFrame].GetDescriptorSetPtr(), *m_FrameDescriptorSets[m_CurrentFrame].GetDescriptorSetPtr() };
			vkCmdBindPipeline(*commandBuffer.GetBufferPtr(), VK_PIPELINE_BIND_POINT_GRAPHICS, *m_GBufferPipelinePtr->GetPipelinePtr());
			vkCmdBindDescriptorSets(*commandBuffer.GetBufferPtr(), VK_PIPELINE_BIND_POINT_GRAPHICS,
									*m_PrepassPipelineLayoutPtr->GetPipelineLayoutPtr(), 0, std::size(descSets), descSets, 0, nullptr);

			VkViewport viewport{};
			viewport.x = .0f;
			viewport.y = .0f;
			viewport.width = static_cast<float>(m_SwapChainPtr->GetExtentPtr()->width);
			viewport.height = static_cast<float>(m_SwapChainPtr->GetExtentPtr()->height);
			viewport.minDepth = .0f;
			viewport.maxDepth = 1.f;
			vkCmdSetViewport(*commandBuffer.GetBufferPtr(), 0, 1, &viewport);

			VkRect2D scissor{};
			scissor.offset = { 0, 0 };
			scissor.extent = *m_SwapChainPtr->GetExtentPtr();
			vkCmdSetScissor(*commandBuffer.GetBufferPtr(), 0, 1, &scissor);

			std::vector<Mesh>& meshes{ m_ScenePtr->GetMeshes() };

			float colour[4]{ 1.f, .0f, .0f, 1.f };
			commandBuffer.BeginLabel("Meshes gbuffer generation", colour);
			for (Mesh& mesh : meshes)
			{
				VkDeviceSize offsets[] = { 0 };

				vkCmdPushConstants(*commandBuffer.GetBufferPtr(), *m_GBufferPipelineLayoutPtr->GetPipelineLayoutPtr(), VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(datatype::TextureIndices), mesh.GetTextureIndices());

				vkCmdBindVertexBuffers(*commandBuffer.GetBufferPtr(), 0, 1, mesh.GetVertexBuffer()->GetBufferPtr(), offsets);
				vkCmdBindIndexBuffer(*commandBuffer.GetBufferPtr(), *mesh.GetIndexBuffer()->GetBufferPtr(), 0, VK_INDEX_TYPE_UINT32);

				vkCmdDrawIndexed(*commandBuffer.GetBufferPtr(), static_cast<uint32_t>(mesh.GetIndexBuffer()->GetSize() / sizeof(uint32_t)), 1, 0, 0, 0);
			}
			commandBuffer.EndLabel();
		}

		vkCmdEndRendering(*commandBuffer.GetBufferPtr());
	}

	{
		Image::Transition transition{};
		{
			transition.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
			transition.srcStage = VK_PIPELINE_STAGE_2_NONE;
			transition.dstStage = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
			transition.srcAccess = VK_ACCESS_2_NONE;
			transition.dstAccess = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
		}
		m_HDRRenderTargetPtr->MakeTransition(m_DevicePtr.get(), &commandBuffer, transition);
	}

	{
		Image::Transition transition{};
		{
			transition.newLayout = VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL;
			transition.srcStage = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
			transition.dstStage = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
			transition.srcAccess = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
			transition.dstAccess = VK_ACCESS_2_COLOR_ATTACHMENT_READ_BIT;
		}
		m_AlbedoTexturePtr->MakeTransition(m_DevicePtr.get(), &commandBuffer, transition);
		m_MaterialPropsTexturePtr->MakeTransition(m_DevicePtr.get(), &commandBuffer, transition);
		transition.layerCount = 6;
		m_CubeMapPtr->MakeTransition(m_DevicePtr.get(), &commandBuffer, transition);
	} 

	{
		Image::Transition transition{};
		{
			transition.newLayout	= VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL;
			transition.srcStage		= VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
			transition.dstStage		= VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT;
			transition.srcAccess	= VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
			transition.dstAccess	= VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT;;
		}
		m_DepthTexturePtr->MakeTransition(m_DevicePtr.get(), &commandBuffer, transition);
	}

	// lighting render pass
	{
		VkRenderingAttachmentInfo colorAttachment{};
		{
			VkClearValue clearColor = { { .0f, .0f, .0f, 1.f } };
			colorAttachment.sType		= VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
			colorAttachment.imageView	= *m_HDRRenderTargetPtr->GetFirstViewPtr();
			colorAttachment.imageLayout = m_HDRRenderTargetPtr->GetCurrentLayout();
			colorAttachment.loadOp		= VK_ATTACHMENT_LOAD_OP_CLEAR;
			colorAttachment.storeOp		= VK_ATTACHMENT_STORE_OP_STORE;
			colorAttachment.clearValue	= clearColor;
		}

		VkRenderingAttachmentInfo depthAttachment{};
		{
			VkClearValue depthClearValue{};
			depthClearValue.depthStencil = { 1.f, 0 };

			depthAttachment.sType		= VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
			depthAttachment.imageView	= *m_DepthTexturePtr->GetFirstViewPtr();
			depthAttachment.imageLayout = m_DepthTexturePtr->GetCurrentLayout();
			depthAttachment.loadOp		= VK_ATTACHMENT_LOAD_OP_LOAD;
			depthAttachment.storeOp		= VK_ATTACHMENT_STORE_OP_DONT_CARE;
			depthAttachment.clearValue	= depthClearValue;
		}

		VkRenderingAttachmentInfo attachments[] { colorAttachment };

		VkRenderingInfo renderingInfo{};
		{
			renderingInfo.sType					= VK_STRUCTURE_TYPE_RENDERING_INFO;
			renderingInfo.renderArea			= VkRect2D{ VkOffset2D{}, swapchainImage.GetExtent() };
			renderingInfo.layerCount			= 1;
			renderingInfo.colorAttachmentCount	= std::size(attachments);
			renderingInfo.pColorAttachments		= attachments;
			renderingInfo.pDepthAttachment		= &depthAttachment;
		}

		vkCmdBeginRendering(*commandBuffer.GetBufferPtr(), &renderingInfo);

		{
			VkDescriptorSet descSets[]{ *m_GlobalDescriptorSets[m_CurrentFrame].GetDescriptorSetPtr(), *m_LocalDescriptorSets[m_CurrentFrame].GetDescriptorSetPtr(), *m_FrameDescriptorSets[m_CurrentFrame].GetDescriptorSetPtr()};
			vkCmdBindPipeline(*commandBuffer.GetBufferPtr(), VK_PIPELINE_BIND_POINT_GRAPHICS, *m_LightingPipelinePtr->GetPipelinePtr());
			vkCmdBindDescriptorSets(*commandBuffer.GetBufferPtr(), VK_PIPELINE_BIND_POINT_GRAPHICS, 
									*m_LightingPipelineLayoutPtr->GetPipelineLayoutPtr(), 0, std::size(descSets), descSets, 0, nullptr);
									 
			VkViewport viewport{};
			viewport.x = .0f;
			viewport.y = .0f;
			viewport.width = static_cast<float>(m_SwapChainPtr->GetExtentPtr()->width);
			viewport.height = static_cast<float>(m_SwapChainPtr->GetExtentPtr()->height);
			viewport.minDepth = .0f;
			viewport.maxDepth = 1.f;
			vkCmdSetViewport(*commandBuffer.GetBufferPtr(), 0, 1, &viewport);

			VkRect2D scissor{};
			scissor.offset = { 0, 0 };
			scissor.extent = *m_SwapChainPtr->GetExtentPtr();
			vkCmdSetScissor(*commandBuffer.GetBufferPtr(), 0, 1, &scissor);

			std::vector<Mesh>& meshes{ m_ScenePtr->GetMeshes() };

			float colour[4]{ 1.f, .0f, .0f, 1.f };
			commandBuffer.BeginLabel("lighting", colour);
			vkCmdDraw(*commandBuffer.GetBufferPtr(), 3, 1, 0, 0);
			commandBuffer.EndLabel();
		}

		vkCmdEndRendering(*commandBuffer.GetBufferPtr());
	}

	{
		Image::Transition transition{};
		{
			transition.newLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
			transition.srcStage = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
			transition.dstStage = VK_PIPELINE_STAGE_2_NONE;
			transition.srcAccess = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
			transition.dstAccess = VK_ACCESS_2_NONE;
		}
		m_DepthTexturePtr->MakeTransition(m_DevicePtr.get(), &commandBuffer, transition);
	}

	{
		Image::Transition transition{};
		{
			transition.newLayout = VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL;
			transition.srcStage = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
			transition.dstStage = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
			transition.srcAccess = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
			transition.dstAccess = VK_ACCESS_2_COLOR_ATTACHMENT_READ_BIT;;
		}
		m_HDRRenderTargetPtr->MakeTransition(m_DevicePtr.get(), &commandBuffer, transition);
	}

	{
		Image::Transition transition{};
		{
			transition.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
			transition.srcStage = VK_PIPELINE_STAGE_2_NONE;
			transition.dstStage = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
			transition.srcAccess = VK_ACCESS_2_NONE;
			transition.dstAccess = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
		}
		swapchainImage.MakeTransition(m_DevicePtr.get(), &commandBuffer, transition);
	}

	// blit pass
	{
		VkRenderingAttachmentInfo colorAttachment{};
		{
			VkClearValue clearColor = { { .0f, .0f, .0f, 1.f } };
			colorAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
			colorAttachment.imageView = *swapchainImage.GetFirstViewPtr();
			colorAttachment.imageLayout = swapchainImage.GetCurrentLayout();
			colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
			colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
			colorAttachment.clearValue = clearColor;
		}

		VkRenderingAttachmentInfo attachments[]{ colorAttachment };

		VkRenderingInfo renderingInfo{};
		{
			renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
			renderingInfo.renderArea = VkRect2D{ VkOffset2D{}, swapchainImage.GetExtent() };
			renderingInfo.layerCount = 1;
			renderingInfo.colorAttachmentCount = std::size(attachments);
			renderingInfo.pColorAttachments = attachments;
			renderingInfo.pDepthAttachment = nullptr;
		}

		vkCmdBeginRendering(*commandBuffer.GetBufferPtr(), &renderingInfo);

		{
			VkDescriptorSet descSets[]{ *m_GlobalDescriptorSets[m_CurrentFrame].GetDescriptorSetPtr(), *m_LocalDescriptorSets[m_CurrentFrame].GetDescriptorSetPtr(), *m_FrameDescriptorSets[m_CurrentFrame].GetDescriptorSetPtr()};
			vkCmdBindPipeline(*commandBuffer.GetBufferPtr(), VK_PIPELINE_BIND_POINT_GRAPHICS, *m_BlitPipelinePtr->GetPipelinePtr());
			vkCmdBindDescriptorSets(*commandBuffer.GetBufferPtr(), VK_PIPELINE_BIND_POINT_GRAPHICS,
									*m_LightingPipelineLayoutPtr->GetPipelineLayoutPtr(), 0, std::size(descSets), descSets, 0, nullptr);

			VkViewport viewport{};
			viewport.x = .0f;
			viewport.y = .0f;
			viewport.width = static_cast<float>(m_SwapChainPtr->GetExtentPtr()->width);
			viewport.height = static_cast<float>(m_SwapChainPtr->GetExtentPtr()->height);
			viewport.minDepth = .0f;
			viewport.maxDepth = 1.f;
			vkCmdSetViewport(*commandBuffer.GetBufferPtr(), 0, 1, &viewport);

			VkRect2D scissor{};
			scissor.offset = { 0, 0 };
			scissor.extent = *m_SwapChainPtr->GetExtentPtr();
			vkCmdSetScissor(*commandBuffer.GetBufferPtr(), 0, 1, &scissor);

			std::vector<Mesh>& meshes{ m_ScenePtr->GetMeshes() };

			float colour[4]{ 1.f, .0f, .0f, 1.f };
			commandBuffer.BeginLabel("blit", colour);
			vkCmdDraw(*commandBuffer.GetBufferPtr(), 3, 1, 0, 0);
			commandBuffer.EndLabel();
		}

		vkCmdEndRendering(*commandBuffer.GetBufferPtr());
	}

	{
		Image::Transition transition{};
		transition.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
		transition.srcStage = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
		transition.dstStage = VK_PIPELINE_STAGE_2_NONE;
		transition.srcAccess = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
		transition.dstAccess = VK_ACCESS_2_NONE;
		swapchainImage.MakeTransition(m_DevicePtr.get(), &commandBuffer, transition);
	}

	commandBuffer.End(m_DevicePtr.get());
}

void DynamicRenderingApp::DrawFrame()
{
	vkWaitForFences(*m_DevicePtr->GetDevicePtr(), 1, &m_InFlightFences[m_CurrentFrame], VK_TRUE, UINT64_MAX);

	uint32_t imageIndex;
	VkResult result = vkAcquireNextImageKHR(*m_DevicePtr->GetDevicePtr(), *m_SwapChainPtr->GetSwapchainPtr(), UINT64_MAX, m_ImageAvailableSemaphores[m_CurrentFrame], VK_NULL_HANDLE, &imageIndex);
	if (result == VK_ERROR_OUT_OF_DATE_KHR)
	{
		RecreateSwapChain();
		return;
	}
	else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
	{
		throw std::runtime_error("failed to acquire swap chain image");
	}

	//std::cout << "flight fence reset\n";
	vkResetFences(*m_DevicePtr->GetDevicePtr(), 1, &m_InFlightFences[m_CurrentFrame]);

	RecordCommandBufferWithPrepass(m_CommandBuffers[m_CurrentFrame], imageIndex);
	//RecordCommandBufferNoPrepass(m_CommandBuffers[m_CurrentFrame], imageIndex);

	UpdateUniformBuffer(m_CurrentFrame);

	SubmitQueue();

	Present(imageIndex);

	++m_CurrentFrame;
	m_CurrentFrame %= MAX_FRAMES_IN_FLIGHT;
}

void DynamicRenderingApp::UpdateUniformBuffer(uint32_t currentImage)
{
	static auto startTime = std::chrono::steady_clock::now();

	auto currentTime = std::chrono::steady_clock::now();
	float deltaTime = std::chrono::duration<float>(currentTime - startTime).count();

	datatype::ModelViewProjection mvp{};
	mvp.model = m_ScenePtr->GetModelMatrix();
	mvp.view = m_CameraPtr->CalculateView();
	mvp.projection = m_CameraPtr->GetProjection();
	m_MVPUBuffers[currentImage].UpdateMappedData(&mvp, sizeof(mvp), 0);
}

void DynamicRenderingApp::SubmitQueue()
{
	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

	VkSemaphore waitSemaphores[] = { m_ImageAvailableSemaphores[m_CurrentFrame] };
	VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = waitSemaphores;
	submitInfo.pWaitDstStageMask = waitStages;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = m_CommandBuffers[m_CurrentFrame].GetBufferPtr();
	VkSemaphore signalSemaphores[] = { m_RenderFinishedSemaphores[m_CurrentFrame] };
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = signalSemaphores;

	if (auto result = vkQueueSubmit(*m_DevicePtr->GetGraphicsQueuePtr(), 1, &submitInfo, m_InFlightFences[m_CurrentFrame]); result != VK_SUCCESS)
	{
		throw std::runtime_error("failed to submit draw command buffer");
	}
}

void DynamicRenderingApp::Present(uint32_t imageIndex)
{
	VkSemaphore waitSemaphores[] = { m_RenderFinishedSemaphores[m_CurrentFrame] };
	VkPresentInfoKHR presentInfo{};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = waitSemaphores;
	VkSwapchainKHR swapChains[] = { *m_SwapChainPtr->GetSwapchainPtr() };
	presentInfo.swapchainCount = std::size(swapChains);
	presentInfo.pSwapchains = swapChains;
	presentInfo.pImageIndices = &imageIndex;
	presentInfo.pResults = nullptr;

	VkResult result = vkQueuePresentKHR(*m_DevicePtr->GetPresentQueuePtr(), &presentInfo);

	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || m_IsFramebufferResized)
	{
		m_IsFramebufferResized = false;
		RecreateSwapChain();
	}
	else
		if (result != VK_SUCCESS)
		{
			throw std::runtime_error("failed to present swap chain image");
		}
}

void DynamicRenderingApp::MainLoop()
{
	while (!glfwWindowShouldClose(m_WindowPtr))
	while (!glfwWindowShouldClose(m_WindowPtr))
	{
		WorldTime::Tick();
		glfwPollEvents();
		m_CameraPtr->Update(m_WindowPtr);
		DrawFrame();
	}

	vkDeviceWaitIdle(*m_DevicePtr->GetDevicePtr());
}

void DynamicRenderingApp::End()
{
	m_DeletionQueue.Flush();

	glfwTerminate();
}
