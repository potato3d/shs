#include "LayerGenerator.h"
#include "Canvas.h"
#include <string>
#include <fstream>
#include <cassert>

#include <osg/NodeVisitor>
#include <osg/Geometry>
#include <osg/Transform>
#include <osg/Geode>

#include <QGLFramebufferObject>
#include <QDir>
#include <QFile>

class ComputeBoundingBoxVisitor : public osg::NodeVisitor
{
public:
	ComputeBoundingBoxVisitor()
		: osg::NodeVisitor( osg::NodeVisitor::TRAVERSE_ALL_CHILDREN )
	{
	}

	virtual void apply( osg::Transform& trans );

	virtual void apply( osg::Geode& geode );

	const osg::BoundingBox& getBoundingBox()
	{ 
		return _boundingBox;
	}

private:
	osg::BoundingBox _boundingBox;
	std::vector<osg::Matrix> _matrixStack;
};

void ComputeBoundingBoxVisitor::apply( osg::Transform& trans )
{
	osg::Matrix matrix;

	if( !_matrixStack.empty() )
		matrix = _matrixStack.back();

	trans.computeLocalToWorldMatrix( matrix, NULL );
	_matrixStack.push_back( matrix );

	for( int i = 0; i < (int)trans.getNumChildren(); i++ )
		trans.getChild( i )->accept( *this );

	_matrixStack.pop_back();
}

void ComputeBoundingBoxVisitor::apply( osg::Geode& geode )
{
	for( int i = 0; i < (int)geode.getNumDrawables(); i++ )
	{
		for( int i = 0; i < (int)geode.getNumDrawables(); i++ )
		{
			if( _matrixStack.empty() )
			{
				_boundingBox.expandBy( geode.getDrawable( i )->getBound() );
			}
			else
			{
				osg::BoundingBox boundingBox = geode.getDrawable( i )->getBound();
				for( int j = 0; j < 8; j++ )
					_boundingBox.expandBy( boundingBox.corner( j ) * _matrixStack.back() );
			}
		}
	}
}

LayerGenerator::LayerGenerator()
: _model( NULL )
{
	 _fbo = 0;
	 _depthBuffer = 0;
	 _refTex = 0;
	 _renderTex = 0;
}

void LayerGenerator::setCurrentModel( tecosg::OsgModel* model )
{
	_model = model;
}

void LayerGenerator::computeBoundingBox()
{
	if( _model == NULL )
		return;

	// Compute AABB
	ComputeBoundingBoxVisitor cbbv;
	_model->rawData()->accept( cbbv );
	const osg::Vec3& vmin = cbbv.getBoundingBox()._min;
	_bbox.minV.set( vmin.x(), vmin.y(), vmin.z() );
	const osg::Vec3& vmax = cbbv.getBoundingBox()._max;
	_bbox.maxV.set( vmax.x(), vmax.y(), vmax.z() );
}

