#pragma once
#ifndef _VR_INTERPOLATION_H_
#define _VR_INTERPOLATION_H_

#include "vec2.h"
#include "vec3.h"

/*!
	\file interpolation.h Contains basic functions to interpolate between values.
 */

namespace vr {

/*!
	Linear interpolation from 'a' to 'b' as 't' goes from 0 to 1.
 */
template<typename real>
inline real lerp( real a, real b, real t )
{
	return ( 1 - t ) * a + b * t;
}

/*!
	Bilinear interpolation.
	The <tt>values</tt> array should contain 4 elements and follow this convention:
	<pre>
             (1,1)
       [2]--[3] 
        |    |
       [0]--[1]
    (0,0)
	</pre>
	Parameter <tt>st</tt> goes from 0 to 1 in both dimensions.
 */
template<typename real>
inline real bilerp( const real* values, const vec2<real>& st )
{
	vec2<real> stc( 1, 1 );
	stc -= st;

	real bottom = ( stc.s * values[0] ) + ( st.s * values[1] );
	real top = ( stc.s * values[2] ) + ( st.s * values[3] );
	return ( stc.t * bottom ) + ( st.t * top );
}

/*!
	Trilinear interpolation.
	The <tt>values</tt> array should contain 4 elements and follow this convention:
	<pre>
        [2]-----[3]
         |       |
     [6]-----[7] |     y
      |  |    |  |     |
      | [0]---|-[1]    |____x
      |       |       /
     [4]-----[5]     /z
	</pre>
	Parameter <tt>stp</tt> goes from 0 to 1 in the three dimensions.
 */
template<typename real>
inline real trilerp( const real* values, const vec3<real>& stp )
{
	real s = stp.s * 2 - 1;
	real t = stp.t * 2 - 1;
	real p = stp.p * 2 - 1;

	real s0 = 1 - s;
	real s1 = 1 + s;
	real t0 = 1 - t;
	real t1 = 1 + t;
	real p0 = 1 - p;
	real p1 = 1 + p;

	real w[8];
	w[0] = ( s0 * t0 * p0 );
	w[1] = ( s1 * t0 * p0 );
	w[2] = ( s0 * t1 * p0 );
	w[3] = ( s1 * t1 * p0 );
	w[4] = ( s0 * t0 * p1 );
	w[5] = ( s1 * t0 * p1 );
	w[6] = ( s0 * t1 * p1 );
	w[7] = ( s1 * t1 * p1 );

	real v = 0;
	for( int i = 0; i < 8; ++i )
		v += w[i] * values[i];

	return v * ( 1.0f / 8.0f );
}

} // namespace vr

#endif
