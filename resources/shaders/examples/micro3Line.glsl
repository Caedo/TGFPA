
void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    vec2 uv = (2.0 * fragCoord - iResolution.xy) / iResolution.y;

    uv.y = 1.0 / abs(uv.y); uv.x *= uv.y; uv.y += iTime;

    fragColor = sin(9.0 * uv.xyxy);
}