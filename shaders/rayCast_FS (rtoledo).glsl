#version 110
//#define testa_layer

/************************************************************************/
/* Varying                                                              */
/************************************************************************/
// Ray direction in tangent space
varying vec3 v_viewDir;

// Vertex position in View Space
varying vec3 v_posViewSpace;

// Ray origin
varying vec3 v_rayOrigin;

/************************************************************************/
/* Uniforms                                                             */
/************************************************************************/
// Heightmap texture
uniform sampler2D u_hm1;
uniform sampler2D u_hm2;
uniform sampler2D u_hm3;
uniform sampler2D u_hm4;

uniform sampler2D u_normal1;
uniform sampler2D u_normal2;
uniform sampler2D u_normal3;
uniform sampler2D u_normal4;


/************************************************************************/
/* Globals                                                              */
/************************************************************************/
#define END    0
#define EXIT   1
#define ENTER  2
float dbg = 0.0;

// Compute limit to quickly terminate linear casting when ray exits box
void computeBoxExitInW( inout vec4 current, inout vec4 step_vec )
{
	vec3 dir01 = sign(step_vec.xyz);  // -1 or 1
	dir01 = dir01*0.5+0.5; // 0 or 1
	vec3 t3 = dir01;
	dir01 -= current.xyz;  // the absolute value of dir01 is the distance to exit bounding box
	dir01 /= step_vec.xyz; // dir01 gives how many steps. Note that the value is surely positive

	if( dir01.x < dir01.y && dir01.x < dir01.z )
	{
		step_vec.w = step_vec.x;
		current.w  = t3.x - current.x;
	}
	else if( dir01.y < dir01.z )
	{
		step_vec.w = step_vec.y;
		current.w  = t3.y - current.y;
	}
	else
	{
		step_vec.w = step_vec.z;
		current.w  = t3.z - current.z;
	}

	// Guarantee that current.w is positive and step_vec.w is negative, 
	// so it will eventually converge to zero (when the fragment is discarded).
	current.w = ( current.w > 0.0 ) ? current.w : -current.w; //distance to exit bbox
	step_vec.w = ( step_vec.w > 0.0 ) ? -step_vec.w : step_vec.w; //step_size in exiting direction
}

// Binary search to refine current ray intersection
void inCastBinary( inout vec4 current, inout vec4 step, int numSteps, sampler2D heightmap )
{
	vec4 pos_neg_step;  //+or- step_vec
	step *= 0.5;
	pos_neg_step = -step;  //start by returning half-step
	float height;
	// recurse around THE TWO LAST SEARCHING pointS (heightS) for closest match
	for( int i = 0; i < numSteps; i++ )
	{
		current += pos_neg_step;
		height = texture2D( heightmap, current.xy ).r;
		step *= 0.5;
		pos_neg_step = ( current.z < height ) ? -step : step;
	}
	current += pos_neg_step;
}

// Binary search to refine current ray intersection
void outCastBinary( inout vec4 current, inout vec4 step, int numSteps, sampler2D heightmap )
{
	vec4 pos_neg_step;  //+or- step_vec
	step *= 0.5;
	pos_neg_step = -step;  //start by returning half-step
	float height;
	// recurse around THE TWO LAST SEARCHING pointS (heightS) for closest match
	for( int i = 0; i < numSteps; i++ )
	{
		current += pos_neg_step;
		height = texture2D( heightmap, current.xy ).r;
		step *= 0.5;
		pos_neg_step = ( current.z >= height ) ? -step : step;
	}
	current += pos_neg_step;
}

// Linear ray intersection
int inCastLinear( inout vec4 current, inout vec4 step, sampler2D heightmap )
{
	float height;

    for( int i = 0; i < 1024; ++i )
	{
		current += step;
		height = texture2D( heightmap, current.xy ).r;
		if( current.z <= height )
			return ENTER;

		current += step;
		height = texture2D( heightmap, current.xy ).r;
		if( current.z <= height )
			return ENTER;

		current += step;
		height = texture2D( heightmap, current.xy ).r;
		if( current.z <= height )
			return ENTER;

		current += step;
		height = texture2D( heightmap, current.xy ).r;
		if( current.z <= height )
			return ENTER;
		
		if( current.w <= 0.0 )
			return END;
	}
}

int outCastLinear( inout vec4 current, inout vec4 step, sampler2D heightmap )
{
	float height;
	vec4 s = step;

    for( int i = 0; i < 1024; ++i )
	{
		current += step;
		height = texture2D( heightmap, current.xy ).r;
		if( current.z > height )
		{
			//outCastBinary( current, s, 5, heightmap );
			//current += s;
			return EXIT;
		}

		//current += step;
		//height = texture2D( heightmap, current.xy ).r;
		//if( current.z > height )
		//	return EXIT;

		//current += step;
		//height = texture2D( heightmap, current.xy ).r;
		//if( current.z > height )
		//	return EXIT;

		//current += step;
		//height = texture2D( heightmap, current.xy ).r;
		//if( current.z > height )
		//	return EXIT;
		
		if( current.w <= 0.0 )
			return END;
	}
}

