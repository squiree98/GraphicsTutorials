#version 330 core

uniform vec3		cameraPos;
uniform vec4		lightColour;
uniform vec3		lightPos;
uniform float		lightRadius;

uniform sampler2D rockTex;
uniform sampler2D grassTex;
uniform sampler2D bumpTex;

in Vertex {
	vec4 colour;
	vec2 texCoord;
	vec3 normal;
	vec3 normalizedNormal;
    vec3 tangent;
	vec3 binormal;
	vec3 worldPos;
    float worldPosY;
} IN;

out vec4 fragment;

void main(void) {
    // calculate incident light as well as the view direction and half direction
    vec3 incident = normalize(lightPos  - IN.worldPos);
    vec3 viewDir  = normalize(cameraPos - IN.worldPos);
    vec3 halfDir  = normalize(incident  + viewDir);

    mat3 TEN = mat3(normalize(IN.tangent), normalize(IN.binormal), normalize(IN.normal));

    // get the texture colours
    float slope = 1-IN.normalizedNormal.y;
	vec4 diffuse;
    if (slope > 0.3f) {
        diffuse = texture(rockTex, IN.texCoord);
    }
    else {
        diffuse = texture(grassTex, IN.texCoord);
    }

    vec3 bumpNormal = texture(bumpTex, IN.texCoord).rgb;
    bumpNormal = normalize(TEN * normalize(bumpNormal * 2.0 - 1.0));

    // get lambert (amount of light hitting surface)
	float lambert		= max(dot(incident, bumpNormal), 0.0f);
	// get distance between light and fragment
	float distance		= length(lightPos - IN.worldPos);
	// get attenuation which is how intense the light is (further away means less intense)
	float attenuation	= 1.0 - clamp(distance/lightRadius, 0.0, 1.0);

	// get the specularity of the light
	// use half angle to find how close we are to a perfect reflection angle
	float specFactor = clamp(dot(halfDir, bumpNormal), 0.0, 1.0);
	// 60 is how shiny we want the surface to be
	specFactor = pow(specFactor, 60.0);

	// get final colour of the fragment
	vec3 surface = (diffuse.rgb * lightColour.rgb);
	fragment.rgb = surface * lambert * attenuation;
	fragment.rgb += (lightColour.rgb * specFactor) * attenuation * 0.33;
	fragment.rgb += surface * 0.1f;
	fragment.a = diffuse.a;
}