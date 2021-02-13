#ifndef GPURT_H
#define GPURT_H

#include <QtGui/QMainWindow>
#include <QKeyEvent>
#include <QLabel>
#include "ui_gpurt.h"
#include "Canvas.h"
#include "LayerGenerator.h"
#include "DlgResizeWindow.h"
#include <osg/Switch>

class gpurt : public QMainWindow
{
    Q_OBJECT

public:
    gpurt(QWidget *parent = 0, Qt::WFlags flags = 0);
    ~gpurt();

	void loadModel( const QString& filename );

protected slots:
	virtual void keyPressEvent( QKeyEvent* e );
	void updateFps( double fps );

	void on_actionResize_triggered();

	void on_actionIdle_toggled( bool enabled );
	void on_actionGeometry_toggled( bool enabled );
	void on_actionBoundingBox_toggled( bool enabled );
	void on_actionHeightmap_toggled( bool enabled );
	void on_actionPostShading_toggled( bool enabled );

	void on_actionLoad_triggered();
	void on_actionLoadLayers_triggered();

	void on_actionLoadSphereScene_triggered();

	void on_actionDeleteLayers_triggered();
	void on_actionDilateNormals_triggered();

	void on_actionComputeBoundingBox_triggered();
	void on_actionGenerateLayers_triggered();

private:
    Ui::gpurtClass ui;
	QLabel _fpsLabel;
	LayerGenerator _layerGen;
	DlgResizeWindow _resizeDialog;
};

#endif // GPURT_H
