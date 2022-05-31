//
// Created by swwguest on 31.05.2022.
//
#include <stdint.h>
#include <iostream>
#include <fstream>
#include <ctime>
#include <iomanip>
#include "headers/MLX90640_API.h"

#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_GREEN   "\x1b[32m"
#define ANSI_COLOR_YELLOW  "\x1b[33m"
#define ANSI_COLOR_BLUE    "\x1b[34m"
#define ANSI_COLOR_MAGENTA "\x1b[35m"
#define ANSI_COLOR_CYAN    "\x1b[36m"
#define ANSI_COLOR_NONE    "\x1b[30m"
#define ANSI_COLOR_RESET   "\x1b[0m"

//#define FMT_STRING "%+06.2f "
#define FMT_STRING "\u2588\u2588"

#define MLX_I2C_ADDR 0x33

#define SKIP_FRAMES 5

#define X_PIXELS 32
#define Y_PIXELS 24

#define DEFAULT_DATA_DIR "/home/pi/Desktop/BSF_temperatures/data/MLX90640_data.csv"

void initialize_MLX90640() {
    MLX90640_SetDeviceMode(MLX_I2C_ADDR, 0);
    MLX90640_SetSubPageRepeat(MLX_I2C_ADDR, 0);
    MLX90640_SetRefreshRate(MLX_I2C_ADDR, 0b010);
    MLX90640_SetChessMode(MLX_I2C_ADDR);
    //MLX90640_SetSubPage(MLX_I2C_ADDR, 0);
}

paramsMLX90640 &dumpEE_and_ExtractParameters(uint16_t *eeMLX90640, paramsMLX90640 &mlx90640) {
    MLX90640_DumpEE(MLX_I2C_ADDR, eeMLX90640);
    MLX90640_ExtractParameters(eeMLX90640, &mlx90640);

    return mlx90640;
}

void moveCursorToStart() { printf("\x1b[32A"); }

void loopThroughPixelsAndPrint(const float *mlx90640To) {
    for(int x = 0; x < X_PIXELS; x++){
        for(int y = 0; y < Y_PIXELS; y++){
            float val = mlx90640To[X_PIXELS * (Y_PIXELS - 1 - y) + x];
            if(val > 99.99) val = 99.99;
            if(val > 32.0){
                printf(ANSI_COLOR_MAGENTA FMT_STRING ANSI_COLOR_RESET, val);
            }
            else if(val > 29.0){
                printf(ANSI_COLOR_RED FMT_STRING ANSI_COLOR_RESET, val);
            }
            else if (val > 26.0){
                printf(ANSI_COLOR_YELLOW FMT_STRING ANSI_COLOR_YELLOW, val);
            }
            else if ( val > 20.0 ){
                printf(ANSI_COLOR_NONE FMT_STRING ANSI_COLOR_RESET, val);
            }
            else if (val > 17.0) {
                printf(ANSI_COLOR_GREEN FMT_STRING ANSI_COLOR_RESET, val);
            }
            else if (val > 10.0) {
                printf(ANSI_COLOR_CYAN FMT_STRING ANSI_COLOR_RESET, val);
            }
            else {
                printf(ANSI_COLOR_BLUE FMT_STRING ANSI_COLOR_RESET, val);
            }
        }
        std::cout << std::endl;
    }

    moveCursorToStart();
}


void loopThroughPixelsAndSave(const float *mlx90640To, std::string data_dir){
    std::ofstream csvFile;
    csvFile.open(data_dir, std::ios_base::app);

    time_t t = time(nullptr);
    std::tm tm = *localtime(&t);
    csvFile << std::put_time(&tm, "%F %T");

    csvFile << ", ";
    for(int y = 0; y < Y_PIXELS; y++){
        for(int x = 0; x < X_PIXELS; x++){
            float val = mlx90640To[  X_PIXELS * (Y_PIXELS - 1 - y) + x];
            csvFile << std::to_string(val);
            csvFile << ", ";
        }
    }
    csvFile << std::endl;
    csvFile.close();
}

void getCorrectedData(uint16_t *frame, paramsMLX90640 &mlx90640, float *mlx90640To) {
    float emissivity = 1;
    float eTa;

    MLX90640_GetFrameData(MLX_I2C_ADDR, frame);
    // MLX90640_InterpolateOutliers(frame, eeMLX90640);

    eTa = MLX90640_GetTa(frame, &mlx90640);
    MLX90640_CalculateTo(frame, &mlx90640, emissivity, eTa, mlx90640To);

    MLX90640_BadPixelsCorrection((&mlx90640)->brokenPixels, mlx90640To, 1, &mlx90640);
    MLX90640_BadPixelsCorrection((&mlx90640)->outlierPixels, mlx90640To, 1, &mlx90640);
}

int main(int argc, char *argv[]){
    static uint16_t eeMLX90640[832];
    uint16_t frame[834];
    static float mlx90640To[768];

    std::string data_dir = DEFAULT_DATA_DIR;

    if(argc > 1){
        data_dir = argv[1];
    }

    paramsMLX90640 mlx90640;

    initialize_MLX90640();
    mlx90640 = dumpEE_and_ExtractParameters(eeMLX90640, mlx90640);

    for (int i = 0; i < SKIP_FRAMES; i++) {
        getCorrectedData(frame, mlx90640, mlx90640To);
        //loopThroughPixelsAndPrint(mlx90640To);
    }
    getCorrectedData(frame, mlx90640, mlx90640To);
    loopThroughPixelsAndSave(mlx90640To, data_dir);

    return 0;
}
