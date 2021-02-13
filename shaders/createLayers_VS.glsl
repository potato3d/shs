#version 110

// Normal to be interpolated to fragment shader
varying vec3 fragNormal;

void main( void )
{
	// Transform the normal into eye space and normalize the result
	// Send normal to be interpolated to fragment shader
	fragNormal = normalize(gl_NormalMatrix * gl_Normal);
		
	gl_Position = ftransform();
	gl_FrontColor = gl_Color;
}
