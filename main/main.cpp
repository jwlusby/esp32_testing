#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "Arduino.h"


#include "esp_wifi.h"
#include "esp_system.h"
#include "esp_event.h"
#include "esp_event_loop.h"

#include "esp_log.h"

#include "nvs_flash.h"
#include "driver/gpio.h"

#include "esp_spi_flash.h"
#include "esp_partition.h"
#include "spiffs.h"
#include "esp_spiffs.h"
#include "spiffs_vfs.h"



#define LOG_LOCAL_LEVEL ESP_LOG_DEBUG

int ledPin = 5;
//0x10000 + (1024*512) + (1024*512) + (1024*512) to hex
#define NVS_PARTITION_START     0x9000
#define NVS_PARTITION_SIZE      0x6000
#define NVS_PHY_START           0xF000
#define NVS_PHY_SIZE            0x1000
#define FACTORY_PARTITION_START 0x10000
#define FACTORY_PARTITION_SIZE  1024*512

#define OTA1_PARTITION_START 	FACTORY_PARTITION_START + FACTORY_PARTITION_SIZE
#define OTA1_PARTITION_SIZE  	FACTORY_PARTITION_SIZE

#define OTA2_PARTITION_START 	OTA1_PARTITION_START + OTA1_PARTITION_SIZE
#define OTA2_PARTITION_SIZE  	FACTORY_PARTITION_SIZE

#define DATA_PARTITION_START 	OTA2_PARTITION_START + OTA2_PARTITION_SIZE
#define DATA_PARTITION_SIZE  	1024*256

#define SECTOR_SIZE             4096
#define PAGE_SIZE               256
#define MEMORY_SIZE             DATA_PARTITION_SIZE
#define MAX_CONCURRENT_FDS      4
static uint8_t spiffs_work_buf[PAGE_SIZE * 2];
static uint8_t spiffs_fds[32 * MAX_CONCURRENT_FDS];
static uint8_t spiffs_cache_buf[(PAGE_SIZE + 32) * 4];
static spiffs fs;
static spiffs_config cfg;

static uint32_t g_wbuf[SECTOR_SIZE / 4];
static uint32_t g_rbuf[SECTOR_SIZE / 4];


void configure_spiffs(){
	// configure the filesystem
	  cfg.phys_size = MEMORY_SIZE;
	  cfg.phys_addr = DATA_PARTITION_START;
	  cfg.phys_erase_block = SECTOR_SIZE;
	  cfg.log_block_size = SECTOR_SIZE;
	  cfg.log_page_size = PAGE_SIZE;
	  cfg.hal_read_f = esp32_spi_flash_read;
	  cfg.hal_write_f = esp32_spi_flash_write;
	  cfg.hal_erase_f = esp32_spi_flash_erase;
}

