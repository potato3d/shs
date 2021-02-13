#ifndef _ARCBALL_H_
#define _ARCBALL_H_

#include <vr/vec2.h>
#include <vr/vec3.h>
#include <vr/quat.h>

using vr::vec2f;
using vr::vec3f;
using vr::quatf;

/*!
	This class implements the Arcball device as described in
	the article "Arcball Rotation Control" by Ken Shoemake
	(shoemake@graphics.cis.upenn.edu) in "Graphics Gems IV",
	Academic Press, 1994.
 */
class Arcball
{
public:
    Arcball();

    void beginDrag( const vec2f &p );

	void updateDrag( const vec2f &p );

	void endDrag() { _is_dragging = false; }
	void cancelDrag() { _q = _start; _is_dragging = false; }

	bool isDragging() const { return _is_dragging; }

	void setOrientation( const quatf &q ) { _q = q; }
	const quatf& getOrientation() const { return _q; }

private:
	vec3f pointOnSphere( const vec2f &p ) const;

private:
    quatf _q;
	quatf _start;
    vec3f _from;
    bool _is_dragging;
};

#endif
