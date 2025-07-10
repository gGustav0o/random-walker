#pragma once

#include <QAbstractListModel>
#include <QVector>

#include "app/data/Rect.hpp"

class SeedsListModel : public QAbstractListModel
{
	Q_OBJECT
public:
	enum Roles {
		TopLeftRole = Qt::UserRole + 1
		, BottomRightRole
	};
	explicit SeedsListModel(QObject *parent = nullptr)
		: QAbstractListModel(parent) {}

	int rowCount(const QModelIndex& parent = QModelIndex()) const override;
	QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
	QHash<int, QByteArray> roleNames() const override;
	Q_INVOKABLE void addSeedRect(QPoint top_left, QPoint bottom_right);
	Q_INVOKABLE void removeSeedRect(int index);
	Q_INVOKABLE void clear();
private:
	QVector<SeedRect> seeds_;
};