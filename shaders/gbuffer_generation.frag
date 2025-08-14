#version 450
#extension GL_EXT_nonuniform_qualifier : require

layout(location = 0) in	 vec2 fragTexCoord;
layout(location = 1) in  mat3 TBN;

layout(location = 0) out vec4 albedo;
layout(location = 1) out vec4 material;

layout(constant_id = 0) const uint TEXTURE_ARRAY_SIZE = 1;

layout(push_constant) uniform constants
{
	uint textureIndex;
	uint roughnessIndex;
	uint metalnessIndex;
	uint normalIndex;
} pushConstants;

// https://knarkowicz.wordpress.com/2014/04/16/octahedron-normal-vector-encoding/
vec2 OctWrap(vec2 v)
{
    return (1.0 - abs(v.yx)) * vec2(v.x >= 0.0 ? 1.0 : -1.0, 
									v.y >= 0.0 ? 1.0 : -1.0);
}
 
vec2 Encode(vec3 n)
{
    n /= (abs(n.x) + abs(n.y) + abs(n.z));
    n.xy = n.z >= 0.0 ? n.xy : OctWrap(n.xy);
    n.xy = n.xy * 0.5 + vec2(0.5, 0.5);
    return n.xy;
}

layout(set = 0, binding = 4) uniform sampler samp;

layout(set = 0, binding = 5) uniform texture2D textures[TEXTURE_ARRAY_SIZE];

layout(early_fragment_tests) in;

void main()
{
	albedo = texture(sampler2D(textures[nonuniformEXT(pushConstants.textureIndex)], samp), fragTexCoord);

	vec3 normal = texture(sampler2D(textures[nonuniformEXT(pushConstants.normalIndex)], samp), fragTexCoord).rgb;
	normal = normal * 2.0 - vec3(1.0, 1.0, 1.0);
	normal = normalize(TBN * normal);
	material.rg = Encode(normal);
	material.b = texture(sampler2D(textures[nonuniformEXT(pushConstants.roughnessIndex)], samp), fragTexCoord).g;
	material.a = texture(sampler2D(textures[nonuniformEXT(pushConstants.metalnessIndex)], samp), fragTexCoord).b;
}