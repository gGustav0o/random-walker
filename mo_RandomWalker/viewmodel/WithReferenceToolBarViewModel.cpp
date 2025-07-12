#include "WithReferenceToolBarViewModel.hpp"

Q_INVOKABLE void WithReferenceToolBarViewModel::on_load_reference()
{
	move_to_next_step();
}

Q_INVOKABLE void WithReferenceToolBarViewModel::on_align()
{
	move_to_next_step();
}

Q_INVOKABLE void WithReferenceToolBarViewModel::on_preprocess()
{
	move_to_next_step();
}

Q_INVOKABLE void WithReferenceToolBarViewModel::on_select_roi()
{
	move_to_next_step();
}

Q_INVOKABLE void WithReferenceToolBarViewModel::on_abs_diff()
{
	move_to_next_step();
}

Q_INVOKABLE void WithReferenceToolBarViewModel::on_segment()
{
	move_to_next_step();
}

Q_INVOKABLE void WithReferenceToolBarViewModel::on_morphology()
{
	move_to_next_step();
}

Q_INVOKABLE void WithReferenceToolBarViewModel::on_cleanup() {
	move_to_next_step();
}

Q_INVOKABLE void WithReferenceToolBarViewModel::on_contour_analysis() {
	move_to_next_step();
}

Q_INVOKABLE void WithReferenceToolBarViewModel::on_features() {
	move_to_next_step();
}

Q_INVOKABLE void WithReferenceToolBarViewModel::on_compare() {
	move_to_next_step();
}

Q_INVOKABLE void WithReferenceToolBarViewModel::on_classify() {
	move_to_next_step();
}

void WithReferenceToolBarViewModel::move_to_next_step() {
	if (current_step_ + 1 < StepCount) {
		++current_step_;
		emit current_step_changed();
	}
}
