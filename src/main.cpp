#include <QtGui/QApplication>
#include "gpurt.h"
#include <QGLFormat>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    gpurt w;
    w.show();
	if( argc > 1 )
		w.loadModel( argv[1] );
    a.connect(&a, SIGNAL(lastWindowClosed()), &a, SLOT(quit()));
    return a.exec();
}
