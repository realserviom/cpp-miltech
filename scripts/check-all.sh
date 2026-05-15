#!/bin/bash

set -e

SCRIPT_DIR=$(dirname "$(readlink -f "$0")")

GREEN='\033[0;32m'
BLUE='\033[0;34m'
NC='\033[0m'

if [ -z "$1" ]; then
    echo -e "\033[0;31mПомилка: Не вказано папку домашнього завдання!${NC}"
    echo "Використання: $0 homework_06"
    exit 1
fi

TARGET_DIR=$1

bash "$SCRIPT_DIR/check-format.sh" "$TARGET_DIR" 
bash "$SCRIPT_DIR/check-quality.sh" "$TARGET_DIR" 
bash "$SCRIPT_DIR/check-test.sh" "$TARGET_DIR"