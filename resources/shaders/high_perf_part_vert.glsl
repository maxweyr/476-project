#version 330 core

layout(location = 0) in vec4 particlePos;   // xyz = position, w = size
layout(location = 1) in vec4 particleVel;   // xyz = velocity, w = damping
layout(location = 2) in vec4 particleCol;   // rgba = color
layout(location = 3) in vec2 particleLife;  // x = current life, y = max life

uniform mat4 P;
uniform mat4 V;
uniform mat4 M;

out vec4 partCol;
out float lifeRatio;

void main() {
    // Skip rendering inactive particles
    if (particleLife.x < 0.0) {
        gl_Position = vec4(0, 0, 0, 0);
        gl_PointSize = 0.0;
        return;
    }
    
    // Calculate life ratio for fragment shader
    lifeRatio = particleLife.x / particleLife.y;
    
    // Billboarding: extract camera right and up vectors
    vec3 CamRight = vec3(V[0][0], V[1][0], V[2][0]);
    vec3 CamUp = vec3(V[0][1], V[1][1], V[2][1]);
    
    // Get position and size
    vec3 position = particlePos.xyz;
    float size = particlePos.w;
    
    // Calculate position in view space
    gl_Position = P * V * vec4(position, 1.0);
    
    // Calculate size based on distance and life
    float dist = length((V * vec4(position, 1.0)).xyz);
    gl_PointSize = 100.0 * size / dist;
    
    // Pass color to fragment shader
    partCol = particleCol;
}