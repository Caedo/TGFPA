#define EPSILON 0.001

COLOR()
uniform vec3 lightColor = vec3(0.1, 0.5, 0.3);

COLOR()
uniform vec3 ballsColor = vec3(0, 0, 0.75);

RANGE(0, 1)
uniform float blend = 0.25;

float sphere(vec3 p, float r) {
    return length(p) - r;
}

float opSmoothUnion( float d1, float d2, float k ) {
    float h = clamp( 0.5 + 0.5*(d2-d1)/k, 0.0, 1.0 );
    return mix( d2, d1, h ) - k*h*(1.0-h);
}

float sceneSDF(vec3 point) {
    float t = iTime * 0.7;
    vec3 s1Pos = vec3(0.0, sin(t), 0.0);
    vec3 s2Pos = vec3(0.5, sin(t - 60.0), 0.0);
    vec3 s3Pos = vec3(-0.2, sin(t - 90.0), 0.0);

    float s1 = sphere(point - s1Pos, 0.6);
    float s2 = sphere(point - s2Pos, 0.5);
    float s3 = sphere(point - s3Pos, 0.45);

    float sd = opSmoothUnion(s1, s2, blend);
    sd = opSmoothUnion(sd, s3, blend);

    return sd;
}

mat4 viewMatrix(vec3 pos, vec3 target, vec3 up) {
    vec3 f = normalize(target - pos);
    vec3 s = normalize(cross(f, up));
    vec3 u = cross(s, f);
    return mat4(
        vec4(s, 0.0),
        vec4(u, 0.0),
        vec4(-f, 0.0),
        vec4(0.0, 0.0, 0.0, 1)
    );
}

float rayMarch(vec3 rayOrigin, vec3 rayDirection, float start, float end) {
    float depth = start;
    for(int i = 0; i < 255; i++) {
        float dist = sceneSDF(rayOrigin + depth * rayDirection);
        
        if(dist < EPSILON) return depth;
        
        depth += dist;
        if(depth >= end) return end;
    }
    
    return end;
}

vec3 normal(vec3 p) {
    return normalize(vec3(
        sceneSDF(vec3(p.x + EPSILON, p.y, p.z)) - sceneSDF(vec3(p.x - EPSILON, p.y, p.z)),
        sceneSDF(vec3(p.x, p.y + EPSILON, p.z)) - sceneSDF(vec3(p.x, p.y - EPSILON, p.z)),
        sceneSDF(vec3(p.x, p.y, p.z  + EPSILON)) - sceneSDF(vec3(p.x, p.y, p.z - EPSILON))
    ));
}

vec3 rayDirection(float fieldOfView, vec2 size, vec2 fragCoord) {
    vec2 xy = fragCoord - size / 2.0;
    float z = size.y / tan(radians(fieldOfView) / 2.0);
    return normalize(vec3(xy, -z));
}

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    vec3 dir = rayDirection(45.0, iResolution.xy, fragCoord);
    vec3 cameraPos = vec3(0.0, 0.0, 0.0);
    
    cameraPos.x = 7.0 * sin(iTime * 0.8);
    cameraPos.z = 7.0 * cos(iTime * 0.8);
    
    mat4 cam = viewMatrix(cameraPos, vec3(0.0, 0.0, 0.0), vec3(0.0, 1.0, 0.0));
    dir = (cam * vec4(dir, 0.0)).xyz;
    
    float dist = rayMarch(cameraPos, dir, 0.0, 100.0);
    
    if(dist > 100.0 - EPSILON) {
        fragColor = vec4(0.0, 0.0, 0.0, 1.0);
        return;
    }
    
    vec3 lightDir = vec3(-1.0, 1.0, sin(iTime));

    vec3 hitPoint = cameraPos + dist * dir;
    float ndotl = dot(normal(hitPoint), lightDir) * 0.5 + 0.5;
        
    vec3 col = ndotl * lightColor + ballsColor;


    // Output to screen
    fragColor = vec4(col,1.0);
    
}