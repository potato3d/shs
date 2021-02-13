/*************************************************************
* Simple Geometry Shader for testing heightmap texture
**************************************************************/

#version 120
#extension GL_EXT_geometry_shader4 : enable
#extension GL_EXT_gpu_shader4 : enable

// Number of nodes in recursion tree
uniform int nodeCount;

// Heighmap texture
uniform sampler2D lodTexSampler;
uniform float invLodTexWidth;
uniform float invLodTexHeight;

// ST texture
uniform sampler1D stTexSampler;

// Path texture
uniform sampler1D pathTexSampler;

void computeVertex( int i )
{
	// Get vertex position
	vec4 vertex = gl_PositionIn[i];
	
	// Compute texel coords
	vec2 texelCoord;
	texelCoord.x = vertex.x * invLodTexWidth;
	texelCoord.y = vertex.y * invLodTexHeight;
	
	// Get vertex height
	vertex.z = texture2D( lodTexSampler, texelCoord, 0 ).r;
	
	// Get vertex color
	vec4 color = gl_FrontColorIn[i];
	
	// Output vertex position
	gl_Position = gl_ModelViewProjectionMatrix * vertex;
	
	// Output color
	gl_FrontColor = color;
}

void main()
{
	int i = 0;
	while( i < gl_VerticesIn )
	{  
		// Vertex 0
		computeVertex( i );
		EmitVertex();
		++i;
    
		// Vertex 1
		computeVertex( i );
		EmitVertex();
		++i;
    
    	// Vertex 2
    	computeVertex( i );
		EmitVertex();
		++i;
    
	    // Finished triangle
	    EndPrimitive();
  }  
} 
