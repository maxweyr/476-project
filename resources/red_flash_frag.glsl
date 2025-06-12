#version 410 core
in vec2 TexCoord;
out vec4 FragColor;

uniform float alpha;

void main() {
    FragColor = vec4(0.7, 0.1, 0.1, alpha); // Red with fade
}
