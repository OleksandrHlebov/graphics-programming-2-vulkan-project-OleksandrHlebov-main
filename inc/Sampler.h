#pragma once
#include "Device.h"
#include "vulkan/vulkan.h"

class Sampler final
{
public:
	~Sampler() = default;

	VkSampler* GetSamplerPtr() { return &m_Sampler; }

	void Destroy(Device* device)
	{
		vkDestroySampler(*device->GetDevicePtr(), m_Sampler, nullptr);
	}

private:
	friend class SamplerBuilder;
	Sampler() = default;
	VkSampler m_Sampler;
};

class SamplerBuilder final
{
public:
	SamplerBuilder()
	{
		m_SamplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		m_SamplerInfo.magFilter = VK_FILTER_LINEAR;
		m_SamplerInfo.minFilter = VK_FILTER_LINEAR;
		m_SamplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		m_SamplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		m_SamplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		m_SamplerInfo.unnormalizedCoordinates = VK_FALSE;
		m_SamplerInfo.compareEnable = VK_FALSE;
		m_SamplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
		m_SamplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	}
	~SamplerBuilder() = default;
	
	SamplerBuilder(const SamplerBuilder&) 				= delete;
	SamplerBuilder(SamplerBuilder&&) noexcept 			= delete;
	SamplerBuilder& operator=(const SamplerBuilder&) 	 	= delete;
	SamplerBuilder& operator=(SamplerBuilder&&) noexcept 	= delete;

	SamplerBuilder& SetFilter(VkFilter filter)
	{
		m_SamplerInfo.magFilter = filter;
		m_SamplerInfo.minFilter = filter;
		return *this;
	}

	SamplerBuilder& SetAddressMode(VkSamplerAddressMode addressMode)
	{
		m_SamplerInfo.addressModeU = addressMode;
		m_SamplerInfo.addressModeV = addressMode;
		m_SamplerInfo.addressModeW = addressMode;
		return *this;
	}

	SamplerBuilder& SetMaxAnisotropy(uint32_t value)
	{
		m_SamplerInfo.anisotropyEnable = VK_TRUE;
		m_SamplerInfo.maxAnisotropy = value;
		return *this;
	}

	SamplerBuilder& SetCompareOp(VkCompareOp op)
	{
		m_SamplerInfo.compareEnable = VK_TRUE;
		m_SamplerInfo.compareOp = op;
		return *this;
	}

	Sampler Build(Device* device)
	{
		Sampler sampler{};
		vkCreateSampler(*device->GetDevicePtr(), &m_SamplerInfo, nullptr, sampler.GetSamplerPtr());
		return sampler;
	}

	void Build(std::unique_ptr<Sampler>& sampler, Device* device)
	{
		sampler.reset(new Sampler(Build(device)));
	}

private:
	VkSamplerCreateInfo m_SamplerInfo{};

};
