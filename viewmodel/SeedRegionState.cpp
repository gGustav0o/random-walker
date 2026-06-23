#include "SeedRegionState.hpp"

#include <utility>

bool SeedRegionState::empty() const noexcept {
    return regions_.empty();
}

const std::vector<random_walker::domain::SeedRegion>&
SeedRegionState::regions() const noexcept {
    return regions_;
}

std::vector<random_walker::domain::SeedRegion>&
SeedRegionState::mutable_regions() noexcept {
    return regions_;
}

int SeedRegionState::pixel_count(
    random_walker::domain::SeedLabel label) const noexcept {
    return random_walker::domain::seed_pixel_count(regions_, label);
}

void SeedRegionState::clear() {
    regions_.clear();
}

void SeedRegionState::add(random_walker::domain::SeedRegion region) {
    regions_.push_back(std::move(region));
}
