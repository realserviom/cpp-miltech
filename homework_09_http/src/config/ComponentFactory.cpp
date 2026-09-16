#include <memory>
#include "providers/ThreadSafeTargetProvider.h"
#include "config/FileConfigLoader.h"
#include "solvers/AnalyticalSolver.h"
#include "solvers/TableSolver.h"

std::shared_ptr<ITargetProvider> createProvider(ProviderType type, const std::string& file_name)
{
  switch (type) {
      // case ProviderType::JSON:
      //     return std::make_shared<JsonTargetProvider>(file_name);

    case ProviderType::TIME:
      return std::make_shared<ThreadSafeTargetProvider>(file_name);

    default:
      return nullptr;
  }
}

std::shared_ptr<IConfigLoader> createLoader(LoaderType type, const std::string& file_drone_config_name)
{
  switch (type) {
    case LoaderType::FILE:
      return std::make_shared<FileConfigLoader>(file_drone_config_name);
    default:
      return nullptr;
  }
}

std::shared_ptr<IBallisticSolver> createSolver(SolverType type)
{
  switch (type) {
    case SolverType::ANALYTICAL:
      return std::make_shared<AnalyticalSolver>();
    case SolverType::TABLE:
      return std::make_shared<TableSolver>("./data/ballistic_table.txt");
    default:
      return nullptr;
  }
}