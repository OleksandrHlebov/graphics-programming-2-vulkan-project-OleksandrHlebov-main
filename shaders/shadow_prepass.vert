#version 450

layout(push_constant) uniform constants
{
	mat4 model;
	uint lightIndex;
} pushConstants;

layout(constant_id = 1) const uint DIRECTIONAL_LIGHT_COUNT = 1;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColour;
layout(location = 2) in vec2 inTexCoord;
layout(location = 3) in vec3 normal;
layout(location = 4) in vec3 tangent;
layout(location = 5) in vec3 bitangent;

layout(location = 1) out vec2 fragTexCoord;
layout(location = 2) out vec3 fragColour;
struct DirectionalLight
{
	vec3 Direction;
	vec3 Color;
	float Lux;
	mat4 View;
	mat4 Projection;
};

layout(std430, binding = 1) readonly buffer DirectionalLightDataSSBO
{
	DirectionalLight	Lights[DIRECTIONAL_LIGHT_COUNT];
} dirLightData;

void main()
{
	gl_Position = dirLightData.Lights[pushConstants.lightIndex].Projection * dirLightData.Lights[pushConstants.lightIndex].View * pushConstants.model * vec4(inPosition, 1.);
	fragColour = inColour;
	fragTexCoord = inTexCoord;
}