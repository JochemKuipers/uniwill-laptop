#pragma once

#include "profiles.hpp"

#include <QColor>
#include <QVector>
#include <QWidget>

class QComboBox;
class QLabel;
class QSlider;
class QSpinBox;

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

class FanCurvePlot : public QWidget
{
	Q_OBJECT
public:
	explicit FanCurvePlot(QWidget *parent = nullptr);

	void setPoints(const QVector<FanPoint> &pts);
	QVector<FanPoint> points() const { return m_points; }
	void setSelected(int idx);
	int selected() const { return m_selected; }

signals:
	void changed();
	void selectedChanged(int idx);

protected:
	void paintEvent(QPaintEvent *event) override;
	void mousePressEvent(QMouseEvent *event) override;
	void mouseMoveEvent(QMouseEvent *event) override;
	void mouseReleaseEvent(QMouseEvent *event) override;

private:
	QRect plotRect() const;
	QPointF toWidget(int temp, int duty) const;
	FanPoint fromWidget(const QPoint &pos) const;
	int hitTest(const QPoint &pos) const;

	QVector<FanPoint> m_points;
	int m_drag = -1;
	int m_selected = 0;
};

class FanCurveEditor : public QWidget
{
	Q_OBJECT
public:
	explicit FanCurveEditor(const QString &title, QWidget *parent = nullptr);

	void setPoints(const QVector<FanPoint> &pts);
	QVector<FanPoint> points() const;

signals:
	void changed();

private:
	void rebuildControls();

	FanCurvePlot *m_plot{};
	QWidget *m_list{};
};

class MainWindow : public QWidget
{
	Q_OBJECT
public:
	explicit MainWindow(QWidget *parent = nullptr);

private:
	QWidget *pageLights();
	QWidget *pageKeys();
	QWidget *pageProfiles();
	QWidget *pagePower();
	QWidget *pageInfo();
	QWidget *toggleRow(const QString &title, const QString &tip, const QString &attr);
	QWidget *sliderBlock(const QString &title, const QString &tip, QSlider *slider);
	QWidget *infoGauge(QLabel **value, const QString &name, const QString &tip);
	void applyKeys();
	void applyBar();
	void refreshInfo();
	void refreshTelemetry();
	void loadProfileToUi(const QString &id);
	void saveUiToProfile();
	void onProfileSelected(int index);
	void applySelectedProfile(bool switchMode);
	void resetSelectedProfile();
	void exportProfiles();
	void importProfiles();

	LaptopView *m_laptop{};
	QSlider *m_kbdHue{};
	QSlider *m_kbdSat{};
	QSlider *m_kbdBright{};
	QSlider *m_barHue{};
	QSlider *m_barSat{};
	QSlider *m_barBright{};
	QLabel *m_status{};
	QLabel *m_modeChip{};
	QLabel *m_teleCpu{};
	QLabel *m_teleGpu{};
	QLabel *m_teleFans{};
	QLabel *m_telePower{};
	QLabel *m_ctgpHint{};

	/* Info page */
	QLabel *m_infoCpuTemp{};
	QLabel *m_infoGpuTemp{};
	QLabel *m_infoFanCpu{};
	QLabel *m_infoFanGpu{};
	QLabel *m_infoFanDuty{};
	QLabel *m_infoFanMode{};
	QLabel *m_infoPl{};
	QLabel *m_infoRapl{};
	QLabel *m_infoCtgp{};
	QLabel *m_infoMode{};
	QLabel *m_infoBattery{};
	QLabel *m_infoAc{};
	QLabel *m_infoNvidia{};
	QLabel *m_infoPlatform{};

	ProfileStore m_store;
	QComboBox *m_profileCombo{};
	QSpinBox *m_pl1{};
	QSpinBox *m_pl2{};
	QSpinBox *m_pl4{};
	QSpinBox *m_ctgp{};
	FanCurveEditor *m_cpuCurve{};
	FanCurveEditor *m_gpuCurve{};
	bool m_uiLoading = false;
};
