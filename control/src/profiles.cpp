#include "profiles.hpp"
#include "sysfs.hpp"

#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>

static QVector<FanPoint> pts(std::initializer_list<FanPoint> list)
{
	return QVector<FanPoint>(list);
}

QString ProfileStore::configPath()
{
	const QString dir =
		QStandardPaths::writableLocation(QStandardPaths::ConfigLocation) +
		"/uniwill-control";
	QDir().mkpath(dir);
	return dir + "/profiles.json";
}

QString ProfileStore::modeToProfileId(const QString &mode)
{
	if (mode == "low-power")
		return "office";
	if (mode == "performance")
		return "turbo";
	if (mode == "custom")
		return "custom";
	return "balanced";
}

QString ProfileStore::profileIdToMode(const QString &id)
{
	if (id == "office")
		return "low-power";
	if (id == "turbo")
		return "performance";
	if (id == "custom")
		return "custom";
	return "balanced";
}

ProfileStore::ProfileStore()
{
	seedDefaults();
	load();
}

void ProfileStore::seedDefaults()
{
	m_profiles = {
		{ "office", "Office", 45, 45, 90, 0,
		  pts({{40, 25}, {55, 45}, {70, 80}, {80, 120}, {90, 170}}),
		  pts({{40, 20}, {55, 40}, {70, 75}, {80, 110}, {90, 160}}) },
		{ "balanced", "Balanced", 75, 75, 120, 20,
		  pts({{40, 35}, {55, 70}, {70, 120}, {80, 160}, {90, 210}}),
		  pts({{40, 30}, {55, 65}, {70, 115}, {80, 155}, {90, 200}}) },
		{ "turbo", "Turbo", 140, 140, 180, 35,
		  pts({{35, 50}, {50, 100}, {65, 160}, {80, 200}, {90, 255}}),
		  pts({{35, 45}, {50, 95}, {65, 155}, {80, 195}, {90, 255}}) },
		{ "custom", "Custom", 75, 75, 120, 20,
		  pts({{40, 35}, {55, 70}, {70, 120}, {80, 160}, {90, 210}}),
		  pts({{40, 30}, {55, 65}, {70, 115}, {80, 155}, {90, 200}}) },
	};
}

Profile ProfileStore::defaultsFor(const QString &id) const
{
	ProfileStore tmp;
	tmp.seedDefaults();
	if (const Profile *p = tmp.find(id))
		return *p;
	return tmp.m_profiles[1];
}

Profile *ProfileStore::find(const QString &id)
{
	for (Profile &p : m_profiles) {
		if (p.id == id)
			return &p;
	}
	return nullptr;
}

const Profile *ProfileStore::find(const QString &id) const
{
	for (const Profile &p : m_profiles) {
		if (p.id == id)
			return &p;
	}
	return nullptr;
}

QString ProfileStore::curveToString(const QVector<FanPoint> &curvePts)
{
	QStringList parts;
	for (const FanPoint &pt : curvePts)
		parts << QString("%1:%2").arg(pt.temp).arg(pt.duty);
	return parts.join(' ');
}

QVector<FanPoint> ProfileStore::curveFromString(const QString &s)
{
	QVector<FanPoint> out;
	if (s.trimmed().isEmpty() || s.trimmed() == "auto")
		return out;
	for (const QString &tok : s.split(' ', Qt::SkipEmptyParts)) {
		const QStringList pair = tok.split(':');
		if (pair.size() != 2)
			continue;
		FanPoint pt;
		pt.temp = pair[0].toInt();
		pt.duty = qBound(0, pair[1].toInt(), 255);
		out.push_back(pt);
	}
	return out;
}

static QJsonObject profileToJson(const Profile &p)
{
	QJsonObject o;
	o["pl1"] = p.pl1;
	o["pl2"] = p.pl2;
	o["pl4"] = p.pl4;
	o["ctgp"] = p.ctgp;
	o["cpu_fan_curve"] = ProfileStore::curveToString(p.cpuCurve);
	o["gpu_fan_curve"] = ProfileStore::curveToString(p.gpuCurve);
	return o;
}

