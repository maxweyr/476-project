#version 330 core

uniform sampler2D alphaTexture;

in vec4 partCol;
in float lifeRatio;

out vec4 outColor;

void main() {
    // Sample texture for alpha mask
    float alpha = texture(alphaTexture, gl_PointCoord).r;
    
    // Apply color and alpha
    outColor = vec4(partCol.rgb, partCol.a * alpha);
    
    // Discard nearly transparent pixels for better performance
    if (outColor.a < 0.01) discard;
}