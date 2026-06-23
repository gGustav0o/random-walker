#pragma once

#include <vector>

#include "model/domain/Seed.hpp"

class SeedRegionState final {
public:
    [[nodiscard]] bool empty() const noexcept;
    [[nodiscard]] const std::vector<random_walker::domain::SeedRegion>&
    regions() const noexcept;
    [[nodiscard]] std::vector<random_walker::domain::SeedRegion>&
    mutable_regions() noexcept;
    [[nodiscard]] int pixel_count(
        random_walker::domain::SeedLabel label) const noexcept;

    void clear();
    void add(random_walker::domain::SeedRegion region);

private:
    std::vector<random_walker::domain::SeedRegion> regions_;
};
