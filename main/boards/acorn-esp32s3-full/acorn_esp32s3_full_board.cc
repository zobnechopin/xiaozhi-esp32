#include "wifi_board.h"
#include "codecs/no_audio_codec.h"
#include "display/lcd_display.h"
#include "display/emoji_collection.h"  // 添加这个头文件
#include "system_reset.h"
#include "application.h"
#include "button.h"
#include "config.h"
#include "mcp_server.h"
#include "lamp_controller.h"
#include "led/single_led.h"
#include "esp32_camera.h"
#include <wifi_station.h>
#include <esp_log.h>
#include <driver/i2c_master.h>
#include <esp_lcd_panel_vendor.h>
#include <esp_lcd_panel_io.h>
#include <esp_lcd_panel_ops.h>
#include <driver/spi_common.h>
#include "power_save_timer.h"
#include "power_manager.h"
#include <driver/rtc_io.h>
#include <esp_sleep.h>
#include "esp_lcd_gc9a01.h"
#include "acorn_display.h"
#include "acorn_display_controller.h"
#include "acorn_assets.h"


// #if defined(LCD_TYPE_GC9A01_SERIAL)

// static const gc9a01_lcd_init_cmd_t gc9107_lcd_init_cmds[] = {
//     //  {cmd, { data }, data_size, delay_ms}
//     {0xfe, (uint8_t[]){0x00}, 0, 0},
//     {0xef, (uint8_t[]){0x00}, 0, 0},
//     {0xb0, (uint8_t[]){0xc0}, 1, 0},
//     {0xb1, (uint8_t[]){0x80}, 1, 0},
//     {0xb2, (uint8_t[]){0x27}, 1, 0},
//     {0xb3, (uint8_t[]){0x13}, 1, 0},
//     {0xb6, (uint8_t[]){0x19}, 1, 0},
//     {0xb7, (uint8_t[]){0x05}, 1, 0},
//     {0xac, (uint8_t[]){0xc8}, 1, 0},
//     {0xab, (uint8_t[]){0x0f}, 1, 0},
//     {0x3a, (uint8_t[]){0x05}, 1, 0},
//     {0xb4, (uint8_t[]){0x04}, 1, 0},
//     {0xa8, (uint8_t[]){0x08}, 1, 0},
//     {0xb8, (uint8_t[]){0x08}, 1, 0},
//     {0xea, (uint8_t[]){0x02}, 1, 0},
//     {0xe8, (uint8_t[]){0x2A}, 1, 0},
//     {0xe9, (uint8_t[]){0x47}, 1, 0},
//     {0xe7, (uint8_t[]){0x5f}, 1, 0},
//     {0xc6, (uint8_t[]){0x21}, 1, 0},
//     {0xc7, (uint8_t[]){0x15}, 1, 0},
//     {0xf0,
//     (uint8_t[]){0x1D, 0x38, 0x09, 0x4D, 0x92, 0x2F, 0x35, 0x52, 0x1E, 0x0C,
//                 0x04, 0x12, 0x14, 0x1f},
//     14, 0},
//     {0xf1,
//     (uint8_t[]){0x16, 0x40, 0x1C, 0x54, 0xA9, 0x2D, 0x2E, 0x56, 0x10, 0x0D,
//                 0x0C, 0x1A, 0x14, 0x1E},
//     14, 0},
//     {0xf4, (uint8_t[]){0x00, 0x00, 0xFF}, 3, 0},
//     {0xba, (uint8_t[]){0xFF, 0xFF}, 2, 0},
// };
// #endif
 
#define TAG "AcornESP32S3Full"

LV_FONT_DECLARE(font_puhui_16_4);
LV_FONT_DECLARE(font_awesome_16_4);

class AcornESP32S3Full : public WifiBoard {
private:
 
