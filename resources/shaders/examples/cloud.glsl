// Test cloud-like texture, will be rewriten later

LIB(noise2D)

RANGE(1, 5)
uniform int octaves = 2;
RANGE(1, 5)
uniform float lacunarity = 2;
RANGE(0, 1)
uniform float persistance = 0.5;
RANGE(1, 10)
uniform float scale = 4;
RANGE(0, 1)
uniform float smoothMin = 0.3f;
RANGE(0, 1)
uniform float smoothMax = 1;

float FBM(vec2 point) {
    float value = 0;

    float amplitude = 1;
    float frequency = 1;
    float maxValue = 1;

    for(int i = 0; i < octaves; i++) {
        value += (snoise(point * frequency * scale) * 0.5 + 0.5) * amplitude;
        maxValue += amplitude;

        frequency *= lacunarity;
        amplitude *= persistance;
    }

    return value / maxValue;
}


void mainImage( out vec4 fragColor, in vec2 fragCoord ) {
    vec2 uv = fragCoord/iResolution.x;

    float s = 1 -  length(uv - vec2(0.5f));
    s = smoothstep(smoothMin, smoothMax, s);
    float c = FBM(uv * scale);
    c = clamp(c + 0.3f, 0, 1);
    float a = clamp(c + 0.3f, 0, 1) * s;
    fragColor = vec4(vec3(c), a);
}
