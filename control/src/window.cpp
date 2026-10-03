#include "window.hpp"
#include "sysfs.hpp"

#include <QCheckBox>
#include <QComboBox>
#include <QDateTime>
#include <QFile>
#include <QFileDialog>
#include <QFont>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QPalette>
#include <QProcess>
#include <QPushButton>
#include <QScrollArea>
#include <QSlider>
#include <QSpinBox>
#include <QTabWidget>
#include <QTimer>
#include <QVBoxLayout>

/* Layout/type only — colors come from the system palette. */
static const char *kStyle = R"(
QLabel#brand {
	color: #E23A22;
	font-family: "Fira Sans Condensed", sans-serif;
	font-size: 26px;
	font-weight: 800;
	letter-spacing: 0.18em;
}
QLabel#tagline {
	color: palette(placeholder-text);
	font-family: "Fira Sans Condensed", sans-serif;
	font-size: 11px;
	font-weight: 700;
	letter-spacing: 0.16em;
}
QLabel#model {
	color: palette(placeholder-text);
	font-family: "Fira Sans Condensed", sans-serif;
	font-size: 14px;
	font-weight: 600;
}
QLabel#modeChip {
	color: palette(window-text);
	background: palette(base);
	border: 1px solid palette(mid);
	border-left: 3px solid #E23A22;
	padding: 6px 12px;
	font-family: "Fira Sans Condensed", sans-serif;
	font-size: 12px;
	font-weight: 700;
	letter-spacing: 0.06em;
}
QLabel#section {
	font-family: "Fira Sans Condensed", sans-serif;
	font-size: 14px;
	font-weight: 700;
}
QLabel#quiet { color: palette(placeholder-text); font-size: 12px; }
QLabel#status {
	color: palette(placeholder-text);
	background: palette(base);
	border-top: 1px solid palette(mid);
	padding: 10px 12px;
	font-size: 12px;
}
QLabel#heat, QLabel#teleVal {
	font-family: "JetBrains Mono", "IBM Plex Mono", monospace;
	font-size: 24px;
	font-weight: 500;
}
QLabel#teleVal { font-size: 14px; }
QWidget#gauge, QWidget#chip {
	background: palette(base);
	border: 1px solid palette(mid);
	border-top: 2px solid palette(highlight);
}
QWidget#row { border-bottom: 1px solid palette(mid); }
QWidget#palm, QScrollArea { background: transparent; border: none; }
QSlider#spectrum::groove:horizontal {
	background: qlineargradient(x1:0, y1:0, x2:1, y2:0,
		stop:0 #c43c2b, stop:0.18 #d4a017, stop:0.36 #3d8c4a,
		stop:0.54 #2a7a8c, stop:0.72 #3d4a9c, stop:0.9 #8a3a7a, stop:1 #c43c2b);
}
QPushButton#primary {
	background: palette(highlight);
	color: palette(highlighted-text);
	border: 1px solid palette(highlight);
	font-weight: 700;
	padding: 8px 16px;
}
)";

static qreal levelOf(int brightness, int maxBrightness)
{
	if (brightness <= 0 || maxBrightness <= 0)
		return 0.05;
	return qBound(0.12, brightness / qreal(maxBrightness), 1.0);
}

LaptopView::LaptopView(QWidget *parent)
	: QWidget(parent)
{
	setMinimumHeight(248);
	setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
	setToolTip("Live keyboard and front LED strip — updates from the Lights tab.");
}

void LaptopView::setKeys(const QColor &color, int brightness, int maxBrightness)
{
	m_keys = color;
	m_keyLevel = levelOf(brightness, maxBrightness);
	update();
}

void LaptopView::setBar(const QColor &color, int brightness, int maxBrightness)
{
	m_bar = color;
	m_barLevel = levelOf(brightness, maxBrightness);
	update();
}

void LaptopView::paintEvent(QPaintEvent *)
{
	QPainter p(this);
	p.setRenderHint(QPainter::Antialiasing);

	const QPalette pal = palette();
	const QColor base = pal.color(QPalette::Base);
	const QColor window = pal.color(QPalette::Window);
	const QColor mid = pal.color(QPalette::Mid);
	const QColor light = pal.color(QPalette::Light);

	const QRectF shell = QRectF(rect()).adjusted(20, 10, -20, 8);

	/* Soft bloom from the keyboard color under the chassis */
	QRadialGradient bloom(shell.center(), shell.width() * 0.55);
	bloom.setColorAt(0.0, QColor(m_keys.red(), m_keys.green(), m_keys.blue(),
				     int(28 + 50 * m_keyLevel)));
	bloom.setColorAt(1.0, Qt::transparent);
	p.fillRect(rect(), bloom);

	QLinearGradient chassis(shell.topLeft(), shell.bottomLeft());
	chassis.setColorAt(0.0, base);
	chassis.setColorAt(0.55, window);
	chassis.setColorAt(1.0, base.darker(115));
	p.setPen(QPen(mid, 1));
	p.setBrush(chassis);
	p.drawRoundedRect(shell, 14, 14);

	/* Thin bezel inset */
	QColor inset = light;
	inset.setAlpha(40);
	p.setPen(QPen(inset, 1));
	p.setBrush(Qt::NoBrush);
	p.drawRoundedRect(shell.adjusted(1.5, 1.5, -1.5, -1.5), 13, 13);

	const QColor keyFill(m_keys.red(), m_keys.green(), m_keys.blue(),
			     int(28 + 120 * m_keyLevel));
	const QColor keyEdge(m_keys.red(), m_keys.green(), m_keys.blue(),
			     int(80 + 150 * m_keyLevel));

	static const QVector<QVector<qreal>> rows = {
		{1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2},
		{1.5, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1.5},
		{1.75, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2.25},
		{2.25, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2.75},
		{1.25, 1.25, 1.25, 6.5, 1.25, 1.25, 1.25, 1.25},
	};

	const qreal gap = 3.0;
	const qreal left = shell.left() + 18;
	const qreal usable = shell.width() - 36;
	const qreal barH = 8;
	const qreal keyArea = shell.height() - 34 - barH;
	const qreal rowH = (keyArea - gap * (rows.size() - 1)) / rows.size();
	qreal y = shell.top() + 14;

	for (const auto &row : rows) {
		qreal units = 0;
		for (qreal u : row)
			units += u;
		qreal x = left;
		for (qreal u : row) {
			const qreal w = usable * (u / units) - gap;
			QPainterPath path;
			path.addRoundedRect(QRectF(x, y, w, rowH), 2.4, 2.4);
			p.fillPath(path, keyFill);
			p.setPen(QPen(keyEdge, 1));
			p.drawPath(path);
			x += w + gap;
		}
		y += rowH + gap;
	}

	const QRectF bar(left, shell.bottom() - 16 - barH, usable - gap, barH);
	QLinearGradient barGrad(bar.topLeft(), bar.topRight());
	barGrad.setColorAt(0.0, QColor(m_bar.red(), m_bar.green(), m_bar.blue(),
				       int(40 + 160 * m_barLevel)));
	barGrad.setColorAt(0.5, QColor(m_bar.red(), m_bar.green(), m_bar.blue(),
				       int(70 + 185 * m_barLevel)));
	barGrad.setColorAt(1.0, QColor(m_bar.red(), m_bar.green(), m_bar.blue(),
				       int(40 + 160 * m_barLevel)));
	p.setPen(Qt::NoPen);
	p.setBrush(barGrad);
	p.drawRoundedRect(bar, 3, 3);
}

