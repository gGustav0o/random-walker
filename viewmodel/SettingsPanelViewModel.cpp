#include "SettingsPanelViewModel.hpp"

double SettingsPanelViewModel::sigma() const
{
	return sigma_;
}

void SettingsPanelViewModel::set_sigma(double value)
{
	if (!qFuzzyCompare(sigma_, value)) {
		sigma_ = value;
		emit sigma_changed();
	}
}

bool SettingsPanelViewModel::is_visible() const
{
	return visible_;
}

void SettingsPanelViewModel::set_visible(bool value)
{
	if(visible_ != value) {
		visible_ = value;
		emit visible_changed();
	}
}


