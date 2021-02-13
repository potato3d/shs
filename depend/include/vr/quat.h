#pragma once
#ifndef _VR_QUAT_H_
#define _VR_QUAT_H_

#include <vr/common.h>
#include <vr/vec3.h>

namespace vr {

/*!
	\brief Quaternion class.
	This is a template class. You should use the classes quatf (for single-precision)
	or quatd (for double-precision) instead.
 */
template<typename real>
class quat
{
public:
	// Constructors
	inline quat(); // Uninitialized
	inline quat( real x, real y, real z, real w );
	inline quat( real radians, const vec3<real>& axis );
	inline quat( real radians1, const vec3<real>& axis1, 
				 real radians2, const vec3<real>& axis2, 
				 real radians3, const vec3<real>& axis3 );

	// Getters
	inline void getRotation( real& radians, real& x, real& y, real& z ) const;
	inline void getRotation( real& radians, vec3<real>& vec ) const;
	inline real* ptr();
	inline real  operator[]( int32 i ) const;
	inline real& operator[]( int32 i );

	// Setters
	inline void set( real x, real y, real z, real w );
	inline void set( const vec3<real>& v, real w );

	// Zero Rotation
	inline bool isZeroRotation() const;
	inline void makeZeroRotation();

	// Assignment operators
	inline const quat<real>& operator=( const quat<real>& other );

	// Arithmetic operations
	inline quat<real> operator+( const quat<real>& other ) const;
	inline quat<real> operator-( const quat<real>& other ) const;
	inline quat<real> operator*( const quat<real>& other ) const;
	inline quat<real> operator*( real scalar ) const;
	inline quat<real> operator-() const;

	// Arithmetic operations (with self assignment)
	inline quat<real>& operator+=( const quat<real>& other );
	inline quat<real>& operator-=( const quat<real>& other );
	inline quat<real>& operator*=( const quat<real>& other );
	inline quat<real>& operator*=( real scalar );

	// Quaternion operations
	inline real length() const;
	inline real length2() const;
	inline quat<real> conjugate() const;
	inline quat<real> inverse() const;

	// Rotation operations
	inline void makeRotation( real radians, real x, real y, real z );
	inline void makeRotation( real radians, const vec3<real>& vec );
	inline void makeRotation( real radians1, const vec3<real>& axis1,
							  real radians2, const vec3<real>& axis2,
							  real radians3, const vec3<real>& axis3 );

	/**
	 *	Make a rotation quat which will rotate vec1 to vec2. Generally take a dot product to get 
	 *  the radians between these and then use a cross product to get the rotation axis.
	 *  Watch out for the two special cases when the vectors are co-incident or opposite in direction.
	 */
	inline void makeRotation( const vec3<real>& from, const vec3<real>& to );

	// Spherical Linear Interpolation.
	// As t goes from 0 to 1, the quat object goes from "from" to "to".
	inline void slerp( real t, const quat<real>& from, const quat<real>& to );

