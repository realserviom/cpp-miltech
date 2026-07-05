#include <memory>
#include "providers/JsonTargetProvider.h"
#include "config/FileConfigLoader.h"
#include "solvers/AnalyticalSolver.h"
#include "solvers/TableSolver.h"

std::unique_ptr<ITargetProvider> createProvider(ProviderType type, const std::string& file_name)
{
  switch (type) {
    case ProviderType::JSON:
      return std::make_unique<JsonTargetProvider>(file_name);
    default:
      return nullptr;
  }
}

std::unique_ptr<IConfigLoader> createLoader(LoaderType type,
                                            const std::string& file_drone_config_name,
                                            const std::string& file_list_ammo_name)
{
  switch (type) {
    case LoaderType::FILE:
      return std::make_unique<FileConfigLoader>(file_drone_config_name, file_list_ammo_name);
    default:
      return nullptr;
  }
}

std::unique_ptr<IBallisticSolver> createSolver(SolverType type)
{
  switch (type) {
    case SolverType::ANALYTICAL:
      return std::make_unique<AnalyticalSolver>();
    case SolverType::TABLE:
      return std::make_unique<TableSolver>("../data/ballistic_table.txt");
    default:
      return nullptr;
  }
}