    Button boot_button_;
    LcdDisplay* display_;
    Esp32Camera* camera_;
    PowerSaveTimer* power_save_timer_;
    PowerManager* power_manager_;
    esp_lcd_panel_io_handle_t panel_io_ = nullptr;
    esp_lcd_panel_handle_t panel_ = nullptr;
    AcornDisplay* acorn_display_;
    AcornDisplayController* display_controller_;
    void InitializeSpi() {
        spi_bus_config_t buscfg = {};
        buscfg.mosi_io_num = DISPLAY_MOSI_PIN;
        buscfg.miso_io_num = GPIO_NUM_NC;
        buscfg.sclk_io_num = DISPLAY_CLK_PIN;
        buscfg.quadwp_io_num = GPIO_NUM_NC;
        buscfg.quadhd_io_num = GPIO_NUM_NC;
        buscfg.max_transfer_sz = DISPLAY_WIDTH * DISPLAY_HEIGHT * sizeof(uint16_t);
        ESP_ERROR_CHECK(spi_bus_initialize(SPI3_HOST, &buscfg, SPI_DMA_CH_AUTO));
    }
    void InitializeCamera() {
        camera_config_t config = {};
        config.ledc_channel = LEDC_CHANNEL_2;   // LEDC通道选择  用于生成XCLK时钟 但是S3不用
        config.ledc_timer = LEDC_TIMER_2;       // LEDC timer选择  用于生成XCLK时钟 但是S3不用
        config.pin_d0 = CAMERA_PIN_D0;
        config.pin_d1 = CAMERA_PIN_D1;
        config.pin_d2 = CAMERA_PIN_D2;
        config.pin_d3 = CAMERA_PIN_D3;
        config.pin_d4 = CAMERA_PIN_D4;
        config.pin_d5 = CAMERA_PIN_D5;
        config.pin_d6 = CAMERA_PIN_D6;
        config.pin_d7 = CAMERA_PIN_D7;
        config.pin_xclk = CAMERA_PIN_XCLK;
        config.pin_pclk = CAMERA_PIN_PCLK;
        config.pin_vsync = CAMERA_PIN_VSYNC;
        config.pin_href = CAMERA_PIN_HREF;
        config.pin_sccb_sda = CAMERA_PIN_SIOD;  // 这里如果写-1 表示使用已经初始化的I2C接口
        config.pin_sccb_scl = CAMERA_PIN_SIOC;
        config.sccb_i2c_port = 1;               //  这里如果写1 默认使用I2C1
        config.pin_pwdn = CAMERA_PIN_PWDN;
        config.pin_reset = CAMERA_PIN_RESET;
        config.xclk_freq_hz = XCLK_FREQ_HZ;
        config.pixel_format = PIXFORMAT_RGB565;
        config.frame_size = FRAMESIZE_VGA;
        config.jpeg_quality = 12;
        config.fb_count = 1;
        config.fb_location = CAMERA_FB_IN_PSRAM;
        config.grab_mode = CAMERA_GRAB_WHEN_EMPTY;

        camera_ = new Esp32Camera(config);
        camera_->SetVFlip(0);
    }
    void InitializePowerManager() {
        power_manager_ = new PowerManager(GPIO_NUM_19);
        power_manager_->OnChargingStatusChanged([this](bool is_charging) {
            if (is_charging) {
                power_save_timer_->SetEnabled(false);
            } else {
                power_save_timer_->SetEnabled(true);
            }
        });
    }

    void InitializePowerSaveTimer() {
        // 绕过 GPIO48 的 RTCIO 配置（本板未使用或非必须，避免无意义报错）
        ESP_LOGI(TAG, "Skipping RTCIO config on GPIO 48");

        power_save_timer_ = new PowerSaveTimer(-1, 60, 36000);
        power_save_timer_->OnEnterSleepMode([this]() {
            ESP_LOGI(TAG, "Enabling sleep mode");
            display_->SetChatMessage("system", "");
            display_->SetEmotion("sleepy");
            GetBacklight()->SetBrightness(1);
        });
        power_save_timer_->OnExitSleepMode([this]() {
            display_->SetChatMessage("system", "");
            display_->SetEmotion("neutral");
            GetBacklight()->RestoreBrightness();
        });
        power_save_timer_->OnShutdownRequest([this]() {
            ESP_LOGI(TAG, "Shutting down");
            // 跳过GPIO48拉低与保持逻辑
            esp_lcd_panel_disp_on_off(panel_, false); //关闭显示
            esp_deep_sleep_start();
        });
        power_save_timer_->SetEnabled(true);
    }
    
