#pragma once
#ifndef _VR_ALGEBRATYPES_H_
#define _VR_ALGEBRATYPES_H_

#include "vec2.h"
#include "vec3.h"
#include "vec4.h"
#include "mat4.h"
#include "quat.h"

/*!
	Imports all basic linear algebra classes into the current namespace.
	This is useful if you want to write <tt>vec3f</tt>, <tt>mat4d</tt>, etc. in your
	program but don't want to fully import the vr namespace.
 */
#define USING_VR_ALGEBRA_TYPES	\
	using vr::vec2f;	\
	using vr::vec2d;	\
	using vr::vec3f;	\
	using vr::vec3d;	\
	using vr::mat4f;	\
	using vr::mat4d;	\
	using vr::quatf;	\
	using vr::quatd;

#endif
