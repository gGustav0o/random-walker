#pragma once

#include <functional>
#include <vector>

#include <QAbstractListModel>
#include <QByteArray>
#include <QHash>
#include <QVariant>

#include "model/domain/Seed.hpp"

class SeedListModel final : public QAbstractListModel {
    Q_OBJECT

public:
    enum Role {
        XRole = Qt::UserRole + 1
        , YRole
        , WidthRole
        , HeightRole
        , LabelRole
    };

    explicit SeedListModel(
        const std::vector<random_walker::domain::SeedRegion>& regions
        , QObject* parent = nullptr
    );

    [[nodiscard]] int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    [[nodiscard]] QVariant data(
        const QModelIndex& index
        , int role = Qt::DisplayRole
    ) const override;

    [[nodiscard]] QHash<int, QByteArray> roleNames() const override;

    void reset(const std::function<void()>& update_regions);

private:
    const std::vector<random_walker::domain::SeedRegion>& regions_;
};
