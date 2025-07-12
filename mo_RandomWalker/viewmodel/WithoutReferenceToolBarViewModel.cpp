#include "WithoutReferenceToolBarViewModel.hpp"

Q_INVOKABLE void WithoutReferenceToolBarViewModel::on_preprocess()
{
	move_to_next_step();
}

Q_INVOKABLE void WithoutReferenceToolBarViewModel::on_select_roi()
{
	move_to_next_step();
}

Q_INVOKABLE void WithoutReferenceToolBarViewModel::on_segment() {
	move_to_next_step();
}

Q_INVOKABLE void WithoutReferenceToolBarViewModel::on_morphology() {
	move_to_next_step();
}

Q_INVOKABLE void WithoutReferenceToolBarViewModel::on_cleanup() {
	move_to_next_step();
}

Q_INVOKABLE void WithoutReferenceToolBarViewModel::on_contour_analysis() {
	move_to_next_step();
}

Q_INVOKABLE void WithoutReferenceToolBarViewModel::on_features() {
	move_to_next_step();
}

Q_INVOKABLE void WithoutReferenceToolBarViewModel::on_compare() {
	move_to_next_step();
}

Q_INVOKABLE void WithoutReferenceToolBarViewModel::on_classify() {
	move_to_next_step();
}

void WithoutReferenceToolBarViewModel::move_to_next_step() {
	if (current_step_ + 1 < StepCount) {
		++current_step_;
		emit current_step_changed();
	}
}