static QSlider *mkSlider(int max, bool spectrum = false)
{
	auto *s = new QSlider(Qt::Horizontal);
	s->setRange(0, max);
	if (spectrum)
		s->setObjectName("spectrum");
	return s;
}

QWidget *MainWindow::sliderBlock(const QString &title, const QString &tip, QSlider *slider)
{
	auto *box = new QWidget;
	auto *lay = new QVBoxLayout(box);
	lay->setContentsMargins(0, 0, 0, 8);
	lay->setSpacing(4);
	auto *lab = new QLabel(title);
	lab->setObjectName("section");
	lab->setToolTip(tip);
	slider->setToolTip(tip);
	lay->addWidget(lab);
	lay->addWidget(slider);
	return box;
}

QWidget *MainWindow::toggleRow(const QString &title, const QString &tip, const QString &attr)
{
	auto *box = new QWidget;
	box->setObjectName("row");
	auto *lay = new QHBoxLayout(box);
	lay->setContentsMargins(4, 10, 4, 10);
	auto *name = new QLabel(title);
	name->setObjectName("section");
	name->setToolTip(tip);
	auto *cb = new QCheckBox;
	cb->setToolTip(tip);
	const QString path = platformAttr(attr);
	if (path.isEmpty()) {
		cb->setEnabled(false);
		name->setEnabled(false);
	} else {
		cb->setChecked(sysRead(path) == "1");
		connect(cb, &QCheckBox::toggled, this, [this, path, cb](bool on) {
			if (!sysWrite(path, on ? "1" : "0")) {
				m_status->setText("Could not save that setting. Is the driver loaded?");
				cb->blockSignals(true);
				cb->setChecked(!on);
				cb->blockSignals(false);
			}
		});
	}
	lay->addWidget(name);
	lay->addStretch();
	lay->addWidget(cb);
	box->setToolTip(tip);
	return box;
}

static QString modeLabel(const QString &mode)
{
	if (mode == "low-power")
		return QStringLiteral("OFFICE");
	if (mode == "performance")
		return QStringLiteral("TURBO");
	if (mode == "custom")
		return QStringLiteral("CUSTOM");
	if (mode == "balanced")
		return QStringLiteral("BALANCED");
	return QStringLiteral("—");
}

MainWindow::MainWindow(QWidget *parent)
	: QWidget(parent)
{
	setObjectName("shell");
	setWindowTitle("Erazer Control");
	resize(920, 760);
	setStyleSheet(kStyle);

	auto *brandCol = new QVBoxLayout;
	brandCol->setSpacing(0);
	brandCol->setContentsMargins(0, 0, 0, 0);
	auto *brand = new QLabel("ERAZER");
	brand->setObjectName("brand");
	auto *tag = new QLabel("CONTROL");
	tag->setObjectName("tagline");
	tag->setToolTip("Erazer Control");
	brandCol->addWidget(brand);
	brandCol->addWidget(tag);

	auto *model = new QLabel("Major 15 X1");
	model->setObjectName("model");
	model->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
	m_modeChip = new QLabel("—");
	m_modeChip->setObjectName("modeChip");
	m_modeChip->setAlignment(Qt::AlignCenter);
	auto *right = new QVBoxLayout;
	right->setSpacing(6);
	right->addWidget(model);
	right->addWidget(m_modeChip, 0, Qt::AlignRight);

	auto *head = new QHBoxLayout;
	head->setContentsMargins(0, 0, 0, 0);
	head->addLayout(brandCol);
	head->addStretch();
	head->addLayout(right);

	auto *tabs = new QTabWidget;
	tabs->setDocumentMode(true);
	tabs->tabBar()->setExpanding(true);
	tabs->addTab(pageLights(), "Lights");
	tabs->addTab(pageKeys(), "Keys");
	tabs->addTab(pageProfiles(), "Profiles");
	tabs->addTab(pagePower(), "Power");
	tabs->addTab(pageInfo(), "Info");
	tabs->setTabToolTip(0, "Keyboard backlight and the LED strip on the front of the laptop.");
	tabs->setTabToolTip(1, "Fn lock, Super key, and the shortcut that disables the touchpad.");
	tabs->setTabToolTip(2, "Office / Balanced / Turbo / Custom power packages: PL limits and fan curves.");
	tabs->setTabToolTip(3, "USB-C power split and what happens when you plug in or shut down.");
	tabs->setTabToolTip(4, "Live temperatures, fans, power limits, battery, and GPU readouts.");

	m_status = new QLabel;
	m_status->setObjectName("status");
	m_status->setWordWrap(true);
	if (platformDir().isEmpty())
		m_status->setText("Driver not loaded. Run install.sh, then open this again.");
	else
		m_status->setText("Hover a control for what it does.");

	auto *root = new QVBoxLayout(this);
	root->setContentsMargins(28, 20, 28, 0);
	root->setSpacing(14);
	root->addLayout(head);
	root->addWidget(tabs, 1);
	root->addWidget(m_status);

	auto *timer = new QTimer(this);
	connect(timer, &QTimer::timeout, this, [this] {
		refreshInfo();
		refreshTelemetry();
	});
	timer->start(1500);
	refreshInfo();
	refreshTelemetry();
}

static constexpr int kPlotTempMin = 20;
static constexpr int kPlotTempMax = 100;
static constexpr int kSafetyTemp = 95;
static constexpr int kSafetyDuty = 77; /* ~30% of 255 */

static int dutyToPercent(int duty)
{
	return qBound(0, int(qRound(duty * 100.0 / 255.0)), 100);
}

static int percentToDuty(int percent)
{
	return qBound(0, int(qRound(percent * 255.0 / 100.0)), 255);
}

FanCurvePlot::FanCurvePlot(QWidget *parent)
	: QWidget(parent)
{
	setFixedHeight(200);
	setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
	setMouseTracking(true);
	setToolTip("Horizontal: temperature (°C). Vertical: fan speed (%).\n"
		   "Drag a point to change it.");
}

void FanCurvePlot::setPoints(const QVector<FanPoint> &pts)
{
	m_points = pts;
	if (m_points.isEmpty())
		m_points = {{40, 40}, {80, 120}, {90, 180}};
	m_selected = qBound(0, m_selected, m_points.size() - 1);
	update();
}

void FanCurvePlot::setSelected(int idx)
{
	m_selected = qBound(0, idx, m_points.size() - 1);
	update();
}

QRect FanCurvePlot::plotRect() const
{
	/* Left/bottom margins leave room for axis ticks and titles */
	return rect().adjusted(44, 12, -12, -36);
}

QPointF FanCurvePlot::toWidget(int temp, int duty) const
{
	const QRect r = plotRect();
	const qreal x = r.left() + (temp - kPlotTempMin) * qreal(r.width()) /
					   (kPlotTempMax - kPlotTempMin);
	const qreal y = r.bottom() - duty * qreal(r.height()) / 255.0;
	return {x, y};
}

