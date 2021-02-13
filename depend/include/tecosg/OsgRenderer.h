#pragma once
#ifndef TECOSG_OSGRENDERER_H
#define TECOSG_OSGRENDERER_H

#include <tecosg/Common.h>
#include <tecosg/OsgModel.h>

// Forward declaration of OpenSceneGraph dependencies
namespace osg {

	class Group;
	template<class T> class ref_ptr;
}
namespace osgUtil {

    class SceneView;
}

namespace tecosg {

/**
 *  Class for rendering geometric models from OpenSceneGraph.
 *  Stores current camera settings and models to be rendered.
 *	Performs frustum culling automatically.
 *  Default behavior:
 *		Restore all previous OpenGL state attributes and matrices after render().
 *		Get current View Matrix from OpenGL for each frame.
 *		Get current Projection Matrix from OpenGL for each frame.
 */
class OsgRenderer
{
public:
    OsgRenderer();
	~OsgRenderer();

    // Add specified model to be rendered.
	// Model must have been previously loaded with valid geometric data.
    void addModel( OsgModel* model );

	// If parameter is true, after calling the render() method all previous OpenGL state attributes and matrices
	// will be restored (default).
	// If parameter is false, after calling the render() method all OpenGL state attributes and matrices 
	// will become undefined.
	void setPreserveOpenGLState( bool preserve );

	// If parameter is true, for each frame the Projection Matrix will be updated automatically from OpenGL (default).
	// If parameter is false, the client is required to supply the Projection Matrix whenever it has changed.
	void setAutoUpdateProjectionMatrix( bool useAuto );

	// If AutoUpdateProjectionMatrix is false (default is true), this method must be called each time the Projection Matrix changes.
	void setProjectionMatrix( float* projectionMatrix );

	// If parameter is true, for each frame the View Matrix will be updated automatically from OpenGL (default).
	// If parameter is false, the client is required to supply the View Matrix whenever it has changed.
	void setAutoUpdateViewMatrix( bool useAuto );

	// If AutoUpdateViewMatrix is false (default is true), this method must be called each time the viewer changes.
	void setViewMatrix( float* viewMatrix );

	// Must be called whenever the canvas is resized (i.e. reshape callback).
	void setViewport( int x, int y, int width, int height );

    // Render stored OpenSceneGraph models
    void render();

	// Get scene bounding sphere center (x, y, z) and radius
	void getBoundingSphere( float& x, float& y, float& z, float& radius );

private:
	bool _preserveOpenGLState;
	bool _autoUpdateViewMatrix;
	bool _autoUpdateProjectionMatrix;
	unsigned int _frameNumber;
	osg::ref_ptr<osg::Group>* _sceneData;
	osg::ref_ptr<osgUtil::SceneView>* _sceneView;
};

}

#endif
