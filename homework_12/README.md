1. sim/.env - можна добавити свою API якщо у WSL
2. cd sim &&  docker compose -f compose.sitl.yml up -d
3. cd edge && docker compose up -d 
4. перевіряємо файл  edge/logs/c2.log
5. Запускаємо QGroundControl для тестування