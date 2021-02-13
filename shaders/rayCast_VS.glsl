#version 110

/************************************************************************/
/* Varying                                                              */
/************************************************************************/
// Ray direction in tangent space
varying vec3 v_viewDir;

// Vertex position in View Space
varying vec3 v_posViewSpace;

// Ray origin
varying vec3 v_rayOrigin;

// TODO: what more to send to frag?

/************************************************************************/
/* Globals                                                              */
/************************************************************************/
// TODO: hard-coded tangent vectors in World Space, what to put in W coordinate?
vec4 g_tangent  = vec4( -1, 0, 0, 1 );
vec4 g_binormal = vec4(  0, 0, 1, 1 );
vec4 g_normal   = vec4(  0, 1, 0, 1 );

/************************************************************************/
/* Main                                                                 */
/************************************************************************/
void main( void )
{
	// Vertex position in View Space = view direction in World Space
	v_posViewSpace = ( gl_ModelViewMatrix * gl_Vertex ).xyz;

	// TODO:
/*
	// Transform tangent vectors from World to View Space
	// These are directions, so use gl_NormalMatrix = 3x3 rotation part of gl_ModelViewMatrix
	vec3 tangent  = gl_NormalMatrix * g_tangent.xyz;
	vec3 binormal = gl_NormalMatrix * g_binormal.xyz;
	vec3 normal   = gl_NormalMatrix * g_normal.xyz;
	
	// View direction in Tangent Space
	v_viewDir.x = dot( tangent,  v_posViewSpace );
	v_viewDir.y = dot( binormal, v_posViewSpace );
	v_viewDir.z = dot( normal,   v_posViewSpace );
*/

	v_viewDir = ( gl_Vertex - gl_ModelViewMatrixInverse[3] ).xyz;
	v_rayOrigin = gl_Vertex.xyz;


	// TODO: lighting computations
	// TODO: z-buffer computations
	
	// Regular pipeline
	gl_Position = gl_ProjectionMatrix * vec4( v_posViewSpace, 1.0 );
	gl_FrontColor = gl_Color; // TODO: remove
}
