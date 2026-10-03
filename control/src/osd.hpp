#pragma once

#include "profiles.hpp"

#include <QPixmap>
#include <QString>
#include <QWidget>

class QLabel;
class QTimer;

enum class OsdKind {
	Performance,
	FnLock,
	SuperKey,
	KeyboardLight,
	TouchpadToggle,
};

class OsdPopup : public QWidget
{
	Q_OBJECT
public:
	explicit OsdPopup(QWidget *parent = nullptr);

	void showToggle(OsdKind kind, bool on);
	void showOnOff(OsdKind kind, bool on, const QString &title);
	void showPerformance(const QString &profile, const QString &subtitle = {});
	void showKeyboardLevel(int level, int maxLevel);

protected:
	void paintEvent(QPaintEvent *event) override;

private:
	void present(const QPixmap &icon, const QString &title, const QString &left, const QString &right,
		     bool leftActive);
	void presentSingle(const QPixmap &icon, const QString &title, const QString &value,
			   const QColor &accent, const QString &subtitle = {});
	void placeAndShow();
	QPixmap iconFor(OsdKind kind, bool on) const;
	QPixmap iconPerformance(const QString &profile) const;
	QPixmap iconKeyboard(int level, int maxLevel) const;

	QLabel *m_icon{};
	QLabel *m_title{};
	QLabel *m_left{};
	QLabel *m_right{};
	QLabel *m_value{};
	QLabel *m_subtitle{};
	QTimer *m_hide{};
	QColor m_accent{226, 58, 34};
};

class OsdWatcher : public QObject
{
	Q_OBJECT
public:
	explicit OsdWatcher(OsdPopup *popup, QObject *parent = nullptr);

private:
	void poll();
	void baseline();
	void applyActiveProfile(bool showOsd);

	OsdPopup *m_popup{};
	QTimer *m_timer{};
	ProfileStore m_store;
	QString m_profile;
	QString m_fnLock;
	QString m_superKey;
	QString m_touchpadToggle;
	int m_kbdBright = -1;
	bool m_ready = false;
};
