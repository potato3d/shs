#ifndef _IMANIPULATOR_H_
#define _IMANIPULATOR_H_

#include <vr/mat4.h>
#include <QMouseEvent>

using vr::mat4f;

class IManipulator
{
public:
	virtual ~IManipulator()
	{;}

	//! Returns the ModelView transformation defined by the manipulator.
	virtual const mat4f& getTransform() const = 0;

	//! Returns the inverse of getTransform().
	virtual const mat4f& getInverseTransform() const = 0;

	//! Resets the ModelView transform to its initial position.
	virtual void reset() = 0;

	/*
	 *	The following methods are called by the OpenGL Canvas Widget.
	 *	They may return true to indicate that the canvas should be repainted.
	 */

	//! Called when the canvas is resized.
	virtual void resizeEvent( int width, int height ) = 0;

	//! Called when mouse events are triggered on the canvas.
	virtual bool mousePressEvent( QMouseEvent* e ) = 0;
	virtual bool mouseMoveEvent( QMouseEvent* e ) = 0;
	virtual bool mouseReleaseEvent( QMouseEvent* e ) = 0;
	
	//! Called when the mouse wheel is triggered on the canvas.
	virtual bool wheelEvent( QWheelEvent* e ) = 0;

	//! Called when a keyboard key is pressed while the canvas has focus.
	virtual bool keyPressEvent( QKeyEvent* e ) = 0;
};

#endif
