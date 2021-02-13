#ifndef _AABB_H_
#define _AABB_H_

#include <vr/vec3.h>

class AABB
{
public:
	vr::vec3d center() const
	{
		return ( minV + maxV ) * 0.5f;
	}
	
	double extent( vr::uint32 axis ) const
	{
		return maxV[axis] - minV[axis];
	}

public:
	vr::vec3d minV;
	vr::vec3d maxV;
};

#endif // _AABB_H_
