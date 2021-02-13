#include "gpurt.h"
#include <tecosg/OsgRenderer.h>
#include <QFileDialog>

#include "AABB.h"
#include <osg/Shape>
#include <osg/ShapeDrawable>
#include <osg/Geode>
#include <osg/Group>
#include <osg/PolygonMode>
#include <osg/Material>
#include <osg/Notify>

#include "ImageDilation.h"
#include <fstream>

gpurt::gpurt(QWidget *parent, Qt::WFlags flags)
    : QMainWindow(parent, flags), _resizeDialog( this )
{
	Canvas::setParent( this );
	ui.setupUi( this );

	_resizeDialog.setWindow( this );

	_fpsLabel.setFixedWidth( 80 );
	_fpsLabel.setAlignment( Qt::AlignRight );

	ui.mainToolBar->hide();
	ui.statusBar->addPermanentWidget( &_fpsLabel );

	setCentralWidget( Canvas::instance() );
	connect( Canvas::instance(), SIGNAL( updateFps( double ) ), this, SLOT( updateFps( double ) ) );

	// Disable actions until model is loaded
	ui.actionComputeBoundingBox->setEnabled( false );
	ui.actionGenerateLayers->setEnabled( false );
	ui.actionGeometry->setEnabled( false );
	ui.actionBoundingBox->setEnabled( false );
	ui.actionHeightmap->setEnabled( false );

	// Hide stupid context menu to show/hide main toolbar.
	setContextMenuPolicy( Qt::NoContextMenu );

	// Disable osg warnings
	osg::setNotifyLevel( osg::ALWAYS );
}

gpurt::~gpurt()
{
}

void gpurt::loadModel( const QString& filename )
{
	tecosg::OsgModel* m = new tecosg::OsgModel;
	m->load( filename.toStdString().c_str() );

	// Set current model to be rendered
	Canvas::instance()->addGeometryModel( m );
	_layerGen.setCurrentModel( m );

	// Update actions
	ui.actionLoad->setEnabled( false );
	ui.actionLoadSphereScene->setEnabled( false );
	ui.actionComputeBoundingBox->setEnabled( true );
	ui.actionGeometry->setEnabled( true );
	ui.actionGeometry->setChecked( true );
}

/************************************************************************/
/* Protected SLOTS                                                      */
/************************************************************************/
void gpurt::keyPressEvent( QKeyEvent* e )
{
	//if( e->key() == Qt::Key_Escape )
	//	close();
	//else
	//	e->ignore();
}

void gpurt::updateFps( double fps )
{
	_fpsLabel.setText( QString::number( fps, 'g', 4 ) + " fps" );
}

void gpurt::on_actionResize_triggered()
{
	_resizeDialog.show();
}

void gpurt::on_actionIdle_toggled( bool enabled )
{
	Canvas::instance()->setIdleEnabled( enabled );
}

void gpurt::on_actionGeometry_toggled( bool enabled )
{
	Canvas::instance()->setRenderMode( Canvas::GEOMETRY, enabled );
}

void gpurt::on_actionBoundingBox_toggled( bool enabled )
{
	Canvas::instance()->setRenderMode( Canvas::BOUNDING_BOX, enabled );
}

void gpurt::on_actionHeightmap_toggled( bool enabled )
{
	Canvas::instance()->setRenderMode( Canvas::HEIGHTMAP, enabled );
}

void gpurt::on_actionPostShading_toggled( bool enabled )
{
	Canvas::instance()->setRenderMode( Canvas::POST_SHADING, enabled );
}

void gpurt::on_actionLoad_triggered()
{
	QString file = QFileDialog::getOpenFileName(
		this, tr("Choose a model file"),
		"c:/models",
		tr("Model files (*.obj; *.3dsx; *.tdgn; *.ive; *.osg; *.tvox; *.cfd; *.def);;All files (*.*)"));

	// TODO: testing 2 spheres
	//if( file.isEmpty() )
	//	return;

	loadModel( file );
}

void gpurt::on_actionLoadLayers_triggered()
{
	QStringList files = QFileDialog::getOpenFileNames(
		this, tr("Choose one or more heightmap files"),
		"../data",
		tr("Heightmap files (*.height);;All files (*.*)"));

	if( files.empty() )
		return;

	_layerGen.beginLayerLoading();

	for( int i = 0; i < files.size(); ++i )
	{
		_layerGen.loadLayerToOpenGL( files[i].toStdString() );
	}

	_layerGen.endLayerLoading();

	resize( _layerGen.width(), _layerGen.height() + 40 );

	ui.actionHeightmap->setEnabled( true );
}