FanPoint FanCurvePlot::fromWidget(const QPoint &pos) const
{
	const QRect r = plotRect();
	FanPoint pt;
	pt.temp = qBound(kPlotTempMin,
			  kPlotTempMin + int((pos.x() - r.left()) * (kPlotTempMax - kPlotTempMin) /
					     qMax(1, r.width())),
			  kPlotTempMax);
	pt.duty = qBound(0, int((r.bottom() - pos.y()) * 255.0 / qMax(1, r.height())), 255);
	return pt;
}

int FanCurvePlot::hitTest(const QPoint &pos) const
{
	for (int i = 0; i < m_points.size(); ++i) {
		if (QLineF(toWidget(m_points[i].temp, m_points[i].duty), pos).length() <= 10)
			return i;
	}
	return -1;
}

void FanCurvePlot::paintEvent(QPaintEvent *)
{
	QPainter p(this);
	p.setRenderHint(QPainter::Antialiasing, true);
	const QPalette pal = palette();
	const QColor base = pal.color(QPalette::Base);
	const QColor mid = pal.color(QPalette::Mid);
	const QColor text = pal.color(QPalette::WindowText);
	const QColor muted = pal.color(QPalette::PlaceholderText);
	const QColor accent = pal.color(QPalette::Highlight);
	const QColor danger(226, 58, 34);
	const QRect r = plotRect();

	p.fillRect(r, base);
	p.setPen(QPen(mid, 1));
	p.drawRect(r.adjusted(0, 0, -1, -1));

	QFont tickFont = font();
	tickFont.setPointSizeF(qMax(9.0, tickFont.pointSizeF() - 1));
	p.setFont(tickFont);

	/* Y grid + labels: fan speed % */
	for (int pct : {0, 25, 50, 75, 100}) {
		const int duty = percentToDuty(pct);
		const qreal y = toWidget(kPlotTempMin, duty).y();
		p.setPen(QPen(mid, 1, pct == 0 ? Qt::SolidLine : Qt::DotLine));
		p.drawLine(QPointF(r.left(), y), QPointF(r.right(), y));
		p.setPen(muted);
		p.drawText(QRectF(2, y - 8, r.left() - 6, 16), Qt::AlignRight | Qt::AlignVCenter,
			   QString::number(pct));
	}

	/* X grid + labels: temperature °C */
	for (int temp = 20; temp <= 100; temp += 20) {
		const qreal x = toWidget(temp, 0).x();
		p.setPen(QPen(mid, 1, temp == 20 ? Qt::SolidLine : Qt::DotLine));
		p.drawLine(QPointF(x, r.top()), QPointF(x, r.bottom()));
		p.setPen(muted);
		p.drawText(QRectF(x - 16, r.bottom() + 4, 32, 14), Qt::AlignHCenter | Qt::AlignTop,
			   QString::number(temp));
	}

	/* Axis titles */
	p.setPen(text);
	QFont titleFont = font();
	titleFont.setBold(true);
	p.setFont(titleFont);
	p.drawText(QRectF(r.left(), height() - 18, r.width(), 16), Qt::AlignHCenter,
		   QStringLiteral("Temperature (°C)"));
	p.save();
	p.translate(14, r.center().y());
	p.rotate(-90);
	p.drawText(QRectF(-r.height() / 2, -12, r.height(), 16), Qt::AlignCenter,
		   QStringLiteral("Fan speed (%)"));
	p.restore();

	/* Safety floor */
	const qreal sx = toWidget(kSafetyTemp, 0).x();
	const qreal sy = toWidget(0, kSafetyDuty).y();
	QColor safety = danger;
	safety.setAlpha(150);
	p.setPen(QPen(safety, 1, Qt::DashLine));
	p.drawLine(QPointF(sx, r.top()), QPointF(sx, r.bottom()));
	p.drawLine(QPointF(r.left(), sy), QPointF(r.right(), sy));
	p.setPen(danger);
	p.setFont(tickFont);
	p.drawText(QPointF(sx + 4, r.top() + 14), QStringLiteral("95°C safety floor"));

	if (m_points.size() >= 2) {
		QPainterPath path;
		path.moveTo(toWidget(m_points[0].temp, m_points[0].duty));
		for (int i = 1; i < m_points.size(); ++i)
			path.lineTo(toWidget(m_points[i].temp, m_points[i].duty));
		p.setPen(QPen(accent, 2.4));
		p.drawPath(path);
	}
	for (int i = 0; i < m_points.size(); ++i) {
		p.setBrush(i == m_selected ? accent : pal.color(QPalette::Link));
		p.setPen(QPen(base, 1.5));
		p.drawEllipse(toWidget(m_points[i].temp, m_points[i].duty), 5.5, 5.5);
	}
}

void FanCurvePlot::mousePressEvent(QMouseEvent *event)
{
	m_drag = hitTest(event->pos());
	if (m_drag >= 0) {
		m_selected = m_drag;
		emit selectedChanged(m_selected);
		update();
	}
}

void FanCurvePlot::mouseMoveEvent(QMouseEvent *event)
{
	if (m_drag < 0 || m_drag >= m_points.size())
		return;
	m_points[m_drag] = fromWidget(event->pos());
	if (m_drag > 0)
		m_points[m_drag].temp = qMax(m_points[m_drag].temp, m_points[m_drag - 1].temp + 1);
	if (m_drag + 1 < m_points.size())
		m_points[m_drag].temp = qMin(m_points[m_drag].temp, m_points[m_drag + 1].temp - 1);
	update();
	emit changed();
}

void FanCurvePlot::mouseReleaseEvent(QMouseEvent *)
{
	m_drag = -1;
}

FanCurveEditor::FanCurveEditor(const QString &title, QWidget *parent)
	: QWidget(parent)
{
	auto *root = new QVBoxLayout(this);
	root->setContentsMargins(0, 0, 0, 0);
	root->setSpacing(6);

	auto *head = new QHBoxLayout;
	auto *lab = new QLabel(title);
	lab->setObjectName("section");
	head->addWidget(lab);
	head->addStretch();
	auto *addBtn = new QPushButton("Add point");
	auto *delBtn = new QPushButton("Remove point");
	head->addWidget(addBtn);
	head->addWidget(delBtn);
	root->addLayout(head);

	auto *help = new QLabel(
		"Each point means: when the sensor hits this temperature, run the fan at this speed. "
		"Drag points on the chart, or edit the list below.");
	help->setObjectName("quiet");
	help->setWordWrap(true);
	root->addWidget(help);

	m_plot = new FanCurvePlot;
	root->addWidget(m_plot);

	m_list = new QWidget;
	auto *listLay = new QVBoxLayout(m_list);
	listLay->setContentsMargins(0, 0, 0, 0);
	listLay->setSpacing(4);
	root->addWidget(m_list);

	connect(m_plot, &FanCurvePlot::changed, this, [this] {
		rebuildControls();
		emit changed();
	});
	connect(m_plot, &FanCurvePlot::selectedChanged, this, [this](int) { rebuildControls(); });

	connect(addBtn, &QPushButton::clicked, this, [this] {
		QVector<FanPoint> pts = m_plot->points();
		FanPoint pt{70, 120};
		if (!pts.isEmpty()) {
			pt.temp = qMin(kPlotTempMax, pts.last().temp + 5);
			pt.duty = qMin(255, pts.last().duty + 20);
		}
		pts.push_back(pt);
		m_plot->setPoints(pts);
		m_plot->setSelected(pts.size() - 1);
		rebuildControls();
		emit changed();
	});
	connect(delBtn, &QPushButton::clicked, this, [this] {
		QVector<FanPoint> pts = m_plot->points();
		if (pts.size() <= 2)
			return;
		const int idx = qBound(0, m_plot->selected(), pts.size() - 1);
		pts.removeAt(idx);
		m_plot->setPoints(pts);
		m_plot->setSelected(qMin(idx, pts.size() - 1));
		rebuildControls();
		emit changed();
	});
}

