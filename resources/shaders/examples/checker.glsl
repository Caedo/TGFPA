DRAG()
uniform float add = 0.1f;

DRAG()
uniform float mul = 0.5f;

void mainImage( out vec4 fragColor, in vec2 fragCoord ) {
    vec2 uv = fragCoord / iResolution.y * 4;

    float fm = mod(floor(uv.x) + floor(uv.y), 2.0) * mul + add;

    fragColor = vec4(vec3(fm), 1.0);
}
