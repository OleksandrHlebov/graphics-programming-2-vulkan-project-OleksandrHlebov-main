#version 450
#extension GL_EXT_nonuniform_qualifier : require
#extension GL_EXT_samplerless_texture_functions : require

layout(location = 0) out vec4 outColour;
layout(location = 1) in vec2 fragTexCoord;

layout(constant_id = 0) const uint POINT_LIGHT_COUNT = 1;
layout(constant_id = 1) const uint DIRECTIONAL_LIGHT_COUNT = 1;
const uint TEXTURE_ARRAY_SIZE = 1;

layout(set = 0, binding = 2) uniform textureCube environmentMap;
layout(set = 0, binding = 3) uniform textureCube irradianceMap;
layout(set = 0, binding = 4) uniform sampler samp;
layout(set = 0, binding = 5) uniform texture2D textures[TEXTURE_ARRAY_SIZE];

layout(set = 1, binding = 0) uniform sampler shadowSampler;
layout(set = 1, binding = 1) uniform texture2D shadowMaps[DIRECTIONAL_LIGHT_COUNT];

layout(set = 2, binding = 1) uniform texture2D albedoTexture;
layout(set = 2, binding = 2) uniform texture2D materialProps;
layout(set = 2, binding = 3) uniform texture2D depthBuffer;

struct PointLight
{
	vec3 Position;
	vec3 Color;
	float Lumen;
};

struct DirectionalLight
{
	vec3 Direction;
	vec3 Color;
	float Lux;
	mat4 View;
	mat4 Projection;
};

layout(std430, binding = 0) readonly buffer PointLightDataSSBO
{
	PointLight			Lights[POINT_LIGHT_COUNT];
} pointLightData;

layout(std430, binding = 1) readonly buffer DirectionalLightDataSSBO
{
	DirectionalLight	Lights[DIRECTIONAL_LIGHT_COUNT];
} dirLightData;

layout(set = 2, binding = 0) uniform ModelViewProjection
{
	mat4 model;
	mat4 view;
	mat4 projection;
} mvp;

float PI = 3.14159265358979323846;

// https://stackoverflow.com/questions/32227283/getting-world-position-from-depth-buffer-value
vec3 WorldPosFromDepth(float depth, vec2 texCoord) {
    vec4 clipSpacePosition = vec4(texCoord * 2.0 - vec2(1.0, 1.0), depth, 1.0);
    vec4 viewSpacePosition = inverse(mvp.projection) * clipSpacePosition;

    // Perspective division
    viewSpacePosition /= viewSpacePosition.w;

    vec4 worldSpacePosition = inverse(mvp.view) * viewSpacePosition;

    return worldSpacePosition.xyz;
}

// https://knarkowicz.wordpress.com/2014/04/16/octahedron-normal-vector-encoding/
vec3 Decode(vec2 f)
{
    f = f * 2.0 - vec2(1.0, 1.0);
 
    // https://twitter.com/Stubbesaurus/status/937994790553227264
    vec3 n = vec3(f.x, f.y, 1.0 - abs(f.x) - abs(f.y));
    float t = clamp(-n.z, 0.0, 1.0);
    n.xy += vec2((n.x >= 0.0) ? -t : t,
				 (n.y >= 0.0) ? -t : t);
    return normalize(n);
}

vec3 FresnelSchlick(float cosTheta, vec3 F0)
{
	return F0 + (1. - F0) * pow(clamp(1. - cosTheta, .0, 1.), 5.);
}

vec3 FresnelSchlickRoughness(float cosTheta, vec3 F0, float roughness)
{
    return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}   

float DistributionGGX(vec3 N, vec3 H, float roughness)
{
    float a      = roughness;
	const bool squareRoughness = false;
	if (squareRoughness)
		a = roughness * roughness;
    const float a2     = a*a;
    const float NdotH  = max(dot(N, H), 0.0);
    const float NdotH2 = NdotH*NdotH;
	
    const float num   = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;
	
    return num / denom;
}

float GeometrySchlickGGX_Direct(float NdotV, float roughness)
{
    const float r = (roughness + 1.0);
    const float k = (r*r) / 8.0;
	
    const float num   = NdotV;
    const float denom = NdotV * (1.0 - k) + k;
	
    return num / denom;
}

float GeometrySchlickGGX_Indirect(float NdotV, float roughness)
{
    const float a = (roughness);
    const float k = (a*a) / 2.0;
	
    const float num   = NdotV;
    const float denom = NdotV * (1.0 - k) + k;
	
    return num / denom;
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness, bool directLighting)
{
    const float NdotV = max(dot(N, V), 0.0);
    const float NdotL = max(dot(N, L), 0.0);
    float ggx2  = 0;
    float ggx1  = 0;
	if (directLighting)
	{
		ggx2 = GeometrySchlickGGX_Direct(NdotV, roughness);
		ggx1 = GeometrySchlickGGX_Direct(NdotL, roughness);
	}
	else
	{
		ggx2 = GeometrySchlickGGX_Indirect(NdotV, roughness);
		ggx1 = GeometrySchlickGGX_Indirect(NdotL, roughness);
	}

    return ggx1 * ggx2;
}

