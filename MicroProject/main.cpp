#include <iostream>
#include <thread>
#include <semaphore.h>
#include <condition_variable>
#include <mutex>

int const countOfRooms = 30; //Количество комнат в отеле
//Все время в миллисекундах
int const minTravelTime = 100; //Минимальное время в дороге к отелю
int const maxTravelTime = 35000; //Максимальное время в дороге к отелю
int const oneDay = 5000; //Длительность одного дня

sem_t hotel; //Отель
std::condition_variable newDay; //Условная переменная отвечающая за начало дня
std::mutex messageMutex; //Мьютекс для корректного вывода
std::mutex mtx; //Мьютекс для локера, передаваемого в условную переменную
int countThreadLeaveFromHotel = 0; //Количество гостей, посетивших отель

void clientThread(int id) {
    srand(id * time(0));
    //Имитируем долгую дорогу гостя к отелю
    std::this_thread::sleep_for(std::chrono::milliseconds(rand() % (maxTravelTime -
                                                                    minTravelTime) + minTravelTime));
    messageMutex.lock();
    printf("[Client %d]\tПрибыл в отель\n", id);
    messageMutex.unlock();
    int countFreeRooms;
    sem_getvalue(&hotel, &countFreeRooms); //Узнаем количество мест в отеле
    if (countFreeRooms <= 0) { //Если мест нет, то остаемся на улице
        messageMutex.lock();
        printf("[Client %d]\tПридется ночевать на улице...\n", id);
        messageMutex.unlock();
    }
    sem_wait(&hotel); //Ожидаем своей очереди
    messageMutex.lock();
    printf("[Client %d]\tЗаселился в отель\n", id);
    messageMutex.unlock();
    std::unique_lock<std::mutex> lock(mtx);
    newDay.wait(lock); //Ждем начала следующего дня
    lock.unlock();
    messageMutex.lock();
    printf("[Client %d]\tГотовится к отъезду\n", id);
    messageMutex.unlock();
    std::this_thread::sleep_for(std::chrono::milliseconds(rand() % oneDay)); //Имитируем сборы гостя
    messageMutex.lock();
    printf("[Client %d]\tВыселяется из отеля\n", id);
    messageMutex.unlock();
    sem_post(&hotel);
    countThreadLeaveFromHotel++;
}

/**
 * Считывает число в отрезке [minValue, maxValue]
 * @param num ссылка по которой будет записано значение
 * @param minValue минимальное значение
 * @param maxValue максимальное значение
 */
void readNumber(int& num, int minValue, int maxValue = INT_MAX) {
    std::cin >> num;
    while (num < minValue || num > maxValue) {
        if (num > maxValue)
            std::cout << "[Main Thread]\tСлишком большое количество посетителей, у нас так много не бывает" << std::endl;
        else
            std::cout << "[Main Thread]\tНу хотя бы один человек к нам-то уж приезжает" << std::endl;
        std::cout << "[Main Thread]\tВведите число гостей еще раз:";
        std::cin >> num;
    }
}

int main() {
    setlocale(LC_ALL, "Russian");
    sem_init(&hotel, 0, countOfRooms);

    printf("[Main Thread]\tВ отеле %d комнат\n", countOfRooms);
    int countOfClients;
    std::cout << "[Main Thread]\tВведите количество ожидаемых гостей:";
    readNumber(countOfClients, 1, 1000);

    std::thread* clients = new std::thread[countOfClients];
    for (int i = 0; i < countOfClients; ++i)
        clients[i] = std::thread(clientThread, i + 1);

    int dayNum = 1;
    while (countThreadLeaveFromHotel < countOfClients) {
        messageMutex.lock();
        printf("\n[Main Thread]\tДень %d\n", dayNum);
        messageMutex.unlock();
        newDay.notify_all();
        std::this_thread::sleep_for(std::chrono::milliseconds(oneDay));
        dayNum++;
    }

    for (int i = 0; i < countOfClients; ++i)
        clients[i].join();

    delete[] clients;
    sem_destroy(&hotel);

    return 0;
}