//Test file read/write using VFS component & spiffs (fopen/fread/fwrite)
void test_memory_vfs()
{
	char tag[] = "test_memory_vfs()";
  printf("Memory test starting...\n");

  int ret;

  configure_spiffs();


  bool forceFormat = true;

  // mount the filesystem
  ESP_LOGI(tag, "Mounting SPIFFS");
  ret = SPIFFS_mount(&fs, &cfg, spiffs_work_buf, spiffs_fds, sizeof(spiffs_fds), spiffs_cache_buf, sizeof(spiffs_cache_buf), NULL);
  if (ret != SPIFFS_OK || forceFormat)
  {
    ESP_LOGI(tag, "Mounting SPIFFS failed with result=%i", ret);


    ESP_LOGI(tag, "Unmounting SPIFFS...");
    SPIFFS_unmount(&fs);


    ESP_LOGI(tag, "Formatting SPIFFS...");
    ret = SPIFFS_format(&fs);
    ESP_LOGI(tag, "   result=%i", ret);

    ESP_LOGI(tag, "Mounting SPIFFS...");
    ret = SPIFFS_mount(&fs, &cfg, spiffs_work_buf, spiffs_fds, sizeof(spiffs_fds), spiffs_cache_buf, sizeof(spiffs_cache_buf), NULL);
    ESP_LOGI(tag, "   result=%i", ret);
  }
  else if (ret == SPIFFS_OK)
  {
    ESP_LOGI(tag, "SPIFFS mounted");
  }

  char fname[]="/data/myfile";
  spiffs_registerVFS("/data", &fs);
  ESP_LOGI(tag, "vfs registered");


  FILE* file = fopen(fname, "w");
  if (file <= NULL)
  {
    ESP_LOGE(tag, "failed to open file '%s'. result=0x%X",fname, (uint32_t)file);
    return;
  }
  else
  {
    ESP_LOGE(tag, "Opened file '%s'. file=0x%X", fname, (uint32_t)file);

    char teststring[]="Hello!";
    int nbytes = fwrite(teststring, 1, 6, file);
    if(nbytes==-1){
      ESP_LOGE(tag, "Failed to write to file. result=%i", nbytes);
    }
    else{
      ESP_LOGI(tag, "Wrote %i bytes to file from buffer at 0x%X", nbytes, (uint32_t)teststring);

      //ret = fflush(file);
      //ESP_LOGI(tag, "fflush=%i", ret);

      ret = fclose(file);
      ESP_LOGI(tag, "fclose=%i", ret);

      FILE* file2 = fopen(fname, "r");

      if(file2 > NULL){
        //ret = fseek(file2, 0, SEEK_SET);
        //ESP_LOGI(tag, "fseek=%i", ret);

        char *buf;
        buf = (char*)malloc(64);
        memset(buf, 0, 64);

        ESP_LOGI(tag, "Attempting to read 6 bytes from file 0x%X into buffer at 0x%X.", (uint32_t)file2, (uint32_t)buf);
        nbytes = fread(buf,1,6,file2);
        ESP_LOGI(tag, "Read %i bytes from file 0x%X into buffer at 0x%X: '%s', ", nbytes, (uint32_t)file2, (uint32_t)buf, buf);
        fclose(file2);
        free(buf);
      }
      else{
        ESP_LOGE(tag, "Failed to open file!");
      }
    }
  }

  printf("Memory test complete!\n");
}

//Test file read/write using just the spiffs file access functions (no vfs)
void test_memory_spiffs_raw()
{
  char tag[] = "test_memory_spiffs_raw";
  printf("Memory test starting...\n");

  int ret;
  bool forceFormat = true;

  configure_spiffs();

  // mount the filesystem
  ESP_LOGI(tag, "Mounting SPIFFS");
  ret = SPIFFS_mount(&fs, &cfg, spiffs_work_buf, spiffs_fds, sizeof(spiffs_fds), spiffs_cache_buf, sizeof(spiffs_cache_buf), NULL);
  if (ret != SPIFFS_OK || forceFormat)
  {
    ESP_LOGI(tag, "Mounting SPIFFS failed with result=%i", ret);


    ESP_LOGI(tag, "Unmounting SPIFFS...");
    SPIFFS_unmount(&fs);


    ESP_LOGI(tag, "Formatting SPIFFS...");
    ret = SPIFFS_format(&fs);
    ESP_LOGI(tag, "   result=%i", ret);

    ESP_LOGI(tag, "Mounting SPIFFS...");
    ret = SPIFFS_mount(&fs, &cfg, spiffs_work_buf, spiffs_fds, sizeof(spiffs_fds), spiffs_cache_buf, sizeof(spiffs_cache_buf), NULL);
    ESP_LOGI(tag, "   result=%i", ret);
  }
  else if (ret == SPIFFS_OK)
  {
    ESP_LOGI(tag, "SPIFFS mounted");
  }


  //spiffs_registerVFS("/data", &fs);
  //ESP_LOGI(tag, "vfs registered");

  char fname[]="myfile";
  spiffs_file file = SPIFFS_open(&fs, fname, SPIFFS_CREAT|SPIFFS_TRUNC|SPIFFS_RDWR, NULL);
  if (file <= NULL)
  {
    ESP_LOGE(tag, "failed to open file '%s'. result=%i",fname, file);
    return;
  }
  else
  {
    ESP_LOGE(tag, "Opened file '%s'. fd=%i", fname, file);

    int nbytes = SPIFFS_write(&fs, file, (void*)"Hello!", 6);
    if(nbytes==-1){
      ESP_LOGE(tag, "Failed to write to file. result=%i", nbytes);
    }
    else{
      ESP_LOGI(tag, "Wrote %i bytes to file", nbytes);

      //ret = fflush(file);
      //ESP_LOGI(tag, "fflush=%i", ret);

      ret = SPIFFS_close(&fs, file);
      ESP_LOGI(tag, "fclose=%i", ret);

      file = SPIFFS_open(&fs,fname, SPIFFS_RDWR, NULL);

      if(file > NULL){
        //ret = fseek(file, 0, SEEK_SET);
        //ESP_LOGI(tag, "fseek=%i", ret);

        char *buf;
        buf = (char*)malloc(64);
        memset(buf, 0, 64);
        ESP_LOGI(tag, "Reading from file: buffer=0x%X, file=0x%X", (uint32_t)buf, file);
        nbytes = SPIFFS_read(&fs, file, buf, 6);
        ESP_LOGI(tag, "Read %i bytes from file: '%s', buffer=0x%X, file=0x%X", nbytes, buf, (uint32_t)buf,(uint32_t)file);
        free(buf);
      }
      else{
        ESP_LOGE(tag, "Failed to open file!");
      }
    }
  }

  printf("Memory test complete!\n");
}


