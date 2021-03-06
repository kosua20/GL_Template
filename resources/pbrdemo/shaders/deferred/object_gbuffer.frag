
#define MATERIAL_ID 1 ///< The material ID.

in INTERFACE {
    mat3 tbn; ///< Normal to view matrix.
	vec2 uv; ///< UV coordinates.
} In ;

layout(binding = 0) uniform sampler2D texture0; ///< Albedo.
layout(binding = 1) uniform sampler2D texture1; ///< Normal map.
layout(binding = 2) uniform sampler2D texture2; ///< Effects map.

layout (location = 0) out vec4 fragColor; ///< Color.
layout (location = 1) out vec3 fragNormal; ///< View space normal.
layout (location = 2) out vec3 fragEffects; ///< Effects.

uniform bool hasUV; ///< Does the mesh have texture coordinates.

/** Transfer albedo and effects along with the material ID, and output the final normal 
	(combining geometry normal and normal map) in view space. */
void main(){
	
	vec4 color = texture(texture0, In.uv);
	if(color.a <= 0.01){
		discard;
	}
	
	// Flip the up of the local frame for back facing fragments.
	mat3 tbn = In.tbn;
	tbn[2] *= (gl_FrontFacing ? 1.0 : -1.0);
	// Compute the normal at the fragment using the tangent space matrix and the normal read in the normal map.
	vec3 n;
	if(hasUV){
		n = texture(texture1, In.uv).rgb ;
		n = normalize(n * 2.0 - 1.0);
		n = normalize(tbn * n);
	} else {
		n = normalize(tbn[2]);
	}
	
	// Store values.
	fragColor.rgb = color.rgb;
	fragColor.a = float(MATERIAL_ID)/255.0;
	
	fragNormal.rgb = n * 0.5 + 0.5;
	fragEffects.rgb = texture(texture2, In.uv).rgb;
	
}