void main()
{
	const vec4 material = texelFetch(sampler2D(materialProps, samp), ivec2(fragTexCoord.xy * textureSize(materialProps, 0)), 0);
	const vec3 normal = normalize(Decode(material.rg));
	const vec3 albedo = pow(texture(sampler2D(albedoTexture, samp), fragTexCoord).rgb, vec3(2.2)); // albedo in linear space
	const float roughness = material.b;
	const float metalness = material.a;

	const float depth = texelFetch(sampler2D(depthBuffer, samp), ivec2(fragTexCoord.xy * textureSize(depthBuffer, 0)), 0).r;

	const vec3 worldPos = WorldPosFromDepth(depth, fragTexCoord);
	const vec3 cameraPos = inverse(mvp.view)[3].xyz;
	const vec3 viewDirection = normalize(cameraPos - worldPos);
	if (depth >= 1.f)
	{
		const vec3 sampleDir = normalize(worldPos);
		outColour = vec4(texture(samplerCube(environmentMap, samp), -viewDirection).rgb, 1.f);
		return;
	}

	const vec3 F0 = mix(vec3(.04), albedo, metalness);

	vec3 Lo = vec3(.0);
	for (int index = 0; index < POINT_LIGHT_COUNT; ++index)
	{
		const vec3 pos = pointLightData.Lights[index].Position;
		const vec3 col = pointLightData.Lights[index].Color;
		const vec3 L = normalize(pos - worldPos);
		const vec3 H = normalize(viewDirection + L);

		const float lumen = pointLightData.Lights[index].Lumen;
		const float luminousIntensity = lumen / (4. * PI);

		const float distance = length(pos - worldPos);
		const float attenuation = 1. / max((distance * distance), 0.0001f);
		const float illuminance = luminousIntensity * attenuation;
		const vec3 irradiance = col * illuminance;

		const float NDF = DistributionGGX(normal, H, roughness);
		const float G = GeometrySmith(normal, viewDirection, L, roughness, true);
		const vec3 F = FresnelSchlick(max(dot(H, viewDirection), .0), F0);

		const vec3 num = NDF * G * F;
		const float denom = 4. * max(dot(normal, viewDirection), .0) * max(dot(normal, viewDirection), .0) + .00001f;
		const vec3 specular = num / denom;

		const vec3 kS = F;
		const vec3 kD = (vec3(1.) - kS) * (1. - metalness);
		const float NdotL = max(dot(normal, L), .0);
		Lo += (kD * albedo / PI + specular) * irradiance * NdotL;
	}

	for (int index = 0; index < DIRECTIONAL_LIGHT_COUNT; ++index)
	{
		const vec3 col = dirLightData.Lights[index].Color;
		const vec3 L = -dirLightData.Lights[index].Direction;
		const vec3 H = normalize(viewDirection + L);

		const float illuminance = dirLightData.Lights[index].Lux;
		const vec3 irradiance = col * illuminance;

		const float NDF = DistributionGGX(normal, H, roughness);
		const float G = GeometrySmith(normal, viewDirection, L, roughness, true);
		const vec3 F = FresnelSchlick(max(dot(H, viewDirection), .0), F0);

		const vec3 num = NDF * G * F;
		const float denom = 4. * max(dot(normal, viewDirection), .0) * max(dot(normal, viewDirection), .0) + .00001f;
		const vec3 specular = num / denom;

		vec4 lightSpacePosition = dirLightData.Lights[index].Projection * dirLightData.Lights[index].View * vec4(worldPos, 1.f);
		lightSpacePosition /= lightSpacePosition.w;
		vec3 shadowMapUV = vec3(lightSpacePosition.xy * .5f + .5f, lightSpacePosition.z);

		const float shadow = texture(sampler2DShadow(shadowMaps[index], shadowSampler), shadowMapUV);

		const vec3 kS = F;
		const vec3 kD = (vec3(1.) - kS) * (1. - metalness);
		const float NdotL = max(dot(normal, L), .0);
		Lo += shadow * (kD * albedo / PI + specular) * irradiance * NdotL;
	}

	const float exposureCompensation = 2.f;
	const vec3 kS = FresnelSchlickRoughness(max(dot(normal, viewDirection), 0.0), F0, roughness); 
	const vec3 kD = (1.0 - kS) * (1.f - metalness);
	const vec3 irradiance = texture(samplerCube(irradianceMap, samp), normal).rgb;
	const vec3 diffuse    = irradiance * albedo;
	const vec3 ambient    = (kD * diffuse * exposureCompensation); 
	const vec3 color = ambient + Lo;

	outColour = vec4(color, 1.);
}