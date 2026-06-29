#include "SeedListModel.hpp"

#include <cstddef>

#include <QtGlobal>

SeedListModel::SeedListModel(
    const std::vector<random_walker::domain::SeedRegion>& regions
    , QObject* parent
)
    : QAbstractListModel(parent)
    , regions_(regions) {
}

int SeedListModel::rowCount(const QModelIndex& parent) const {
    return parent.isValid() ? 0 : static_cast<int>(regions_.size());
}

QVariant SeedListModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid()
        || index.row() < 0
        || index.row() >= static_cast<int>(regions_.size())) {
        return {};
    }

    const random_walker::domain::SeedRegion& region =
        regions_[static_cast<std::size_t>(index.row())];

    switch (role) {
    case XRole:
        return region.area.x;
    case YRole:
        return region.area.y;
    case WidthRole:
        return region.area.width;
    case HeightRole:
        return region.area.height;
    case LabelRole:
        return static_cast<int>(region.label);
    default:
        return {};
    }
}

QHash<int, QByteArray> SeedListModel::roleNames() const {
    return {
        { XRole, "seedX" }
        , { YRole, "seedY" }
        , { WidthRole, "seedWidth" }
        , { HeightRole, "seedHeight" }
        , { LabelRole, "seedLabel" }
    };
}

void SeedListModel::reset(const std::function<void()>& update_regions) {
    Q_ASSERT(update_regions);
    beginResetModel();
    update_regions();
    endResetModel();
}
