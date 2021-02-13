#include "Canvas.h"

#include <QKeyEvent>
#include <QMessageBox>

#include <iostream>
#include <fstream>

#include <QtOpenGL/QGLFramebufferObject>

static QWidget* s_parent = NULL;
static Canvas* s_canvasInstance = NULL;

void Canvas::setParent( QWidget* parent )
{
	s_parent = parent;
}

Canvas* Canvas::instance()
{
	if( s_canvasInstance ==	NULL )
		s_canvasInstance = new Canvas( s_parent );

	return s_canvasInstance;
}

Canvas::Canvas( QWidget* parent )
: QGLWidget( QGLFormat( QGL::StencilBuffer | QGL::AlphaChannel ), parent ), _frameCounter( 0 ), _renderMode( GEOMETRY )
{
	setFocusPolicy( Qt::StrongFocus );
	_fbo = 0;
}

Canvas::~Canvas()
{
}

void Canvas::addGeometryModel( tecosg::OsgModel* model )
{
	_modelRenderer.addModel( model );

	vr::vec3f center;
	float radius;
	_modelRenderer.getBoundingSphere( center.x, center.y, center.z, radius );

	_examManip.setObjectCenter( center );
	_examManip.setObjectDiameter( radius * 2.0f );
	_examManip.reset();
	updateCamera();
	updateGL();
}

void Canvas::setGeometryBoundingBox( tecosg::OsgModel* box )
{
	_boxRenderer.addModel( box );
	updateGL();
}

void Canvas::setIdleEnabled( bool enabled )
{
	if( enabled )
		_idleId = startTimer( 0 );
	else
		killTimer( _idleId );
}

ShaderManager& Canvas::shaderManager()
{
	return _shaderManager;
}

void Canvas::resetShaders()
{
	_shaderManager.reset();
	_shaderManager.setFragmentProgram( "../shaders/test_FS.glsl" );
	_shaderManager.initShaders();
}

ShaderManager& Canvas::layerShaderManager()
{
	return _layerShaderManager;
}

void Canvas::resetLayerShaders()
{
	_layerShaderManager.reset();
	_layerShaderManager.setFragmentProgram( "../shaders/rayCast_FS.glsl" );
	_layerShaderManager.setVertexProgram( "../shaders/rayCast_VS.glsl" );
	_layerShaderManager.initShaders();
}

void Canvas::setRenderMode( Canvas::RenderMode mode, bool enabled )
{
	unsigned int current = static_cast<unsigned int>( _renderMode );

	if( enabled )
		current |= mode;
	else
		current &= ~mode;

	_renderMode = static_cast<Canvas::RenderMode>( current );

	if( !enabled )
		return;

	vr::vec3f center( 0.5f,0.5f,0.5f );
	float radius = 0.5f;

	if( mode == HEIGHTMAP )
	{
		_boxRenderer.getBoundingSphere( center.x, center.y, center.z, radius );
	}
	else if( ( mode == GEOMETRY ) || ( mode == BOUNDING_BOX ) )
	{
		//_modelRenderer.getBoundingSphere( center.x, center.y, center.z, radius );
		;
	}
 	else if( mode == POST_SHADING )
 	{
 		glGenFramebuffersEXT( 1, &_fbo );
 		glBindFramebufferEXT( GL_FRAMEBUFFER_EXT, _fbo );
 
 		// Attach depth buffer
 		unsigned int depthBuffer;
 		glGenRenderbuffersEXT( 1, &depthBuffer );
 		glBindRenderbufferEXT( GL_RENDERBUFFER_EXT, depthBuffer );
 		glRenderbufferStorageEXT( GL_RENDERBUFFER_EXT, GL_DEPTH_COMPONENT, width(), height() );
 		glFramebufferRenderbufferEXT( GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT, GL_RENDERBUFFER_EXT, depthBuffer );
 
		glEnable( GL_TEXTURE_2D );
 		glActiveTexture( GL_TEXTURE7 );
 		glGenTextures( 1, &_renderTex );
 		glBindTexture( GL_TEXTURE_2D, _renderTex );
 		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
 		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
 		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
 		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
		//glTexImage2D( GL_TEXTURE_2D, 0, GL_LUMINANCE32F_ARB, width(), height(), 0, GL_LUMINANCE, GL_FLOAT, NULL );
		glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA32F_ARB, width(), height(), 0, GL_RGBA, GL_FLOAT, NULL );
 
 		glFramebufferTexture2DEXT( GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_2D, _renderTex, 0 );
 
 		GLenum status = glCheckFramebufferStatusEXT( GL_FRAMEBUFFER_EXT );
 		if( status != GL_FRAMEBUFFER_COMPLETE_EXT )
 			printf( "Warning: failed to initialize FBO!\n" );
 
 		glBindFramebufferEXT( GL_FRAMEBUFFER_EXT, 0 );
 
 		_saveDepthShaderManager.reset();
 		_saveDepthShaderManager.setFragmentProgram( "../shaders/saveDepth_FS.glsl" );
 		_saveDepthShaderManager.initShaders();
 
 		_postShadingShaderManager.reset();
 		_postShadingShaderManager.setFragmentProgram( "../shaders/postShading_FS.glsl" );
		_postShadingShaderManager.addUniformf( "u_invTexWidth", 1.0f/(float)width() );
		_postShadingShaderManager.addUniformf( "u_invTexHeight", 1.0f/(float)height() );
		_postShadingShaderManager.addUniformi( "u_texDepth", 7 );
 		_postShadingShaderManager.initShaders();
		updateGL();
		return;
	}

	_examManip.setObjectCenter( center );
	_examManip.setObjectDiameter( 1.0f );
	_examManip.reset();
	updateCamera();
	updateGL();
}

