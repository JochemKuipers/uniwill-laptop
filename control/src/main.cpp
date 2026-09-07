#include "window.hpp"

#include <QApplication>
#include <QFont>
#include <QToolTip>

int main(int argc, char **argv)
{
	QApplication app(argc, argv);
	app.setApplicationName("ERAZER Major 15 X1");
	QToolTip::setFont(QFont("Noto Sans", 11));
	app.setOrganizationName("uniwill-control");
	MainWindow w;
	w.show();
	return app.exec();
}