	// Rotate a vector by this quat.
	inline void transform( vec3<real>& v ) const;

private:
	real _v[4];
};

/************************************************************************/
/* quat<real> Members                                                   */
/************************************************************************/

template<typename real>
quat<real>::quat()
{
	// Do not initialize for performance!
}

template<typename real>
quat<real>::quat( real x, real y, real z, real w )
{
	set( x, y, z, w );
}

template<typename real>
quat<real>::quat( real radians, const vec3<real>& axis )
{
	makeRotation( radians, axis );
}

template<typename real>
quat<real>::quat( real radians1, const vec3<real>& axis1,  
				  real radians2, const vec3<real>& axis2,  
				  real radians3, const vec3<real>& axis3 )
{
	makeRotation( radians1, axis1, radians2, axis2, radians3, axis3 );
}

template<typename real>
void quat<real>::getRotation( real& radians, real& x, real& y, real& z ) const
{
	void _vr_quat_getRotation( const quat<real>& q, real& radians, real& x, real& y, real& z );
	_vr_quat_getRotation( *this, radians, x, y, z );
}

template<typename real>
void quat<real>::getRotation( real& radians, vec3<real>& vec ) const
{
	getRotation( radians, vec.x, vec.y, vec.z );
}

template<typename real>
real* quat<real>::ptr()
{
	return _v;
}

template<typename real>
real quat<real>::operator[]( int32 i ) const
{
	return _v[i];
}

template<typename real>
real& quat<real>::operator[]( int32 i )
{
	return _v[i];
}

template<typename real>
void quat<real>::set( real x, real y, real z, real w )
{
	_v[0] = x;
	_v[1] = y;
	_v[2] = z;
	_v[3] = w;
}

template<typename real>
void quat<real>::set( const vec3<real>& v, real w )
{
	_v[0] = v.x;
	_v[1] = v.y;
	_v[2] = v.z;
	_v[3] = w;
}

template<typename real>
bool quat<real>::isZeroRotation() const
{
	return	vr::isEqual<real>( _v[0], 0 ) && 
			vr::isEqual<real>( _v[1], 0 ) && 
			vr::isEqual<real>( _v[2], 0 ) && 
			vr::isEqual<real>( _v[3], 1 );
}

template<typename real>
void quat<real>::makeZeroRotation()
{
	set( 0, 0, 0, 1 );
}

// Assignment operators
template<typename real>
const quat<real>& quat<real>::operator=( const quat<real>& other )
{
	_v[0] = other._v[0];
	_v[1] = other._v[1];
	_v[2] = other._v[2];
	_v[3] = other._v[3];
	return other;
}

template<typename real>
quat<real> quat<real>::operator+( const quat<real>& other ) const
{
	return quat<real>( _v[0] + other._v[0], _v[1] + other._v[1], _v[2] + other._v[2], _v[3] + other._v[3] );
}

template<typename real>
quat<real> quat<real>::operator-( const quat<real>& other ) const
{
	return quat<real>( _v[0] - other._v[0], _v[1] - other._v[1], _v[2] - other._v[2], _v[3] - other._v[3] );
}

template<typename real>
quat<real> quat<real>::operator*( const quat<real>& other ) const
{
	return quat<real>( other._v[3]*_v[0] + other._v[0]*_v[3] + other._v[1]*_v[2] - other._v[2]*_v[1],
					   other._v[3]*_v[1] - other._v[0]*_v[2] + other._v[1]*_v[3] + other._v[2]*_v[0],
					   other._v[3]*_v[2] + other._v[0]*_v[1] - other._v[1]*_v[0] + other._v[2]*_v[3],
					   other._v[3]*_v[3] - other._v[0]*_v[0] - other._v[1]*_v[1] - other._v[2]*_v[2] );
}

template<typename real>
quat<real> quat<real>::operator*( real scalar ) const
{
	return quat<real>( _v[0] * scalar, _v[1] * scalar, _v[2] * scalar, _v[3] * scalar );
}

template<typename real>
quat<real> quat<real>::operator-() const
{
	return quat<real>( -_v[0], -_v[1], -_v[2], -_v[3] );
}

template<typename real>
quat<real>& quat<real>::operator+=( const quat<real>& other )
{
	_v[0] += other._v[0];
	_v[1] += other._v[1];
	_v[2] += other._v[2];
	_v[3] += other._v[3];
	return *this;
}

template<typename real>
quat<real>& quat<real>::operator-=( const quat<real>& other )
{
	_v[0] -= other._v[0];
	_v[1] -= other._v[1];
	_v[2] -= other._v[2];
	_v[3] -= other._v[3];
	return *this;
}

template<typename real>
quat<real>& quat<real>::operator*=( const quat<real>& other )
{
	real x = other._v[3]*_v[0] + other._v[0]*_v[3] + other._v[1]*_v[2] - other._v[2]*_v[1];
	real y = other._v[3]*_v[1] - other._v[0]*_v[2] + other._v[1]*_v[3] + other._v[2]*_v[0];
	real z = other._v[3]*_v[2] + other._v[0]*_v[1] - other._v[1]*_v[0] + other._v[2]*_v[3];
	_v[3]  = other._v[3]*_v[3] - other._v[0]*_v[0] - other._v[1]*_v[1] - other._v[2]*_v[2];
	_v[2] = z;
	_v[1] = y;
	_v[0] = x;

	return *this;
}

template<typename real>
quat<real>& quat<real>::operator*=( real scalar )
{
	_v[0] *= scalar;
	_v[1] *= scalar;
	_v[2] *= scalar;
	_v[3] *= scalar;
	return *this;
}

template<typename real>
real quat<real>::length() const
{
	return sqrt( length2() );
}

template<typename real>
real quat<real>::length2() const
{
	return _v[0]*_v[0] + _v[1]*_v[1] + _v[2]*_v[2] + _v[3]*_v[3];
}

template<typename real>
quat<real> quat<real>::conjugate() const
{
	return quat<real>( -_v[0], -_v[1], -_v[2], _v[3] );
}

template<typename real>
quat<real> quat<real>::inverse() const
{
	return conjugate() * ( static_cast<real>(1.0) / length2() );
}

template<typename real>
void quat<real>::makeRotation( real radians, real x, real y, real z )
{
	void _vr_quat_makeRotation( quat<real>& q, real radians, real x, real y, real z );
	_vr_quat_makeRotation( *this, radians, x, y, z );
}

template<typename real>
void quat<real>::makeRotation( real radians, const vec3<real>& vec )
{
	makeRotation( radians, vec.x, vec.y, vec.z );
}

template<typename real>
void quat<real>::makeRotation( real radians1, const vec3<real>& axis1,
							   real radians2, const vec3<real>& axis2,
							   real radians3, const vec3<real>& axis3 )
{
	*this = quat<real>( radians1, axis1 ) * quat<real>( radians2, axis2 ) * quat<real>( radians3, axis3 );
}

template<typename real>
void quat<real>::makeRotation( const vec3<real>& from, const vec3<real>& to )
{
	void _vr_quat_makeRotation( quat<real>& q, const vec3<real>& from, const vec3<real>& to );
	_vr_quat_makeRotation( *this, from, to );
}

template<typename real>
void quat<real>::slerp( real t, const quat<real>& from, const quat<real>& to )
{
	void _vr_quat_slerp( quat<real>& q, real t, const quat<real>& from, const quat<real>& to );
	_vr_quat_slerp( *this, t, from, to );
}

template<typename real>
void quat<real>::transform( vec3<real>& v ) const
{
	// nVidia SDK implementation
	vec3<real> uv, uuv;
	vec3<real> qvec( _v[0], _v[1], _v[2] );
	uv = qvec.cross( v );
	uuv = qvec.cross( uv );
	uv *= ( static_cast<real>( 2.0 ) * _v[3] );
	uuv *= 2;
	v += uv + uuv;
}

/************************************************************************/
/* quat<real> Instantiations                                            */
/************************************************************************/

/*
	Quaternion class with a double-precision implementation (recommended).
 */
class VRBASE_EXPORT quatd : public quat<double>
{
public:
	//! Constructs an uninitialized quaternion.
	inline quatd() : quat<double>() {;}

