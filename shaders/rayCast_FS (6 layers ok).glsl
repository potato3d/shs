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
uniform sampler2D u_hm5;
uniform sampler2D u_hm6;

uniform sampler2D u_normal1;
uniform sampler2D u_normal2;
uniform sampler2D u_normal3;
uniform sampler2D u_normal4;
uniform sampler2D u_normal5;
uniform sampler2D u_normal6;


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


// Linear ray intersection
int inCastLinear( inout vec4 current, in vec4 step, sampler2D heightmap )
{
	float height;
	bool detail_search = false;

    for( int i = 0; i < 1024; ++i )
	{
		current += step;
		height = texture2D( heightmap, current.xy ).r;
		if( current.z <= height )
		{
			if (detail_search)
				return ENTER;
			detail_search = true;
			current -= step;
			step *= 0.1;
			continue;
		}

		if( current.w <= 0.0 )
			return END;
	}
}

int outCastLinear( inout vec4 current, in vec4 step, sampler2D heightmap )
{
	float height;
	vec4 s = step;
	bool detail_search = false;

    for( int i = 0; i < 1024; ++i )
	{
		current += step;
		height = texture2D( heightmap, current.xy ).r;
		if( current.z > height )
		{
			if (detail_search)
				return EXIT;
			detail_search = true;
			current -= step;
			step *= 0.1;
			continue;
		}

		if( current.w <= 0.0 )
			return END;
	}
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
			if (detail_search)
				return ENTER;
			detail_search = true;
			current -= step;
			step *= 0.1;
			continue;
		}

		heightOut = texture2D( heightmapOut, current.xy ).r;
		if( current.z > heightOut )
		{
			if (detail_search)
				return EXIT;
			detail_search = true;
			current -= step;
			step *= 0.1;
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

vec3 computeHalfInterpolation( vec4 current, vec3 currNormal, sampler2D otherNormalMap, float otherHeight )
{
	float threshold = 0.075;//0.005;
	float diff = abs( current.z - otherHeight );
	//vec3 currNormal = texture2D( currNormalMap, current.xy ).rgb;

	if( diff > threshold )
		return currNormal;

	float factor = (diff / threshold)*0.5 + 0.5;
	//float factor = smoothstep (0.0,threshold,diff)*0.5 + 0.5;
	return mix( texture2D( otherNormalMap, current.xy ).rgb, currNormal, factor );
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
	step.xyz *= 0.004;

	// Compute limit to quickly terminate linear casting when ray exits box
	computeBoxExitInW( current, step );

	int condition;
 	vec3 normal = vec3(1,1,1);
	float height;
    float count;
	float diff2;
	float diff3;

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
#ifdef testa_layer
			normal.xyz = vec3(1,0,0);
			//normal.xyz = vec3(0,0,0);
#else
			//normal = texture2D( u_normal1, current.xy ).rgb;
			vec3 normal1 = texture2D( u_normal1, current.xy ).rgb;
			normal = computeHalfInterpolation( current, normal1, u_normal2, height );
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
				if( current.z > height )
					break; // go back to outer loop -> even -= 2

				// Draw even
				// At this point current is just outside hm2, 
				// so we bring it back to just inside hm2 to access correct normal
				current -= step*0.1;
				float diff2 = ( texture2D( u_hm2, current.xy ).r - current.z );
				if ( abs(diff2) > 0.2 ) 
					current -= step*0.1;

#ifdef testa_layer
				normal.xyz = vec3(0,1,0);
				//normal.xyz = vec3(0,0,0);
#else
				//normal = texture2D( u_normal2, current.xy ).rgb;
				float height1 = texture2D( u_hm1, current.xy ).r;
				float height3 = texture2D( u_hm3, current.xy ).r;
				float diff1 = abs( current.z - height1 );
				float diff3 = abs( current.z - height3 );
				vec3 normal2 = texture2D( u_normal2, current.xy ).rgb;
				if( diff1 < diff3 )
					normal = computeHalfInterpolation( current, normal2, u_normal1, height1 );
				else
					normal = computeHalfInterpolation( current, normal2, u_normal3, height3 );
#endif
				// exit all loops because at this point height <= current.z (see if above)
				height = 2.0;
				break;
			}
dbg += 0.25;

			// If not enter even + 2
			height = texture2D( u_hm4, current.xy ).r;
			if( current.z > height * 1.1 ) // TODO: fixes bunny head
			{
				// Draw odd
#ifdef testa_layer
				normal.xyz = vec3(1,1,0);
				//normal.xyz = vec3(0,0,0);
#else
				//normal = texture2D( u_normal3, current.xy ).rgb;
				float height2 = texture2D( u_hm2, current.xy ).r;
				float height4 = texture2D( u_hm4, current.xy ).r;
				float diff2 = abs( current.z - height2 );
				float diff4 = abs( current.z - height4 );
				vec3 normal3 = texture2D( u_normal3, current.xy ).rgb;
				if( diff2 < diff4 )
					normal = computeHalfInterpolation( current, normal3, u_normal2, height2 );
				else
					normal = computeHalfInterpolation( current, normal3, u_normal4, height4 );
#endif
				height = 2.0;
				break;
			}

			do
			{

				// even == 4
				// odd  == 5
				condition = inOutCastLinear( current, step, u_hm5, u_hm4 );

				if( condition == END )
					discard;

				if( condition == EXIT )
				{
					// If exit even - 1
					height = texture2D( u_hm3, current.xy ).r;
					if( current.z > height )
						break; // go back to outer loop -> even -= 2

					// Draw even
					// At this point current is just outside hm2, 
					// so we bring it back to just inside hm4 to access correct normal
					current -= step*0.1;
					float diff2 = ( texture2D( u_hm4, current.xy ).r - current.z );
					if ( abs(diff2) > 0.2 ) 
						current -= step*0.1;

	#ifdef testa_layer
					normal.xyz = vec3(0,1,0);
					//normal.xyz = vec3(0,0,0);
	#else
					//normal = texture2D( u_normal2, current.xy ).rgb;
					float height3 = texture2D( u_hm3, current.xy ).r;
					float height5 = texture2D( u_hm5, current.xy ).r;
					float diff3 = abs( current.z - height3 );
					float diff5 = abs( current.z - height5 );
					vec3 normal4 = texture2D( u_normal4, current.xy ).rgb;
					if( diff3 < diff5 )
						normal = computeHalfInterpolation( current, normal4, u_normal3, height3 );
					else
						normal = computeHalfInterpolation( current, normal4, u_normal5, height5 );
	#endif
					// exit all loops because at this point height <= current.z (see if above)
					height = 2.0;
					break;
				}
	dbg += 0.25;

				// If not enter even + 2
				height = texture2D( u_hm6, current.xy ).r;
				if( current.z > height * 1.1 ) // TODO: fixes bunny head
				{
					// Draw odd
	#ifdef testa_layer
					normal.xyz = vec3(1,0,1);
					//normal.xyz = vec3(0,0,0);
	#else
					//normal = texture2D( u_normal3, current.xy ).rgb;
					float height4 = texture2D( u_hm4, current.xy ).r;
					float height6 = texture2D( u_hm6, current.xy ).r;
					float diff4 = abs( current.z - height4 );
					float diff6 = abs( current.z - height6 );
					vec3 normal5 = texture2D( u_normal5, current.xy ).rgb;
					if( diff4 < diff6 )
						normal = computeHalfInterpolation( current, normal5, u_normal4, height4 );
					else
						normal = computeHalfInterpolation( current, normal5, u_normal6, height6 );
	#endif
					height = 2.0;
					break;
				}

				// TODO: testing with only 6 layers
				// even == 6
				// odd  == 7
				condition = outCastLinear( current, step, u_hm6 );
				if( condition == END )
					discard;

				// If exit even - 1
				height = texture2D( u_hm5, current.xy ).r;
				if( current.z > height )
					continue; // go back to outer loop -> even -= 2

				// Draw even
	#ifdef testa_layer
				normal.xyz = vec3(1,1,1);
				//normal.xyz = vec3(0,0,0);
	#else
				//normal = texture2D( u_normal4, current.xy ).rgb;
				vec3 normal6 = texture2D( u_normal6, current.xy ).rgb;
				normal = computeHalfInterpolation( current, normal6, u_normal5, height );
	#endif
				// exit all loops because at this point height <= current.z (see if above)
				break;

			} while( current.z > height );

		} while( current.z > height );

	} while( current.z > height );

	//if( diff2 < diff3 )
	//	normal.xyz = vec3(1.0);
	
	vec3 lightDir;

	lightDir = normalize (vec3(1,1,0));
	//normal = gl_NormalMatrix * norm
	//lightDir.x = dot( gl_ModelViewMatrix[0].xyz, gl_LightSource[0].position.xyz );
	//lightDir.y = dot( gl_ModelViewMatrix[1].xyz, gl_LightSource[0].position.xyz );
	//lightDir.z = dot( gl_ModelViewMatrix[2].xyz, gl_LightSource[0].position.xyz );
	//lightDir = normalize( lightDir );   //todo: do we need this?


#ifdef testa_layer
	gl_FragColor.rgb = normal.xyz;
	//gl_FragColor.rgb = vec3(0.6);
	//gl_FragColor.rgb = current.xyz;
	//gl_FragColor.rgb = vec3(1.0-dbg);
	//gl_FragColor.rgb = vec3(dbg);
#else
	//gl_FragColor.r = length(normal.xyz) * 0.5;
	//return;
	normal = normalize(normal);
	//normal = abs(normal);
	//normal =- normal;
	gl_FragColor.rgb = ( vec3(dot( lightDir, normal )) )*0.8 + vec3(0.2);
	//gl_FragColor.rgb = normal.xyz;
	//gl_FragColor.rgb = current.zzz;
#endif

	return;

	// TODO: Z-buffer computations
	// TODO: Lighting computations

	// TODO: remove
	gl_FragColor = gl_Color;
}

