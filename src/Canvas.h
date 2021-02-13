#pragma once
#ifndef _CANVAS_H_
#define _CANVAS_H_

#include <gl/glew.h>
#include <QtOpenGL/QGLWidget>

#include <vr/timer.h>
#include <tecosg/OsgRenderer.h>

#include "ExamineManipulator.h"
#include "ShaderManager.h"

class Canvas : public QGLWidget
{
	Q_OBJECT

public:
	enum RenderMode
	{
		GEOMETRY			= 0x0001,
		BOUNDING_BOX		= 0x0010,
		HEIGHTMAP			= 0x0100,
		POST_SHADING        = 0x1000,
	};

public:
	static void setParent( QWidget* parent );
	static Canvas* instance();

public:
	void addGeometryModel( tecosg::OsgModel* model );
	void setGeometryBoundingBox( tecosg::OsgModel* box );

	void setIdleEnabled( bool enabled );

	ShaderManager& shaderManager();
	void resetShaders();

	ShaderManager& layerShaderManager();
	void resetLayerShaders();

	void setRenderMode( RenderMode mode, bool enabled = true );
	RenderMode renderMode() const;

signals:
	void updateFps( double fps );

protected:
	virtual void initializeGL();
	virtual void resizeGL( int w, int h );
	virtual void paintGL();

protected slots:
	virtual void timerEvent( QTimerEvent* e );
	virtual void keyPressEvent( QKeyEvent* e );
	virtual void mousePressEvent( QMouseEvent* e );
	virtual void mouseMoveEvent( QMouseEvent* e );

private:
	Canvas( QWidget* parent );
	~Canvas();

	void updateCamera();

private:
	int _idleId;
	vr::Timer _timer;
	unsigned int _frameCounter;

	ShaderManager _shaderManager;
	ShaderManager _layerShaderManager;
	tecosg::OsgRenderer _modelRenderer;
	tecosg::OsgRenderer _boxRenderer;
	ExamineManipulator _examManip;

	RenderMode _renderMode;

	unsigned int _fbo;
	unsigned int _renderTex;
	ShaderManager _saveDepthShaderManager;
	ShaderManager _postShadingShaderManager;

};

#endif
