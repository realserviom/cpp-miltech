#!/bin/bash

set -e

GREEN='\033[0;32m'
BLUE='\033[0;34m'
NC='\033[0m'

if [ -z "$1" ]; then
    echo -e "\033[0;31mПомилка: Не вказано папку домашнього завдання!${NC}"
    echo "Використання: $0 homework_06"
    exit 1
fi

TARGET_DIR=$1

echo -e "${BLUE}Перевірка в директорії: $TARGET_DIR${NC}"

echo -e "${BLUE}2. Лінтер (clang-tidy)...${NC}"
CPP_FILES=$(find "$TARGET_DIR/src" -name "*.cpp")

for file in $CPP_FILES; do
    echo "Аналіз $file..."
    # -p build/ бере налаштування компіляції з CMake
    # -header-filter дозволяє бачити помилки в .hpp
    # -I вказує шлях до заголовків конкретної папки
    clang-tidy -p build/ --warnings-as-errors='*' "$file" \
               -header-filter="$TARGET_DIR/.*" \
               -- -I"$TARGET_DIR/include"
done

echo -e "${GREEN}Все чудово! Код готовий до PR.${NC}"