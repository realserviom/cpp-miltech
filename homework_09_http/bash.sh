#!/usr/bin/env bash

# Переривати виконання при помилках копіювання чи відсутності директорій
set -e

DATA_DIR="data"
TASKS_DIR="data/all_tasks_data"
EXECUTABLE="./build/targets3"

# Перевірка наявності бінарника перед запуском
if [ ! -f "$EXECUTABLE" ]; then
    echo "[ERROR] Бінарник $EXECUTABLE не знайдено! Спочатку збереіть проєкт (cmake --build build)."
    exit 1
fi

echo "=========================================="
echo "Запуск автоматичного прогону тестів T01-T10"
echo "=========================================="

# Цикл від 1 до 10
for i in $(seq 1 10); do
    TEST_ID=$(printf "T%02d" $i)
    SOURCE_DIR="$TASKS_DIR/$TEST_ID"

    echo ""
    echo "------------------------------------------"
    echo ">>> Обробка тесту: $TEST_ID"
    echo "------------------------------------------"

    # Перевірка, чи існує папка із вхідними даними
    if [ -d "$SOURCE_DIR" ]; then
        echo "Копіювання файлів з $SOURCE_DIR -> $DATA_DIR/ ..."
        cp -fv "$SOURCE_DIR"/* "$DATA_DIR/"
    else
        echo "[WARNING] Папку $SOURCE_DIR не знайдено, пропуск..."
        continue
    fi

    echo "Запуск $EXECUTABLE $TEST_ID..."
    $EXECUTABLE "$TEST_ID" || echo "[ERROR] Тест $TEST_ID завершився з помилкою."

done

echo ""
echo "=========================================="
echo "Всі тести оброблено!"
echo "=========================================="