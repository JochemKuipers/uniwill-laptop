#include "window.hpp"
#include "sysfs.hpp"

#include <QCheckBox>
#include <QComboBox>
#include <QFile>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QPainter>
#include <QPainterPath>
#include <QScrollArea>
#include <QSlider>
#include <QSpinBox>
#include <QTabWidget>
#include <QTimer>
#include <QVBoxLayout>

static const char *kStyle = R"(
QWidget#shell {
	background: #1B1916;
	color: #E7E0D4;
	font-family: "Noto Sans";
	font-size: 13px;
}
QLabel#brand {
	color: #E23A22;
	font-family: "Fira Sans Condensed";
	font-size: 22px;
	font-weight: 800;
	letter-spacing: 0.14em;
}
QLabel#model {
	color: #9A9286;
	font-family: "Fira Sans Condensed";
	font-size: 16px;
	font-weight: 600;
}
QLabel#section {
	color: #E7E0D4;
	font-family: "Fira Sans Condensed";
	font-size: 15px;
	font-weight: 700;
}
QLabel#quiet { color: #9A9286; }
QLabel#status { color: #9A9286; font-size: 12px; }
QLabel#heat {
	color: #E7E0D4;
	font-family: "JetBrainsMono Nerd Font", "JetBrains Mono", monospace;
	font-size: 28px;
	font-weight: 500;
}
QTabWidget::pane {
	border: none;
	background: #24211D;
	top: -1px;
}
QTabBar::tab {
	background: transparent;
	color: #9A9286;
	font-family: "Fira Sans Condensed";
	font-size: 14px;
	font-weight: 700;
	padding: 10px 18px;
	border: none;
	border-bottom: 2px solid transparent;
}
QTabBar::tab:selected {
	color: #E7E0D4;
	border-bottom: 2px solid #E23A22;
}
QTabBar::tab:hover { color: #E7E0D4; }
QSlider::groove:horizontal {
	height: 5px;
	background: #3A362F;
	border-radius: 2px;
}
QSlider::handle:horizontal {
	width: 14px;
	height: 14px;
	margin: -5px 0;
	background: #E7E0D4;
	border-radius: 7px;
}
QSlider#spectrum::groove:horizontal {
	background: qlineargradient(x1:0, y1:0, x2:1, y2:0,
		stop:0 #c43c2b, stop:0.18 #d4a017, stop:0.36 #3d8c4a,
		stop:0.54 #2a7a8c, stop:0.72 #3d4a9c, stop:0.9 #8a3a7a, stop:1 #c43c2b);
}
QCheckBox { spacing: 8px; color: #E7E0D4; }
QCheckBox::indicator {
	width: 16px; height: 16px;
	border: 1px solid #6A6358;
	background: #0C0B0A;
}
QCheckBox::indicator:checked { background: #E23A22; border-color: #E23A22; }
QComboBox, QSpinBox {
	color: #E7E0D4;
	background: #0C0B0A;
	border: 1px solid #3A362F;
	padding: 6px 10px;
	min-height: 28px;
}
QComboBox QAbstractItemView {
	background: #1B1916;
	color: #E7E0D4;
	selection-background-color: #3A362F;
}
QWidget#palm { background: #24211D; }
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
	setMinimumHeight(228);
	setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
	setToolTip("This is the keyboard and the front LED strip on the laptop.\n"
		   "It updates as you change the Lights tab.");
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

	const QRectF shell = QRectF(rect()).adjusted(18, 6, -18, 4);
	p.setPen(Qt::NoPen);
	p.setBrush(QColor("#0C0B0A"));
	p.drawRoundedRect(shell, 12, 12);

	const QColor keyFill(m_keys.red(), m_keys.green(), m_keys.blue(),
			     int(24 + 110 * m_keyLevel));
	const QColor keyEdge(m_keys.red(), m_keys.green(), m_keys.blue(),
			     int(70 + 150 * m_keyLevel));

	static const QVector<QVector<qreal>> rows = {
		{1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2},
		{1.5, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1.5},
		{1.75, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2.25},
		{2.25, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2.75},
		{1.25, 1.25, 1.25, 6.5, 1.25, 1.25, 1.25, 1.25},
	};

	const qreal gap = 3.2;
	const qreal left = shell.left() + 16;
	const qreal usable = shell.width() - 32;
	const qreal barH = 9;
	const qreal keyArea = shell.height() - 28 - barH;
	const qreal rowH = (keyArea - gap * (rows.size() - 1)) / rows.size();
	qreal y = shell.top() + 12;

	for (const auto &row : rows) {
		qreal units = 0;
		for (qreal u : row)
			units += u;
		qreal x = left;
		for (qreal u : row) {
			const qreal w = usable * (u / units) - gap;
			QPainterPath path;
			path.addRoundedRect(QRectF(x, y, w, rowH), 2.8, 2.8);
			p.fillPath(path, keyFill);
			p.setPen(QPen(keyEdge, 1));
			p.drawPath(path);
			x += w + gap;
		}
		y += rowH + gap;
	}

	const QRectF bar(left, shell.bottom() - 14 - barH, usable - gap, barH);
	const QColor barFill(m_bar.red(), m_bar.green(), m_bar.blue(),
			     int(30 + 180 * m_barLevel));
	p.setPen(Qt::NoPen);
	p.setBrush(barFill);
	p.drawRoundedRect(bar, 4, 4);
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
	auto *lay = new QHBoxLayout(box);
	lay->setContentsMargins(0, 6, 0, 6);
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

MainWindow::MainWindow(QWidget *parent)
	: QWidget(parent)
{
	setObjectName("shell");
	setWindowTitle("ERAZER Major 15 X1");
	resize(860, 720);
	setStyleSheet(kStyle);

	auto *brand = new QLabel("ERAZER");
	brand->setObjectName("brand");
	auto *model = new QLabel("Major 15 X1");
	model->setObjectName("model");
	auto *head = new QHBoxLayout;
	head->addWidget(brand);
	head->addStretch();
	head->addWidget(model);

	m_laptop = new LaptopView;
	QColor kcol, bcol;
	int kb = 0, km = 4, bb = 0, bm = 200;
	if (readLed(ledDir("multicolor:kbd_backlight"), &kcol, &kb, &km))
		m_laptop->setKeys(kcol, kb, km);
	if (readLed(ledDir("multicolor:status"), &bcol, &bb, &bm))
		m_laptop->setBar(bcol, bb, bm);

	auto *tabs = new QTabWidget;
	tabs->setDocumentMode(true);
	tabs->tabBar()->setExpanding(true);
	tabs->addTab(pageLights(), "Lights");
	tabs->addTab(pageKeys(), "Keys");
	tabs->addTab(pagePower(), "Power");
	tabs->addTab(pageHeat(), "Heat");
	tabs->setTabToolTip(0, "Keyboard backlight and the LED strip on the front of the laptop.");
	tabs->setTabToolTip(1, "Fn lock, Super key, and the shortcut that disables the touchpad.");
	tabs->setTabToolTip(2, "USB-C power split, GPU watts, and what happens when you plug in or shut down.");
	tabs->setTabToolTip(3, "CPU/GPU temperature and how hard the fans are working. Read-only.");

	m_status = new QLabel;
	m_status->setObjectName("status");
	m_status->setWordWrap(true);
	if (platformDir().isEmpty())
		m_status->setText("The laptop driver is not loaded. Run install.sh in this repo, then open this app again.");
	else
		m_status->setText("Hover any control for what it actually does.");

	auto *root = new QVBoxLayout(this);
	root->setContentsMargins(22, 16, 22, 12);
	root->setSpacing(12);
	root->addLayout(head);
	root->addWidget(m_laptop);
	root->addWidget(tabs, 1);
	root->addWidget(m_status);

	auto *timer = new QTimer(this);
	connect(timer, &QTimer::timeout, this, &MainWindow::refreshHeat);
	timer->start(1500);
	refreshHeat();
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
	barTitle->setToolTip("The LED bar on the front edge of the laptop, under the keyboard in the picture above.");
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
	lay->addStretch();
	auto *area = new QScrollArea;
	area->setWidgetResizable(true);
	area->setFrameShape(QFrame::NoFrame);
	area->setWidget(page);
	area->setStyleSheet("QScrollArea { background: #24211D; border: none; }");
	return area;
}

void MainWindow::applyKeys()
{
	QColor c = QColor::fromHsv(m_kbdHue->value(), m_kbdSat->value(), 255);
	m_laptop->setKeys(c, m_kbdBright->value(), m_kbdBright->maximum());
	if (!writeLed(ledDir("multicolor:kbd_backlight"), c, m_kbdBright->value()))
		m_status->setText("Could not change the keyboard lights. Re-run install.sh so your user can write them.");
}

void MainWindow::applyBar()
{
	QColor c = QColor::fromHsv(m_barHue->value(), m_barSat->value(), 255);
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
		const QString tip = "Adds watts on top of the GPU's base power limit (cTGP).\n"
				    "Higher can mean a faster NVIDIA GPU and more heat and fan noise.\n"
				    "0 is the factory base. Maxing this out also leaves no headroom for Dynamic Boost.";
		lab->setToolTip(tip);
		auto *spin = new QSpinBox;
		spin->setToolTip(tip);
		spin->setSuffix(" W");
		spin->setRange(0, 40);
		spin->setValue(sysRead(ctgp).toInt());
		connect(spin, &QSpinBox::valueChanged, this, [this, ctgp](int v) {
			if (!sysWrite(ctgp, QString::number(v)))
				m_status->setText("Could not change extra GPU power.");
		});
		lay->addSpacing(8);
		lay->addWidget(lab);
		lay->addWidget(spin);
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

QWidget *MainWindow::pageHeat()
{
	auto *page = new QWidget;
	page->setObjectName("palm");
	auto *lay = new QVBoxLayout(page);
	lay->setContentsMargins(18, 16, 18, 16);

	auto metric = [](QLabel **value, const QString &name, const QString &tip) {
		auto *box = new QWidget;
		box->setToolTip(tip);
		auto *v = new QVBoxLayout(box);
		auto *n = new QLabel(name);
		n->setObjectName("quiet");
		n->setToolTip(tip);
		*value = new QLabel("—");
		(*value)->setObjectName("heat");
		(*value)->setToolTip(tip);
		v->addWidget(n);
		v->addWidget(*value);
		return box;
	};

	auto *row = new QHBoxLayout;
	row->addWidget(metric(&m_cpu, "CPU", "Package temperature from the embedded controller."));
	row->addWidget(metric(&m_gpu, "GPU", "Discrete GPU temperature from the embedded controller."));
	lay->addLayout(row);
	auto *row2 = new QHBoxLayout;
	row2->addWidget(metric(&m_fan1, "CPU fan", "Main fan speed in revolutions per minute. Firmware chooses the speed."));
	row2->addWidget(metric(&m_fan2, "GPU fan", "Second fan speed, usually the GPU side. Firmware chooses the speed."));
	lay->addLayout(row2);
	auto *n = new QLabel("Fan effort");
	n->setObjectName("quiet");
	n->setToolTip("How hard the firmware is driving each fan, 0–100%. This app cannot set a fan curve yet.");
	m_pwm = new QLabel("—");
	m_pwm->setObjectName("heat");
	m_pwm->setToolTip(n->toolTip());
	lay->addWidget(n);
	lay->addWidget(m_pwm);
	auto *h = new QLabel("Fans are automatic. There is no safe manual speed control in this driver yet.");
	h->setObjectName("quiet");
	h->setWordWrap(true);
	lay->addWidget(h);
	lay->addStretch();
	return page;
}

void MainWindow::refreshHeat()
{
	if (!m_cpu)
		return;
	const QString dir = hwmonDir();
	if (dir.isEmpty()) {
		m_cpu->setText("—");
		return;
	}
	m_cpu->setText(QString::number(sysRead(dir + "/temp1_input").toInt() / 1000) + "°");
	m_gpu->setText(QString::number(sysRead(dir + "/temp2_input").toInt() / 1000) + "°");
	m_fan1->setText(sysRead(dir + "/fan1_input") + " rpm");
	m_fan2->setText(sysRead(dir + "/fan2_input") + " rpm");
	const int p1 = sysRead(dir + "/pwm1").toInt() * 100 / 255;
	const int p2 = sysRead(dir + "/pwm2").toInt() * 100 / 255;
	m_pwm->setText(QString("%1%  ·  %2%").arg(p1).arg(p2));
}
