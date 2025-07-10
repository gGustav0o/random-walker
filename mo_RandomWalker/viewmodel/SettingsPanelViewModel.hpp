#pragma once

#include <QObject>

class SettingsPanelViewModel : public QObject
{
	Q_OBJECT
	Q_PROPERTY(double sigma READ sigma WRITE set_sigma NOTIFY sigma_changed)
	Q_PROPERTY(bool visible READ is_visible WRITE set_visible NOTIFY visible_changed)
public:
	explicit SettingsPanelViewModel(QObject *parent = nullptr)
		: QObject(parent){}
	
	double sigma() const;
	void set_sigma(double value);

	bool is_visible() const;
	void set_visible(bool value);
signals:
	void sigma_changed();
	void visible_changed();
private:
	double sigma_ = 1.0;
	bool visible_ = false;
};