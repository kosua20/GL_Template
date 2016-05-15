#version 330

// First attribute: vertex position
layout(location = 0) in vec3 v;
// Second attribute: color
layout(location = 1) in vec3 c;

// Uniform: the MVP matrix
uniform mat4 mvp;

// Output: color (for interpolation)
out vec3 color; 

void main(){
	// We multiply the coordinates by the MVP matrix, and ouput the result.
	gl_Position = mvp * vec4(v, 1.0);

	color = c;
}
