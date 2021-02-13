#pragma once
#ifndef TECOSG_OSGMODEL_H
#define TECOSG_OSGMODEL_H

#include <tecosg/Common.h>

// Forward declaration of OpenSceneGraph dependencies
namespace osg {

	class Node;
	class Switch;
	class PositionAttitudeTransform;
	template<class T> class ref_ptr;
}

namespace tecosg {

/**
 *  Class for handling an OpenSceneGraph geometric model
 */
class OsgModel
{
public:
    OsgModel();
	~OsgModel();

    // Loads specified model file.
	// Creates additional control Nodes above raw loaded data, as follows:
	// Switch -> PositionAttitudeTransform -> RawModelData
    void load( const char* filename );

	// Manually set model
	void set( osg::Node* model );

	// Verifies if model has valid data
	bool valid() const;

	// Access model data with control Nodes.
	// Mainly used for rendering of the model.
	osg::Node* data() const;

	// Access raw model data, without control Nodes
	osg::Node* rawData() const;

	// Get bounding sphere center (x, y, z) and radius
	void getBoundingSphere( float& x, float& y, float& z, float& radius );

	// Switch rendering of the model
	void setVisible( bool visible );
	bool isVisible() const;

	// Model transformations
	void setScale( double x, double y, double z );
	void setTranslation( double x, double y, double z );
	void setRotation( double radians, double x, double y, double z );
	void setRotation( double radians1, double x1, double y1, double z1, 
		              double radians2, double x2, double y2, double z2, 
					  double radians3, double x3, double y3, double z3 );

	void getScale( double& x, double& y, double& z );
	void getTranslation( double& x, double& y, double& z );
	void getRotation( double& radians, double& x, double& y, double& z );

private:
	osg::ref_ptr<osg::Switch>* _root;
	osg::ref_ptr<osg::PositionAttitudeTransform>* _pat;
	osg::ref_ptr<osg::Node>* _model;
};

}

#endif