	//! Constructs a quaternion with the specified internal parameters.
	inline quatd( double x, double y, double z, double w ) : quat<double>( x, y, z, w ) {;}

	//! Constructs a quaternion that matches the rotation of an angle (in 'radians') around an 'axis'.
	inline quatd( double radians, const vec3<double>& axis ) : quat<double>( radians, axis ) {;}

	//! Constructs a quaternion that matches the accumulated rotations of three angles around three axes.
	inline quatd( double radians1, const vec3<double>& axis1, 
				  double radians2, const vec3<double>& axis2, 
				  double radians3, const vec3<double>& axis3 ) 
		: quat<double>( radians1, axis1, radians2, axis2, radians3, axis3 ) {;}

	using quat<double>::operator=;
};

/*
	Quaternion class with a single-precision implementation.
	Use this class only if performance is a major concern and precision is not a problem.
 */
class VRBASE_EXPORT quatf : public quat<float>
{
public:
	//! Constructs an uninitialized quaternion.
	inline quatf() : quat<float>() {;}

	//! Constructs a quaternion with the specified internal parameters.
	inline quatf( float x, float y, float z, float w ) : quat<float>( x, y, z, w ) {;}

	//! Constructs a quaternion that matches the rotation of an angle (in 'radians') around an 'axis'.
	inline quatf( float radians, const vec3<float>& axis ) : quat<float>( radians, axis ) {;}

	//! Constructs a quaternion that matches the accumulated rotations of three angles around three axes.
	inline quatf( float radians1, const vec3<float>& axis1, 
				  float radians2, const vec3<float>& axis2, 
				  float radians3, const vec3<float>& axis3 ) 
		: quat<float>( radians1, axis1, radians2, axis2, radians3, axis3 ) {;}

	using quat<float>::operator=;
};

} // namespace vr

#endif