    void InitializeLcdDisplay() {
        // 液晶屏控制IO初始化
        ESP_LOGD(TAG, "Install panel IO");
        esp_lcd_panel_io_spi_config_t io_config = {};
        io_config.cs_gpio_num = DISPLAY_CS_PIN;
        io_config.dc_gpio_num = DISPLAY_DC_PIN;
        io_config.spi_mode = DISPLAY_SPI_MODE;
        io_config.pclk_hz = 40 * 1000 * 1000;
        io_config.trans_queue_depth = 10;
        io_config.lcd_cmd_bits = 8;
        io_config.lcd_param_bits = 8;
        ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi(SPI3_HOST, &io_config, &panel_io_));  // 赋值给成员变量

        // 初始化液晶屏驱动芯片
        ESP_LOGD(TAG, "Install LCD driver");
        esp_lcd_panel_dev_config_t panel_config = {};
        panel_config.reset_gpio_num = DISPLAY_RST_PIN;
        panel_config.rgb_ele_order = DISPLAY_RGB_ORDER;
        panel_config.bits_per_pixel = 16;
#if defined(LCD_TYPE_ILI9341_SERIAL)
        ESP_ERROR_CHECK(esp_lcd_new_panel_ili9341(panel_io_, &panel_config, &panel_));  // 赋值给成员变量
#elif defined(LCD_TYPE_GC9A01_SERIAL)
        ESP_ERROR_CHECK(esp_lcd_new_panel_gc9a01(panel_io_, &panel_config, &panel_));   // 赋值给成员变量
#else
        ESP_ERROR_CHECK(esp_lcd_new_panel_st7789(panel_io_, &panel_config, &panel_));   // 赋值给成员变量
#endif
        
        esp_lcd_panel_reset(panel_);
        esp_lcd_panel_init(panel_);
        esp_lcd_panel_invert_color(panel_, DISPLAY_INVERT_COLOR);
        esp_lcd_panel_swap_xy(panel_, DISPLAY_SWAP_XY);
        esp_lcd_panel_mirror(panel_, DISPLAY_MIRROR_X, DISPLAY_MIRROR_Y);
        esp_lcd_panel_disp_on_off(panel_, true);
        
        // 使用 AcornDisplay 替代 SpiLcdDisplay，使用 config.h 中的镜像设置
        acorn_display_ = new AcornDisplay(panel_io_, panel_, DISPLAY_WIDTH, DISPLAY_HEIGHT, 
                                        DISPLAY_OFFSET_X, DISPLAY_OFFSET_Y, 
                                        DISPLAY_MIRROR_X, DISPLAY_MIRROR_Y, DISPLAY_SWAP_XY);
        display_ = acorn_display_;  // 将 AcornDisplay 赋值给基类指针
        
        // 创建后设置自定义样式
        EmojiCollection* emoji_collection = DISPLAY_HEIGHT >= 240 ? 
            static_cast<EmojiCollection*>(new Twemoji64()) : 
            static_cast<EmojiCollection*>(new Twemoji32());
        
