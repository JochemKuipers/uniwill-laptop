#pragma once

#include <QString>
#include <QVector>

struct FanPoint {
	int temp = 40;
	int duty = 80;
};

struct Profile {
	QString id;
	QString title;
	int pl1 = 75;
	int pl2 = 75;
	int pl4 = 100;
	int ctgp = 0;
	QVector<FanPoint> cpuCurve;
	QVector<FanPoint> gpuCurve;
};

class ProfileStore
{
public:
	static QString configPath();
	static QString modeToProfileId(const QString &mode);
	static QString profileIdToMode(const QString &id);

	ProfileStore();

	QVector<Profile> profiles() const { return m_profiles; }
	Profile *find(const QString &id);
	const Profile *find(const QString &id) const;
	Profile defaultsFor(const QString &id) const;

	bool load();
	bool save() const;
	bool exportTo(const QString &path) const;
	bool importFrom(const QString &path);

	void reset(const QString &id);
	void setProfile(const Profile &p);

	/** Write PL/cTGP/curves for profile id; skip missing sysfs attrs. */
	bool apply(const QString &id, QString *error = nullptr) const;
	bool applyMode(const QString &mode, QString *error = nullptr) const;

	static QString curveToString(const QVector<FanPoint> &pts);
	static QVector<FanPoint> curveFromString(const QString &s);

private:
	void seedDefaults();
	QVector<Profile> m_profiles;
};
