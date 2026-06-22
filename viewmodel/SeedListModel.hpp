#pragma once

#include <vector>

#include <QAbstractListModel>
#include <QByteArray>
#include <QHash>
#include <QVariant>

#include "model/domain/Seed.hpp"

class SeedListModel final : public QAbstractListModel
{
    Q_OBJECT

public:
    struct Rectangle
    {
        int x = 0;
        int y = 0;
        int width = 0;
        int height = 0;
        random_walker::domain::SeedLabel label =
            random_walker::domain::SeedLabel::Background;
    };

    enum Role
    {
        XRole = Qt::UserRole + 1,
        YRole,
        WidthRole,
        HeightRole,
        LabelRole
    };

    explicit SeedListModel(QObject* parent = nullptr);

    [[nodiscard]] int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    [[nodiscard]] QVariant data(
        const QModelIndex& index,
        int role = Qt::DisplayRole
    ) const override;

    [[nodiscard]] QHash<int, QByteArray> roleNames() const override;

    void append(Rectangle rectangle);
    void clear();

    [[nodiscard]] int pixel_count(
        random_walker::domain::SeedLabel label) const noexcept;
    [[nodiscard]] std::vector<random_walker::domain::Seed>
        expanded_seeds() const;

private:
    std::vector<Rectangle> rectangles_;
};
