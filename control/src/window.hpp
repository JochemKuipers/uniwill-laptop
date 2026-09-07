#pragma once

#include <QColor>
#include <QWidget>

class QLabel;
class QSlider;
class QTabWidget;

class LaptopView : public QWidget
{
	Q_OBJECT
public:
	explicit LaptopView(QWidget *parent = nullptr);
	void setKeys(const QColor &color, int brightness, int maxBrightness);
	void setBar(const QColor &color, int brightness, int maxBrightness);

protected:
	void paintEvent(QPaintEvent *event) override;

private:
	QColor m_keys{226, 58, 34};
	QColor m_bar{226, 58, 34};
	qreal m_keyLevel = 0.7;
	qreal m_barLevel = 0.7;
};

class MainWindow : public QWidget
{
	Q_OBJECT
public:
	explicit MainWindow(QWidget *parent = nullptr);

private:
	QWidget *pageLights();
	QWidget *pageKeys();
	QWidget *pagePower();
	QWidget *pageHeat();
	QWidget *toggleRow(const QString &title, const QString &tip, const QString &attr);
	QWidget *sliderBlock(const QString &title, const QString &tip, QSlider *slider);
	void applyKeys();
	void applyBar();
	void refreshHeat();

	LaptopView *m_laptop{};
	QSlider *m_kbdHue{};
	QSlider *m_kbdSat{};
	QSlider *m_kbdBright{};
	QSlider *m_barHue{};
	QSlider *m_barSat{};
	QSlider *m_barBright{};
	QLabel *m_cpu{};
	QLabel *m_gpu{};
	QLabel *m_fan1{};
	QLabel *m_fan2{};
	QLabel *m_pwm{};
	QLabel *m_status{};
};
