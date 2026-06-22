#pragma once
#include "../interfaces/ITargetProvider.h" 
#include "../interfaces/IConfigLoader.h"
#include "../interfaces/IBallisticSolver.h"

ITargetProvider* createProvider(ProviderType type, const std::string& file_name);

IConfigLoader* createLoader(LoaderType type, const std::string& file_drone_config_name, const std::string& file_list_ammo_name);

IBallisticSolver* createSolver(SolverType type);