void FanCurveEditor::setPoints(const QVector<FanPoint> &pts)
{
	m_plot->setPoints(pts);
	rebuildControls();
}

QVector<FanPoint> FanCurveEditor::points() const
{
	return m_plot->points();
}

void FanCurveEditor::rebuildControls()
{
	auto *lay = qobject_cast<QVBoxLayout *>(m_list->layout());
	if (!lay)
		return;
	while (QLayoutItem *it = lay->takeAt(0)) {
		delete it->widget();
		delete it;
	}
	const QVector<FanPoint> pts = m_plot->points();
	const int selected = m_plot->selected();
	for (int i = 0; i < pts.size(); ++i) {
		auto *row = new QWidget;
		row->setObjectName("row");
		auto *h = new QHBoxLayout(row);
		h->setContentsMargins(4, 4, 4, 4);
		h->setSpacing(8);

		auto *idxLab = new QLabel(QString("Point %1").arg(i + 1));
		idxLab->setObjectName("section");
		idxLab->setMinimumWidth(56);

		auto *atLab = new QLabel("At");
		atLab->setObjectName("quiet");
		auto *temp = new QSpinBox;
		temp->setRange(kPlotTempMin, kPlotTempMax);
		temp->setSuffix(" °C");
		temp->setValue(pts[i].temp);
		temp->setToolTip("Sensor temperature for this point.");

		auto *runLab = new QLabel("run fan at");
		runLab->setObjectName("quiet");
		auto *speed = new QSpinBox;
		speed->setRange(0, 100);
		speed->setSuffix(" %");
		speed->setValue(dutyToPercent(pts[i].duty));
		speed->setToolTip("How hard to spin the fan at that temperature (0% = off, 100% = full).");

		h->addWidget(idxLab);
		h->addWidget(atLab);
		h->addWidget(temp);
		h->addWidget(runLab);
		h->addWidget(speed);
		h->addStretch();
		lay->addWidget(row);

		if (i == selected) {
			QPalette pal = row->palette();
			pal.setColor(QPalette::Window, pal.color(QPalette::Highlight).lighter(180));
			row->setAutoFillBackground(true);
			row->setPalette(pal);
		}

		connect(temp, &QSpinBox::valueChanged, this, [this, i](int v) {
			QVector<FanPoint> cur = m_plot->points();
			if (i >= cur.size())
				return;
			cur[i].temp = v;
			m_plot->setSelected(i);
			m_plot->setPoints(cur);
			emit changed();
		});
		connect(speed, &QSpinBox::valueChanged, this, [this, i](int v) {
			QVector<FanPoint> cur = m_plot->points();
			if (i >= cur.size())
				return;
			cur[i].duty = percentToDuty(v);
			m_plot->setSelected(i);
			m_plot->setPoints(cur);
			emit changed();
		});
	}
}

QWidget *MainWindow::pageLights()
{
	auto *page = new QWidget;
	page->setObjectName("palm");
	auto *lay = new QVBoxLayout(page);
	lay->setContentsMargins(18, 16, 18, 16);

	const QString kdir = ledDir("multicolor:kbd_backlight");
	m_kbdHue = mkSlider(359, true);
	m_kbdSat = mkSlider(255);
	m_kbdBright = mkSlider(4);
	if (!kdir.isEmpty()) {
		QColor c;
		int b = 0, m = 4;
		if (readLed(kdir, &c, &b, &m)) {
			m_kbdHue->setValue(qMax(0, c.hue()));
			m_kbdSat->setValue(c.saturation());
			m_kbdBright->setRange(0, m);
			m_kbdBright->setValue(b);
		}
		auto apply = [this] { applyKeys(); };
		connect(m_kbdHue, &QSlider::valueChanged, this, apply);
		connect(m_kbdSat, &QSlider::valueChanged, this, apply);
		connect(m_kbdBright, &QSlider::valueChanged, this, apply);
	} else {
		m_kbdHue->setEnabled(false);
		m_kbdSat->setEnabled(false);
		m_kbdBright->setEnabled(false);
	}

	auto *keysTitle = new QLabel("Keys");
	keysTitle->setObjectName("section");
	keysTitle->setToolTip("The backlight under the keyboard.");
	lay->addWidget(keysTitle);
	lay->addWidget(sliderBlock("Color",
				   "Hue of the keyboard lights, from red through yellow, green, and blue.",
				   m_kbdHue));
	lay->addWidget(sliderBlock("Vividness",
				   "How strong the color is. All the way left is almost grey; right is full color.",
				   m_kbdSat));
	lay->addWidget(sliderBlock("Brightness",
				   "How bright the keys are. All the way left turns the backlight off.\n"
				   "This hardware cannot do true black as a color — use brightness to turn lights off.",
				   m_kbdBright));

	const QString bdir = ledDir("multicolor:status");
	m_barHue = mkSlider(359, true);
	m_barSat = mkSlider(255);
	m_barBright = mkSlider(200);
	if (!bdir.isEmpty()) {
		QColor c;
		int b = 0, m = 200;
		if (readLed(bdir, &c, &b, &m)) {
			m_barHue->setValue(qMax(0, c.hue()));
			m_barSat->setValue(c.saturation());
			m_barBright->setRange(0, m);
			m_barBright->setValue(b);
		}
		auto apply = [this] { applyBar(); };
		connect(m_barHue, &QSlider::valueChanged, this, apply);
		connect(m_barSat, &QSlider::valueChanged, this, apply);
		connect(m_barBright, &QSlider::valueChanged, this, apply);
	} else {
		m_barHue->setEnabled(false);
		m_barSat->setEnabled(false);
		m_barBright->setEnabled(false);
	}

	auto *barTitle = new QLabel("Front strip");
	barTitle->setObjectName("section");
	barTitle->setToolTip("The LED bar on the front edge of the laptop — shown on the preview below.");
	lay->addSpacing(8);
	lay->addWidget(barTitle);
	lay->addWidget(sliderBlock("Color",
				   "Color of the front LED strip. Rainbow mode below overrides this while the laptop is awake.",
				   m_barHue));
	lay->addWidget(sliderBlock("Vividness",
				   "How strong the front-strip color is.",
				   m_barSat));
	lay->addWidget(sliderBlock("Brightness",
				   "How bright the front strip is. Left is off.",
				   m_barBright));
	lay->addWidget(toggleRow("Rainbow while awake",
				 "Cycle rainbow on the front strip while you are using the laptop.\n"
				 "The color sliders above are ignored until you turn this off.",
				 "rainbow_animation"));
	lay->addWidget(toggleRow("Pulse while charging in sleep",
				 "When the laptop is sleeping and plugged in, slowly fade the front strip.\n"
				 "Does nothing on battery. Some BIOS versions ignore this.",
				 "breathing_in_suspend"));

	m_laptop = new LaptopView;
	QColor kcol, bcol;
	int kb = 0, km = 4, bb = 0, bm = 200;
	if (readLed(ledDir("multicolor:kbd_backlight"), &kcol, &kb, &km))
		m_laptop->setKeys(kcol, kb, km);
	if (readLed(ledDir("multicolor:status"), &bcol, &bb, &bm))
		m_laptop->setBar(bcol, bb, bm);
	lay->addSpacing(12);
	auto *previewLab = new QLabel("Preview");
	previewLab->setObjectName("section");
	lay->addWidget(previewLab);
	lay->addWidget(m_laptop);
	lay->addStretch();

	auto *area = new QScrollArea;
	area->setWidgetResizable(true);
	area->setFrameShape(QFrame::NoFrame);
	area->setWidget(page);
	return area;
}

