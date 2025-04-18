#version 330 core 
uniform bool hasTexture;
uniform sampler2D textureSampler;

in vec3 fragNor;
in vec2 fragTex;
uniform vec3 MatDif;

out vec4 color;

void main()
{
    if (hasTexture) {
        // Use material diffuse color if no per-vertex color
        color = texture(textureSampler, fragTex) * vec4(MatDif, 1.0);
    } else {
        vec3 normal = normalize(fragNor);
        // Map normal in the range [-1, 1] to color in range [0, 1];
        vec3 Ncolor = 0.5*normal + 0.5;
        color = vec4(Ncolor, 1.0);
    }
}