void LayerGenerator::generateLayers()
{
	// Get OpenGL state attributes
	glGetIntegerv( GL_VIEWPORT, _vp );
	_width = _vp[2];
	_height = _vp[3];

	// Multi-use pixel container, to send values to textures and to read back from gl buffers
	std::vector<float> pixels( _width*_height*4 );

	// Disable face culling, to guarantee we only discard fragments through z-test and our frag. shader
	osg::StateSet* ss = _model->rawData()->getOrCreateStateSet();
	ss->setMode( GL_CULL_FACE, osg::StateAttribute::OFF );
	glDisable( GL_CULL_FACE );

	// FBO for testing
	QGLFramebufferObject qfbo( _width, _height, QGLFramebufferObject::Depth );

	// Store best matrices for generating layers later
	float bestModelView[16];
	float bestProjection[16];

	// Bounding box data
	const vr::vec3d& center = _bbox.center();
	vr::vec3d eye;

	// Setup fragment shader for depth peeling.
	// Send correct shader parameters.
	ShaderManager& shaders = Canvas::instance()->shaderManager();
	shaders.reset();
	shaders.setVertexProgram( "../shaders/createLayers_VS.glsl" );
	shaders.setFragmentProgram( "../shaders/createLayers_FS.glsl" );
	shaders.addUniformi( "depthTexSampler", 0 ); // shader always reads from unit 0
	shaders.addUniformf( "invDepthTexWidth", 1.0f / (float)_width );
	shaders.addUniformf( "invDepthTexHeight", 1.0f / (float)_height );
	shaders.initShaders();

	// Save previous OpenGL matrices
	glMatrixMode( GL_MODELVIEW );
	glPushMatrix();
	glMatrixMode( GL_PROJECTION );
	glPushMatrix();
	
	//////////////////////////////////////////////////////////////////////////
	// Minimize number of layers needed from 3 box faces: towards -X, -Y and -Z
	
	//////////////////////////////////////////////////////////////////////////
	// Toward -Z
	//////////////////////////////////////////////////////////////////////////
	eye = center;
	eye.z += _bbox.extent(2)*0.5;
	
	glMatrixMode( GL_PROJECTION );
	glLoadIdentity();
	glOrtho( -_bbox.extent(0)*0.51, _bbox.extent(0)*0.51, 
		     -_bbox.extent(1)*0.51, _bbox.extent(1)*0.51, 
		                     -0.01, _bbox.extent(2)*1.01 );

	glMatrixMode( GL_MODELVIEW );
	glLoadIdentity();
	gluLookAt( eye.x, eye.y, eye.z, center.x, center.y, center.z, 0, 1, 0 );

	// Test camera view
	shaders.setEnabled( false );
	qfbo.bind();
	Canvas::instance()->updateGL();
	qfbo.release();
	qfbo.toImage().save( "cameraTestZ.bmp" );
	shaders.setEnabled( true );
	
	// Compute number of layers needed for current viewpoint
	unsigned int layerCountZ = computeLayersNeededWithQuery();
	printf( "Layers needed (-z): %d\n\n", layerCountZ );

	// Save as best position
	unsigned int minLayerCount = layerCountZ;
	// TODO: 
	if( 0 )
	{
		glGetFloatv( GL_MODELVIEW_MATRIX, bestModelView );
		glGetFloatv( GL_PROJECTION_MATRIX, bestProjection );
	}

	//////////////////////////////////////////////////////////////////////////
	// Toward -X
	//////////////////////////////////////////////////////////////////////////
	eye = center;
	eye.x += _bbox.extent(0)*0.5;

	glMatrixMode( GL_PROJECTION );
	glLoadIdentity();
	glOrtho( -_bbox.extent(2)*0.51, _bbox.extent(2)*0.51, 
		     -_bbox.extent(1)*0.51, _bbox.extent(1)*0.51, 
		                     -0.01, _bbox.extent(0)*1.01 );

	glMatrixMode( GL_MODELVIEW );
	glLoadIdentity();
	gluLookAt( eye.x, eye.y, eye.z, center.x, center.y, center.z, 0, 1, 0 );

	// Test camera view
	shaders.setEnabled( false );
	qfbo.bind();
	Canvas::instance()->updateGL();
	qfbo.release();
	qfbo.toImage().save( "cameraTestX.bmp" );
	shaders.setEnabled( true );

	// Compute number of layers needed for current viewpoint
	unsigned int layerCountX = computeLayersNeededWithQuery();
	printf( "Layers needed (-x): %d\n\n", layerCountX );

	// TODO: 
	if( 0 )//layerCountX < minLayerCount )
	{
		// Save as best camera position
		minLayerCount = layerCountX;
		glGetFloatv( GL_MODELVIEW_MATRIX, bestModelView );
		glGetFloatv( GL_PROJECTION_MATRIX, bestProjection );
	}

	//////////////////////////////////////////////////////////////////////////
	// Toward -Y
	//////////////////////////////////////////////////////////////////////////
	eye = center;
	eye.y += _bbox.extent(1)*0.5;

	// Set projection view based on bounding box to generate correct layer images
	glMatrixMode( GL_PROJECTION );
	glLoadIdentity();
	glOrtho( -_bbox.extent(0)*0.51, _bbox.extent(0)*0.51, 
		     -_bbox.extent(2)*0.51, _bbox.extent(2)*0.51, 
		                     -0.01, _bbox.extent(1)*1.01 );

	// Set camera view based on bounding box to generate correct layer images
	glMatrixMode( GL_MODELVIEW );
	glLoadIdentity();
	gluLookAt( eye.x, eye.y, eye.z, center.x, center.y, center.z, 0, 0, 1 );

	// Test camera view
	shaders.setEnabled( false );
	qfbo.bind();
	Canvas::instance()->updateGL();
	qfbo.release();
	qfbo.toImage().save( "cameraTestY.bmp" );
	shaders.setEnabled( true );

	// Compute number of layers needed for current viewpoint
	unsigned int layerCountY = computeLayersNeededWithQuery();
	printf( "Layers needed (-y): %d\n\n", layerCountY );

	// TODO: 
	if( 1 )//layerCountY < minLayerCount )
	{
		// Save as best camera position
		minLayerCount = layerCountY;
		glGetFloatv( GL_MODELVIEW_MATRIX, bestModelView );
		glGetFloatv( GL_PROJECTION_MATRIX, bestProjection );
	}

	// Set camera to generate the minimum number of layers
	glMatrixMode( GL_PROJECTION );
	glLoadMatrixf( bestProjection );
	glMatrixMode( GL_MODELVIEW );
	glLoadMatrixf( bestModelView );

	// Sanity check with occlusion queries
	unsigned int queryId;
	unsigned int queryResult;
	glGenQueries( 1, &queryId );

	// Setup layer generation
	beginLayerGeneration();

	printf( "*** Generating Layers ***\n" );

	// Loop through all layers
	char layerName[64];
	int layerId = 1;
	for( int i = 0; i < minLayerCount; ++i, ++layerId )
	{
		glBeginQuery( GL_SAMPLES_PASSED, queryId );

		Canvas::instance()->updateGL();

		glEndQuery( GL_SAMPLES_PASSED );

		glGetQueryObjectuiv( queryId, GL_QUERY_RESULT, &queryResult );
		printf( "LAYER ID: %d     SAMPLES PASSED: %d\n", layerId, queryResult );

		if( queryResult == 0 )
		{
			printf( "No samples passed, terminating...\n" );
			break;
		}

		// Read pixels from fbo
		glReadPixels( _vp[0], _vp[1], _width, _height, GL_RGBA, GL_FLOAT, &pixels[0] );

		// Save render texture to file
		sprintf_s( layerName, "../data/out/layer%d", layerId );
		saveLayer( layerName, &pixels[0] );

		// Swap reference texture <-> render texture.
		// The fragment shader always reads from texture unit 0 (zero).
		if( i % 2 )
		{
			// Shader texture
			glActiveTexture( GL_TEXTURE0 );
			glBindTexture( GL_TEXTURE_2D, _refTex );

			// Render texture
			glActiveTexture( GL_TEXTURE1 );
			glBindTexture( GL_TEXTURE_2D, _renderTex );
			glFramebufferTexture2DEXT( GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_2D, _renderTex, 0 );
		}
		else
		{
			// Shader texture
			glActiveTexture( GL_TEXTURE0 );
			glBindTexture( GL_TEXTURE_2D, _renderTex );

			// Render texture
			glActiveTexture( GL_TEXTURE1 );
			glBindTexture( GL_TEXTURE_2D, _refTex );
			glFramebufferTexture2DEXT( GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_2D, _refTex, 0 );
		}
	}

	// Cleanup
	glDeleteQueries( 1, &queryId );

	endLayerGeneration();

	glMatrixMode( GL_MODELVIEW );
	glPopMatrix();
	glMatrixMode( GL_PROJECTION );
	glPopMatrix();

	Canvas::instance()->resetShaders();
}

