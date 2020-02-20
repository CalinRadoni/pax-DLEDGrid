/**
This file is part of pax-devices (https://github.com/CalinRadoni/pax-devices)
Copyright (C) 2019+ by Calin Radoni

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_system.h"

#include <string.h>

#include "driver/ledc.h"

#include "BoardDev34.h"

#include "ESP32SimpleOTA.h"
#include "DStrip.h"
#include "DLEDController.h"
#include "ESP32Timers.h"
#include "HTTPC2Server.h"

#include "sdkconfig.h"

const char* TAG = "Init";

static const uint16_t cfgLEDcount = (uint16_t)(CONFIG_NUMBER_OF_LEDS & 0xFFFF);
static const uint8_t  cfgMaxCCV = (uint8_t)(CONFIG_MAX_LED_COLOR_VALUE & 0xFF);

static const uint32_t dTimerPeriod = 5; // ms
static const uint32_t dTimerPPS = 1000 / dTimerPeriod;

// TODO: Make a class to control the onboard LED based on the driver/ledc API

BoardDev34 board;
DStrip strip;
DLEDController LEDcontroller;
ESP32RMTChannel rmt0;
HTTPC2Server theHTTPServer;

// TODO Enumerate task like here https://docs.espressif.com/projects/esp-idf/en/release-v4.0/api-reference/system/freertos.html#_CPPv420uxTaskGetSystemStatePC12TaskStatus_tK11UBaseType_tPC8uint32_t

extern "C" {

    static void TimerTask(void *taskParameter) {
        uint32_t animationTick = 0;
        uint32_t secondTick = 0;
        ESP32TimerEvent timerEvent;
        uint16_t step = 64;
        uint16_t keyPressCount = 0;

        strip.Create(3, cfgLEDcount, cfgMaxCCV);
        rmt0.Initialize((rmt_channel_t)0, (gpio_num_t)14, cfgLEDcount * 24);
        rmt0.ConfigureForWS2812x();
        LEDcontroller.SetLEDType(LEDType::WS2812);

        board.PowerOn();

        strip.SetPixel(27, 1, 0, 0);
        strip.SetPixel(28, 1, 1, 0);
        strip.SetPixel(36, 0, 0, 1);
        LEDcontroller.SetLEDs(strip.description.data, strip.description.dataLen, &rmt0);

        board.debouncer.SetUpdateTime(dTimerPeriod);
        board.debouncer.SetKeyRepeat(500, 50);

        for(;;) {
            if (xQueueReceive(timers.timerQueue, &timerEvent, portMAX_DELAY) == pdPASS) {
                if (timerEvent.group == timer_group_t::TIMER_GROUP_0) {
                    if (timerEvent.index == timer_idx_t::TIMER_0) {
                        // read and debounce onboard button
                        board.debouncer.Update(board.OnboardButtonPressed());

                        animationTick++;
                        if (animationTick >= 20) {
                            animationTick -= 20;
                            if (board.debouncer.IsDown()) {
                                keyPressCount = board.debouncer.GetCurrentPressCount();
                                for (uint16_t i = 0; i < cfgLEDcount; i++)
                                    strip.SetColorByIndex(i, step + keyPressCount);
                            }
                            else {
                                if (keyPressCount != 0) {
                                    step += keyPressCount;
                                    keyPressCount = 0;
                                }
                            }

                            // Display a frame
                            LEDcontroller.SetLEDs(strip.description.data, strip.description.dataLen, &rmt0);
                        }

                        secondTick++;
                        if (secondTick >= dTimerPPS) {
                            secondTick -= dTimerPPS;
                        }
                    }
                }
            }
        }

        // the next lines are here only for "completion"
        timers.DestroyTimer(timer_group_t::TIMER_GROUP_0, timer_idx_t::TIMER_0);
        timers.Destroy();
        vTaskDelete(NULL);
    }

    static void HTTPTask(void *taskParameter) {
        HTTPCommand httpCmd;

        for(;;) {
            if (xQueueReceive(theHTTPServer.serverQueue, &httpCmd, portMAX_DELAY) == pdPASS) {
                switch (httpCmd.command) {
                    case 0:
                        for (uint16_t i = 0; i < cfgLEDcount; i++)
                            strip.SetPixel(i, 0);
                        LEDcontroller.SetLEDs(strip.description.data, strip.description.dataLen, &rmt0);
                        break;
                    case 1:
                        for (uint16_t i = 0; i < cfgLEDcount; i++)
                            strip.SetPixel(i, httpCmd.data);
                        LEDcontroller.SetLEDs(strip.description.data, strip.description.dataLen, &rmt0);
                        break;

                    case 0xFE:
                        vTaskDelay (5000 / portTICK_PERIOD_MS);
                        esp_restart();
                        break;

                    default: break;
                }
            }
        }

        // the next lines are here only for "completion"
        vTaskDelete(NULL);
    }

    void app_main()
    {
        simpleOTA.CheckApplicationImage();

        esp_err_t err = board.Initialize();
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "Initialization failed !");
            board.DoNothingForever();
        }

        err = board.StartAP();
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "Failed to start AP mode !");
            board.DoNothingForever();
        }

        err = theHTTPServer.StartServer();
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "Failed to start HTTP server !");
            board.DoNothingForever();
        }

        if (!timers.Create()) {
            ESP_LOGE(TAG, "Failed to create the timers object !");
            board.DoNothingForever();
        }

        if (!timers.CreateTimer(timer_group_t::TIMER_GROUP_0, timer_idx_t::TIMER_0, dTimerPeriod, true, false)) {
            ESP_LOGE(TAG, "Failed to create the timer G0T0 !");
            board.DoNothingForever();
        }

        TaskHandle_t xHandleTimerTask = NULL;
        xTaskCreate(TimerTask, "Timer handling task", 2048, NULL, uxTaskPriorityGet(NULL) + 5, &xHandleTimerTask);
        if (xHandleTimerTask != NULL) {
            ESP_LOGI(TAG, "Timer task created.");
        }
        else {
            ESP_LOGE(TAG, "Failed to create the timer task !");
            board.DoNothingForever();
        }

        TaskHandle_t xHandleHTTP = NULL;
        xTaskCreate(HTTPTask, "HTTP command handling task", 2048, NULL, uxTaskPriorityGet(NULL) + 1, &xHandleHTTP);
        if (xHandleHTTP != NULL) {
            ESP_LOGI(TAG, "HTTP command handling task created.");
        }
        else {
            ESP_LOGE(TAG, "Failed to create the HTTP command handling task !");
            board.DoNothingForever();
        }
    }
}
