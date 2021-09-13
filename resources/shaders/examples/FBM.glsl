LIB("noise2D")

RANGE(1, 5)
uniform int octaves = 3;

RANGE(1, 5)
uniform float lacunarity = 2;

RANGE(0, 1)
uniform float persistance = 0.5;

RANGE(1, 10)
uniform float scale = 3;

float FBM(vec2 point) {
    float value = 0;

    float amplitude = 1;
    float frequency = 1;
    float maxValue = 0;

    for(int i = 0; i < octaves; i++) {
        value += (snoise(point * frequency * scale) * 0.5 + 0.5) * amplitude;
        maxValue += amplitude;

        frequency *= lacunarity;
        amplitude *= persistance;
    }

    return value / maxValue;
}

void mainImage( out vec4 fragColor, in vec2 fragCoord ) {
    vec2 uv = fragCoord.xy / iResolution.x;

    float n = FBM(uv);

    fragColor = vec4(n, n, n, 1);
}
