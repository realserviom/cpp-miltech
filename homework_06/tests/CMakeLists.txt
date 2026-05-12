# Тестуємо програму за допомогою тесту CMake. Це основий тест який показує що файл працездатний.
add_test(NAME IntegrationDroneAttack
         COMMAND drone_attack "${CMAKE_SOURCE_DIR}/homework_06/data/ammo_m67.txt"
                 "${CMAKE_SOURCE_DIR}/homework_06/data/output.txt")

# Додатковий тест, який порівнює результат із файлом очікування expected_output.txt
add_test(
    NAME CheckOutputFiles
    COMMAND ${CMAKE_COMMAND} -E compare_files "${CMAKE_SOURCE_DIR}/homework_06/data/output.txt"
            "${CMAKE_SOURCE_DIR}/homework_06/data/expected_output.txt")

set_tests_properties(CheckOutputFiles PROPERTIES DEPENDS IntegrationDroneAttack)

set_tests_properties(IntegrationDroneAttack PROPERTIES COST 100)

# --- Додаємо тести бібліотеки ballistics ---
add_executable(ballistics_test ballistics_test.cpp)

# Лінкуємо нашу лібу + GTest
target_link_libraries(ballistics_test PRIVATE ballistics GTest::gtest_main)

# Реєструємо функції автоматизації щоб можна було реєструвати тести в системі CTest
include(GoogleTest)

# gtest_discover_tests аналізує скомпільований бінарник, автоматично витягує з нього список усіх
# TEST() і реєструє кожен із них у CTest як окремий тест.
gtest_discover_tests(ballistics_test)
