#version 330 core

uniform sampler2D rockTex;
uniform sampler2D grassTex;

in Vertex {
	vec2 texCoord;
    vec3 normal;
    float worldPosY;
} IN;

out vec4 fragColour;

void main(void) {
    float slope = 1-IN.normal.y;
    if (slope > 0.3f) {
        fragColour = texture(rockTex, IN.texCoord);
     }
    else {
        fragColour = texture(grassTex, IN.texCoord);
    }
}