void MainWindow::applyKeys()
{
	QColor c = QColor::fromHsv(m_kbdHue->value(), m_kbdSat->value(), 255);
	if (m_laptop)
		m_laptop->setKeys(c, m_kbdBright->value(), m_kbdBright->maximum());
	if (!writeLed(ledDir("multicolor:kbd_backlight"), c, m_kbdBright->value()))
		m_status->setText("Could not change the keyboard lights. Re-run install.sh so your user can write them.");
}

void MainWindow::applyBar()
{
	QColor c = QColor::fromHsv(m_barHue->value(), m_barSat->value(), 255);
	if (m_laptop)
		m_laptop->setBar(c, m_barBright->value(), m_barBright->maximum());
	if (!writeLed(ledDir("multicolor:status"), c, m_barBright->value()))
		m_status->setText("Could not change the front strip. Re-run install.sh so your user can write it.");
}

QWidget *MainWindow::pageKeys()
{
	auto *page = new QWidget;
	page->setObjectName("palm");
	auto *lay = new QVBoxLayout(page);
	lay->setContentsMargins(18, 16, 18, 16);
	lay->addWidget(toggleRow("F-keys do shortcuts by themselves",
				 "On: F1–F12 change brightness, volume, and so on without holding Fn.\n"
				 "Off: F1–F12 type F1–F12; hold Fn for the shortcuts.",
				 "fn_lock"));
	lay->addWidget(toggleRow("Super key works",
				 "On: the Super (Windows) key opens the overview / start menu.\n"
				 "Off: that key is ignored, which helps if you hit it by accident while gaming.",
				 "super_key_enable"));
	lay->addWidget(toggleRow("Keyboard can disable the touchpad",
				 "On: a Fn shortcut can turn the touchpad off.\n"
				 "Off: that shortcut does nothing, so the pad cannot vanish on you.",
				 "touchpad_toggle_enable"));
	lay->addStretch();
	return page;
}

QWidget *MainWindow::pageProfiles()
{
	auto *page = new QWidget;
	page->setObjectName("palm");
	auto *lay = new QVBoxLayout(page);
	lay->setContentsMargins(18, 16, 18, 16);
	lay->setSpacing(10);

	auto *modeRow = new QHBoxLayout;
	auto *modeLab = new QLabel("Mode");
	modeLab->setObjectName("section");
	m_profileCombo = new QComboBox;
	for (const Profile &p : m_store.profiles())
		m_profileCombo->addItem(p.title, p.id);
	modeRow->addWidget(modeLab);
	modeRow->addWidget(m_profileCombo, 1);
	lay->addLayout(modeRow);

	auto *tele = new QHBoxLayout;
	auto teleBox = [this](QLabel **lab, const QString &name) {
		auto *box = new QWidget;
		box->setObjectName("chip");
		auto *v = new QVBoxLayout(box);
		v->setContentsMargins(8, 6, 8, 6);
		v->setSpacing(2);
		auto *n = new QLabel(name);
		n->setObjectName("quiet");
		*lab = new QLabel("—");
		(*lab)->setObjectName("teleVal");
		v->addWidget(n);
		v->addWidget(*lab);
		return box;
	};
	tele->addWidget(teleBox(&m_teleCpu, "CPU"));
	tele->addWidget(teleBox(&m_teleGpu, "GPU"));
	tele->addWidget(teleBox(&m_teleFans, "Fans"));
	tele->addWidget(teleBox(&m_telePower, "Power"));
	lay->addLayout(tele);

	auto *plRow = new QHBoxLayout;
	auto mkPl = [](const QString &name, QSpinBox **spin) {
		auto *box = new QWidget;
		auto *v = new QVBoxLayout(box);
		v->setContentsMargins(0, 0, 0, 0);
		auto *n = new QLabel(name);
		n->setObjectName("quiet");
		*spin = new QSpinBox;
		(*spin)->setRange(15, 200);
		(*spin)->setSuffix(" W");
		v->addWidget(n);
		v->addWidget(*spin);
		return box;
	};
	plRow->addWidget(mkPl("PL1", &m_pl1));
	plRow->addWidget(mkPl("PL2", &m_pl2));
	plRow->addWidget(mkPl("PL4", &m_pl4));
	auto *ctgpBox = new QWidget;
	auto *ctgpLay = new QVBoxLayout(ctgpBox);
	ctgpLay->setContentsMargins(0, 0, 0, 0);
	auto *ctgpLab = new QLabel("cTGP");
	ctgpLab->setObjectName("quiet");
	m_ctgp = new QSpinBox;
	m_ctgp->setRange(0, 40);
	m_ctgp->setSuffix(" W");
	ctgpLay->addWidget(ctgpLab);
	ctgpLay->addWidget(m_ctgp);
	plRow->addWidget(ctgpBox);
	lay->addLayout(plRow);

	m_cpuCurve = new FanCurveEditor("CPU fan curve");
	m_gpuCurve = new FanCurveEditor("GPU fan curve");
	lay->addWidget(m_cpuCurve);
	lay->addWidget(m_gpuCurve);

	auto *actions = new QHBoxLayout;
	auto *applyBtn = new QPushButton("Apply now");
	applyBtn->setObjectName("primary");
	auto *resetBtn = new QPushButton("Reset to defaults");
	auto *exportBtn = new QPushButton("Export");
	auto *importBtn = new QPushButton("Import");
	actions->addWidget(applyBtn);
	actions->addWidget(resetBtn);
	actions->addStretch();
	actions->addWidget(exportBtn);
	actions->addWidget(importBtn);
	lay->addLayout(actions);

	auto *hint = new QLabel(
		"Flat CoolerControl PWM overrides curves until pwm_enable is set back to auto. "
		"The kernel keeps ≥30% duty above 95°C.");
	hint->setObjectName("quiet");
	hint->setWordWrap(true);
	lay->addWidget(hint);

	connect(applyBtn, &QPushButton::clicked, this, [this] { applySelectedProfile(true); });
	connect(resetBtn, &QPushButton::clicked, this, &MainWindow::resetSelectedProfile);
	connect(exportBtn, &QPushButton::clicked, this, &MainWindow::exportProfiles);
	connect(importBtn, &QPushButton::clicked, this, &MainWindow::importProfiles);
	auto markDirty = [this] {
		if (!m_uiLoading)
			saveUiToProfile();
	};
	connect(m_pl1, &QSpinBox::valueChanged, this, markDirty);
	connect(m_pl2, &QSpinBox::valueChanged, this, markDirty);
	connect(m_pl4, &QSpinBox::valueChanged, this, markDirty);
	connect(m_ctgp, &QSpinBox::valueChanged, this, markDirty);
	connect(m_cpuCurve, &FanCurveEditor::changed, this, markDirty);
	connect(m_gpuCurve, &FanCurveEditor::changed, this, markDirty);

	QString mode = sysRead(platformAttr("performance_mode"));
	if (mode.isEmpty())
		mode = sysRead("/sys/firmware/acpi/platform_profile");
	const QString id = ProfileStore::modeToProfileId(mode);
	const int idx = m_profileCombo->findData(id);
	m_profileCombo->setCurrentIndex(idx >= 0 ? idx : 1);
	loadProfileToUi(m_profileCombo->currentData().toString());
	connect(m_profileCombo, &QComboBox::currentIndexChanged, this, &MainWindow::onProfileSelected);

	auto *area = new QScrollArea;
	area->setWidgetResizable(true);
	area->setFrameShape(QFrame::NoFrame);
	area->setWidget(page);
	return area;
}

