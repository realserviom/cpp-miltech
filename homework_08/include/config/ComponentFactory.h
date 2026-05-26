#pragma once
#include "../interfaces/ITargetProvider.h" 
#include "../interfaces/IConfigLoader.h"
#include "../interfaces/IBallisticSolver.h"

ITargetProvider* createProvider(ProviderType type, const char* file_name);
IConfigLoader* createLoader(LoaderType type, const char* file_drone_config_name, const char* file_list_ammo_name);
IBallisticSolver* createSolver(SolverType type);