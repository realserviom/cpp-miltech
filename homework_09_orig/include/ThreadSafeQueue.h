#pragma once

#include <queue>
#include <mutex>
#include <condition_variable>
#include <memory>

template <typename T>
class ThreadSafeQueue {
private:
    std::queue<T> m_queue;                 // Звичайна стандартна черга всередині
    mutable std::mutex m_mutex;            // М'ютекс для захисту даних
    std::condition_variable m_cond_var;    // Сигналізація для потоків-читачів

public:
    ThreadSafeQueue() = default;
    
    // Забороняємо копіювання черги, щоб уникнути багів з потоками
    ThreadSafeQueue(const ThreadSafeQueue&) = delete;
    ThreadSafeQueue& operator=(const ThreadSafeQueue&) = delete;

    // Додати елемент в чергу (Потік-відправник)
    void push(T newValue) {
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_queue.push(std::move(newValue));
        }
        m_cond_var.notify_one(); // Сповіщаємо один із потоків, що чекають
    }

    // Зачекати та дістати елемент (Потік-отримувач / Дрон)
    // Цей метод блокує потік, якщо черга порожня, і «засинає» без навантаження на процесор
    void wait_and_pop(T& value) {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_cond_var.wait(lock, [this] { return !m_queue.empty(); });
        value = std::move(m_queue.front());
        m_queue.pop();
    }

    // Альтернативний wait_and_pop, який повертає smart-pointer (зручно для об'єктів команд)
    std::unique_ptr<T> wait_and_pop() {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_cond_var.wait(lock, [this] { return !m_queue.empty(); });
        
        std::unique_ptr<T> res(std::make_unique<T>(std::move(m_queue.front())));
        m_queue.pop();
        return res;
    }

    // Спробувати дістати елемент без очікування (якщо порожня — повертає false)
    bool try_pop(T& value) {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_queue.empty()) {
            return false;
        }
        value = std::move(m_queue.front());
        m_queue.pop();
        return true;
    }

    // Перевірка чи черга порожня
    bool empty() const {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_queue.empty();
    }

    // Поточний розмір черги
    size_t size() const {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_queue.size();
    }
};