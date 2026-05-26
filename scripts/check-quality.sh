#!/bin/bash

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "$SCRIPT_DIR/includes/check.sh"

validate_homework_arg "$1"

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