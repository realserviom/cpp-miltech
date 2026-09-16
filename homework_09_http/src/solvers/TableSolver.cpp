#include "solvers/TableSolver.h"
#include "BallisticTable.h"

// Внутрішній метод, який перевіряє, чи змінилися параметри, та працює з LUT
BallisticTable::Result TableSolver::getOrUpdateCache(float z, float v, float m, float d, float l)
{
  // Якщо параметри ті самі, що й минулого виклику — повертаємо кеш
  if (m_cache.z == z && m_cache.v == v && m_cache.m == m && m_cache.d == d && m_cache.l == l) {
    return m_cache.result;
  }

  // Якщо параметри нові — робимо пошук по таблиці й оновлюємо кеш
  m_cache.z = z;
  m_cache.v = v;
  m_cache.m = m;
  m_cache.d = d;
  m_cache.l = l;

  m_cache.result = m_table.lookup(z, v, m, d, l);
  return m_cache.result;
}

// Конструктор
TableSolver::TableSolver(const std::string &path)
{
  if (!m_table.load(path.c_str())) {
    throw std::runtime_error("[TableSolver] Critical Error: Failed to load table from " + path);
  }
}

// 1. Метод для отримання часу польоту
float TableSolver::getTimePol(const float &d, const float &l, const float &m, const float &zd, const float &attackSpeed)
{
  // zd - висота, attackSpeed - швидкість V0
  auto res = getOrUpdateCache(zd, attackSpeed, m, d, l);
  return res.t;
}

// 2. Метод для отримання дистанції під час падіння
double TableSolver::getDistDuringFall(float &t_pol, DroneConfig &myDrone, const AmmoParams *ammo)
{
  // Перевірка на nullptr, про всяк випадок
  if (!ammo)
    return 0.0;

  // Зчитуємо реальні балістичні коефіцієнти з ammo
  float m = static_cast<float>(ammo->mass);
  float d = static_cast<float>(ammo->drag);
  float l = static_cast<float>(ammo->lift);

  // Висоту та швидкість атаки беремо з поточного стану дрона
  float z = static_cast<float>(myDrone.altitude);     // висота дрона над ціллю
  float v = static_cast<float>(myDrone.attackSpeed);  // швидкість дрона в момент скидання

  // Оновлюємо кеш або беремо готове з таблиці
  auto res = getOrUpdateCache(z, v, m, d, l);

  // Записуємо порахований час назад у t_pol за посиланням
  t_pol = res.t;

  // Повертаємо горизонтальну дистанцію від точки скидання до цілі
  return static_cast<double>(res.hDist);
}