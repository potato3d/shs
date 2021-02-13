#ifndef _LAYERGENERATOR_H_
#define _LAYERGENERATOR_H_

#include <vector>
#include <vr/vec3.h>
#include "AABB.h"
#include "ShaderManager.h"
#include <tecosg/OsgRenderer.h>

class LayerGenerator
{
public:
	LayerGenerator();

	void setCurrentModel( tecosg::OsgModel* model );
	void computeBoundingBox();
	void generateLayers();

	void deleteAllLayers();

	tecosg::OsgModel* currentModel();
	const AABB& boundingBox();

	// Converts height (red component) to 1 - value
	void saveLayer( const std::string& filename, float* pixels );

	// Load height and normal maps
	void beginLayerLoading();
	void loadLayerToOpenGL( const std::string& filename );
	void endLayerLoading();

	int width() const;
	int height() const;

private:
	void minMaxVertices( vr::vec3d& minVertex, vr::vec3d& maxVertex, const double* vertices, unsigned int size ) const;
	unsigned int computeLayersNeededWithStencil() const;
	unsigned int computeLayersNeededWithQuery();

	void beginLayerGeneration();
	void endLayerGeneration();

private:
	tecosg::OsgModel* _model;
	AABB _bbox;
	int _vp[4];
	int _width;
	int _height;
	unsigned int _fbo;
	unsigned int _depthBuffer;
	unsigned int _refTex;
	unsigned int _renderTex;
};

#endif // _LAYERGENERATOR_H_
