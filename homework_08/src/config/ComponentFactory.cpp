#include "config/ComponentFactory.h"
#include "providers/JsonTargetProvider.h"
#include "config/FileConfigLoader.h"
#include "solvers/AnalyticalSolver.h"

ITargetProvider* createProvider(ProviderType type, const char* file_name)
{
  switch (type) {
    case ProviderType::JSON:
      return new JsonTargetProvider(file_name);
    default:
      return nullptr;
  }
}

IConfigLoader* createLoader(LoaderType type, const char* file_drone_config_name, const char* file_list_ammo_name)
{
  switch (type) {
    case LoaderType::FILE:
      return new FileConfigLoader(file_drone_config_name, file_list_ammo_name);
    default:
      return nullptr;
  }
}

IBallisticSolver* createSolver(SolverType type)
{
  switch (type) {
    case SolverType::ANALYTICAL:
      return new AnalyticalSolver();
    default:
      return nullptr;
  }
}