#include "ViewModelCallbackGate.hpp"

namespace random_walker::viewmodel {

    void ViewModelCallbackGate::attach(QObject& receiver) noexcept {
        std::lock_guard lock(mutex_);
        receiver_ = &receiver;
    }

    void ViewModelCallbackGate::detach() noexcept {
        std::lock_guard lock(mutex_);
        receiver_ = nullptr;
    }
}