void LayerGenerator::deleteAllLayers()
{
	// Remove previously generated files
	QDir outDir( "../data/out" ) ;
	QStringList exts;
	exts << "*.height" << "*.normal" << "*.bmp";
	QStringList files = outDir.entryList( exts );
	for( unsigned int i = 0; i < files.size(); ++i )
	{
		outDir.remove( files[i] );
	}
}

tecosg::OsgModel* LayerGenerator::currentModel()
{
	return _model;
}

const AABB& LayerGenerator::boundingBox()
{
	return _bbox;
}

void LayerGenerator::saveLayer( const std::string& filename, float* pixels )
{
	unsigned int count = _width * _height;

	//////////////////////////////////////////////////////////////////////////
	// Height Map
	//////////////////////////////////////////////////////////////////////////
	std::ofstream heightOut( (filename + ".height").c_str(), std::ios_base::binary );
	if( !heightOut )
		return;

	// Save height map as unsigned char image for debugging
	// Note: will be inverted since qt uses different image origin
	QImage testImgHeight( _width, _height, QImage::Format_RGB32 );
	int i = 0;
	for( int y = 0; y < _height; ++y )
	{
		for( int x = 0; x < _width; ++x )
		{
			// Convert R value to "height"
			pixels[i] = ( pixels[i] == 0.0f ) ? 0.0f : 1.0f - pixels[i];
			testImgHeight.setPixel( x, y, qRgb( (int)(pixels[i]*255.0f), 0, 0 ) );
			i+=4;
		}
	}
	testImgHeight.save( (filename + ".height.bmp").c_str() );

	// Save image description in binary file
	heightOut.write( (const char*)&_width, sizeof(int) );
	heightOut.write( (const char*)&_height, sizeof(int) );

	// Save R component as height map
	for( int i = 0, k = 0; i < count; ++i, k+=4 )
	{
		//pixels[k] = ( pixels[k] == 0.0f ) ? 0.0f : 1.0f - pixels[k];
		heightOut.write( (const char*)(pixels + k), sizeof(float) );
	}

	heightOut.close();

	//////////////////////////////////////////////////////////////////////////
	// Normal Map
	//////////////////////////////////////////////////////////////////////////
	std::ofstream normalOut( (filename + ".normal").c_str(), std::ios_base::binary );
	if( !normalOut )
		return;

	// Save normal map as unsigned char image for debugging
	// Note: will be inverted since qt uses different image origin
	QImage testImgNormal( _width, _height, QImage::Format_RGB32 );
	i = 0;
	for( int y = 0; y < _height; ++y )
	{
		for( int x = 0; x < _width; ++x )
		{
			testImgNormal.setPixel( x, y, qRgb( (int)( vr::abs(pixels[i+1]*255.0f) ), 
				                                (int)( vr::abs(pixels[i+2]*255.0f) ), 
												(int)( vr::abs(pixels[i+3]*255.0f) ) ) );
			i+=4;
		}
	}
	testImgNormal.save( (filename + ".normal.bmp").c_str() );

	// Save image description in binary file
	normalOut.write( (const char*)&_width, sizeof(int) );
	normalOut.write( (const char*)&_height, sizeof(int) );

	// Save GBA components as normal map
	for( int i = 0, k = 0; i < count; ++i, k+=4 )
	{
		normalOut.write( (const char*)(pixels + k + 1), sizeof(float)*3 );
	}

	normalOut.close();
}