Canvas::RenderMode Canvas::renderMode() const
{
	return _renderMode;
}

/************************************************************************/
/* Protected                                                            */
/************************************************************************/
void Canvas::initializeGL()
{
	glewInit();

	resetShaders();
	resetLayerShaders();

	glEnable( GL_DEPTH_TEST );

	/// Setup light
	glEnable( GL_LIGHTING );
	glEnable( GL_LIGHT0 );

	// Position headlight
	float lpos[4] = {0.0f,0.0f,0.0f,1.0f};
	glMatrixMode( GL_MODELVIEW );
	glPushMatrix();
	glLoadIdentity();
	glLightfv( GL_LIGHT0, GL_POSITION, lpos );
	glPopMatrix();

	//glEnable( GL_COLOR_MATERIAL );
	//glColorMaterial( GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE );

	_timer.restart();
}

void Canvas::resizeGL( int w, int h )
{
	_examManip.resizeEvent( w, h );
	_modelRenderer.setViewport( 0, 0, w, h );
	_boxRenderer.setViewport( 0, 0, w, h );
	glViewport( 0, 0, w, h );
	glMatrixMode( GL_PROJECTION );
	glLoadIdentity();
	gluPerspective( 60.0, (double)w/(double)h, 0.001, 100.0 );
}