void gpurt::on_actionLoadSphereScene_triggered()
{
	// Create sphere shapes
	osg::Sphere* s1 = new osg::Sphere( osg::Vec3( 0,0,0 ), 0.2 );
	osg::Sphere* s2 = new osg::Sphere( osg::Vec3( 0.3,0.3,0 ), 0.2 );

	// Add shape to shape drawable
	osg::ShapeDrawable* sd1 = new osg::ShapeDrawable( s1 );
	osg::ShapeDrawable* sd2 = new osg::ShapeDrawable( s2 );

	// Add shape drawable to geode
	osg::Geode* g = new osg::Geode;
	g->addDrawable( sd1 );
	g->addDrawable( sd2 );

	// Add box to rendering
	tecosg::OsgModel* spheresModel = new tecosg::OsgModel;
	spheresModel->set( g );
	Canvas::instance()->addGeometryModel( spheresModel );
	_layerGen.setCurrentModel( spheresModel );

	// Update actions
	ui.actionLoad->setEnabled( false );
	ui.actionLoadSphereScene->setEnabled( false );
	ui.actionComputeBoundingBox->setEnabled( true );
	ui.actionGeometry->setEnabled( true );
	ui.actionGeometry->setChecked( true );
}

void gpurt::on_actionDeleteLayers_triggered()
{
	_layerGen.deleteAllLayers();
}

void gpurt::on_actionDilateNormals_triggered()
{
	// Get normal filename
	QStringList files = QFileDialog::getOpenFileNames(
		this, tr("Choose one or more normal files"),
		"../data",
		tr("Normal files (*.normal);;All files (*.*)"));

	// Use current layer size
	int w = _layerGen.width();
	int h = _layerGen.height();
	int count = w*h*3;

	for( int i = 0; i < files.size(); ++i )
	{
		QString file = files[i];

		// Open normal file
		std::ifstream normalIn( file.toStdString().c_str(), std::ios_base::binary );
		if( !normalIn )
			return;

		// Read pixels
		std::vector<float> pixels( count );
		normalIn.read( (char*)( &pixels[0] ), count*sizeof(float) );
		normalIn.close();

		OGFImage* srcImg = new OGFImage( w, h );

		// Convert to OGFImage
		for( int src = 0, dest = 0; src < count; src+=3, dest+=4 )
		{
			srcImg->base_mem()[dest] = pixels[src];
			srcImg->base_mem()[dest+1] = pixels[src+1];
			srcImg->base_mem()[dest+2] = pixels[src+2];
			if( (pixels[src] != 0.0f) || (pixels[src+1] != 0.0f) || (pixels[src+2] != 0.0f) )
				srcImg->base_mem()[dest+3] = 1.0f;
			else
				srcImg->base_mem()[dest+3] = 0.0f;
		}

		// Dilate
		image_dilation( srcImg, 2 );

		std::fill( pixels.begin(), pixels.end(), 1.0f );

		// Convert back from OGFImage
		for( int src = 0, dest = 0; dest < count; src+=4, dest+=3 )
		{
			pixels[dest] = srcImg->base_mem()[src];
			pixels[dest+1] = srcImg->base_mem()[src+1];
			pixels[dest+2] = srcImg->base_mem()[src+2];
		}

		// Print new image to check dilation
		QImage testImgNormal( w, h, QImage::Format_RGB32 );
		int i = 0;
		for( int y = 0; y < h; ++y )
		{
			for( int x = 0; x < w; ++x )
			{
				testImgNormal.setPixel( x, y, qRgb( (int)( vr::abs(pixels[i]*255.0f) ), 
					(int)( vr::abs(pixels[i+1]*255.0f) ), 
					(int)( vr::abs(pixels[i+2]*255.0f) ) ) );
				i+=3;
			}
		}
		testImgNormal.save( (file.toStdString() + ".dilated.bmp").c_str() );

		// TODO: 
		std::ofstream normalOut( ( file.toStdString() + ".dilated" ).c_str(), std::ios_base::binary );
		normalOut.write( (const char*)&pixels[0], count*sizeof(float) );
		normalOut.close();
	}
}

void gpurt::on_actionComputeBoundingBox_triggered()
{
	// Compute bounding box from current model
	_layerGen.computeBoundingBox();
	const AABB& box = _layerGen.boundingBox();

	// Create bounding box shape
	const vr::vec3d bc = box.center();
	osg::Vec3 center( bc.x, bc.y, bc.z );
	osg::Box* b = new osg::Box( center, box.extent( 0 ), box.extent( 1 ), box.extent( 2 ) );

	// Add shape to shape drawable
	osg::ShapeDrawable* sd = new osg::ShapeDrawable( b );

	// Add shape drawable to geode
	osg::Geode* g = new osg::Geode;
	g->addDrawable( sd );

	// Add box to rendering
	tecosg::OsgModel* boxModel = new tecosg::OsgModel;
	boxModel->set( g );
	Canvas::instance()->setGeometryBoundingBox( boxModel );

	// Update actions
	ui.actionGenerateLayers->setEnabled( true );
	ui.actionBoundingBox->setEnabled( true );
}

void gpurt::on_actionGenerateLayers_triggered()
{
	Canvas::RenderMode prevMode = Canvas::instance()->renderMode();

	Canvas::instance()->setRenderMode( Canvas::BOUNDING_BOX, false );
	Canvas::instance()->setRenderMode( Canvas::GEOMETRY, true );

	_layerGen.generateLayers();

	Canvas::instance()->setRenderMode( prevMode );
}