void MainWindow::loadProfileToUi(const QString &id)
{
	const Profile *p = m_store.find(id);
	if (!p || !m_pl1)
		return;
	m_uiLoading = true;
	m_pl1->setValue(p->pl1);
	m_pl2->setValue(p->pl2);
	m_pl4->setValue(p->pl4);
	m_ctgp->setValue(p->ctgp);
	m_cpuCurve->setPoints(p->cpuCurve);
	m_gpuCurve->setPoints(p->gpuCurve);
	m_uiLoading = false;
}

void MainWindow::saveUiToProfile()
{
	if (!m_profileCombo || !m_pl1)
		return;
	Profile p;
	p.id = m_profileCombo->currentData().toString();
	if (const Profile *cur = m_store.find(p.id))
		p.title = cur->title;
	p.pl1 = m_pl1->value();
	p.pl2 = m_pl2->value();
	p.pl4 = m_pl4->value();
	p.ctgp = m_ctgp->value();
	p.cpuCurve = m_cpuCurve->points();
	p.gpuCurve = m_gpuCurve->points();
	m_store.setProfile(p);
	m_store.save();
}

void MainWindow::onProfileSelected(int)
{
	if (!m_profileCombo)
		return;
	/* Edits to the previous profile were already persisted by markDirty. */
	const QString id = m_profileCombo->currentData().toString();
	loadProfileToUi(id);
	const QString modePath = platformAttr("performance_mode");
	if (!modePath.isEmpty()) {
		if (!sysWrite(modePath, ProfileStore::profileIdToMode(id)))
			m_status->setText("Could not switch performance mode.");
		else
			m_status->setText(QString("Switched to %1 — OSD applies PL and fan curves.")
						  .arg(m_profileCombo->currentText()));
	}
}

void MainWindow::applySelectedProfile(bool switchMode)
{
	saveUiToProfile();
	const QString id = m_profileCombo->currentData().toString();
	if (switchMode) {
		const QString modePath = platformAttr("performance_mode");
		if (!modePath.isEmpty())
			sysWrite(modePath, ProfileStore::profileIdToMode(id));
	}
	QString err;
	if (!m_store.apply(id, &err))
		m_status->setText(err.isEmpty() ? "Could not apply profile." : err);
	else
		m_status->setText(QString("Applied %1 package.").arg(m_profileCombo->currentText()));
}

void MainWindow::resetSelectedProfile()
{
	const QString id = m_profileCombo->currentData().toString();
	m_store.reset(id);
	m_store.save();
	loadProfileToUi(id);
	m_status->setText("Profile reset to defaults.");
}

void MainWindow::exportProfiles()
{
	saveUiToProfile();
	const QString path = QFileDialog::getSaveFileName(this, "Export profiles",
							  QDir::homePath() + "/uniwill-profiles.json",
							  "JSON (*.json)");
	if (path.isEmpty())
		return;
	if (!m_store.exportTo(path))
		m_status->setText("Export failed.");
	else
		m_status->setText("Exported profiles.");
}

void MainWindow::importProfiles()
{
	const QString path = QFileDialog::getOpenFileName(this, "Import profiles", QDir::homePath(),
							  "JSON (*.json)");
	if (path.isEmpty())
		return;
	if (!m_store.importFrom(path)) {
		m_status->setText("Import failed.");
		return;
	}
	loadProfileToUi(m_profileCombo->currentData().toString());
	m_status->setText("Imported profiles.");
}

void MainWindow::refreshTelemetry()
{
	if (m_modeChip) {
		QString mode = sysRead(platformAttr("performance_mode"));
		if (mode.isEmpty())
			mode = sysRead("/sys/firmware/acpi/platform_profile");
		m_modeChip->setText(modeLabel(mode));
	}
	if (!m_teleCpu)
		return;
	const QString dir = hwmonDir();
	if (!dir.isEmpty()) {
		m_teleCpu->setText(QString::number(sysRead(dir + "/temp1_input").toInt() / 1000) + "°C");
		m_teleGpu->setText(QString::number(sysRead(dir + "/temp2_input").toInt() / 1000) + "°C");
		m_teleFans->setText(QString("%1 / %2 rpm")
					    .arg(sysRead(dir + "/fan1_input"))
					    .arg(sysRead(dir + "/fan2_input")));
	}

	QString power;
	const QString rapl =
		"/sys/class/powercap/intel-rapl:0/constraint_0_power_limit_uw";
	const QString energy = "/sys/class/powercap/intel-rapl:0/energy_uj";
	if (QFile::exists(rapl))
		power = QString("PL %1 W").arg(sysRead(rapl).toLongLong() / 1000000);
	const QString pl1 = platformAttr("pl1_watt");
	if (!pl1.isEmpty()) {
		if (!power.isEmpty())
			power += " · ";
		power += QString("EC %1 W").arg(sysRead(pl1));
	}
	QProcess nvsmi;
	nvsmi.start("nvidia-smi",
		    {"--query-gpu=power.draw", "--format=csv,noheader,nounits"});
	if (nvsmi.waitForFinished(300) && nvsmi.exitCode() == 0) {
		const QString draw = QString::fromUtf8(nvsmi.readAllStandardOutput()).trimmed();
		if (!draw.isEmpty()) {
			if (!power.isEmpty())
				power += " · ";
			power += "GPU " + draw + " W";
		}
	}
	Q_UNUSED(energy);
	m_telePower->setText(power.isEmpty() ? "—" : power);
}

