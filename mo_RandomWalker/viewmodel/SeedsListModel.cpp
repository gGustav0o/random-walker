#include "SeedsListModel.hpp"

int SeedsListModel::rowCount(const QModelIndex& parent) const
{
	Q_UNUSED(parent);
	return seeds_.size();
}

QVariant SeedsListModel::data(const QModelIndex& index, int role) const
{
	if (!index.isValid() || index.row() >= seeds_.size()) {
		return QVariant();
	}

	const SeedRect& seed_rect = seeds_[index.row()];
	switch (role)
	{
	case TopLeftRole:
		return QVariant::fromValue(seed_rect.top_left_);
	case BottomRightRole:
		return QVariant::fromValue(seed_rect.bottom_right_);
	default:
		return QVariant();
	}
}

QHash<int, QByteArray> SeedsListModel::roleNames() const
{
	QHash<int, QByteArray> roles;
	roles[TopLeftRole] = "topLeft";
	roles[BottomRightRole] = "bottomRight";
	return roles;
}

Q_INVOKABLE void SeedsListModel::addSeedRect(QPoint top_left, QPoint bottom_right)
{
	beginInsertRows(QModelIndex(), seeds_.size(), seeds_.size());
	seeds_.append(SeedRect{ top_left, bottom_right });
	endInsertRows();
}

Q_INVOKABLE void SeedsListModel::removeSeedRect(int index)
{
	if (index < 0 || index >= seeds_.size()) {
		return;
	}
	beginRemoveRows(QModelIndex(), index, index);
	seeds_.removeAt(index);
	endRemoveRows();
}

Q_INVOKABLE void SeedsListModel::clear() {
	if (seeds_.isEmpty()) return;
	beginRemoveRows(QModelIndex(), 0, seeds_.size() - 1);
	seeds_.clear();
	endRemoveRows();
}


