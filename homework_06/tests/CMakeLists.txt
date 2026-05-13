# Тестуємо програму за допомогою тесту CMake. Це основий тест який показує що файл працездатний.
add_test(NAME IntegrationDroneAttack
         COMMAND drone_attack "${CMAKE_SOURCE_DIR}/homework_06/data/fixtures/test_ammo.txt"
                 "${CMAKE_SOURCE_DIR}/homework_06/data/output.txt")

# Додатковий тест, який порівнює результат із файлом очікування expected_output.txt
add_test(
    NAME CheckOutputFiles
    COMMAND ${CMAKE_COMMAND} -E compare_files "${CMAKE_SOURCE_DIR}/homework_06/data/output.txt"
            "${CMAKE_SOURCE_DIR}/homework_06/data/fixtures/test_expected_output.txt")

# Тестуємо програму за допомогою тесту CMake. Це тест який показує що файл працездатний при 0 висоті
# дрона
add_test(NAME IntegrationDroneAttackZDEqZero
         COMMAND drone_attack "${CMAKE_SOURCE_DIR}/homework_06/data/fixtures/test_ammo_zd0.txt"
                 "${CMAKE_SOURCE_DIR}/homework_06/data/output.txt")

# Тестуємо програму за допомогою тесту CMake. Це тест який показує що файл працездатний при відємній
# висоті дрона
add_test(
    NAME IntegrationDroneAttackZDLessZero
    COMMAND drone_attack "${CMAKE_SOURCE_DIR}/homework_06/data/fixtures/test_ammo_zd_less0.txt"
            "${CMAKE_SOURCE_DIR}/homework_06/data/output.txt")

# Тестуємо програму за допомогою тесту CMake. Це тест який показує що файл працездатний при нульовій
# відстані до цілі
add_test(NAME IntegrationDroneAttackLengthEqZero
         COMMAND drone_attack "${CMAKE_SOURCE_DIR}/homework_06/data/fixtures/test_ammo_length0.txt"
                 "${CMAKE_SOURCE_DIR}/homework_06/data/output.txt")

set_tests_properties(CheckOutputFiles PROPERTIES DEPENDS IntegrationDroneAttack)

set_tests_properties(IntegrationDroneAttack PROPERTIES COST 100)

# --- Додаємо тести бібліотеки ballistics ---
add_executable(ballistics_test ballistics_test.cpp)

# Лінкуємо нашу лібу + GTest
target_link_libraries(ballistics_test PRIVATE ballistics GTest::gtest_main)

# Передаємо шлях до папки з даними (те, що ми обговорювали)
target_compile_definitions(
    ballistics_test PRIVATE DATA_PATH="${CMAKE_SOURCE_DIR}/homework_06/data/"
                            FIXTURES_PATH="${CMAKE_SOURCE_DIR}/homework_06/data/fixtures/")

# Реєструємо функції автоматизації щоб можна було реєструвати тести в системі CTest
include(GoogleTest)

# gtest_discover_tests аналізує скомпільований бінарник, автоматично витягує з нього список усіх
# TEST() і реєструє кожен із них у CTest як окремий тест.
gtest_discover_tests(ballistics_test)
