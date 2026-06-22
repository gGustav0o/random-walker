#include "SeedListModel.hpp"

#include <cstddef>
#include <utility>

SeedListModel::SeedListModel(QObject* parent)
    : QAbstractListModel(parent)
{
}

int SeedListModel::rowCount(const QModelIndex& parent) const
{
    return parent.isValid() ? 0 : static_cast<int>(rectangles_.size());
}

QVariant SeedListModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid()
        || index.row() < 0
        || index.row() >= static_cast<int>(rectangles_.size())) {
        return {};
    }

    const Rectangle& rectangle =
        rectangles_[static_cast<std::size_t>(index.row())];

    switch (role) {
    case XRole:
        return rectangle.x;
    case YRole:
        return rectangle.y;
    case WidthRole:
        return rectangle.width;
    case HeightRole:
        return rectangle.height;
    case LabelRole:
        return static_cast<int>(rectangle.label);
    default:
        return {};
    }
}

QHash<int, QByteArray> SeedListModel::roleNames() const
{
    return {
        { XRole, "seedX" },
        { YRole, "seedY" },
        { WidthRole, "seedWidth" },
        { HeightRole, "seedHeight" },
        { LabelRole, "seedLabel" }
    };
}

void SeedListModel::append(Rectangle rectangle)
{
    const int row = static_cast<int>(rectangles_.size());
    beginInsertRows(QModelIndex(), row, row);
    rectangles_.push_back(std::move(rectangle));
    endInsertRows();
}

void SeedListModel::clear()
{
    if (rectangles_.empty()) {
        return;
    }

    beginResetModel();
    rectangles_.clear();
    endResetModel();
}

int SeedListModel::pixel_count(
    random_walker::domain::SeedLabel label) const noexcept
{
    int result = 0;
    for (const Rectangle& rectangle : rectangles_) {
        if (rectangle.label == label) {
            result += rectangle.width * rectangle.height;
        }
    }
    return result;
}

std::vector<random_walker::domain::Seed>
SeedListModel::expanded_seeds() const
{
    std::size_t seed_count = 0;
    for (const Rectangle& rectangle : rectangles_) {
        seed_count += static_cast<std::size_t>(rectangle.width)
            * static_cast<std::size_t>(rectangle.height);
    }

    std::vector<random_walker::domain::Seed> result;
    result.reserve(seed_count);

    for (const Rectangle& rectangle : rectangles_) {
        for (int row = rectangle.y;
             row < rectangle.y + rectangle.height;
             ++row) {
            for (int column = rectangle.x;
                 column < rectangle.x + rectangle.width;
                 ++column) {
                result.push_back({
                    .position = { .x = column, .y = row },
                    .label = rectangle.label
                });
            }
        }
    }

    return result;
}
