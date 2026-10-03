#include "osd.hpp"
#include "sysfs.hpp"

#include <QGuiApplication>
#include <QHBoxLayout>
#include <QLabel>
#include <QPainter>
#include <QPainterPath>
#include <QScreen>
#include <QTimer>
#include <QVBoxLayout>

static QPixmap makeCanvas(int size = 48)
{
	QPixmap px(size, size);
	px.fill(Qt::transparent);
	return px;
}

static void strokeIcon(QPainter &p, const QColor &c)
{
	p.setRenderHint(QPainter::Antialiasing, true);
	p.setPen(QPen(c, 2.4, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
	p.setBrush(Qt::NoBrush);
}

OsdPopup::OsdPopup(QWidget *parent)
	: QWidget(parent)
{
	setObjectName("osd");
	setWindowFlags(Qt::FramelessWindowHint | Qt::Tool | Qt::WindowStaysOnTopHint |
		       Qt::WindowDoesNotAcceptFocus);
	setAttribute(Qt::WA_TranslucentBackground);
	setAttribute(Qt::WA_ShowWithoutActivating);
	setAttribute(Qt::WA_TransparentForMouseEvents);
	setFixedHeight(64);
	setMinimumWidth(280);

	auto *root = new QHBoxLayout(this);
	root->setContentsMargins(14, 8, 18, 8);
	root->setSpacing(12);

	m_icon = new QLabel;
	m_icon->setFixedSize(48, 48);
	m_icon->setAlignment(Qt::AlignCenter);
	root->addWidget(m_icon);

	auto *col = new QVBoxLayout;
	col->setContentsMargins(0, 0, 0, 0);
	col->setSpacing(0);

	m_title = new QLabel;
	m_title->setObjectName("osdTitle");
	col->addWidget(m_title);

	auto *row = new QHBoxLayout;
	row->setContentsMargins(0, 0, 0, 0);
	row->setSpacing(12);
	m_left = new QLabel;
	m_left->setObjectName("osdOpt");
	m_right = new QLabel;
	m_right->setObjectName("osdOpt");
	m_value = new QLabel;
	m_value->setObjectName("osdValue");
	m_subtitle = new QLabel;
	m_subtitle->setObjectName("osdSub");
	row->addWidget(m_left);
	row->addWidget(m_right);
	row->addWidget(m_value);
	row->addStretch(1);
	col->addLayout(row);
	col->addWidget(m_subtitle);
	root->addLayout(col, 1);

	m_hide = new QTimer(this);
	m_hide->setSingleShot(true);
	m_hide->setInterval(1600);
	connect(m_hide, &QTimer::timeout, this, &QWidget::hide);

	setStyleSheet(R"(
		QLabel#osdTitle {
			color: #9A9286;
			font-family: "Fira Sans Condensed", "Noto Sans";
			font-size: 11px;
			font-weight: 700;
			letter-spacing: 0.08em;
		}
		QLabel#osdOpt, QLabel#osdValue {
			color: #E7E0D4;
			font-family: "Fira Sans Condensed", "Noto Sans";
			font-size: 22px;
			font-weight: 700;
		}
		QLabel#osdSub {
			color: #9A9286;
			font-family: "Fira Sans Condensed", "Noto Sans";
			font-size: 12px;
			font-weight: 600;
		}
	)");
}

void OsdPopup::paintEvent(QPaintEvent *)
{
	QPainter p(this);
	p.setRenderHint(QPainter::Antialiasing, true);
	QPainterPath path;
	path.addRoundedRect(QRectF(rect()).adjusted(0.5, 0.5, -0.5, -0.5), 10, 10);
	p.fillPath(path, QColor(12, 11, 10, 230));
	p.setPen(QPen(QColor(58, 54, 47), 1));
	p.drawPath(path);
	p.setPen(QPen(m_accent, 3));
	p.drawLine(QPoint(10, 12), QPoint(10, height() - 12));
}

void OsdPopup::placeAndShow()
{
	adjustSize();
	setFixedWidth(qMax(280, sizeHint().width()));
	const QScreen *screen = QGuiApplication::primaryScreen();
	const QRect geo = screen ? screen->availableGeometry() : QRect(0, 0, 1280, 800);
	move(geo.left() + 48, geo.top() + 48);
	show();
	raise();
	m_hide->start();
}

