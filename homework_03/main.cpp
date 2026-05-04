/******************************************************************************

Welcome to GDB Online.
GDB online is an online compiler and debugger tool for C, C++, Python, Java, PHP, Ruby, Perl,
C#, OCaml, VB, Swift, Pascal, Fortran, Haskell, Objective-C, Assembly, HTML, CSS, JS, SQLite, Prolog.
Code, Compile, Run and Debug online from anywhere in world.

*******************************************************************************/
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <cmath>
#include <iomanip>
#include <string.h>
#include <fstream>
#include "constants.h"
#include "types.h"
#include "functions.h"
#include "debug.h"

//Визначення константи Пі, якщо її немає в cmath
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif


int main()
{

    // лічильник
    int counter = 0;
      
    // поточний напрямок дрона в радіанах відносто осі X
    float curAngularState = 0;
    
    // масив який містить час підльоту до кожної цілі
    float targetTimes[NUMBER_OF_TARGETS];
    
    // масив який містить відстань під поточного положення дрону до кожної цілі
    float targetDistances[NUMBER_OF_TARGETS];
       
    // масив який містить кут напрямку для кожної цілі відносто осі X в радіанах 
    float targetAngles[NUMBER_OF_TARGETS];   

    DroneConfig myDrone;
    
    if (!getDataFromInputTxtFile(myDrone)) {
        return 1;
    }

    // запускаємо для розрахунку додаткових параметрів
    myDrone.updateCalculatedParams();

    float ratio = myDrone.arrayTimeStep / myDrone.simTimeStep;
    
    if (std::abs(ratio - std::floor(ratio)) > 0.0001f) {
        printf("Помилка: Крок масиву не ділиться націло на крок симуляції!\n");
        return 1;
    }

    
    // кожну кількість numberCounterInTimeSpot в нас міняються координати цілей тобто проходить arrayTimeStep секунд
    const int numberCounterInTimeSpot =  static_cast<int>(ratio);

    int targetCount;
    Coord** targets = loadTargetCoordinates(targetCount);

    if (targets == nullptr) {
        return 1;
    }

    CurrentDroneParameters curMyDrone;

    // поточний стан дрона і його кута польоту, координати дрона, ціль
    curMyDrone.pos.x = myDrone.startPos.x;
    curMyDrone.pos.y = myDrone.startPos.y;
    curMyDrone.speed = 0;
    curMyDrone.angularState = myDrone.initialDir;
    curMyDrone.state = STOPPED;
    curMyDrone.target = 0;

    // ключ що відповідає за переключання цілей дрона
    bool keyChangeTarget = true;
    
    // ключ що відповідає за зміну кута напрямку дрона
    bool keyChangeAngle = true;;
   
    DEBUG("Час зупинки або прискорення: " << std::fixed << std::setprecision(2) << myDrone.timeAcceleration << " с");
    DEBUG("Прискорення дрона: " << myDrone.acceleration << " м/с2 ---");
    DEBUG("Величина оберту дрона за ітерацію: " << myDrone.radInIteration << " р/с ---");
    DEBUG("===========================");
    
    const int ammoCount = 5;
    
    AmmoParams* ammoTable = new AmmoParams[ammoCount];
 
    fillAmmoData(ammoTable);

    SimStep* steps = new SimStep[MAX_STEPS];
    
    const AmmoParams* ammo = getAmmoParameters(ammoTable, ammoCount, myDrone.ammoName);
    
    if (ammo == nullptr) {
        printf("Помилка: боєприпас %s не знайдено в базі!\n", myDrone.ammoName);
        return 1;
    }
    
    DEBUG("Знайдено боєприпас: " << myDrone.ammoName);
    DEBUG("Параметри: mass: " << std::fixed << std::setprecision(2) << ammo->mass 
          << ", drag: " << ammo->drag 
          << ", lift: " << ammo->lift);
        
    float t_pol;

    if (!getTimePol(t_pol, ammo->drag, ammo->lift, ammo->mass, myDrone.altitude, myDrone.attackSpeed)) {
        return 1;
    } 
  
    float distDuringFall = calculateDistDuringFall(t_pol, myDrone.attackSpeed, ammo->mass, ammo->drag, ammo->lift, physics::GRAVIT);
        
    DEBUG("Горизонтальна дистанція яку проходить дрон за час " << t_pol << " сек. рівна " << distDuringFall << " м.");
    DEBUG("-----------------------------------");
    
    if (distDuringFall <= 0) {
        std::cout << "Горизонтальна дистанція повинна бути додатня" << std::endl;
        return 1;
    }
        
    bool addingStep = false;
    
    while(true) {
        
        // запис даних в масиви з яких будемо формувати вихідний файл
        // при першій ітерації не записуємо точку в масив
        if (addingStep) {
            steps[counter - 1] = {
                {curMyDrone.pos.x, curMyDrone.pos.y},
                curMyDrone.angularState,
                curMyDrone.state, 
                curMyDrone.target,
                {curMyDrone.dropPoint.x, curMyDrone.dropPoint.y},
                {curMyDrone.aimPoint.x, curMyDrone.aimPoint.y},
                {curMyDrone.predictedTarget.x, curMyDrone.predictedTarget.y},
            };
        }
        
        addingStep = true;
            
        // розраховуємо всі дані для визначення поточної найближчої цілі
        const int timeIteration = getTimeIteration(counter, numberCounterInTimeSpot, NUMBER_OF_TIMES);
        
      
        DEBUG("--- counter = " << counter << " ---");
        DEBUG("--- timeIteration = " << timeIteration << " ---");
        DEBUG("--- curDroneX = " << std::fixed << std::setprecision(8) << curMyDrone.pos.x << " м ---");
        DEBUG("--- curDroneY = " << std::fixed << std::setprecision(8) << curMyDrone.pos.y << " м ---");
        DEBUG("--- curAngularState = " << std::fixed << std::setprecision(2) << curMyDrone.angularState << " р. ---");
        DEBUG("--- curDroneSpeed = " << curMyDrone.speed << " ---");
        DEBUG("--- curDroneState = " << getDroneStateName(curMyDrone.state) << " ---");
        DEBUG("--- currentTarget = " << curMyDrone.target << " ---");

           
        if (keyChangeTarget) {
            for (int i = 0; i < NUMBER_OF_TARGETS; i++) {
             
                // Допоміжні функції для Coord (вільні або як методи):
                // float length(Coord c) — довжина вектора (hypot) - треба зробити
                float length = calculateLength(targets[i][timeIteration].x,
                    targets[i][timeIteration].y, curMyDrone.pos.x, curMyDrone.pos.y);

                    //printf("Координати %lf, %lf", targets[i][timeIteration].x, targets[i][timeIteration].y);
    
                // я не розглядаю задачу де можуть бути задані відємні координати положення цілі
                // в цьому випадку треба буде переписати формули з модулями і також кути обертання треба буде перераховувати
                // в залежності від знаку + чи - перед координатами положення цілі
                
                float deltaX = targets[i][timeIteration].x - curMyDrone.pos.x;
                float deltaY = targets[i][timeIteration].y - curMyDrone.pos.y; 

                                
                // Функція acos повертає результат у радіанах
                // це кут цілі відносно положення дрона
                float angle_in_rad = atan2(deltaY, deltaX);
                float angle_deg = angle_in_rad * (180.0f / M_PI);
                
                //printf("Початковий targetAngle для цілі %d : %.4f м/с\n", i , angle_in_rad);
                
                targetAngles[i] = angle_in_rad;
                                
                // час протягом якого дрон буде повертатися у випадку невірного напрямку 
                float timeTurned = (targetAngles[i] - curMyDrone.angularState) > myDrone.turnThreshold ? 
                    (targetAngles[i] - curMyDrone.angularState) / myDrone.angularSpeed : 0;
                    
                // час за який дрон долетить до цілі з вичитанням шляху падіння боєприпасу а також шляхом на розгін
                float t = calculateArrivalTime(length, timeTurned, myDrone.timeAcceleration, myDrone.accelPath, distDuringFall, myDrone.attackSpeed);
                
                // якщо ми митєво долітаємо до цілі в межах наступної часової ітерації по координаті a це arrayTimeStep секунд
                // тоді враховуємо відхилення цілі за час дольоту до неї 
                // іноді ціль може рухатися до дрону тоді підльот буде тривати ще менше часу
                // час не враховує те що коли боєприпас скинутий ціль ще зміщується
                // в цьому випадку ми вже ціль не можемо поміняти тому що недоцільно зупиняти і набирати знову швидкість 
                // Хоча при умові що пороговий кут в межаш похибки тоді можна міняти але з іншої сторони ми можемо перепригувати 
                // із цілі на ціль що дасть велику похибку (цікаво як роблять виробники ПЗ для ППО ?)
                if (t < myDrone.arrayTimeStep) {
                    
                    keyChangeTarget = false;
                    
                    int nextIteration = getNextIteration(timeIteration);
                    
                    float distWithoutPoint = calculateLength(targets[i][nextIteration].x,
                        targets[i][nextIteration].y, targets[i][timeIteration].x, targets[i][timeIteration].y);
                    
                    // швидкість цілі i між двома точками    
                    float Vtarget = distWithoutPoint / myDrone.arrayTimeStep;

                    // швидкість Vxtarget це швидкість зміни координати x може бути відємною
                    float Vxtarget = (targets[i][nextIteration].x - targets[i][timeIteration].x) / myDrone.arrayTimeStep;

                     
                    // швидкість Vytarget це швидкість зміни координати y може бути відємною
                    float Vytarget = (targets[i][nextIteration].y - targets[i][timeIteration].y) / myDrone.arrayTimeStep;
                    
                    // знайшли зміщення маючи час руху до цілі
                    double targetXEndPoint = targets[i][timeIteration].x + (Vxtarget * t);
                    double targetYEndPoint = targets[i][timeIteration].y + (Vytarget * t);
                    
                     // вирахували нову відстань
                    float length = calculateLength(targetXEndPoint, targetYEndPoint, curMyDrone.pos.x, curMyDrone.pos.y);
                             
                    targetDistances[i] = length;
                    
                    // тут ми маємо визначити час протягом якого дрон досягне цілі 
                    // враховуючи стан дрона (статус), координати, швидкість дрона, час падіння боєприпасу 
                    // і кут повороту (на даний момент ми нехтуємо кутом повороту будемо повертати під час польоту)
                    float t_new = calculateSmallArrivalTime(length - distDuringFall, myDrone.attackSpeed, curMyDrone.speed, myDrone.acceleration);
                    
                                        
                    if(std::abs(t_new - t) < myDrone.timeHitRadius) {
                        t = t_new;
                    } else if (t_new < t) {
                        t = (t + t_new) / 2;
                    } else {
                        t = t_new; 
                    }
                    
          
                    // знайшли зміщення маючи час руху до цілі t_new
                    targetXEndPoint = targets[i][timeIteration].x + (Vxtarget * t);
                    targetYEndPoint = targets[i][timeIteration].y + (Vytarget * t);
                
                    
                    // Кут, під яким дрон МАЄ летіти, щоб влучити в точку зустрічі 
                    // Ми врахували зміщення до цілі і тому перераховуємо кут нахилу дрона до цілі
                    float targetAngle = atan2(targetYEndPoint - curMyDrone.pos.y, targetXEndPoint - curMyDrone.pos.x);
                    
                    //printf("кут targetAngle для цілі %d = : %.4f м/с\n", i, targetAngle);
                    // записуємо тільки один раз кут зміщення це коли вже пряма наводка до цілі
                    targetAngles[i] = targetAngle;
                    
                }
                
                DEBUG("До " << i + 1 << " цілі " << std::fixed << std::setprecision(2) << length 
                << "; Кут у радіанах: " << std::setprecision(4) << targetAngles[i] 
                << " rad; Час польоту: " << std::setprecision(2) << t << " с");
            
                targetTimes[i] = t;
            }
        }
        
        // визначаємо ціль за мінімальним часом підльоту
      
        int newTarget = getIndexByMinValue(targetTimes);

        if(newTarget != curMyDrone.target) {
            DEBUG("Нова ціль: " << newTarget);
        }
         
        // Умова при якій програма находить точку скиду
        // Координа падіння боєприпасу дрона через t_pol секунд
        // Але t_pol дрона в нас досягається при досягані швидкості атаки якщо швидкість менша треба перераховувати
        // В цій задачі ми не будемо перераховувати цей час. Будемо перелітати і йти на друге коло польоту на ціль
        // якщо відстані не співпадуть

        // на скільки я знаю треба працювати без cos і sin тоу що це для процесора важкі операції
        // в майбутньому перепишу
        Coord droneDir = { (float)cos(curMyDrone.angularState), (float)sin(curMyDrone.angularState) };

        // Знаходимо точку, куди прилетить боєприпас
        Coord targetAmmoPos = curMyDrone.pos + normalize(droneDir) * distDuringFall;
      
        // мітка часу коли боєприпас долетить до землі якщо буде випущений в даний момент
        int Tt = counter * myDrone.simTimeStep + t_pol;
        
        // ітерація координати (кожних 5 секунд нова координата)
        int futureIteration = std::floor(Tt / myDrone.arrayTimeStep);
        
        int nextFutureIteration = getNextIteration(futureIteration);
        
        float remainderTimeInSpot = (counter % numberCounterInTimeSpot) * myDrone.simTimeStep;

        Coord deltaPos = targets[newTarget][nextFutureIteration] - targets[newTarget][futureIteration];

        // швидкість Vtarget це швидкість зміни координатів

        Coord Vtarget = deltaPos / myDrone.arrayTimeStep;

        Coord targetEndPoint = targets[newTarget][futureIteration] + Vtarget * remainderTimeInSpot;

        // прогнозована позиція цілі
        curMyDrone.predictedTarget.x = targetEndPoint.x;    
        curMyDrone.predictedTarget.y = targetEndPoint.y;	

        // точка скиду (куди летить дрон)
        Coord dirToDrone = normalize(curMyDrone.pos - curMyDrone.predictedTarget);
        Coord releasePoint = curMyDrone.predictedTarget + dirToDrone * (float)distDuringFall;
        curMyDrone.dropPoint.x = releasePoint.x;    
        curMyDrone.dropPoint.y = releasePoint.y;


        // куди впаде бомба (якщо скинути зараз)
        curMyDrone.aimPoint.x = targetAmmoPos.x;    
        curMyDrone.aimPoint.y = targetAmmoPos.y;


      
        Coord delta = targetAmmoPos - targetEndPoint;
        
        double finalDistance = length(delta);
        
        if (finalDistance <= myDrone.hitRadius) {
            DEBUG("--- БОЄПРИПАС СКИНУТИЙ! Ураження : " << std::fixed << std::setprecision(2) << finalDistance << " м від цілі номер " << newTarget << " ---");
            DEBUG("--- remainderTimeInSpot: " << std::setprecision(4) << remainderTimeInSpot << " ---");
            DEBUG("--- curDrone: (" << curMyDrone.pos.x << ", " << curMyDrone.pos.y << ") ---");
            DEBUG("--- targetAmmo: (" << targetAmmoPos.x << ", " << targetAmmoPos.y << ") ---");
            DEBUG("--- targetEndPoint: (" << targetEndPoint.x << ", " << targetEndPoint.y << ") ---");
            
            DEBUG("--- [futurePos]: (" << targets[newTarget][futureIteration].x << ", " << targets[newTarget][futureIteration].y << ") ---");
            DEBUG("--- [nextFuturePos]: (" << targets[newTarget][nextFutureIteration].x << ", " << targets[newTarget][nextFutureIteration].y << ") ---");
            break;
        } 
        
        // якщо в нас відстань між дроном і цілю менше ніж distDuringFall + 5 метрів тоді включаємо пошук цілі знову
        // тому що дрон не вийшов на позицію
        if (!keyChangeTarget) {
            double distanceWithDroneAndTarget = length(curMyDrone.pos - targetEndPoint);
            
            if (distanceWithDroneAndTarget > distDuringFall + 5 &&  distanceWithDroneAndTarget > myDrone.arrayTimeStep * 2) {
                keyChangeTarget = true;
            }
        }
  
        // State == STOPPED тільки коли стартує
        if(curMyDrone.state == STOPPED) {
            curMyDrone.state = updateRotation(targetAngles[newTarget], curMyDrone.angularState, myDrone.radInIteration, myDrone.turnThreshold) ? TURNING : ACCELERATING;
        } else if (curMyDrone.state == DECELERATING) {

            // якщо в нас статус DECELERATING ми зупиняємо дрон до повної зупинки щоб потім його повернути
            // це коли в нас змінилася ціль (тобто зміну цілі ми не провіряємо бо вона вже змінена :) як банально це не звучить )  
            updateDronePosition(myDrone, curMyDrone);
            
            curMyDrone.speed -= (myDrone.acceleration * myDrone.simTimeStep);
            if (curMyDrone.speed <= 0) {
                curMyDrone.speed = 0;
                curMyDrone.state = TURNING;
            }
        }  else if (newTarget != curMyDrone.target) {
            // ціль змінюється тільки коли keyChangeTarget = true
            // тому змінюємо на true
            keyChangeTarget = true;
           
            // якщо змінилася ціль ми провіряємо чи кут напрямку в межах нової цілі
            // якщо в межаш тоді продовжуємо рух 
            // якщо ні тоді зупиняємося до зупинки і повертаємо дрон
            if(needDroneRotation(targetAngles[newTarget], curMyDrone.angularState, myDrone.radInIteration, myDrone.turnThreshold)) {
                if (curMyDrone.state == TURNING) {
                    // якщо  false ми зупиняємо поворот
                    if(!updateRotation(targetAngles[newTarget], curMyDrone.angularState, myDrone.radInIteration, myDrone.turnThreshold)) {
                        curMyDrone.state = ACCELERATING;
                    }
                } else {
                   
                    updateDronePosition(myDrone, curMyDrone);
                    curMyDrone.state = DECELERATING;
                    if (curMyDrone.speed <= 0) {
                        curMyDrone.state = TURNING;
                    }
                }   
                
            } else {
                curMyDrone.state = curMyDrone.speed <= myDrone.attackSpeed ? ACCELERATING : MOVING;
                updateRotation(targetAngles[newTarget], curMyDrone.angularState, myDrone.radInIteration);
                updateDronePosition(myDrone, curMyDrone);
                // збільшуємо швидкість
                curMyDrone.speed += (myDrone.acceleration * myDrone.simTimeStep);
                
                if (curMyDrone.speed >= myDrone.attackSpeed) {
                    curMyDrone.speed = myDrone.attackSpeed; 
                    curMyDrone.state = MOVING;
                }
                 
            }

        } else {
            // записуємо нові координати дрона що рухається 
            if(curMyDrone.state == ACCELERATING) {
                // перед тим ще паралельно руху до цілі будемо повертати дрон поки напряки цілі і дрону не співпадуть
                // останній параметр тобто поріг встановлюємо 0
                updateRotation(targetAngles[newTarget], curMyDrone.angularState, myDrone.radInIteration);
                updateDronePosition(myDrone, curMyDrone);
                curMyDrone.speed += (myDrone.acceleration * myDrone.simTimeStep);
             
                if (curMyDrone.speed >= myDrone.attackSpeed) {
                    curMyDrone.speed = myDrone.attackSpeed; 
                    curMyDrone.state = MOVING;
                }
            
            } else if(curMyDrone.state == TURNING) {
                // якщо resultRotation == false ми зупиняємо поворот
                if(!updateRotation(targetAngles[newTarget], curMyDrone.angularState, myDrone.radInIteration, myDrone.turnThreshold)) {
                    curMyDrone.state = ACCELERATING;
                }
            } else if(curMyDrone.state == MOVING) {
                // Рівномірний рух  з мінімальним обертанням якщо треба
                updateRotation(targetAngles[newTarget], curMyDrone.angularState, myDrone.radInIteration);
                updateDronePosition(myDrone, curMyDrone);
            }       
        }
                
        curMyDrone.target = newTarget;
    
        // ################ the end ################
         
        counter++;

        if (counter > MAX_STEPS) {
            LOG("============== спрацював автомат ми перевищили ліміт циклів ==============");
            break;
        }
    }
    
    saveOutputFileByStep(counter - 1, steps);

    // видаляжмо масиви цілей
    for (int i = 0; i < targetCount; i++) {
	    delete[] targets[i];
    }

    delete[] targets;

    // видаляємо масив точок
    delete[] steps;

    // видаляємо масив боєприпасів
    delete[] ammoTable;

    return 0;
}