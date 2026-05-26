#!/bin/bash

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "$SCRIPT_DIR/includes/check.sh"

validate_homework_arg "$1"

TARGET_DIR=$1

echo -e "${BLUE}Перевірка в директорії: $TARGET_DIR${NC}"

echo -e "${BLUE}3. Запуск тестів (ctest)...${NC}"

# TARGET_DIR — це, наприклад, homework_06
# Ми конструюємо шлях до папки з тестами автоматично
TEST_PATH="build/debug/$TARGET_DIR"

if [ -d "$TEST_PATH" ]; then
    echo "Запуск тестів у: $TEST_PATH"
    ctest --test-dir "$TEST_PATH" --output-on-failure
else
    echo -e "${YELLOW}Попередження: Тестів для $TARGET_DIR не знайдено за шляхом $TEST_PATH${NC}"
    # Можна не переривати скрипт, якщо тести ще не написані
fi

echo -e "${GREEN}Все чудово! Код готовий до PR.${NC}"