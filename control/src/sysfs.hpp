#pragma once

#include <QColor>
#include <QDir>
#include <QFile>
#include <QString>

inline QString sysRead(const QString &path)
{
	QFile f(path);
	if (!f.open(QIODevice::ReadOnly | QIODevice::Text))
		return {};
	return QString::fromUtf8(f.readAll()).trimmed();
}

inline bool sysWrite(const QString &path, const QString &value)
{
	QFile f(path);
	if (!f.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate))
		return false;
	return f.write(value.toUtf8()) >= 0;
}

inline QString platformDir()
{
	const QDir dir("/sys/bus/platform/devices");
	const auto names = dir.entryList({"INOU0000:*"}, QDir::Dirs);
	return names.isEmpty() ? QString() : dir.filePath(names.first());
}

inline QString platformAttr(const QString &name)
{
	const QString base = platformDir();
	if (base.isEmpty())
		return {};
	const QString path = base + "/" + name;
	return QFile::exists(path) ? path : QString();
}

inline QString hwmonDir()
{
	const QDir dir("/sys/class/hwmon");
	for (const QString &name : dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot)) {
		if (sysRead(dir.filePath(name) + "/name") == "uniwill")
			return dir.filePath(name);
	}
	return {};
}

inline QString ledDir(const QString &suffix)
{
	const QString path = "/sys/class/leds/uniwill:" + suffix;
	return QFile::exists(path + "/brightness") ? path : QString();
}

inline bool readLed(const QString &dir, QColor *color, int *brightness, int *maxBrightness)
{
	if (dir.isEmpty())
		return false;
	const QStringList rgb = sysRead(dir + "/multi_intensity").split(' ', Qt::SkipEmptyParts);
	if (rgb.size() < 3)
		return false;
	const int maxI = qMax(1, sysRead(dir + "/multi_max_intensity").toInt());
	color->setRgb(qBound(0, rgb[0].toInt() * 255 / maxI, 255),
		      qBound(0, rgb[1].toInt() * 255 / maxI, 255),
		      qBound(0, rgb[2].toInt() * 255 / maxI, 255));
	*brightness = sysRead(dir + "/brightness").toInt();
	*maxBrightness = qMax(1, sysRead(dir + "/max_brightness").toInt());
	return true;
}

inline bool writeLed(const QString &dir, const QColor &color, int brightness)
{
	if (dir.isEmpty())
		return false;
	const int maxI = qMax(1, sysRead(dir + "/multi_max_intensity").toInt());
	const QString rgb = QString("%1 %2 %3")
				    .arg(color.red() * maxI / 255)
				    .arg(color.green() * maxI / 255)
				    .arg(color.blue() * maxI / 255);
	return sysWrite(dir + "/multi_intensity", rgb) &&
	       sysWrite(dir + "/brightness", QString::number(brightness));
}
