// Board bring-up: print what the firmware sees before adding any sensor code.

#include <inttypes.h>

#include "sdkconfig.h"
#include "esp_chip_info.h"
#include "esp_flash.h"
#include "esp_idf_version.h"
#include "esp_log.h"
#include "esp_psram.h"

static const char *TAG = "bringup";

void app_main(void)
{
    esp_chip_info_t info;
    esp_chip_info(&info);

    // esp_flash_get_size() only returns the size written into the image header
    // (CONFIG_ESPTOOLPY_FLASHSIZE). The physical size comes from the flash chip ID.
    uint32_t flash_phys = 0;
    uint32_t flash_cfg = 0;
    if (esp_flash_get_physical_size(NULL, &flash_phys) != ESP_OK) {
        ESP_LOGW(TAG, "could not read physical flash size");
    }
    if (esp_flash_get_size(NULL, &flash_cfg) != ESP_OK) {
        ESP_LOGW(TAG, "could not read configured flash size");
    }

    ESP_LOGI(TAG, "ESP-IDF %s", esp_get_idf_version());
    ESP_LOGI(TAG, "target %s, %d core(s), chip revision v%d.%d",
             CONFIG_IDF_TARGET, info.cores, info.revision / 100, info.revision % 100);
    ESP_LOGI(TAG, "flash %" PRIu32 " MB (build set to %" PRIu32 " MB)",
             flash_phys / (1024 * 1024), flash_cfg / (1024 * 1024));
    ESP_LOGI(TAG, "psram %u MB", (unsigned)(esp_psram_get_size() / (1024 * 1024)));
}
