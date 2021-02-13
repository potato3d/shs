#include "ArcBall.h"

Arcball::Arcball()
{
	_q.makeZeroRotation();
	_is_dragging = false;
}

void Arcball::beginDrag( const vec2f &p )
{
	_start = _q;
	_from = pointOnSphere( p );
	_is_dragging = true;
}

void Arcball::updateDrag( const vec2f &p )
{
	const vec3f to = pointOnSphere( p );
	_q.makeRotation( _from, to );
	_q = _start * _q;
}

vec3f Arcball::pointOnSphere( const vec2f &p ) const
{
	vec3f p1( p.x, p.y, 0 );

	float m = p.length2();
	if( m < 1.0 )
	{
		// point (x, y) is above the unit sphere.
		p1.z = sqrt( 1.0f - m );
	}
	else
	{
		// if the point does not lie on the sphere, push it back to the sphere border.
		m = sqrt( m );
		p1 *= 1.0 / m;
	}

	return p1;
}