void LayerGenerator::beginLayerLoading()
{
	// Get shader manager
	ShaderManager& layerShaderManager = Canvas::instance()->layerShaderManager();
	layerShaderManager.reset();
	layerShaderManager.setFragmentProgram( "../shaders/rayCast_FS.glsl" );
	layerShaderManager.setVertexProgram( "../shaders/rayCast_VS.glsl" );
}

void LayerGenerator::loadLayerToOpenGL( const std::string& filename )
{
	// Get slash and dot position
	std::string::size_type slash( filename.find_last_of( '/' ) );
	if( slash == std::string::npos )
		slash = filename.find_last_of( '\\' );
	
	std::string::size_type dot( filename.find_last_of( '.' ) );

	// Extract base file name
	std::string filePath = filename.substr( 0, slash + 1 );
	std::string baseName = filename.substr( slash + 1, dot - slash - 1 );

	// Get number position
	std::string::size_type nb( baseName.find_first_of( "0123456789" ) );
	if( nb == std::string::npos )
	{
		printf( "Invalid layer file\n" );
		return;
	}

	// Extract layer id string
	std::string layerIdStr = baseName.substr( nb, baseName.length() - nb );
	// Convert to number
	unsigned int layerId = QString( layerIdStr.c_str() ).toInt();

	// Base height uniform name
	std::string baseHeightUniformName( "u_hm" );
	std::string baseNormalUniformName( "u_normal" );

	std::vector<float> pixels;
	GLuint texId;

	// Load heightmap
	std::ifstream heightIn( (filePath + baseName + ".height").c_str(), std::ios_base::binary );
	if( !heightIn )
		return;

	heightIn.read( (char*)&_width, sizeof(int) );
	heightIn.read( (char*)&_height, sizeof(int) );

	unsigned int count = _width*_height;

	pixels.reserve( count*3 );
	pixels.resize( count*3, -1.0f );

	heightIn.read( (char*)( &pixels[0] ), sizeof(float)*count );
	heightIn.close();

	// Setup texture
	glActiveTexture( GL_TEXTURE0 + layerId ); // TODO: is this guaranteed to work?
	glGenTextures( 1, &texId );
	glBindTexture( GL_TEXTURE_2D, texId );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
	glTexImage2D( GL_TEXTURE_2D, 0, GL_LUMINANCE32F_ARB, _width, _height, 0, GL_LUMINANCE, GL_FLOAT, &pixels[0] );

	Canvas::instance()->layerShaderManager().addUniformi( ( baseHeightUniformName + layerIdStr ).c_str(), layerId );

	// TODO: loading hard-coded texture unit 4 after height textures
	// TODO: in other words, we assume we have only 4 heightmaps, if this changes we need to change here as well!
	// Load normal map
	std::ifstream normalIn( (filePath + baseName + ".normal").c_str(), std::ios_base::binary );
	if( !normalIn )
		return;

	std::fill( pixels.begin(), pixels.end(), -1.0f );

	normalIn.read( (char*)&_width, sizeof(int) );
	normalIn.read( (char*)&_height, sizeof(int) );

	normalIn.read( (char*)( &pixels[0] ), sizeof(float)*count*3 );
	normalIn.close();

	// Setup texture
	// TODO: should be GL_LINEAR for improved image quality
	// TODO: doesn't work in Quadro 3400
	glActiveTexture( GL_TEXTURE0 + layerId + 6 );
	glGenTextures( 1, &texId );
	glBindTexture( GL_TEXTURE_2D, texId );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, /*GL_LINEAR*/ GL_NEAREST );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, /*GL_LINEAR*/ GL_NEAREST );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
	glTexImage2D( GL_TEXTURE_2D, 0, GL_RGB32F_ARB, _width, _height, 0, GL_RGB, GL_FLOAT, &pixels[0] );

	Canvas::instance()->layerShaderManager().addUniformi( ( baseNormalUniformName + layerIdStr ).c_str(), layerId + 6 );
}

