#include "ExamineManipulator.h"
#include <QToolTip>
#include <QStatusBar>
#include <QMouseEvent>

#ifdef _WIN32
#include <windows.h>
#endif
#include <GL/gl.h>
#include <GL/glu.h>

ExamineManipulator::ExamineManipulator()
{
	_objectCenter.set( 0, 0, 0 );
	_objectDiameter = 1;
	reset();
}

ExamineManipulator::~ExamineManipulator()
{
	// empty
}

const mat4f & ExamineManipulator::getTransform() const
{
	return _lookAt;
}

const mat4f & ExamineManipulator::getInverseTransform() const
{
	return _invLookAt;
}

void ExamineManipulator::reset()
{
	_arcball.setOrientation( quatf( 0, 0, 0, 1 ) );
	_translation.set( 0, 0, ( _objectDiameter * -0.5 ) / atan( vr::toRadians( 30.0f /* half fovy */ ) ) );
	updateTransform();
}

void ExamineManipulator::resizeEvent( int width, int height )
{
	_width = width;
	_height = height;
	_translationScale = _objectDiameter / vr::max( width, height );
}

inline vec2f normalize( const QPoint& p, int w, int h )
{
	float s = vr::min( w, h );
	return vec2f( ( ( p.x() / s ) - w/s * 0.5 ) * 2.0, ( -( p.y() / s ) + h/s * 0.5 ) * 2.0 );
}

bool ExamineManipulator::mousePressEvent( QMouseEvent* e )
{
	_lastMousePos = e->pos();
	_lastClickPos = e->pos();

	if( e->buttons() & Qt::LeftButton )
		_arcball.beginDrag( normalize( _lastClickPos, _width, _height ) );

	return false;
}

bool ExamineManipulator::mouseMoveEvent( QMouseEvent* e )
{
	if( e->buttons() == Qt::NoButton )
	{
		return false;
	}

	QPoint pos( e->pos() );
	vec3f dMouse( pos.x() - _lastMousePos.x(), -( pos.y() - _lastMousePos.y() ), 0 );

	// rotate
	if( e->buttons() & Qt::LeftButton )
	{
		_arcball.updateDrag( normalize( pos, _width, _height ) );
	}

	// translate on the Z axis.
	if( e->buttons() & Qt::RightButton )
	{
		_translation[2] -= dMouse.y * _translationScale * 10;
	}

	// translate on the XY plane
	if( e->buttons() & Qt::MidButton )
	{
		_translation += dMouse * _translationScale;
	}

	_lastMousePos = pos;

	updateTransform();

	return true;
}

bool ExamineManipulator::mouseReleaseEvent( QMouseEvent* e )
{
	_arcball.endDrag();
	return false;
}

bool ExamineManipulator::wheelEvent( QWheelEvent* e )
{
	return false;
}

bool ExamineManipulator::keyPressEvent( QKeyEvent* e )
{
	return false;
}

void ExamineManipulator::updateTransform()
{
	mat4f translation;
	translation.makeTranslation( -_objectCenter );

	_lookAt.set( _arcball.getOrientation() );
	_lookAt.product( translation, _lookAt );

	translation.makeTranslation( _translation );

	_lookAt.product( _lookAt, translation );

	_invLookAt = _lookAt;
	_invLookAt.invert();
}
