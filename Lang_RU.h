#pragma once
#include <Arduino.h>

constexpr const char* TXT_CHARGE        = "Заряд";
constexpr const char* TXT_LIFO          = "LIFO";
constexpr const char* TXT_FIFO          = "FIFO";
constexpr const char* TXT_LEFT_TO_UNLOAD= "Осталось выгрузить %d";
constexpr const char* TXT_QUEUE_FULL    = "! ОЧЕРЕДЬ ЗАПОЛНЕНА !";
constexpr const char* TXT_ERR_PREFIX    = "! ОШИБКА %d !";
constexpr const char* TXT_NO_ERRORS     = "Нет ошибок";
constexpr const char* TXT_MANUAL_CMD    = "Ручная команда";

static const char* const SHUTTLE_STATUS_STRINGS[] = {
    "Запрос статуса", "Ручной режим", "Загрузка", "Выгрузка",
    "Уплотнение", "Эвакуация", "DEMO", "Подсчет паллет",
    "Испытания", "Обнаружены ошибки", "Ожидание...", "Прод. загрузка",
    "Прод. выгрузка", "Прод. выгрузка", "  Вперед...", "  Назад...",
    "  Вверх...", "  Вниз...", "  Инициация..."
};

static const char* const ERROR_STRINGS[] = {
    "Нет ошибок", "Сенсор канала F", "Сенсор канала R", "Сенсор канала DF",
    "Сенсор канала DR", "Сенсор паллет F", "Сенсор паллет R", "Сенсор паллет DF",
    "Сенсор паллет DR", "Подъемник", "Привод движ.", "Низкий заряд",
    "Столкновение", "Перегрев", "", ""
};