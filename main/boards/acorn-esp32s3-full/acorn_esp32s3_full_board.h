#pragma once

#include "wifi_board.h"
#include "acorn_display_controller.h"

// 删除这行，因为已经包含了头文件
// class AcornDisplayController;

class AcornESP32S3Full : public WifiBoard {
public:
    // 新增：获取显示控制器的接口
    AcornDisplayController* GetDisplayController();
};

// 声明创建函数
DECLARE_BOARD(AcornESP32S3Full)



