#pragma once

#include <deque>
#include <cstddef>
#include "Types.h"

class RollingTargetStack {
private:
    std::deque<Target> m_data;
    const std::size_t m_maxSize = 20;

  public:
    RollingTargetStack() = default;
    explicit RollingTargetStack(std::size_t maxSize);

    void push(const Target& target);
    void push(Target&& target);

    void pop();
    void clear() noexcept;

    [[nodiscard]] Target& top();
    [[nodiscard]] const Target& top() const;
    [[nodiscard]] Target& bottom();
    [[nodiscard]] const Target& bottom() const;

    [[nodiscard]] bool empty() const noexcept;
    [[nodiscard]] std::size_t size() const noexcept;

    [[nodiscard]] Target& operator[](std::size_t index);
    [[nodiscard]] const Target& operator[](std::size_t index) const;
};