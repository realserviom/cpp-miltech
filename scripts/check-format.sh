#!/bin/bash

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "$SCRIPT_DIR/includes/check.sh"

validate_homework_arg "$1"

TARGET_DIR=$1

echo -e "${BLUE}Перевірка в директорії: $TARGET_DIR${NC}"

echo -e "${BLUE}1. Форматування (clang-format)...${NC}"
# Тут ми шукаємо і .cpp і .hpp
find "$TARGET_DIR" \( -name "*.cpp" -o -name "*.hpp" \) | xargs clang-format -i

echo -e "${GREEN}Все чудово! Код готовий до PR.${NC}"