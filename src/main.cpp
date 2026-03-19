#include "esp_log.h"
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "system/sys_mgr.h"
#include "system/boot_ctrl.h"
#include "managers/input_mgr.h"
#include "services/wifi_ap.h"
#include "services/web_server.h"
#include "ui/ui_task.h"

extern "C" void app_main(void) {
    printf("--- ESP32-S3 Custom Modular Launcher ---\n");

    /*
     * STEP 0: Boot decision controller.
     *
     * MUST run before any subsystem init. It may call esp_restart() to
     * redirect to user firmware. If it returns, we stay in the Launcher.
     * Requires NVS to be initialized first — sys_mgr_init() handles that.
     */
    sys_mgr_init();
    boot_ctrl_check(); /* Blocks up to 4s, may not return (redirects to ota_1) */

    // 2. Start GPIO Hardware Pollers
    input_mgr_init();

    // 3. Start Heavy UI Renderer
    ui_task_init();

    // 4. Initialize Wi-Fi AP Mode (Blocking until network stack is ready)
    wifi_ap_init();

    // 5. Start HTTP Server for OTA uploads
    web_server_init();

    /*
     * STEP FINAL: Mark the Launcher as valid and reset boot-loop counter.
     * Required for CONFIG_BOOTLOADER_APP_ROLLBACK_ENABLE safety.
     */
    boot_ctrl_mark_valid();

    // The main task can now sleep — everything else is event-driven
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(10000));
    }
}