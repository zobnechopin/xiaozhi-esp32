#pragma once
#include "acorn_assets.h"
#include "../../display/lcd_display.h"

class GifController {
public:
    GifController(SpiLcdDisplay* display) : display_(display) {}
    
    void ShowIdleGif() {
        if (!display_) return;
        // 随机选择一个 IDLE GIF
        static const char* idle_gifs[] = {"idle_status_1", "idle_status_2", "idle_status_blink"};
        int index = rand() % 3;
        const lv_image_dsc_t* gif = AcornAssets::GetGif(idle_gifs[index]);
        if (gif) {
            display_->SetCustomGif(gif);
        }
    }
    
    void ShowListeningGif() {
        const lv_image_dsc_t* gif = AcornAssets::GetGif("listen_start");
        if (gif && display_) {
            display_->SetCustomGif(gif);
        }
    }
    
    void ShowSpeakingGif() {
        const lv_image_dsc_t* gif = AcornAssets::GetGif("talk_loop");
        if (gif && display_) {
            display_->SetCustomGif(gif);
        }
    }
    
private:
    SpiLcdDisplay* display_;
};