QWidget *MainWindow::pagePower()
{
	auto *page = new QWidget;
	page->setObjectName("palm");
	auto *lay = new QVBoxLayout(page);
	lay->setContentsMargins(18, 16, 18, 16);

	const QString prio = platformAttr("usb_c_power_priority");
	if (!prio.isEmpty()) {
		auto *lab = new QLabel("On USB-C power");
		lab->setObjectName("section");
		const QString tip = "When the charger is USB-C, the laptop splits power between the battery and the CPU.\n"
				    "Fill the battery first: charging wins, the CPU may slow down.\n"
				    "Feed the CPU first: the processor stays faster, the battery fills more slowly.";
		lab->setToolTip(tip);
		auto *box = new QComboBox;
		box->setToolTip(tip);
		box->addItem("Fill the battery first", "charging");
		box->addItem("Feed the CPU first", "performance");
		const bool perf = sysRead(prio).contains("performance");
		box->setCurrentIndex(perf ? 1 : 0);
		connect(box, &QComboBox::currentIndexChanged, this, [this, prio, box](int i) {
			if (!sysWrite(prio, box->itemData(i).toString()))
				m_status->setText("Could not change USB-C power split.");
		});
		lay->addWidget(lab);
		lay->addWidget(box);
	}

	const QString ctgp = platformAttr("ctgp_offset");
	if (!ctgp.isEmpty()) {
		auto *lab = new QLabel("Extra GPU power");
		lab->setObjectName("section");
		m_ctgpHint = new QLabel;
		m_ctgpHint->setObjectName("quiet");
		m_ctgpHint->setWordWrap(true);
		m_ctgpHint->setText(
			QString("Current cTGP offset: %1 W. Edit this per mode on the Profiles tab — "
				"the OSD reapplies the active profile's value when you switch modes.")
				.arg(sysRead(ctgp)));
		lay->addSpacing(8);
		lay->addWidget(lab);
		lay->addWidget(m_ctgpHint);
	}

	lay->addSpacing(8);
	lay->addWidget(toggleRow("Turn on when you plug in the charger",
				 "The laptop powers up by itself when AC is connected.\n"
				 "Do not enable this together with “USB ports stay on when the laptop is off”.",
				 "ac_auto_boot"));
	lay->addWidget(toggleRow("USB ports stay on when the laptop is off",
				 "USB ports keep supplying power in shutdown or hibernate, so a phone can still charge.\n"
				 "Do not enable this together with “Turn on when you plug in the charger”.",
				 "usb_powershare_high"));

	if (!QFile::exists("/sys/class/power_supply/BAT0/charge_types")) {
		auto *h = new QLabel("Battery charge limit is not available on this laptop. It is left off on purpose — on some Uniwill machines that setting can damage the battery.");
		h->setObjectName("quiet");
		h->setWordWrap(true);
		h->setToolTip("This firmware does not expose charge profiles, and the kernel driver refuses the dangerous charge-limit register even in test mode.");
		lay->addSpacing(8);
		lay->addWidget(h);
	}
	lay->addStretch();
	return page;
}

QWidget *MainWindow::infoGauge(QLabel **value, const QString &name, const QString &tip)
{
	auto *box = new QWidget;
	box->setObjectName("gauge");
	box->setToolTip(tip);
	auto *v = new QVBoxLayout(box);
	v->setContentsMargins(10, 8, 10, 8);
	auto *n = new QLabel(name);
	n->setObjectName("quiet");
	n->setToolTip(tip);
	*value = new QLabel("—");
	(*value)->setObjectName("heat");
	(*value)->setWordWrap(true);
	(*value)->setToolTip(tip);
	v->addWidget(n);
	v->addWidget(*value);
	return box;
}

QWidget *MainWindow::pageInfo()
{
	auto *page = new QWidget;
	page->setObjectName("palm");
	auto *lay = new QVBoxLayout(page);
	lay->setContentsMargins(18, 16, 18, 16);
	lay->setSpacing(10);

	auto section = [&](const QString &title) {
		auto *lab = new QLabel(title);
		lab->setObjectName("section");
		lay->addWidget(lab);
	};

	section("Temperatures");
	auto *temps = new QHBoxLayout;
	temps->addWidget(infoGauge(&m_infoCpuTemp, "CPU", "Embedded-controller CPU package temperature."));
	temps->addWidget(infoGauge(&m_infoGpuTemp, "GPU (EC)", "Embedded-controller discrete GPU temperature."));
	lay->addLayout(temps);

	section("Fans");
	auto *fans = new QHBoxLayout;
	fans->addWidget(infoGauge(&m_infoFanCpu, "CPU fan", "Main fan speed from the EC."));
	fans->addWidget(infoGauge(&m_infoFanGpu, "GPU fan", "Second fan speed from the EC."));
	lay->addLayout(fans);
	auto *fanDuty = new QHBoxLayout;
	fanDuty->addWidget(infoGauge(&m_infoFanDuty, "Fan duty",
				     "How hard each fan is driven (0–100%). Edit curves on Profiles."));
	fanDuty->addWidget(infoGauge(&m_infoFanMode, "Fan control",
				     "auto = firmware/curve, manual = flat CoolerControl-style PWM."));
	lay->addLayout(fanDuty);

	section("Power limits");
	auto *power = new QHBoxLayout;
	power->addWidget(infoGauge(&m_infoPl, "EC PL1 / PL2 / PL4",
				   "Package power limits programmed in the embedded controller."));
	power->addWidget(infoGauge(&m_infoRapl, "RAPL",
				   "Intel RAPL package power limit and estimated draw when available."));
	lay->addLayout(power);
	auto *power2 = new QHBoxLayout;
	power2->addWidget(infoGauge(&m_infoCtgp, "cTGP offset",
				    "Configurable TGP offset for the NVIDIA GPU."));
	power2->addWidget(infoGauge(&m_infoMode, "Performance mode",
				    "Active Office / Balanced / Turbo / Custom package."));
	lay->addLayout(power2);

	section("Battery & AC");
	auto *batt = new QHBoxLayout;
	batt->addWidget(infoGauge(&m_infoBattery, "Battery",
				  "Charge level, status, and power when the kernel exposes it."));
	batt->addWidget(infoGauge(&m_infoAc, "AC / USB-C",
				  "Adapter online state and USB-C power priority."));
	lay->addLayout(batt);

	section("NVIDIA");
	lay->addWidget(infoGauge(&m_infoNvidia, "GPU (nvidia-smi)",
				 "Live draw, temperature, and utilization when the NVIDIA driver is loaded."));

	section("Platform");
	lay->addWidget(infoGauge(&m_infoPlatform, "Driver / sysfs",
				 "Driver presence and other platform knobs."));

	auto *hint = new QLabel("Values refresh every 1.5s. Change limits and fan curves on the Profiles tab.");
	hint->setObjectName("quiet");
	hint->setWordWrap(true);
	lay->addWidget(hint);
	lay->addStretch();

	auto *area = new QScrollArea;
	area->setWidgetResizable(true);
	area->setFrameShape(QFrame::NoFrame);
	area->setWidget(page);
	return area;
}

