#pragma once
#include "../interfaces/ITargetProvider.h" 
#include "../interfaces/IConfigLoader.h"
#include "../interfaces/IBallisticSolver.h"
#include <memory>

std::shared_ptr<ITargetProvider> createProvider(ProviderType type, const std::string& file_name);
std::shared_ptr<IConfigLoader> createLoader(LoaderType type,
                                            const std::string& file_drone_config_name,
                                            const std::string& file_list_ammo_name);
std::shared_ptr<IBallisticSolver> createSolver(SolverType type);

