#version 110

// Reference depth texture to discard incoming pixels
uniform sampler2D depthTexSampler;
uniform float invDepthTexWidth;
uniform float invDepthTexHeight;

// Normal interpolated from vertex shader
varying vec3 fragNormal;

void main( void )
{	
	// Compute texel coords
	vec2 texelCoord;
	texelCoord.x = gl_FragCoord.x * invDepthTexWidth;
	texelCoord.y = gl_FragCoord.y * invDepthTexHeight;
	
	// Get previous z-buffer value
	float prevDepth = texture2D( depthTexSampler, texelCoord ).r;
	
	// If current z value is less than or equal to previous, we are from a previously rendered layer.
	// Therefore, we must get out of the way for incoming fragments that are beneath us (have higher depth).
	// We will let the default z-test handle subsequent layers, so the next one should come out on top of the others.
	
	// If previous z value is zero, we already rendered the current pixel in it's final layer.
	// Therefore, all fragments from now on must be discarded (we're done for this (x,y) frag coord).
	if( ( prevDepth == 0.0 ) || ( gl_FragCoord.z <= prevDepth ) )
		discard;
	
	// Save current z value for next iteration.
	// We will swap render texture with reference depth texture.
	gl_FragColor.r = gl_FragCoord.z;
	
	// Save fragment normal in color.gba
	gl_FragColor.gba = normalize( fragNormal );
	
	// TODO: 
	// I think this is needed to avoid early z-cull of incoming fragments.
	// We need to force the execution of this shader for EVERY fragment.
	gl_FragDepth = gl_FragCoord.z;
}
