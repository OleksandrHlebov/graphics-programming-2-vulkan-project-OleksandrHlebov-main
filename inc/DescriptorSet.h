#pragma once
#include <vulkan/vulkan.h>
#include <vector>
#include <memory>

class Buffer;
class Device;
class Image;
class DescriptorSet final
{
public:
	~DescriptorSet() = default;

	VkDescriptorSet* GetDescriptorSetPtr() { return &m_Set; }

	DescriptorSet& AddWriteDescriptorSet(Buffer* buffer, uint32_t offset, uint32_t binding, uint32_t arrayElement, VkDescriptorType type);
	DescriptorSet& AddWriteDescriptorSet(Image* image, VkSampler sampler, uint32_t binding, uint32_t arrayElement);
	DescriptorSet& AddWriteDescriptorSet(Image* image, uint32_t binding, uint32_t arrayElement);
	DescriptorSet& AddWriteDescriptorSet(VkSampler sampler, uint32_t binding, uint32_t arrayElement);
	DescriptorSet& AddWriteDescriptorSet(std::vector<Image>& images, uint32_t binding, uint32_t arrayElement);

	void Update(Device* device);

private:
	struct WriteDescriptorSet
	{
		WriteDescriptorSet(const VkWriteDescriptorSet& set, const VkDescriptorBufferInfo& info) : m_Set{ set }, m_Info{ info }, m_ImageInfo{ }
		{
		}
		WriteDescriptorSet(const VkWriteDescriptorSet& set, const VkDescriptorImageInfo& info) : m_Set{ set }, m_ImageInfo{ info }, m_Info{ }
		{
		}
		~WriteDescriptorSet() = default;

		VkWriteDescriptorSet m_Set;
		VkDescriptorBufferInfo m_Info;
		VkDescriptorImageInfo m_ImageInfo;
	};


	friend class DescriptorSetBuilder;
	DescriptorSet(VkDescriptorSet set) : m_Set{ set } {}
	DescriptorSet() = default;

	VkDescriptorSet m_Set;
	std::vector<std::tuple<VkWriteDescriptorSet, VkDescriptorBufferInfo, std::vector<VkDescriptorImageInfo>>> m_WriteDescriptorSets;
};

class DescriptorSetBuilder final
{
public:
	DescriptorSetBuilder() = default;
	~DescriptorSetBuilder() = default;
	
	DescriptorSetBuilder(const DescriptorSetBuilder&) 				= delete;
	DescriptorSetBuilder(DescriptorSetBuilder&&) noexcept 			= delete;
	DescriptorSetBuilder& operator=(const DescriptorSetBuilder&) 	 	= delete;
	DescriptorSetBuilder& operator=(DescriptorSetBuilder&&) noexcept 	= delete;

	void Build(std::vector<DescriptorSet>& sets, Device* device, uint32_t count, VkDescriptorPool pool, VkDescriptorSetLayout* layouts);
	void Build(std::unique_ptr<DescriptorSet>& set, Device* device, uint32_t count, VkDescriptorPool pool, VkDescriptorSetLayout* layouts);
	DescriptorSet Build(Device* device, uint32_t count, VkDescriptorPool pool, VkDescriptorSetLayout* layouts);

private:
};