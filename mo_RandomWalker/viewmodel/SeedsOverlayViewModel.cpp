#include "SeedsOverlayViewModel.hpp"

SeedsOverlayViewModel::SeedsOverlayViewModel(QObject* parent)
	: QObject(parent)
	, object_seeds_(new SeedsListModel(this))
	, background_seeds_(new SeedsListModel(this)) {}

SeedsListModel* SeedsOverlayViewModel::object_seeds() const {
	return object_seeds_;
}

SeedsListModel* SeedsOverlayViewModel::background_seeds() const {
	return background_seeds_;
}