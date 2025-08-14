#version 450
#extension GL_EXT_nonuniform_qualifier : require

layout(location = 1) in	 vec2 fragTexCoord;

layout(constant_id = 0) const uint TEXTURE_ARRAY_SIZE = 1;

layout(push_constant) uniform constants
{
	mat4 model;
	uint lightIndex;
	uint textureIndex;
	uint roughnessIndex;
	uint metalnessIndex;
	uint normalIndex;
} pushConstants;

layout(set = 0, binding = 4) uniform sampler samp;

layout(set = 0, binding = 5) uniform texture2D textures[TEXTURE_ARRAY_SIZE];

void main()
{
	const float alphaThreshold = .95f;
	if (texture(sampler2D(textures[nonuniformEXT(pushConstants.textureIndex)], samp), fragTexCoord).a < alphaThreshold)
		discard;
}