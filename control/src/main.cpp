#include "osd.hpp"
#include "window.hpp"

#include <QApplication>

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
	/* Desktop style + system palette; do not force Fusion/dark. */
	app.setDesktopFileName(QStringLiteral("uniwill-control"));

	const QStringList args = app.arguments();
	if (args.contains("--osd") || args.contains("--daemon"))
		return runOsd(app);

	app.setApplicationName("Erazer Control");
	MainWindow w;
	w.show();
	return app.exec();
}
