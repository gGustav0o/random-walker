#pragma once

#include <memory>
#include <mutex>
#include <type_traits>
#include <utility>

#include <QMetaObject>
#include <QObject>
#include <Qt>

namespace random_walker::viewmodel {

    class ViewModelCallbackGate final {
    public:
        void attach(QObject& receiver) noexcept;
        void detach() noexcept;

        template<typename Receiver, typename Payload>
        void post(
            Payload payload
            , void (Receiver::*handler)(std::decay_t<Payload>)
        ) const {
            auto stored_payload =
                std::make_shared<std::decay_t<Payload>>(std::move(payload));

            std::lock_guard lock(mutex_);
            if (!receiver_) {
                return;
            }

            QMetaObject::invokeMethod(
                receiver_
                , [receiver = receiver_, handler, stored_payload]() mutable {
                    auto* typed_receiver = static_cast<Receiver*>(receiver);
                    (typed_receiver->*handler)(std::move(*stored_payload));
                }
                , Qt::QueuedConnection
            );
        }

    private:
        mutable std::mutex mutex_;
        QObject* receiver_ = nullptr;
    };
}
