#pragma once

#include <ctime>
#include <deque>
#include <unordered_map>
#include <cstddef>
#include <stdexcept>
#include "Types.h"

class RollingTargetStack {
private:
  // Для кожного TargetId зберігається свій незалежний deque
  std::unordered_map<TargetId, std::deque<Target>> m_data;

public:
  std::size_t m_maxSize;
  explicit RollingTargetStack(int timeStep);

  // Додавання елементів (L-value та R-value для оптимізації)
  void push(const TargetId& id, const Target& target);
  void push(const TargetId& id, Target&& target);

  // Видалення елементів
  void pop(const TargetId& id);          // Видалити найстаріший елемент конкретної цілі
  void clearTarget(const TargetId& id);  // Очистити історію конкретної цілі
  void clearAll() noexcept;              // Очистити взагалі всі цілі

  // Доступ до елементів
  [[nodiscard]] Target& top(const TargetId& id);  // Найновіший (в кінці деку)
  [[nodiscard]] const Target& top(const TargetId& id) const;

  [[nodiscard]] Target& bottom(const TargetId& id);  // Найстаріший (в початку деку)
  [[nodiscard]] const Target& bottom(const TargetId& id) const;

  // Доступ за індексом всередині конкретної цілі
  [[nodiscard]] Target& at(const TargetId& id, std::size_t index);
  [[nodiscard]] const Target& at(const TargetId& id, std::size_t index) const;

  // Стан контейнера
  [[nodiscard]] bool empty(const TargetId& id) const noexcept;      // Чи пустий дек конкретної цілі
  [[nodiscard]] bool hasTarget(const TargetId& id) const noexcept;  // Чи взагалі існує така ціль в мапі
  [[nodiscard]] std::size_t size(const TargetId& id) const noexcept;
  [[nodiscard]] std::size_t targetCount() const noexcept;  // Кількість унікальних цілей

  [[nodiscard]] bool isFull(const TargetId& id) const noexcept;
};