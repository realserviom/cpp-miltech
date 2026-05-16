.PHONY: format lint test quality

# Команда для форматування коду
format:
	find src include -name "*.cpp" -o -name "*.hpp" | xargs clang-format -i

# Команда для запуску лінтера
lint:
	clang-tidy -p build/ src/*.cpp -- -Iinclude

# Команда для запуску тестів (якщо вони є)
test:
	cd .. && ctest --test-dir build/debug/ --output-on-failure

# Одна команда для всього
quality: format lint test