void LayerGenerator::endLayerLoading()
{
	Canvas::instance()->layerShaderManager().initShaders();
}

int LayerGenerator::width() const
{
	return _width;
}

int LayerGenerator::height() const
{
	return _height;
}

/************************************************************************/
/* Private                                                              */
/************************************************************************/

void LayerGenerator::minMaxVertices( vr::vec3d& minVertex, vr::vec3d& maxVertex, const double* vertices, unsigned int size ) const
{
	assert( vertices != NULL );
	assert( size >= 3 );
	assert( ( size % 3 ) == 0 );

	// Get initial values
	minVertex.set( &vertices[0] );
	maxVertex.set( &vertices[0] );

	// For each vertex beyond first
	for( unsigned int i = 3; i < size; i+=3 )
	{
		if( vertices[i] < minVertex[0] )
			minVertex[0] = vertices[i];
		else if( vertices[i] > maxVertex[0] )
			maxVertex[0] = vertices[i];

		if( vertices[i+1] < minVertex[1] )
			minVertex[1] = vertices[i+1];
		else if( vertices[i+1] > maxVertex[1] )
			maxVertex[1] = vertices[i+1];

		if( vertices[i+2] < minVertex[2] )
			minVertex[2] = vertices[i+2];
		else if( vertices[i+2] > maxVertex[2] )
			maxVertex[2] = vertices[i+2];
	}
}

unsigned int LayerGenerator::computeLayersNeededWithStencil() const
{
	// Setup stencil test and op to count overdraw for each pixel
	glClearStencil( 0 );
	glClear( GL_STENCIL_BUFFER_BIT );
	glEnable( GL_STENCIL_TEST );
	glStencilFunc( GL_ALWAYS, 0, 0xFFFFFFFF ); // For every pixel we draw...
	glStencilOp( GL_KEEP, GL_INCR, GL_INCR );  // ...increment the stencil buffer (specially if it fails z-test!).

	// Render frame and read back stencil buffer
	int size = _width*_height;
	unsigned int* stencilValues = new unsigned int[size];

	Canvas::instance()->updateGL();

	glReadPixels( _vp[0], _vp[1], _width, _height, GL_STENCIL_INDEX, GL_UNSIGNED_INT, stencilValues );

	// Disable stencil test and op
	glDisable( GL_STENCIL_TEST );

	// Run through stencil and get maximum value = number of layers needed
	unsigned int layerCount = 0;
	for( int i = 0; i < size; ++i )
	{
		layerCount = vr::max( layerCount, stencilValues[i] );
	}

	// Cleanup
	delete [] stencilValues;
	return layerCount;
}

