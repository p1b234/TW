#include "tw_coremainwindow.h"
#include <QtWidgets/QApplication>
#include <cuda.h>

int main(int argc, char *argv[])
{
	qRegisterMetaType<QVector<float> >("QVector<float>");

	QSurfaceFormat format;
    format.setDepthBufferSize(24);
    format.setStencilBufferSize(8);
    format.setVersion(3, 3);
    format.setProfile(QSurfaceFormat::CoreProfile);
    QSurfaceFormat::setDefaultFormat(format);

	QApplication a(argc, argv);
	TW_CoreMainWindow w;
	
	w.show();

	cudaSetDevice(0);

	return a.exec();
}