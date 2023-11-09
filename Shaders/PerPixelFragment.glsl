#version 330 core

uniform sampler2D	diffuseTex;
uniform vec3		cameraPos;
uniform vec4		lightColour;
uniform vec3		lightPos;
uniform float		lightRadius;

in Vertex {
	vec4 colour;
	vec2 texCoord;
	vec3 normal;
	vec3 worldPos;
} IN;

out vec4 fragment;

void main(void) {
	// compute the incident, view, and half angle vectors
	vec3 incident	=		normalize(lightPos - IN.worldPos);
	vec3 viewDir	=		normalize(cameraPos - IN.worldPos);
	vec3 halfDir	=		normalize(incident + viewDir);
	// get texture colour of object
	vec4 diffuse	=		texture(diffuseTex, IN.texCoord);

	// get lambert (amount of light hitting surface)
	float lambert		= max(dot(incident, IN.normal), 0.0f);
	// get distance between light and fragment
	float distance		= length(lightPos - IN.worldPos);
	// get attenuation which is how intense the light is (further away means less intense)
	float attenuation	= 1.0 - clamp(distance/lightRadius, 0.0, 1.0);

	// get the specularity of the light
	// use half angle to find how close we are to a perfect reflection angle
	float specFactor = clamp(dot(halfDir, IN.normal), 0.0, 1.0);
	// 60 is how shiny we want the surface to be
	specFactor = pow(specFactor, 60.0);

	// get final colour of the fragment
	vec3 surface = (diffuse.rgb * lightColour.rgb);
	fragment.rgb = surface * lambert * attenuation;
	fragment.rgb += (lightColour.rgb) * attenuation * 0.33;
	fragment.rgb += surface * 0.1f;
	fragment.a = diffuse.a;
}