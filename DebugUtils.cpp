#include "DebugUtils.h"

const char* DebugUtils::getEventName(InputEvent event) {
    switch(event) {
        case InputEvent::UP_PRESS:          return "UP_PRESS";
        case InputEvent::DOWN_PRESS:        return "DOWN_PRESS";
        case InputEvent::OK_SHORT_PRESS:    return "OK_SHORT_PRESS";
        case InputEvent::OK_LONG_PRESS:     return "OK_LONG_PRESS";
        case InputEvent::BACK_PRESS:        return "BACK_PRESS";
        case InputEvent::STOP_PRESS:        return "STOP_PRESS";
        case InputEvent::MANUAL_MODE_PRESS: return "MANUAL_MODE_PRESS";
        case InputEvent::LOAD_PRESS:        return "LOAD_PRESS";
        case InputEvent::LONG_LOAD_PRESS:   return "LONG_LOAD_PRESS";
        case InputEvent::UNLOAD_PRESS:      return "UNLOAD_PRESS";
        case InputEvent::LONG_UNLOAD_PRESS: return "LONG_UNLOAD_PRESS";
        case InputEvent::LIFT_UP_PRESS:     return "LIFT_UP_PRESS";
        case InputEvent::LIFT_DOWN_PRESS:   return "LIFT_DOWN_PRESS";
        case InputEvent::DEMO_PRESS:        return "DEMO_PRESS";
        case InputEvent::KEY_1_PRESS:       return "KEY_1_PRESS";
        case InputEvent::KEY_2_PRESS:       return "KEY_2_PRESS";
        case InputEvent::KEY_3_PRESS:       return "KEY_3_PRESS";
        case InputEvent::KEY_4_PRESS:       return "KEY_4_PRESS";
        case InputEvent::KEY_A_PRESS:       return "KEY_A_PRESS";
        case InputEvent::KEY_B_PRESS:       return "KEY_B_PRESS";
        case InputEvent::NONE:              return "NONE";
        default:                            return "UNKNOWN_INPUT";
    }
}

const char* DebugUtils::getSystemEventName(SystemEvent event) {
    switch(event) {
        case SystemEvent::TELEMETRY_UPDATED:   return "TELEMETRY_UPDATED";
        case SystemEvent::SENSORS_UPDATED:     return "SENSORS_UPDATED";
        case SystemEvent::STATS_UPDATED:       return "STATS_UPDATED";
        case SystemEvent::CONFIG_UPDATED:      return "CONFIG_UPDATED";
        case SystemEvent::ERROR_OCCURRED:      return "ERROR_OCCURRED";
        case SystemEvent::BATTERY_LOW:         return "BATTERY_LOW";
        case SystemEvent::QUEUE_FULL:          return "QUEUE_FULL";
        case SystemEvent::QUEUE_OK:            return "QUEUE_OK";
        case SystemEvent::CONNECTION_LOST:     return "CONNECTION_LOST";
        case SystemEvent::CONNECTION_RESTORED: return "CONNECTION_RESTORED";
        case SystemEvent::LOCAL_BATT_UPDATED:  return "LOCAL_BATT_UPDATED";
        case SystemEvent::CMD_DISPATCHED:      return "CMD_DISPATCHED";
        case SystemEvent::CMD_ACKED:           return "CMD_ACKED";
        case SystemEvent::CMD_FAILED:          return "CMD_FAILED";
        default:                               return "UNKNOWN_SYS_EVENT";
    }
}

const char* DebugUtils::getMsgIdName(uint8_t msgId) {
    switch(msgId) {
        case SP::MSG_HEARTBEAT:     return "MSG_HEARTBEAT";
        case SP::MSG_SENSORS:       return "MSG_SENSORS";
        case SP::MSG_STATS:         return "MSG_STATS";
        case SP::MSG_REQ_HEARTBEAT: return "MSG_REQ_HEARTBEAT";
        case SP::MSG_REQ_SENSORS:   return "MSG_REQ_SENSORS";
        case SP::MSG_REQ_STATS:     return "MSG_REQ_STATS";
        case SP::MSG_LOG:           return "MSG_LOG";
        case SP::MSG_CONFIG_SET:    return "MSG_CONFIG_SET";
        case SP::MSG_CONFIG_GET:    return "MSG_CONFIG_GET";
        case SP::MSG_CONFIG_REP:    return "MSG_CONFIG_REP";
        case SP::MSG_CMD_SIMPLE:    return "MSG_CMD_SIMPLE";
        case SP::MSG_CMD_WITH_ARG:  return "MSG_CMD_WITH_ARG";
        case SP::MSG_ACK:           return "MSG_ACK";
        default:                    return "UNKNOWN_MSG_ID";
    }
}

const char* DebugUtils::getTxStateName(CommLink::TxState state) {
    switch(state) {
        case CommLink::TxState::IDLE:          return "IDLE";
        case CommLink::TxState::WAITING_ACK:   return "WAITING_ACK";
        case CommLink::TxState::TIMEOUT_ERROR: return "TIMEOUT_ERROR";
        default:                               return "UNKNOWN_STATE";
    }
}

const char* DebugUtils::getUICommandName(uint8_t cmdType) {
    switch (cmdType) {
        case SP::CMD_STOP:            return "Стоп";
        case SP::CMD_STOP_MANUAL:     return "Стоп (Р)";
        case SP::CMD_MOVE_RIGHT_MAN:  return "Вправо";
        case SP::CMD_MOVE_LEFT_MAN:   return "Влево";
        case SP::CMD_LIFT_UP:         return "Подъем";
        case SP::CMD_LIFT_DOWN:       return "Спуск";
        case SP::CMD_LOAD:            return "Загрузка";
        case SP::CMD_UNLOAD:          return "Выгрузка";
        case SP::CMD_MOVE_DIST_R:     return "Движение";
        case SP::CMD_MOVE_DIST_F:     return "Движение";
        case SP::CMD_CALIBRATE:       return "Калибр.";
        case SP::CMD_DEMO:            return "Демо";
        case SP::CMD_COUNT_PALLETS:   return "Счет пал.";
        case SP::CMD_SAVE_EEPROM:     return "Сохр. настр.";
        case SP::CMD_COMPACT_F:       return "Уплотнение";
        case SP::CMD_COMPACT_R:       return "Уплотнение";
        case SP::CMD_GET_CONFIG:      return "Чтение настр.";
        case SP::CMD_EVACUATE_ON:     return "Эвакуация";
        case SP::CMD_LONG_LOAD:       return "Длин. Загр.";
        case SP::CMD_LONG_UNLOAD:     return "Длин. Выгр.";
        case SP::CMD_LONG_UNLOAD_QTY: return "Длин. Выгр.";
        case SP::CMD_RESET_ERROR:     return "Сброс ош.";
        case SP::CMD_MANUAL_MODE:     return "Ручной реж.";
        case SP::CMD_LOG_MODE:        return "Лог реж.";
        case SP::CMD_HOME:            return "Домой";
        case SP::CMD_FIRMWARE_UPDATE: return "Обновление";
        case SP::CMD_SYSTEM_RESET:    return "Сброс сист.";
        default:                      return "Команда";
    }
}