        DisplayStyle custom_style = {
            .text_font = &font_puhui_16_4,
            .icon_font = &font_awesome_16_4,
            .emoji_collection = emoji_collection,
        };
        display_->UpdateStyle(custom_style);
    }


 
    void InitializeButtons() {
        boot_button_.OnClick([this]() {
            auto& app = Application::GetInstance();
            if (app.GetDeviceState() == kDeviceStateStarting && !WifiStation::GetInstance().IsConnected()) {
                ResetWifiConfiguration();
            }
            app.ToggleChatState();
        });
    }


    // 物联网初始化，添加对 AI 可见设备
    void InitializeIot() {
#if CONFIG_IOT_PROTOCOL_XIAOZHI
        auto& thing_manager = iot::ThingManager::GetInstance();
        thing_manager.AddThing(iot::CreateThing("Speaker"));
        thing_manager.AddThing(iot::CreateThing("Screen"));
        thing_manager.AddThing(iot::CreateThing("Lamp"));
#elif CONFIG_IOT_PROTOCOL_MCP
        static LampController lamp(LAMP_GPIO);
#endif
    }

public:
    AcornESP32S3Full() : boot_button_(BOOT_BUTTON_GPIO) {
        InitializeSpi();
        InitializeLcdDisplay();  // 这里创建 AcornDisplay + HaloUI
        InitializeButtons();
        InitializePowerManager();
        InitializePowerSaveTimer();
        InitializeIot();
        InitializeCamera();
        
        // 设置背光
        if (DISPLAY_BACKLIGHT_PIN != GPIO_NUM_NC) {
            GetBacklight()->SetBrightness(100);
        }
        
        // 使用 HaloUI + 我们的控制器
        display_controller_ = new AcornDisplayController(acorn_display_);
        display_controller_->ForceIdle();
    }

    virtual Led* GetLed() override {
        static SingleLed led(BUILTIN_LED_GPIO);
        return &led;
    }
    virtual Camera* GetCamera() override {
        return camera_;
    }
    virtual AudioCodec* GetAudioCodec() override {
#ifdef AUDIO_I2S_METHOD_SIMPLEX
        static NoAudioCodecSimplex audio_codec(AUDIO_INPUT_SAMPLE_RATE, AUDIO_OUTPUT_SAMPLE_RATE,
            AUDIO_I2S_SPK_GPIO_BCLK, AUDIO_I2S_SPK_GPIO_LRCK, AUDIO_I2S_SPK_GPIO_DOUT, AUDIO_I2S_MIC_GPIO_SCK, AUDIO_I2S_MIC_GPIO_WS, AUDIO_I2S_MIC_GPIO_DIN);
#else
        static NoAudioCodecDuplex audio_codec(AUDIO_INPUT_SAMPLE_RATE, AUDIO_OUTPUT_SAMPLE_RATE,
            AUDIO_I2S_GPIO_BCLK, AUDIO_I2S_GPIO_WS, AUDIO_I2S_GPIO_DOUT, AUDIO_I2S_GPIO_DIN);
#endif
        return &audio_codec;
    }

    virtual Display* GetDisplay() override {
        return display_;
    }

    virtual Backlight* GetBacklight() override {
        if (DISPLAY_BACKLIGHT_PIN != GPIO_NUM_NC) {
            static PwmBacklight backlight(DISPLAY_BACKLIGHT_PIN, DISPLAY_BACKLIGHT_OUTPUT_INVERT);
            static bool initialized = false;
            if (!initialized) {
                backlight.SetBrightness(100);  // 移除日志语句
                initialized = true;
            }
            return &backlight;
        }
        return nullptr;
    }
    virtual bool GetBatteryLevel(int& level, bool& charging, bool& discharging) override {
        static bool last_discharging = false;
        charging = power_manager_->IsCharging();
        discharging = power_manager_->IsDischarging();
        if (discharging != last_discharging) {
            power_save_timer_->SetEnabled(discharging);
            last_discharging = discharging;
        }
        level = power_manager_->GetBatteryLevel();
        return true;
    }

    virtual void SetPowerSaveMode(bool enabled) override {
        if (!enabled) {
            power_save_timer_->WakeUp();
        }
        WifiBoard::SetPowerSaveMode(enabled);
    }

    // 新增：获取显示控制器的接口
    AcornDisplayController* GetDisplayController() {
        return display_controller_;
    }
};

DECLARE_BOARD(AcornESP32S3Full);
