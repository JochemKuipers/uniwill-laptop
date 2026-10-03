#include "osd.hpp"
#include "window.hpp"

#include <QApplication>
#include <QFont>
#include <QToolTip>

static int runOsd(QApplication &app)
{
	app.setQuitOnLastWindowClosed(false);
	app.setApplicationName("uniwill-osd");
	auto *popup = new OsdPopup;
	new OsdWatcher(popup, &app);
	return app.exec();
}

int main(int argc, char **argv)
{
	QApplication app(argc, argv);
	app.setOrganizationName("uniwill-control");
	QToolTip::setFont(QFont("Noto Sans", 11));

	const QStringList args = app.arguments();
	if (args.contains("--osd") || args.contains("--daemon"))
		return runOsd(app);

	app.setApplicationName("ERAZER Major 15 X1");
	MainWindow w;
	w.show();
	return app.exec();
}
