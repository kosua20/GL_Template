#version 330

// Input: UV coordinates
in INTERFACE {
	vec2 uv;
} In ; ///< vec2 uv;

layout(binding = 0) uniform sampler2D screenTexture; ///< Lighting buffer to filter. 
uniform float luminanceTh = 1.0;

layout(location = 0) out vec3 fragColor; ///< Scene color.

/** Extract the areas of the image where the luminance is higher than 1.0. */
void main(){
	fragColor = vec3(0.0);
	
	vec3 color = textureLod(screenTexture, In.uv, 0.0).rgb;
	// Compute intensity (luminance). If > 1.0, bloom should be visible.
	if(dot(color, vec3(0.289, 0.527, 0.184)) > luminanceTh){
		fragColor = color;
	}
}
