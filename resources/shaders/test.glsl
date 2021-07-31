
uniform int    uniform_int;
uniform uint   uniform_uint;
uniform bool   uniform_bool;
uniform float  uniform_float;
// uniform double uniform_double;

uniform ivec2 uniform_int2;
uniform ivec3 uniform_int3;
uniform ivec4 uniform_int4;

uniform uvec2 uniform_uint2;
uniform uvec3 uniform_uint3;
uniform uvec4 uniform_uint4;

uniform vec2 uniform_float2;
uniform vec3 uniform_float3;
uniform vec4 uniform_float4;

RANGE(-5, 5)
uniform int   uniform_int_range;
RANGE(-5, 5)
uniform uint  uniform_uint_range;
RANGE(-5, 5)
uniform bool  uniform_bool_range;
RANGE(-5, 5)
uniform float uniform_float_range;

RANGE(-5, 5)
uniform ivec2 uniform_int2_range;
RANGE(-5, 5)
uniform ivec3 uniform_int3_range;
RANGE(-5, 5)
uniform ivec4 uniform_int4_range;

RANGE(-5, 5)
uniform uvec2 uniform_uint2_range;
RANGE(-5, 5)
uniform uvec3 uniform_uint3_range;
RANGE(-5, 5)
uniform uvec4 uniform_uint4_range;

RANGE(-5, 5)
uniform vec2 uniform_float2_range;
RANGE(-5, 5)
uniform vec3 uniform_float3_range;
RANGE(-5, 5)
uniform vec4 uniform_float4_range;

COLOR()
uniform vec3 uniform_color3;

COLOR()
uniform vec4 uniform_color4;

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    vec2 uv = fragCoord/iResolution.xy;

    vec3 color = 0.5 + 0.5 * cos(iTime + uv.xyy);
    color.z = 1;

    fragColor = vec4(color, 1.0);
}