void readWriteTask(void *pvParameters)
{
    srand(0);

    for (uint32_t base_addr = 0x200000;
         base_addr < 0x300000;
         base_addr += SECTOR_SIZE)
    {
        delay(500);

        printf("erasing sector %x\n", base_addr / SECTOR_SIZE);
        spi_flash_erase_sector(base_addr / SECTOR_SIZE);

        for (int i = 0; i < sizeof(g_wbuf)/sizeof(g_wbuf[0]); ++i) {
            g_wbuf[i] = rand();
        }

        printf("writing at %x\n", base_addr);
        spi_flash_write(base_addr, g_wbuf, sizeof(g_wbuf));

        memset(g_rbuf, 0, sizeof(g_rbuf));
        printf("reading at %x\n", base_addr);
        spi_flash_read(base_addr, g_rbuf, sizeof(g_rbuf));
        for (int i = 0; i < sizeof(g_rbuf)/sizeof(g_rbuf[0]); ++i) {
            if (g_rbuf[i] != g_wbuf[i]) {
                printf("failed writing or reading at %d\n", base_addr + i * 4);
                printf("got %08x, expected %08x\n", g_rbuf[i], g_wbuf[i]);
                return;
            }
        }

        printf("done at %x\n", base_addr);
        base_addr += SECTOR_SIZE;
    }

    printf("read/write/erase test done\n");
    //vTaskDelete(NULL);
}


esp_err_t event_handler(void *ctx, system_event_t *event)
{
    return ESP_OK;
}


/*
void setup(){
	Serial.begin(115200);
}*/

extern "C" void app_main(void)
{
	/*
	ESP_LOGI(tag,"Hello from ESP-IDF!")
	Serial.println("Hello from Arduino on ESP32!");
	delay(1000);
	esp_restart();
	*/

	char tag[]="app_main";
	initArduino();
	Serial.begin(115200);

    nvs_flash_init();
    /*
    tcpip_adapter_init();
    ESP_ERROR_CHECK( esp_event_loop_init(event_handler, NULL) );
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK( esp_wifi_init(&cfg) );
    ESP_ERROR_CHECK( esp_wifi_set_storage(WIFI_STORAGE_RAM) );
    ESP_ERROR_CHECK( esp_wifi_set_mode(WIFI_MODE_STA) );
    wifi_config_t sta_config = {
        .sta = {
            .ssid = "access_point_name",
            .password = "password",
            .bssid_set = false
        }
    };
    ESP_ERROR_CHECK( esp_wifi_set_config(WIFI_IF_STA, &sta_config) );
    ESP_ERROR_CHECK( esp_wifi_start() );
    ESP_ERROR_CHECK( esp_wifi_connect() );
    */


    gpio_set_direction(GPIO_NUM_4, GPIO_MODE_OUTPUT);
    int level = 0;
    while (true) {
    	ESP_LOGI(tag,"Hello from ESP-IDF!")
    	Serial.println("Hello from Arduino on ESP32!");

        gpio_set_level(GPIO_NUM_4, level);
        level = !level;
        vTaskDelay(300 / portTICK_PERIOD_MS);

        //test_memory_spiffs_raw(); /***This works***/
        test_memory_vfs();			/***This fails***/

        vTaskDelay(60000 / portTICK_PERIOD_MS);
    }

}

