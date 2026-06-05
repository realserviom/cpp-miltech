/******************************************************************************

Welcome to GDB Online.
GDB online is an online compiler and debugger tool for C, C++, Python, Java, PHP, Ruby, Perl,
C#, OCaml, VB, Swift, Pascal, Fortran, Haskell, Objective-C, Assembly, HTML, CSS, JS, SQLite, Prolog.
Code, Compile, Run and Debug online from anywhere in world.

*******************************************************************************/
#include <iostream>
#include <stdlib.h>
#include <cmath>
#include "JsonTargetProvider.h"
#include "FileConfigLoader.h"
#include "AnalyticalSolver.h"
#include "MissionProcessor.h"

//Визначення константи Пі, якщо її немає в cmath
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif


int main()
{
    std::unique_ptr<IConfigLoader> configLoader{createLoader(LoaderType::FILE, "../data/config.json", "../data/ammo.json")};
    std::unique_ptr<ITargetProvider> targetProvider{createProvider(ProviderType::JSON, "../data/targets.json")};
    std::unique_ptr<IBallisticSolver> analyticalSolver{createSolver(SolverType::ANALYTICAL)};

    MissionProcessor processor(targetProvider.get(), analyticalSolver.get(), configLoader.get());

    try {
        processor.executeMission();
    } 
    catch (const std::runtime_error& e) {
        std::cout << e.what() << std::endl;
        return -1;
    }

    return 0;
}