unsigned int LayerGenerator::computeLayersNeededWithQuery()
{
	unsigned int queryId;
	unsigned int queryResult;
	unsigned int layerCount;
	glGenQueries( 1, &queryId );

	beginLayerGeneration();

	// While we generate fragments
	for( layerCount = 0; layerCount < 100; ++layerCount )
	{
		glBeginQuery( GL_SAMPLES_PASSED, queryId );

		Canvas::instance()->updateGL();

		glEndQuery( GL_SAMPLES_PASSED );

		glGetQueryObjectuiv( queryId, GL_QUERY_RESULT, &queryResult );
		printf( "LAYER ID: %d     SAMPLES PASSED: %d\n", layerCount, queryResult );

		if( queryResult == 0 )
		{
			printf( "No samples passed, terminating...\n" );
			break;
		}

		// Swap reference texture <-> render texture.
		// The fragment shader always reads from texture unit 0 (zero).
		if( layerCount % 2 )
		{
			// Shader texture
			glActiveTexture( GL_TEXTURE0 );
			glBindTexture( GL_TEXTURE_2D, _refTex );

			// Render texture
			glActiveTexture( GL_TEXTURE1 );
			glBindTexture( GL_TEXTURE_2D, _renderTex );
			glFramebufferTexture2DEXT( GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_2D, _renderTex, 0 );
		}
		else
		{
			// Shader texture
			glActiveTexture( GL_TEXTURE0 );
			glBindTexture( GL_TEXTURE_2D, _renderTex );

			// Render texture
			glActiveTexture( GL_TEXTURE1 );
			glBindTexture( GL_TEXTURE_2D, _refTex );
			glFramebufferTexture2DEXT( GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_2D, _refTex, 0 );
		}
	}

	glDeleteQueries( 1, &queryId );
	endLayerGeneration();

	return layerCount;
}

void LayerGenerator::beginLayerGeneration()
{
	// Create FBO to store frame buffer image in floating point precision
	glGenFramebuffersEXT( 1, &_fbo );
	glBindFramebufferEXT( GL_FRAMEBUFFER_EXT, _fbo );

	// Attach depth buffer
	glGenRenderbuffersEXT( 1, &_depthBuffer );
	glBindRenderbufferEXT( GL_RENDERBUFFER_EXT, _depthBuffer );
	glRenderbufferStorageEXT( GL_RENDERBUFFER_EXT, GL_DEPTH_COMPONENT, _width, _height );
	glFramebufferRenderbufferEXT( GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT, GL_RENDERBUFFER_EXT, _depthBuffer );

	// Setup reference texture
	glActiveTexture( GL_TEXTURE0 );
	glGenTextures( 1, &_refTex );
	glBindTexture( GL_TEXTURE_2D, _refTex );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );

	// Initial values should be small enough so first test always pass
	std::vector<float> pixels( _width*_height*4 );
	std::fill( pixels.begin(), pixels.end(), -1000.0f );
	glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA32F_ARB, _width, _height, 0, GL_RGBA, GL_FLOAT, &pixels[0] );

	// Setup render texture
	glActiveTexture( GL_TEXTURE1 );
	glGenTextures( 1, &_renderTex );
	glBindTexture( GL_TEXTURE_2D, _renderTex );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
	glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA32F_ARB, _width, _height, 0, GL_RGBA, GL_FLOAT, NULL );

	glFramebufferTexture2DEXT( GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_2D, _renderTex, 0 );

	// Check FBO
	GLenum status = glCheckFramebufferStatusEXT( GL_FRAMEBUFFER_EXT );
	if( status != GL_FRAMEBUFFER_COMPLETE_EXT )
		printf( "Warning: failed to initialize FBO in layer generation!\n" );
}

void LayerGenerator::endLayerGeneration()
{
	glDeleteRenderbuffersEXT( 1, &_depthBuffer );
	glDeleteFramebuffersEXT( 1, &_fbo );
	glBindFramebufferEXT( GL_FRAMEBUFFER_EXT, 0 );

	glDeleteTextures( 1, &_refTex );
	glDeleteTextures( 1, &_renderTex );

	glActiveTexture( GL_TEXTURE1 );
	glBindTexture( GL_TEXTURE_2D, 0 );

	glActiveTexture( GL_TEXTURE0 );
	glBindTexture( GL_TEXTURE_2D, 0 );
}
