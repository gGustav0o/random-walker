#pragma once

#include <vector>

#include <QObject>

#include "app/data/Rect.hpp"
#include "viewmodel/SeedsListModel.hpp"

class SeedsOverlayViewModel : public QObject
{
	Q_OBJECT
	Q_PROPERTY(SeedsListModel* object_seeds_ READ object_seeds CONSTANT)
	Q_PROPERTY(SeedsListModel* background_seeds_ READ background_seeds CONSTANT)
public:
	explicit SeedsOverlayViewModel(QObject* parent = nullptr);
	SeedsListModel* object_seeds() const;
	SeedsListModel* background_seeds() const;
private:
	SeedsListModel* object_seeds_;
	SeedsListModel* background_seeds_;
};
