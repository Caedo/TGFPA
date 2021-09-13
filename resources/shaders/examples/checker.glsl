RANGE(2, 15)
uniform int checkerCount = 4;

COLOR()
uniform vec3 color1 = vec3(1,1,1);

COLOR()
uniform vec3 color2 = vec3(0,0,0);

void mainImage( out vec4 fragColor, in vec2 fragCoord ) {
    vec2 uv = fragCoord / iResolution.y * checkerCount;

    float fm = mod(floor(uv.x) + floor(uv.y), 2.0);
    vec3 col = mix(color1, color2, fm);

    fragColor = vec4(col, 1.0);
}
