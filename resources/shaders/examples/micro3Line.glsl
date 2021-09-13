// Based On Inigo Quilez's micro demo shaders:
// https://twitter.com/iquilezles/status/1415562589346492419

RANGE(0, 2)
uniform float mixValue;

vec4 demo1(vec2 fragCoord) {
    vec2 uv = (2.0 * fragCoord - iResolution.xy) / iResolution.y;

    uv.y = 1.0 / abs(uv.y); uv.x *= uv.y; uv.y += iTime;

    return vec4(vec3(sin(9.0 * uv.xyxy)), 1);
}

vec4 demo2(vec2 fragCoord) {
    vec2 uv = (2.0 * fragCoord - iResolution.xy) / iResolution.y;

    uv = uv / dot(uv, uv) + iTime;

    return vec4(vec3(sin(9.0 * uv.xyxy)), 1);
}

vec4 demo3(vec2 fragCoord) {
    vec2 uv = (2.0 * fragCoord - iResolution.xy) / iResolution.y;

    uv = vec2(atan(uv.y, uv.x), iTime + 1.0 / length(uv));

    return vec4(vec3(sin(9.0 * uv.xyxy)), 1);
}

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    float m1 = clamp(mixValue, 0, 1);
    float m2 = clamp(mixValue - 1, 0, 1);

    vec4 c1 = mix(demo1(fragCoord), demo2(fragCoord), m1);
    vec4 c2 = mix(demo2(fragCoord), demo3(fragCoord), m2);

    c1 = mixValue <= 1 ? c1 : vec4(0);
    c2 = mixValue >= 1 ? c2 : vec4(0);

    fragColor = c1 + c2;
}