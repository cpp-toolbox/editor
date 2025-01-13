#ifndef EXPIRING_TEMPORAL_VECTOR_TPP
#define EXPIRING_TEMPORAL_VECTOR_TPP

#include "expiring_temporal_vector.hpp"
#include <algorithm>

template <typename T>
ExpiringTemporalVector<T>::ExpiringTemporalVector(std::chrono::seconds time_limit)
    : time_limit_(time_limit) {}

template <typename T>
void ExpiringTemporalVector<T>::add(const T& element) {
    auto now = Clock::now();
    elements_.emplace_back(element, now);
    clean_expired_elements();
}

template <typename T>
std::vector<T> ExpiringTemporalVector<T>::get_elements() {
    clean_expired_elements();
    std::vector<T> result;
    for (const auto& e : elements_) {
        result.push_back(e.value);
    }
    return result;
}

template <typename T>
std::vector<T> ExpiringTemporalVector<T>::get_elements_since(TimePoint time) {
    clean_expired_elements();
    std::vector<T> result;
    for (const auto& e : elements_) {
        if (e.timestamp >= time) {
            result.push_back(e.value);
        }
    }
    return result;
}

template <typename T>
void ExpiringTemporalVector<T>::clean_expired_elements() {
    auto now = Clock::now();
    elements_.erase(std::remove_if(elements_.begin(), elements_.end(),
        [this, now](const Element& e) {
            return (now - e.timestamp) > time_limit_;
        }),
        elements_.end());
}

#endif // EXPIRING_TEMPORAL_VECTOR_TPP
