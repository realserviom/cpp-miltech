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

echo -e "${BLUE}1. Форматування (clang-format)...${NC}"
# Тут ми шукаємо і .cpp і .hpp
find "$TARGET_DIR" \( -name "*.cpp" -o -name "*.hpp" \) | xargs clang-format -i

echo -e "${GREEN}Все чудово! Код готовий до PR.${NC}"