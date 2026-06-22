#include "providers/ThreadSafeTargetProvider.h"

ThreadSafeTargetProvider::ThreadSafeTargetProvider(std::unique_ptr<ITargetProvider> provider)
  : m_underlyingProvider(std::move(provider))
{
  // Значення поля інтерфейсу синхронізуємо з оригіналом (якщо воно використовується безпосередньо)
  if (m_underlyingProvider) {
    arrayTimeStep = m_underlyingProvider->arrayTimeStep;
  }
}

int ThreadSafeTargetProvider::getTargetCount()
{
  std::lock_guard<std::mutex> lock(m_mutex);
  return m_underlyingProvider->getTargetCount();
}

Coord** ThreadSafeTargetProvider::getTargets()
{
  std::lock_guard<std::mutex> lock(m_mutex);
  return m_underlyingProvider->getTargets();
}

void ThreadSafeTargetProvider::setArrayTimeStep(float time)
{
  std::lock_guard<std::mutex> lock(m_mutex);
  arrayTimeStep = time;  // Оновлюємо внутрішнє поле інтерфейсу
  m_underlyingProvider->setArrayTimeStep(time);
}

float ThreadSafeTargetProvider::getArrayTimeStep() const
{
  std::lock_guard<std::mutex> lock(m_mutex);
  return m_underlyingProvider->getArrayTimeStep();
}

Coord ThreadSafeTargetProvider::getTargetPosition(const int target, const float time)
{
  std::lock_guard<std::mutex> lock(m_mutex);
  return m_underlyingProvider->getTargetPosition(target, time);
}

void ThreadSafeTargetProvider::init(int& numberCounterInTimeSpot)
{
  std::lock_guard<std::mutex> lock(m_mutex);
  m_underlyingProvider->init(numberCounterInTimeSpot);
  // Після ініціалізації оновлюємо копію поля, якщо оригінальний клас його змінив
  arrayTimeStep = m_underlyingProvider->arrayTimeStep;
}