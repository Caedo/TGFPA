// Simple "dot" or "spark" for typical particle systems. Will be rewritten later

RANGE(0, 1)
uniform float smoothMin = 0.3f;

RANGE(0, 1)
uniform float smoothMax = 1f;

void mainImage( out vec4 fragColor, in vec2 fragCoord ) {
    vec2 uv = fragCoord/iResolution.x;

    float c = 1 -  length(uv - vec2(0.5f));
    c = smoothstep(smoothMin, smoothMax, c);

    fragColor = vec4(c);
}
