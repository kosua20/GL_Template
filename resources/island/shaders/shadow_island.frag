#version 400

in INTERFACE {
	vec2 uv; ///< UV coordinates.
} In ;

uniform vec3 lDir;
uniform uint stepCount;
uniform float horiz;
uniform vec2 lDir2;
uniform vec2 tMaxInit;
uniform vec2 tDelta;

layout(binding = 0) uniform sampler2D heightMap; ///< Color to output.

layout(location = 0) out vec2 shadow; ///< Color.

/** Just pass the input image as-is, without any resizing. */
void main(){

	shadow = vec2(1.0);
	if(abs(lDir.y) >= 0.999){
		// Vertical sun, no shadowing.
		return;
	}

	// Precompute the steps using a conservative stepping computation.
	// Base on A Fast Voxel Traversal Algorithm for Ray Tracing, J. Amanatides and A. Woo, Eurographics, 1987
	int dx = lDir2.x >= 0.0 ? 1 : -1;
	int dy = lDir2.y >= 0.0 ? 1 : -1;

	ivec2 wh = ivec2(textureSize(heightMap, 0).xy);
	ivec2 p = ivec2(In.uv * wh);
	ivec2 sp = p;
	vec2 tMax = tMaxInit;
	float hStart = texelFetch(heightMap, sp, 0).r;

	// Compute ray height.
	bool occGround = false;
	bool occWater = false;

	for(uint i = 0; i < stepCount; ++i){
		if(tMax.x < tMax.y){
			tMax.x += tDelta.x;
			sp.x += dx;
		} else {
			tMax.y += tDelta.y;
			sp.y += dy;
		}
		// Check if we are outside the map.
		if(any(lessThan(sp, ivec2(0))) || any(greaterThanEqual(sp, wh))){
			break;
		}

		float hDelta = horiz * length(vec2(sp - p));
		float hRef = texelFetch(heightMap, sp, 0).r;
		if(hStart + hDelta < hRef){
			occGround = true;
		}
		if(hDelta < hRef){
			occWater = true;
		}
		if(occGround && occWater){
			break;
		}
	}
	shadow.x = float(!occGround);
	shadow.y = float(!occWater);
}