bool checkEnter( inout vec4 current, vec4 step, sampler2D heightmapIn )
{
	vec4 prevCurrent = current - step;
	vec4 sstep = step * 0.01;

	for( int i = 0; i < 100; ++i )
	{
		prevCurrent += sstep;
		float heightIn = texture2D( heightmapIn, prevCurrent.xy ).r;
		if( current.z <= heightIn )
		{
			current = prevCurrent;
			return true;
		}
	}

	return false;
}

bool checkExit( inout vec4 current, vec4 step, sampler2D heightmapOut )
{
	vec4 prevCurrent = current - step;
	vec4 sstep = step * 0.01;

	for( int i = 0; i < 100; ++i )
	{
		prevCurrent += sstep;
		float heightOut = texture2D( heightmapOut, prevCurrent.xy ).r;
		if( current.z > heightOut )
		{
			current = prevCurrent;
			return true;
		}
	}

	return false;
}

int inOutCastLinear( inout vec4 current, in vec4 step, sampler2D heightmapIn, sampler2D heightmapOut )
{
	float heightIn;
	float heightOut;
	vec4 s = step;
	bool detail_search = false;

    for( int i = 0; i < 1024; ++i )
	{
		current += step;
		heightIn = texture2D( heightmapIn, current.xy ).r;
		if( current.z <= heightIn )
		{
			//inCastBinary( current, s, 5, heightmapIn );
			//current += s;
			return ENTER;
		}
		
		heightOut = texture2D( heightmapOut, current.xy ).r;
		if( current.z > heightOut )
		{
			if (detail_search)
			{
				return EXIT;
			}
			detail_search = true;
			current -= step;
			step *= 0.1;
			continue;

			//if( checkEnter( current, step, heightmapIn ) )
				//return ENTER;
			//outCastBinary( current, s, 5, heightmapOut );
			//current += s;
		}

		if( current.w <= 0.0 )
			return END;
	}
}

vec3 neighborsDiff( vec3 current_pos, vec3 shift, sampler2D heightMap )
{
  vec3 v1 = current_pos.xyz + shift;
  v1.z = texture2D(heightMap, v1.xy).r;
  vec3 v2 = current_pos.xyz - shift;
  v2.z = texture2D(heightMap, v2.xy).r;
  return vec3(v1-v2);
}

vec3 computeNormal( float neigb_dist, vec3 current_pos, sampler2D heightMap )
{
  vec3 shift, v1, v2;
  shift.x = neigb_dist;
  shift.yz = vec2(0.0,0.0);
  v1 = neighborsDiff(current_pos, shift, heightMap);
  shift.x = 0.0;
  shift.y = neigb_dist;
  v2 = neighborsDiff(current_pos, shift, heightMap);
  v1 = cross(v1,v2);
  v1.z *= 2.0;
  return normalize(v1);
}

vec3 computeFinalNormal( vec4 current, sampler2D currNormalMap, sampler2D nextNormalMap, float nextHeight )
{
	float threshold = 0.05;//0.005;
	float diff = abs( current.z - nextHeight );
	vec3 currNormal = texture2D( currNormalMap, current.xy ).rgb;

	if( diff > threshold )
		return currNormal;
		//return vec3(1,0,0);

	//return vec3(1,1,0);

	//return mix( currNormal, texture2D( nextNormalMap, current.xy ).rgb, (diff / threshold)*0.5 + 0.5 );
	return mix( texture2D( nextNormalMap, current.xy ).rgb, currNormal, (diff / threshold)*0.5 + 0.5 );
}

