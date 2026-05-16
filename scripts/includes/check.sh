#!/bin/bash

GREEN='\033[0;32m'
BLUE='\033[0;34m'
RED='\033[0;31m'
NC='\033[0m'

log_error() {
    echo -e "${RED}Помилка:${NC} $1"
}

validate_homework_arg() {
    if [ -z "$1" ]; then
        log_error "Не вказано папку домашнього завдання!"
        echo "Використання: $0 homework_06"
        exit 1
    fi
}