static void profileFromJson(Profile &p, const QJsonObject &o)
{
	if (o.contains("pl1"))
		p.pl1 = o["pl1"].toInt(p.pl1);
	if (o.contains("pl2"))
		p.pl2 = o["pl2"].toInt(p.pl2);
	if (o.contains("pl4"))
		p.pl4 = o["pl4"].toInt(p.pl4);
	if (o.contains("ctgp"))
		p.ctgp = o["ctgp"].toInt(p.ctgp);
	if (o.contains("cpu_fan_curve"))
		p.cpuCurve = ProfileStore::curveFromString(o["cpu_fan_curve"].toString());
	if (o.contains("gpu_fan_curve"))
		p.gpuCurve = ProfileStore::curveFromString(o["gpu_fan_curve"].toString());
}

bool ProfileStore::load()
{
	QFile f(configPath());
	if (!f.open(QIODevice::ReadOnly | QIODevice::Text))
		return false;
	const QJsonDocument doc = QJsonDocument::fromJson(f.readAll());
	if (!doc.isObject())
		return false;
	const QJsonObject root = doc.object();
	const QJsonObject profiles = root.value("profiles").toObject();
	for (Profile &p : m_profiles) {
		if (profiles.contains(p.id))
			profileFromJson(p, profiles.value(p.id).toObject());
	}
	return true;
}

bool ProfileStore::save() const
{
	QJsonObject profiles;
	for (const Profile &p : m_profiles)
		profiles[p.id] = profileToJson(p);
	QJsonObject root;
	root["version"] = 1;
	root["profiles"] = profiles;
	QFile f(configPath());
	if (!f.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate))
		return false;
	f.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
	return true;
}

bool ProfileStore::exportTo(const QString &path) const
{
	QJsonObject profiles;
	for (const Profile &p : m_profiles)
		profiles[p.id] = profileToJson(p);
	QJsonObject root;
	root["version"] = 1;
	root["profiles"] = profiles;
	QFile f(path);
	if (!f.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate))
		return false;
	f.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
	return true;
}

bool ProfileStore::importFrom(const QString &path)
{
	QFile f(path);
	if (!f.open(QIODevice::ReadOnly | QIODevice::Text))
		return false;
	const QJsonDocument doc = QJsonDocument::fromJson(f.readAll());
	if (!doc.isObject())
		return false;
	const QJsonObject profiles = doc.object().value("profiles").toObject();
	if (profiles.isEmpty())
		return false;
	for (Profile &p : m_profiles) {
		if (profiles.contains(p.id))
			profileFromJson(p, profiles.value(p.id).toObject());
	}
	return save();
}

void ProfileStore::reset(const QString &id)
{
	const Profile def = defaultsFor(id);
	if (Profile *p = find(id)) {
		p->pl1 = def.pl1;
		p->pl2 = def.pl2;
		p->pl4 = def.pl4;
		p->ctgp = def.ctgp;
		p->cpuCurve = def.cpuCurve;
		p->gpuCurve = def.gpuCurve;
	}
}

void ProfileStore::setProfile(const Profile &p)
{
	if (Profile *dst = find(p.id))
		*dst = p;
}

bool ProfileStore::apply(const QString &id, QString *error) const
{
	const Profile *p = find(id);
	if (!p) {
		if (error)
			*error = "Unknown profile";
		return false;
	}

	auto writeOpt = [](const QString &attr, const QString &value) {
		const QString path = platformAttr(attr);
		if (path.isEmpty())
			return true;
		return sysWrite(path, value);
	};

	if (!writeOpt("pl1_watt", QString::number(p->pl1)) ||
	    !writeOpt("pl2_watt", QString::number(p->pl2)) ||
	    !writeOpt("pl4_watt", QString::number(p->pl4)) ||
	    !writeOpt("ctgp_offset", QString::number(p->ctgp))) {
		if (error)
			*error = "Could not write power limits";
		return false;
	}

	const QString cpu = curveToString(p->cpuCurve);
	const QString gpu = curveToString(p->gpuCurve);
	if (!cpu.isEmpty() && !writeOpt("cpu_fan_curve", cpu)) {
		if (error)
			*error = "Could not write CPU fan curve";
		return false;
	}
	if (!gpu.isEmpty() && !writeOpt("gpu_fan_curve", gpu)) {
		if (error)
			*error = "Could not write GPU fan curve";
		return false;
	}
	return true;
}

bool ProfileStore::applyMode(const QString &mode, QString *error) const
{
	return apply(modeToProfileId(mode), error);
}