void OsdPopup::present(const QPixmap &icon, const QString &title, const QString &left,
			const QString &right, bool leftActive)
{
	m_icon->setPixmap(icon);
	m_title->setText(title.toUpper());
	m_left->setText(left);
	m_right->setText(right);
	m_value->clear();
	m_subtitle->clear();
	m_subtitle->setVisible(false);
	m_left->setVisible(true);
	m_right->setVisible(true);
	m_value->setVisible(false);
	m_left->setStyleSheet(QString("color: %1;").arg(leftActive ? "#E7E0D4" : "#5C564C"));
	m_right->setStyleSheet(QString("color: %1;").arg(leftActive ? "#5C564C" : "#E7E0D4"));
	m_accent = QColor(226, 58, 34);
	placeAndShow();
}

void OsdPopup::presentSingle(const QPixmap &icon, const QString &title, const QString &value,
			     const QColor &accent, const QString &subtitle)
{
	m_icon->setPixmap(icon);
	m_title->setText(title.toUpper());
	m_value->setText(value);
	m_subtitle->setText(subtitle);
	m_subtitle->setVisible(!subtitle.isEmpty());
	m_left->clear();
	m_right->clear();
	m_left->setVisible(false);
	m_right->setVisible(false);
	m_value->setVisible(true);
	m_value->setStyleSheet(QString("color: %1;").arg(accent.name()));
	m_accent = accent;
	placeAndShow();
}

QPixmap OsdPopup::iconFor(OsdKind kind, bool on) const
{
	QPixmap px = makeCanvas();
	QPainter p(&px);
	const QColor c = on ? QColor(226, 58, 34) : QColor(154, 146, 134);
	strokeIcon(p, c);

	switch (kind) {
	case OsdKind::FnLock: {
		p.drawRoundedRect(QRectF(12, 10, 24, 28), 4, 4);
		p.drawText(QRect(12, 14, 24, 20), Qt::AlignCenter, QStringLiteral("Fn"));
		if (on)
			p.drawLine(16, 40, 32, 40);
		break;
	}
	case OsdKind::SuperKey: {
		p.drawRoundedRect(QRectF(11, 11, 26, 26), 3, 3);
		p.drawLine(24, 11, 24, 37);
		p.drawLine(11, 24, 37, 24);
		if (!on) {
			p.setPen(QPen(QColor(226, 58, 34), 2.6, Qt::SolidLine, Qt::RoundCap));
			p.drawLine(14, 34, 34, 14);
		}
		break;
	}
	case OsdKind::TouchpadToggle: {
		p.drawRoundedRect(QRectF(10, 14, 28, 20), 4, 4);
		p.drawLine(24, 26, 24, 34);
		if (!on) {
			p.setPen(QPen(QColor(226, 58, 34), 2.6, Qt::SolidLine, Qt::RoundCap));
			p.drawLine(14, 34, 34, 14);
		}
		break;
	}
	default:
		break;
	}
	return px;
}

QPixmap OsdPopup::iconPerformance(const QString &profile) const
{
	QPixmap px = makeCanvas();
	QPainter p(&px);
	p.setRenderHint(QPainter::Antialiasing, true);

	QColor c(226, 58, 34);
	if (profile == "low-power")
		c = QColor(74, 158, 122);
	else if (profile == "balanced")
		c = QColor(212, 160, 23);
	else if (profile == "performance")
		c = QColor(226, 58, 34);
	else if (profile == "custom")
		c = QColor(120, 160, 220);

	p.setPen(Qt::NoPen);
	p.setBrush(c);
	const QPointF pts[] = {{10, 30}, {20, 12}, {24, 22}, {38, 10}, {30, 36}, {22, 26}};
	p.drawPolygon(pts, 6);
	return px;
}

QPixmap OsdPopup::iconKeyboard(int level, int maxLevel) const
{
	QPixmap px = makeCanvas();
	QPainter p(&px);
	strokeIcon(p, QColor(231, 224, 212));
	p.drawRoundedRect(QRectF(8, 16, 32, 16), 3, 3);
	p.drawLine(14, 32, 34, 32);

	const int n = qMax(1, maxLevel);
	const int lit = qBound(0, level, n);
	p.setPen(Qt::NoPen);
	for (int i = 0; i < n; ++i) {
		p.setBrush(i < lit ? QColor(226, 58, 34) : QColor(58, 54, 47));
		p.drawRoundedRect(QRectF(11 + i * 6.2, 8, 5, 5), 1, 1);
	}
	return px;
}

void OsdPopup::showToggle(OsdKind kind, bool on)
{
	QString title;
	switch (kind) {
	case OsdKind::FnLock:
		title = QStringLiteral("Fn Lock");
		break;
	case OsdKind::SuperKey:
		title = QStringLiteral("Windows Key");
		break;
	default:
		return;
	}
	present(iconFor(kind, on), title, QStringLiteral("Lock"), QStringLiteral("Unlock"), on);
}

