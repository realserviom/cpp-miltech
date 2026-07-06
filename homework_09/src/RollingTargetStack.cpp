#include "RollingTargetStack.h"
#include <string>

RollingTargetStack::RollingTargetStack(int timeStep)
  : m_maxSize(2 * (1 / timeStep) + 1)
{
}

// Додавання копіюванням
void RollingTargetStack::push(const TargetId& id, const Target& target)
{
  auto& deque = m_data[id];  // Створює новий deque, якщо id раніше не існувало
  if (deque.size() >= m_maxSize) {
    deque.pop_front();  // Виштовхуємо найстарішу точку
  }
  deque.push_back(target);
}

// Додавання переміщенням (оптимізація для R-value)
void RollingTargetStack::push(const TargetId& id, Target&& target)
{
  auto& deque = m_data[id];
  if (deque.size() >= m_maxSize) {
    deque.pop_front();
  }
  deque.push_back(std::move(target));
}

// Видалити один найстаріший елемент (з початку історії)
void RollingTargetStack::pop(const TargetId& id)
{
  auto it = m_data.find(id);
  if (it != m_data.end() && !it->second.empty()) {
    it->second.pop_front();
    // Якщо після цього історія цілі пуста, можна видалити сам id з мапи
    if (it->second.empty()) {
      m_data.erase(it);
    }
  }
}

// Повністю видалити історію однієї цілі
void RollingTargetStack::clearTarget(const TargetId& id)
{
  m_data.erase(id);
}

// Повне очищення всієї карти
void RollingTargetStack::clearAll() noexcept
{
  m_data.clear();
}

// Найновіший елемент (остання додана точка)
Target& RollingTargetStack::top(const TargetId& id)
{
  auto it = m_data.find(id);
  if (it == m_data.end() || it->second.empty()) {
    throw std::out_of_range("RollingTargetStack: Target ID not found or empty");
  }
  return it->second.back();
}

const Target& RollingTargetStack::top(const TargetId& id) const
{
  auto it = m_data.find(id);
  if (it == m_data.end() || it->second.empty()) {
    throw std::out_of_range("RollingTargetStack: Target ID not found or empty");
  }
  return it->second.back();
}

// Найстаріший елемент, який ще тримається в пам'яті
Target& RollingTargetStack::bottom(const TargetId& id)
{
  auto it = m_data.find(id);
  if (it == m_data.end() || it->second.empty()) {
    throw std::out_of_range("RollingTargetStack: Target ID not found or empty");
  }
  return it->second.front();
}

const Target& RollingTargetStack::bottom(const TargetId& id) const
{
  auto it = m_data.find(id);
  if (it == m_data.end() || it->second.empty()) {
    throw std::out_of_range("RollingTargetStack: Target ID not found or empty");
  }
  return it->second.front();
}

// Доступ до історії за індексом (0 - найстаріший, size()-1 - найновіший)
Target& RollingTargetStack::at(const TargetId& id, std::size_t index)
{
  auto it = m_data.find(id);
  if (it == m_data.end() || index >= it->second.size()) {
    throw std::out_of_range("RollingTargetStack: Index or Target ID out of range. Index: " + std::to_string(index) +
                            ", target: " + std::to_string(id));
  }
  return it->second[index];
}

const Target& RollingTargetStack::at(const TargetId& id, std::size_t index) const
{
  auto it = m_data.find(id);
  if (it == m_data.end() || index >= it->second.size()) {
    throw std::out_of_range("RollingTargetStack: Index or Target ID out of range. Index: " + std::to_string(index) +
                            ", target: " + std::to_string(id));
  }
  return it->second[index];
}

// Перевірка на пустоту
bool RollingTargetStack::empty(const TargetId& id) const noexcept
{
  auto it = m_data.find(id);
  if (it == m_data.end())
    return true;
  return it->second.empty();
}

// Перевірка чи трекається ціль взагалі
bool RollingTargetStack::hasTarget(const TargetId& id) const noexcept
{
  return m_data.find(id) != m_data.end();
}

// Розмір історії конкретної цілі
std::size_t RollingTargetStack::size(const TargetId& id) const noexcept
{
  auto it = m_data.find(id);
  if (it == m_data.end())
    return 0;
  return it->second.size();
}

// Скільки всього цілей зараз у буфері
std::size_t RollingTargetStack::targetCount() const noexcept
{
  return m_data.size();
}

bool RollingTargetStack::isFull(const TargetId& id) const noexcept
{
  auto it = m_data.find(id);
  if (it == m_data.end()) {
    return false;  // Якщо цілі взагалі немає, її стек точно не заповнений
  }
  // Перевіряємо, чи розмір деку досяг або перевищив ліміт
  return it->second.size() >= m_maxSize;
}