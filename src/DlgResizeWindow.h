#ifndef _DLGRESIZEWINDOW_H_
#define _DLGRESIZEWINDOW_H_

#include <QMainWindow>
#include <QDialog>
#include "ui_DlgResizeWindow.h"

class DlgResizeWindow : public QDialog
{
	Q_OBJECT

public:
	DlgResizeWindow( QWidget* parent = 0, Qt::WFlags flags = 0 )
		: QDialog( parent, flags )
	{
		ui.setupUi( this );
	}

	~DlgResizeWindow()
	{
		// empty
	}

	void setWindow( QMainWindow* window )
	{
		_window = window;
	}

	void showEvent(QShowEvent * e )
	{
		ui.lineEditWidth->setText( QString::number( _window->width() ) );
		ui.lineEditHeight->setText( QString::number( _window->height() - 40 ) );
	}

public slots:
	void on_pushButtonUpdate_clicked()
	{
		_window->resize( ui.lineEditWidth->text().toInt(), ui.lineEditHeight->text().toInt() + 40 );
	}

private:
	Ui::DlgResizeWindow ui;
	QMainWindow* _window;

};

#endif // _DLGRESIZEWINDOW_H_