void OsdPopup::showOnOff(OsdKind kind, bool on, const QString &title)
{
	present(iconFor(kind, on), title, QStringLiteral("On"), QStringLiteral("Off"), on);
}

void OsdPopup::showPerformance(const QString &profile, const QString &subtitle)
{
	QString label = QStringLiteral("Balance");
	QColor accent(212, 160, 23);
	if (profile == "low-power") {
		label = QStringLiteral("Office");
		accent = QColor(74, 158, 122);
	} else if (profile == "performance") {
		label = QStringLiteral("Turbo");
		accent = QColor(226, 58, 34);
	} else if (profile == "custom") {
		label = QStringLiteral("Custom");
		accent = QColor(120, 160, 220);
	}
	presentSingle(iconPerformance(profile), QStringLiteral("Performance Mode"), label, accent,
		      subtitle);
}

void OsdPopup::showKeyboardLevel(int level, int maxLevel)
{
	const QString label = level <= 0 ? QStringLiteral("Off")
					 : QStringLiteral("Level %1").arg(level);
	presentSingle(iconKeyboard(level, maxLevel), QStringLiteral("Keyboard Light"), label,
		      level <= 0 ? QColor(154, 146, 134) : QColor(226, 58, 34));
}

OsdWatcher::OsdWatcher(OsdPopup *popup, QObject *parent)
	: QObject(parent)
	, m_popup(popup)
{
	m_timer = new QTimer(this);
	m_timer->setInterval(150);
	connect(m_timer, &QTimer::timeout, this, &OsdWatcher::poll);
	baseline();
	m_timer->start();
}

void OsdWatcher::applyActiveProfile(bool showOsd)
{
	QString mode = sysRead(platformAttr("performance_mode"));
	if (mode.isEmpty())
		mode = sysRead("/sys/firmware/acpi/platform_profile");
	if (mode.isEmpty())
		return;

	m_store.load();
	QString err;
	m_store.applyMode(mode, &err);

	if (!showOsd)
		return;

	QString subtitle;
	if (const Profile *p = m_store.find(ProfileStore::modeToProfileId(mode)))
		subtitle = QStringLiteral("PL1 %1 W").arg(p->pl1);
	m_popup->showPerformance(mode, subtitle);
}

void OsdWatcher::baseline()
{
	QString mode = sysRead(platformAttr("performance_mode"));
	if (mode.isEmpty())
		mode = sysRead("/sys/firmware/acpi/platform_profile");
	m_profile = mode;
	m_fnLock = sysRead(platformAttr("fn_lock"));
	m_superKey = sysRead(platformAttr("super_key_enable"));
	m_touchpadToggle = sysRead(platformAttr("touchpad_toggle_enable"));
	const QString kbd = ledDir("multicolor:kbd_backlight");
	m_kbdBright = kbd.isEmpty() ? -1 : sysRead(kbd + "/brightness").toInt();
	m_ready = true;
	applyActiveProfile(false);
}

void OsdWatcher::poll()
{
	if (!m_ready)
		return;

	QString profile = sysRead(platformAttr("performance_mode"));
	if (profile.isEmpty())
		profile = sysRead("/sys/firmware/acpi/platform_profile");
	if (!profile.isEmpty() && profile != m_profile) {
		m_profile = profile;
		applyActiveProfile(true);
	}

	const QString fn = sysRead(platformAttr("fn_lock"));
	if (!fn.isEmpty() && fn != m_fnLock) {
		m_fnLock = fn;
		m_popup->showToggle(OsdKind::FnLock, fn == "1");
	}

	const QString superKey = sysRead(platformAttr("super_key_enable"));
	if (!superKey.isEmpty() && superKey != m_superKey) {
		m_superKey = superKey;
		/* sysfs is "enable"; OSD Lock means Windows key locked/disabled */
		m_popup->showToggle(OsdKind::SuperKey, superKey == "0");
	}

	const QString touchpad = sysRead(platformAttr("touchpad_toggle_enable"));
	if (!touchpad.isEmpty() && touchpad != m_touchpadToggle) {
		m_touchpadToggle = touchpad;
		m_popup->showOnOff(OsdKind::TouchpadToggle, touchpad == "1",
				  QStringLiteral("Touchpad Hotkey"));
	}

	const QString kbd = ledDir("multicolor:kbd_backlight");
	if (!kbd.isEmpty()) {
		const int bright = sysRead(kbd + "/brightness").toInt();
		const int maxB = qMax(1, sysRead(kbd + "/max_brightness").toInt());
		if (m_kbdBright >= 0 && bright != m_kbdBright)
			m_popup->showKeyboardLevel(bright, maxB);
		m_kbdBright = bright;
	}
}
