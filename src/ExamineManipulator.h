#ifndef _EXAMINEMANIPULATOR_H_
#define _EXAMINEMANIPULATOR_H_

#include "ArcBall.h"
#include "IManipulator.h"

/*!
	3D Manipulator based on an ArcBall.
 */
class ExamineManipulator : public IManipulator
{
public:
	ExamineManipulator();
	virtual ~ExamineManipulator();

	//! Sets the diameter of the object that's being examined.
	inline void setObjectDiameter( float diameter )
	{
		_objectDiameter = diameter;
	}

	//! Sets the center of the object that's being examined.
	inline void setObjectCenter( const vec3f& center )
	{
		_objectCenter = center;
	}

	virtual const mat4f& getTransform() const;
	virtual const mat4f& getInverseTransform() const;

	virtual void reset();

	virtual void resizeEvent( int width, int height );
	virtual bool mousePressEvent( QMouseEvent* e );
	virtual bool mouseMoveEvent( QMouseEvent* e );
	virtual bool mouseReleaseEvent( QMouseEvent* e );
	virtual bool wheelEvent( QWheelEvent* e );
	virtual bool keyPressEvent( QKeyEvent* e );

private:
	void updateTransform();

private:
	vec3f _objectCenter;
	float _objectDiameter;

	int _width;		// screen width
	int _height;	// screen height

	mat4f _projection;

	QPoint _lastMousePos;
	QPoint _lastClickPos;

	float _translationScale;

	Arcball _arcball;	// model rotation
	vec3f _translation;	// model translation

	mat4f _lookAt;		// model/view transform
	mat4f _invLookAt;
};

#endif
