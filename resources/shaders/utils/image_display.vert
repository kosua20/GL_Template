#version 330

// Output: UV coordinates
out INTERFACE {
	vec2 uv;
} Out ; ///< vec2 uv;

uniform float screenRatio;
uniform float imageRatio;
uniform float widthRatio;
uniform bool isHDR;

/**
 Generate one triangle covering the whole screen
 2: (-1,3),(0,2)
 *
 | \
 |	 \
 |	   \
 |		 \
 |		   \
 *-----------*  1: (3,-1), (2,0)
 0: (-1,-1), (0,0)
 */
void main(){
	vec2 temp = 2.0 * vec2(gl_VertexID == 1, gl_VertexID == 2);
	gl_Position.xy = 2.0 * temp - 1.0;
	gl_Position.zw = vec2(1.0);
	// Center uvs.
	vec2 uv = gl_Position.xy;
	// Image and screen ratio corrections.
	float HDRflip = (isHDR ? -1.0 : 1.0);
	uv *= vec2(imageRatio, HDRflip*screenRatio);
	uv *= widthRatio;
	Out.uv = uv * 0.5 + 0.5;
}