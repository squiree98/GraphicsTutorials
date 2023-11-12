#version 330 core

uniform sampler2D rockTex;
uniform sampler2D grassTex;

in Vertex {
	vec2 texCoord;
    float worldPosY;
} IN;

out vec4 fragColour;

void main(void) {
    if (IN.worldPosY > 100.0f) {
	    fragColour = texture(grassTex, IN.texCoord);
    }
    else {
	    fragColour = texture(rockTex, IN.texCoord);
    }
}