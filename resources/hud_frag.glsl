#version 410 core

in vec2 TexCoord;
in float HealthPecentage;
in float barStartX;
in float barWidth;

out vec4 FragColor;

void main() {
    float screenX = gl_FragCoord.x;
    float cutoffX = barStartX + (barWidth * HealthPecentage);

    if (screenX > cutoffX) {
        FragColor = vec4(0.0, 0.0, 0.0, 1.0); // black (empty health)
    } else {
        FragColor = vec4(0.541, 0.145, 0.145, 1.0); // red (filled health)
    }
}