void MainWindow::refreshInfo()
{
	if (!m_infoCpuTemp)
		return;

	const QString hw = hwmonDir();
	if (!hw.isEmpty()) {
		m_infoCpuTemp->setText(QString::number(sysRead(hw + "/temp1_input").toInt() / 1000) +
				       " °C");
		m_infoGpuTemp->setText(QString::number(sysRead(hw + "/temp2_input").toInt() / 1000) +
				       " °C");
		m_infoFanCpu->setText(sysRead(hw + "/fan1_input") + " rpm");
		m_infoFanGpu->setText(sysRead(hw + "/fan2_input") + " rpm");
		const int p1 = sysRead(hw + "/pwm1").toInt() * 100 / 255;
		const int p2 = sysRead(hw + "/pwm2").toInt() * 100 / 255;
		m_infoFanDuty->setText(QString("CPU %1%  ·  GPU %2%").arg(p1).arg(p2));
		const QString e1 = sysRead(hw + "/pwm1_enable");
		const QString e2 = sysRead(hw + "/pwm2_enable");
		auto modeName = [](const QString &e) {
			if (e == "1")
				return QStringLiteral("manual");
			if (e == "2")
				return QStringLiteral("auto");
			return e.isEmpty() ? QStringLiteral("—") : e;
		};
		m_infoFanMode->setText(QString("CPU %1  ·  GPU %2").arg(modeName(e1), modeName(e2)));
	} else {
		m_infoCpuTemp->setText("—");
		m_infoGpuTemp->setText("—");
		m_infoFanCpu->setText("—");
		m_infoFanGpu->setText("—");
		m_infoFanDuty->setText("—");
		m_infoFanMode->setText("—");
	}

	const QString pl1 = sysRead(platformAttr("pl1_watt"));
	const QString pl2 = sysRead(platformAttr("pl2_watt"));
	const QString pl4 = sysRead(platformAttr("pl4_watt"));
	m_infoPl->setText(QString("%1 / %2 / %3 W")
				   .arg(pl1.isEmpty() ? "—" : pl1, pl2.isEmpty() ? "—" : pl2,
					pl4.isEmpty() ? "—" : pl4));

	QString rapl;
	const QString raplLimit =
		"/sys/class/powercap/intel-rapl:0/constraint_0_power_limit_uw";
	const QString raplName = "/sys/class/powercap/intel-rapl:0/constraint_0_name";
	const QString raplEnergy = "/sys/class/powercap/intel-rapl:0/energy_uj";
	if (QFile::exists(raplLimit)) {
		rapl = QString("%1 %2 W")
			       .arg(sysRead(raplName).isEmpty() ? "PL" : sysRead(raplName),
				    QString::number(sysRead(raplLimit).toLongLong() / 1000000));
		if (QFile::exists(raplEnergy)) {
			static qint64 lastEnergy = -1;
			static qint64 lastMs = -1;
			const qint64 energy = sysRead(raplEnergy).toLongLong();
			const qint64 now = QDateTime::currentMSecsSinceEpoch();
			if (lastEnergy >= 0 && lastMs >= 0 && now > lastMs && energy >= lastEnergy) {
				const double watts =
					(energy - lastEnergy) / 1e6 / ((now - lastMs) / 1000.0);
				rapl += QString("  ·  ~%1 W now").arg(watts, 0, 'f', 1);
			}
			lastEnergy = energy;
			lastMs = now;
		}
	}
	m_infoRapl->setText(rapl.isEmpty() ? "—" : rapl);

	const QString ctgp = sysRead(platformAttr("ctgp_offset"));
	m_infoCtgp->setText(ctgp.isEmpty() ? "—" : ctgp + " W");

	QString mode = sysRead(platformAttr("performance_mode"));
	if (mode.isEmpty())
		mode = sysRead("/sys/firmware/acpi/platform_profile");
	m_infoMode->setText(modeLabel(mode));

	QString batt = "—";
	const QDir psy("/sys/class/power_supply");
	for (const QString &name : psy.entryList(QDir::Dirs | QDir::NoDotAndDotDot)) {
		const QString base = psy.filePath(name);
		if (sysRead(base + "/type") != "Battery")
			continue;
		QStringList parts;
		const QString cap = sysRead(base + "/capacity");
		const QString status = sysRead(base + "/status");
		if (!cap.isEmpty())
			parts << cap + "%";
		if (!status.isEmpty())
			parts << status;
		const qint64 uw = sysRead(base + "/power_now").toLongLong();
		if (uw > 0)
			parts << QString("%1 W").arg(uw / 1000000.0, 0, 'f', 1);
		else {
			const qint64 ua = sysRead(base + "/current_now").toLongLong();
			const qint64 uv = sysRead(base + "/voltage_now").toLongLong();
			if (ua > 0 && uv > 0)
				parts << QString("%1 W").arg((ua / 1e6) * (uv / 1e6), 0, 'f', 1);
		}
		const QString end = sysRead(base + "/charge_control_end_threshold");
		if (!end.isEmpty())
			parts << "limit " + end + "%";
		batt = parts.isEmpty() ? name : parts.join("  ·  ");
		break;
	}
	m_infoBattery->setText(batt);

	QString ac = "—";
	for (const QString &name : psy.entryList(QDir::Dirs | QDir::NoDotAndDotDot)) {
		const QString base = psy.filePath(name);
		const QString type = sysRead(base + "/type");
		if (type != "Mains" && type != "USB")
			continue;
		ac = sysRead(base + "/online") == "1" ? "Plugged in" : "On battery";
		break;
	}
	const QString prio = sysRead(platformAttr("usb_c_power_priority"));
	if (!prio.isEmpty())
		ac += "  ·  USB-C " + prio;
	m_infoAc->setText(ac);

	QString nvidia = "—";
	QProcess nvsmi;
	nvsmi.start("nvidia-smi",
		    {"--query-gpu=power.draw,temperature.gpu,utilization.gpu,clocks.sm",
		     "--format=csv,noheader,nounits"});
	if (nvsmi.waitForFinished(400) && nvsmi.exitCode() == 0) {
		const QStringList cols =
			QString::fromUtf8(nvsmi.readAllStandardOutput()).trimmed().split(',');
		if (cols.size() >= 3) {
			nvidia = QString("%1 W  ·  %2 °C  ·  %3% util")
					 .arg(cols[0].trimmed(), cols[1].trimmed(),
					      cols[2].trimmed());
			if (cols.size() >= 4)
				nvidia += "  ·  " + cols[3].trimmed() + " MHz";
		}
	}
	m_infoNvidia->setText(nvidia);

	QStringList plat;
	plat << (platformDir().isEmpty() ? "driver missing" : "driver loaded");
	const QString fn = sysRead(platformAttr("fn_lock"));
	if (!fn.isEmpty())
		plat << QString("Fn lock %1").arg(fn == "1" ? "on" : "off");
	const QString curve = sysRead(platformAttr("cpu_fan_curve"));
	if (!curve.isEmpty())
		plat << (curve == "auto" ? "fan curve auto" : "fan curve active");
	m_infoPlatform->setText(plat.join("  ·  "));
}