void Canvas::paintGL()
{
	++_frameCounter;

	// TODO: changed only to take screenshots!
	//if( _renderMode & GEOMETRY )
	//	glClearColor( 0, 0, 0, 0 ); // need to clear alpha!
	//if( _renderMode & HEIGHTMAP )
		glClearColor( 1, 1, 1, 1 );


	if( _renderMode & POST_SHADING )
	{
		// Render depth to fbo
		//QGLFramebufferObject qfbo( width(), height(), QGLFramebufferObject::Depth );
		//qfbo.bind();

		glBindFramebufferEXT( GL_FRAMEBUFFER_EXT, _fbo );
		glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

		_saveDepthShaderManager.bindProgram();
		_modelRenderer.render();
		_saveDepthShaderManager.unbindProgram();

		glBindFramebufferEXT( GL_FRAMEBUFFER_EXT, 0 );

		//qfbo.release();
		//qfbo.toImage().save( "depth.bmp" );
		//exit( 1 );

		// Render quad and compute shading from depth
		glMatrixMode( GL_PROJECTION );
		glPushMatrix();
		glLoadIdentity();
		glOrtho( 0, 1, 0, 1, -1, 1 );
		glMatrixMode( GL_MODELVIEW );
		glPushMatrix();
		glLoadIdentity();
		glDisable( GL_DEPTH_TEST );
		glDisable( GL_LIGHTING );
		_postShadingShaderManager.bindProgram();

		glColor3f( 0, 0, 1 );
		glBegin( GL_QUADS );
		glVertex2f( 0, 0 );
		glVertex2f( 1, 0 );
		glVertex2f( 1, 1 );
		glVertex2f( 0, 1 );
		glEnd();

		_postShadingShaderManager.unbindProgram();
		glEnable( GL_LIGHTING );
		glEnable( GL_DEPTH_TEST );
		glMatrixMode( GL_MODELVIEW );
		glPopMatrix();
		glMatrixMode( GL_PROJECTION );
		glPopMatrix();
	}
	else
	{
		glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

		if( _renderMode & GEOMETRY )
		{
			_shaderManager.bindProgram();
			_modelRenderer.render();
			_shaderManager.unbindProgram();
		}
		if( _renderMode & BOUNDING_BOX )
		{
			// Set wireframe
			glPolygonMode( GL_FRONT_AND_BACK, GL_LINE );
			_boxRenderer.render();
			glPolygonMode( GL_FRONT_AND_BACK, GL_FILL );
		}
		if( _renderMode & HEIGHTMAP )
		{
			glCullFace( GL_FRONT );
			glEnable( GL_CULL_FACE );
			_layerShaderManager.bindProgram();
			glColor3f( 1, 1, 1 );
			glBegin( GL_QUAD_STRIP );
			glVertex3f( 0, 0, 0 );
			glVertex3f( 1, 0, 0 );
			glVertex3f( 0, 1, 0 );
			glVertex3f( 1, 1, 0 );
			glVertex3f( 0, 1, 1 );
			glVertex3f( 1, 1, 1 );
			glVertex3f( 0, 0, 1 );
			glVertex3f( 1, 0, 1 );
			glEnd();
			glBegin( GL_QUAD_STRIP );
			glVertex3f( 0, 1, 0 );
			glVertex3f( 0, 1, 1 );
			glVertex3f( 0, 0, 0 );
			glVertex3f( 0, 0, 1 );
			glVertex3f( 1, 0, 0 );
			glVertex3f( 1, 0, 1 );
			glVertex3f( 1, 1, 0 );
			glVertex3f( 1, 1, 1 );
			glEnd();
			_layerShaderManager.unbindProgram();
			glDisable( GL_CULL_FACE );
			glCullFace( GL_BACK );
		}
	}

	double elapsed = _timer.elapsed();
	if( elapsed >= 1.0 )
	{
		emit updateFps( _frameCounter / elapsed );
		_frameCounter = 0;
		_timer.restart();
	}
}

/************************************************************************/
/* Slots                                                                */
/************************************************************************/
void Canvas::timerEvent( QTimerEvent* e )
{
	updateGL();
}

void Canvas::keyPressEvent( QKeyEvent* e )
{
	switch( e->key() )
	{
	case Qt::Key_F5:
		_shaderManager.reloadShaders();
		_layerShaderManager.reloadShaders();
		_saveDepthShaderManager.reloadShaders();
		_postShadingShaderManager.reloadShaders();
		break;

	case Qt::Key_Space:
		_examManip.reset();
		updateCamera();
		break;

	default:
		e->ignore();
	    return;
	}

	// Update state
	e->accept();
	updateGL();
}

void Canvas::mousePressEvent( QMouseEvent* e )
{
	if( _examManip.mousePressEvent( e ) )
	{
		updateCamera();
		updateGL();
	}
}

void Canvas::mouseMoveEvent( QMouseEvent* e )
{
	if( _examManip.mouseMoveEvent( e ) )
	{
		updateCamera();
		updateGL();
	}
}

/************************************************************************/
/* Private                                                              */
/************************************************************************/
void Canvas::updateCamera()
{
	glMatrixMode( GL_MODELVIEW );
	glLoadMatrixf( _examManip.getTransform().ptr() );
}
