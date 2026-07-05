#include "RollingTargetStack.h"
#include <utility>

RollingTargetStack::RollingTargetStack(std::size_t maxSize)
  : m_maxSize(maxSize)
{
}

void RollingTargetStack::push(const Target& target)
{
  m_data.push_back(target);
  if (m_data.size() > m_maxSize) {
    m_data.pop_front();
  }
}

void RollingTargetStack::push(Target&& target)
{
  m_data.push_back(std::move(target));
  if (m_data.size() > m_maxSize) {
    m_data.pop_front();
  }
}

void RollingTargetStack::pop()
{
  if (!m_data.empty()) {
    m_data.pop_back();
  }
}

void RollingTargetStack::clear() noexcept
{
  m_data.clear();
}

Target& RollingTargetStack::top()
{
  return m_data.back();
}

const Target& RollingTargetStack::top() const
{
  return m_data.back();
}

Target& RollingTargetStack::bottom()
{
  return m_data.front();
}

const Target& RollingTargetStack::bottom() const
{
  return m_data.front();
}

bool RollingTargetStack::empty() const noexcept
{
  return m_data.empty();
}

std::size_t RollingTargetStack::size() const noexcept
{
  return m_data.size();
}

Target& RollingTargetStack::operator[](std::size_t index)
{
  return m_data[index];
}

const Target& RollingTargetStack::operator[](std::size_t index) const
{
  return m_data[index];
}