/************************************************************************/
/* Main                                                                 */
/************************************************************************/
void main( void )
{
	// Ray origin and direction
	vec4 current;
	vec4 step;

	// Setup ray direction
	vec3 viewDir = normalize( v_viewDir );
	step.xyz = viewDir;

	// Current is at ray origin (on one of the box faces)
	current.xyz = v_rayOrigin;

	// TODO: compute number of steps in linear and binary searches
	step.xyz *= 0.008;

	// Compute limit to quickly terminate linear casting when ray exits box
	computeBoxExitInW( current, step );

	

/*
	x is even
	y is odd

	while( ENTER box )

	if EXIT x
		if ENTER x - 1
			draw x and return
		else
			x -= 2

		y = x + 1

		if ENTER y
			if ENTER x + 2
				x += 2
			else
				draw y and return
*/

	int condition;
 	vec3 normal = vec3(1,1,1);
	float height;
    float count;

	//current += step;

	do
	{
		// even == 0
		// odd  == 1
		condition = inCastLinear( current, step, u_hm1 );

		if( condition == END )
			discard;

		// If not enter even + 2
		height = texture2D( u_hm2, current.xy ).r;
		if( current.z > height )
		{
			// Draw odd
			inCastBinary( current, step, 4, u_hm1 );
			current += step;
#ifdef testa_layer
			normal.xyz = vec3(1,0,0);
#else
			normal = texture2D( u_normal1, current.xy ).rgb;
			//normal = computeFinalNormal( current, u_normal1, u_normal2, height );
#endif
			break;
		}

		do
		{
			// even == 2
			// odd  == 3
			condition = inOutCastLinear( current, step, u_hm3, u_hm2 );

			if( condition == END )
				discard;

			if( condition == EXIT )
			{
				// If exit even - 1
				height = texture2D( u_hm1, current.xy ).r;
				if( current.z > height/* * 0.9*/ )
				{
					/*height *= 0.5;*/
					break; // go back to outer loop -> even -= 2
				}

				// Draw even
				//current+= step;
			    //outCastBinary( current, step, 1, u_hm2 );
#ifdef testa_layer
				normal.xyz = vec3(0,1,0);
				//normal.xyz = vec3(0,0,0);
#else
				normal = texture2D( u_normal2, current.xy ).rgb;
				//normal = computeFinalNormal( current, u_normal2, u_normal1, height );
#endif
				// exit all loops because at this point height <= current.z (see if above)
			    dbg = 1.0;
				break;
			}

			// If not enter even + 2
			height = texture2D( u_hm4, current.xy ).r;
			if( current.z > height )
			{
				// Draw odd
				inCastBinary( current, step, 4, u_hm3 );
				current += step;
#ifdef testa_layer
				normal.xyz = vec3(1,1,0);
				//normal.xyz = vec3(0,0,0);
#else
				normal = texture2D( u_normal3, current.xy ).rgb;
#endif
				//normal = computeFinalNormal( current, u_normal3, u_normal4, height );
				height = 2.0;
				break;
			}

			// TODO: testing with only 4 layers
			// even == 4
			// odd  == 5
			condition = outCastLinear( current, step, u_hm4 );
			if( condition == END )
				discard;

			// If exit even - 1
			height = texture2D( u_hm3, current.xy ).r;
			if( current.z > height )
				continue; // go back to outer loop -> even -= 2

			// Draw even
			outCastBinary( current, step, 4, u_hm4 );
			current-= step;
#ifdef testa_layer
			normal.xyz = vec3(0,1,1);
#else
			normal = texture2D( u_normal4, current.xy ).rgb;
#endif
			//normal = computeFinalNormal( current, u_normal4, u_normal3, height );
			// exit all loops because at this point height <= current.z (see if above)
			break;

		} while( current.z > height );

	} while( current.z > height );

	
	vec3 lightDir;

	lightDir = normalize (vec3(1,1,0));
	//normal = gl_NormalMatrix * norm
	//lightDir.x = dot( gl_ModelViewMatrix[0].xyz, gl_LightSource[0].position.xyz );
	//lightDir.y = dot( gl_ModelViewMatrix[1].xyz, gl_LightSource[0].position.xyz );
	//lightDir.z = dot( gl_ModelViewMatrix[2].xyz, gl_LightSource[0].position.xyz );
	//lightDir = normalize( lightDir );   //todo: do we need this?


	//gl_FragColor.r = length(normal.xyz) * 0.5;
	//return;
	//normal = normalize(normal);
	normal = abs(normal);
	gl_FragColor.rgb = ( vec3(dot( lightDir, normal )) )*0.8 + vec3(0.2);

//#ifdef testa_layer
	gl_FragColor.rgb = normal.xyz;
	//gl_FragColor.rgb = vec3(0.6);
	//gl_FragColor.rgb = current.xyz;
	//gl_FragColor.rgb = vec3(1.0-dbg);
//#endif
	return;

	// TODO: Z-buffer computations
	// TODO: Lighting computations

	// TODO: remove
	gl_FragColor = gl_Color;
}

	// Region 1
// 	do
// 	{
// 		inCastLinear( current, step, u_hm0 );
// 		if( current.w <= 0.0 )
// 			discard;
// 		
// 		// Region 2
// 		depth = texture2D( u_hm1, current.xy ).r;
// 		if ( current.z > depth )
// 		{
// 			normal = computeNormal( 0.01, current.xyz, u_hm0 );
// 			break;
// 		}
// 		
// 		outCastLinear( current, step, u_hm1 );
// 		if( current.w <= 0.0 )
// 			discard;
// 
// 		depth = texture2D( u_hm0, current.xy ).r;
// 		if( current.z <= depth )
// 		{
// 			normal = computeNormal( 0.01, current.xyz, u_hm1 );
// 			normal = -normal;
// 			break;
// 		}
// 
// 	} while( current.z > depth );

