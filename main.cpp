#include <stdint.h>
#include <iostream>
#include <stdlib.h>
#include <algorithm>
#include <vector>
#include <fstream>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define CHANNEL_RGB 3
#define CHANNEL_CMYK 4


void ditheringAlgo(uint8_t* in, int width, int height, int* thresholds, uint8_t* out) {
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            // Index pour accéder aux données CMYK de chaque pixel
            int idx = (y * width + x) * CHANNEL_CMYK;

            // Extraction des valeurs CMYK du pixel
            uint8_t c = in[idx];
            uint8_t m = in[idx + 1];
            uint8_t y = in[idx + 2];
            uint8_t k = in[idx + 3];

            // Compression des valeurs CMYK dans un seul octet utilisant les seuils
            uint8_t compressedValue = 0;
            compressedValue |= (c > thresholds[0]) ? 0b11000000 : (c > thresholds[1]) ? 0b10000000 : (c > thresholds[2]) ? 0b01000000 : 0;
            compressedValue |= (m > thresholds[0]) ? 0b00110000 : (m > thresholds[1]) ? 0b00100000 : (m > thresholds[2]) ? 0b00010000 : 0;
            compressedValue |= (y > thresholds[0]) ? 0b00001100 : (y > thresholds[1]) ? 0b00001000 : (y > thresholds[2]) ? 0b00000100 : 0;
            compressedValue |= (k > thresholds[0]) ? 0b00000011 : (k > thresholds[1]) ? 0b00000010 : (k > thresholds[2]) ? 0b00000001 : 0;

            // Stockage de la valeur compressée
            out[y * width + x] = compressedValue;
        }
    }
}


uint8_t lookupTable(uint8_t val)
{
    if(val == 0) return 0;
    else if(val == 1) return 33;
    else if(val == 2) return 66;
    else return 100;
}

void cmykToRgb(uint8_t cmyk, uint8_t* rgb)
{
    uint8_t mask_c = 192;  //0b11000000
    uint8_t mask_m = 48;   //0b00110000
    uint8_t mask_y = 12;   //0b00001100
    uint8_t mask_k = 3;    //0b00000011

    uint8_t c = lookupTable(mask_c & cmyk);
    uint8_t m = lookupTable(mask_m & cmyk);
    uint8_t y = lookupTable(mask_y & cmyk);
    uint8_t k = lookupTable(mask_k & cmyk);

    rgb[0] = 255 * (100 - c) * (100 - k) / 10000;       // R = 255*(100-C)*(100-K)/10000；
    rgb[1] = 255 * (100 - m) * (100 - k) / 10000;       // G = 255*(100-M)*(100-K)/10000;
    rgb[2] = 255 * (100 - y) * (100 - k) / 10000;       // B = 255*(100-Y)*(100-K)/10000;
}

void compressedCmykImgToRgbImg(uint8_t *compressedCmyk, uint8_t* rgbImg, int height, int width)
{
    uint8_t rgb[3] = {0};
    for(int i = 0; i < height; i++)
    {
        for(int j = 0; j < width; j++)
        {
            cmykToRgb(*(compressedCmyk + i * width + j), rgb);

            *(rgbImg + i * width * CHANNEL_RGB + (CHANNEL_RGB * j + 0)) = rgb[0];
            *(rgbImg + i * width * CHANNEL_RGB + (CHANNEL_RGB * j + 1)) = rgb[1];
            *(rgbImg + i * width * CHANNEL_RGB + (CHANNEL_RGB * j + 2)) = rgb[2];
        }
    }
}

void rgbToCmyk(uint8_t r, uint8_t g, uint8_t b, uint8_t* c, uint8_t* m, uint8_t* y, uint8_t* k)
{
    double dr = (double)r / 255.0;
    double dg = (double)g / 255.0;
    double db = (double)b / 255.0;

    double dk = 1 - std::max(std::max(dr, dg), db);
    double dc = (1 - dr - dk) / (1 - dk);
    double dm = (1 - dg - dk) / (1 - dk);
    double dy = (1 - db - dk) / (1 - dk);

    *k = (uint8_t) (dk * 255);
    *c = (uint8_t) (dc * 255);
    *m = (uint8_t) (dm * 255);
    *y = (uint8_t) (dy * 255);
}

void rgbImgToCmykImg(uint8_t* input, uint8_t* cmyk_image, int width, int height)
{

    for(int i = 0; i < height; i++)
    {
        for(int j = 0; j < width; j++)
        {
            uint8_t r = *(input + i * width * CHANNEL_RGB + (CHANNEL_RGB * j + 0));
            uint8_t g = *(input + i * width * CHANNEL_RGB + (CHANNEL_RGB * j + 1));
            uint8_t b = *(input + i * width * CHANNEL_RGB + (CHANNEL_RGB * j + 2));

            rgbToCmyk(r, g, b, (cmyk_image + i * width * CHANNEL_CMYK + (CHANNEL_CMYK * j + 0)), \
                               (cmyk_image + i * width * CHANNEL_CMYK + (CHANNEL_CMYK * j + 1)), \
                               (cmyk_image + i * width * CHANNEL_CMYK + (CHANNEL_CMYK * j + 2)), \
                               (cmyk_image + i * width * CHANNEL_CMYK + (CHANNEL_CMYK * j + 3)));

        }
    }
}

// -------------------------------------------------------------------------------------------------------- //
// BMP Header  source: https://github.com/lambda-pixel/tuto-youtube/blob/01-bmp/main.cpp

#pragma pack(1)
struct BMP_Header
{
    uint8_t  type[2];
    uint32_t file_size;
    uint8_t  reserved[4];
    uint32_t offset_start_framebuffer;
};

#pragma pack(1)
struct BMP_Info_Header
{
    uint32_t header_size;
    uint32_t width;
    uint32_t height;
    uint16_t n_color_planes;
    uint16_t bits_per_pixel;
    uint32_t compression_method;
    uint32_t raw_size_framebuffer;
    int32_t  h_res;
    int32_t  v_res;
    uint32_t n_color_palettes;
    uint32_t n_important_colors;
};


struct BMP_Color
{
    uint8_t b;
    uint8_t g;
    uint8_t r;
    uint8_t _;
};

// -------------------------------------------------------------------------------------------------------- //

int main(int argc, char *argv[]) {

    if(argc != 2)
    {
        std::cout << "The number of the argument is not 2; perhaps you forgot the name of an image." << std::endl;
        return 0;
    }

    int width = 0, height = 0, bpp = 0;

    int thresholds[4] = {0, 100, 180, 255};

    uint8_t* input = stbi_load(argv[1], &width, &height, &bpp, 3);

    std::cout << "The width of the image is:  " << width <<  "." << std::endl;
    std::cout << "The height of the image is: " << height << "." << std::endl;

    uint8_t* cmyk_image = (uint8_t*)malloc(width * height * CHANNEL_CMYK);

    rgbImgToCmykImg(input, cmyk_image, width, height);

    uint8_t* output = (uint8_t*)malloc(width * height);

    memset(output, 0, width * height);

    ditheringAlgo(cmyk_image, width, height, thresholds, output);    

    std::string fileNa = std::string(argv[1]);
    std::string outputFile = "results/" + fileNa + ".bmp";

    uint8_t* outputImage = (uint8_t*)malloc(width * height * CHANNEL_RGB);

    compressedCmykImgToRgbImg(output, outputImage, height, width);

    char imgName[100];

    strcpy(imgName, outputFile.c_str()); 

    // Specification des donnees des headers
    const uint32_t size_framebuffer = width * height * sizeof(BMP_Color);

    const BMP_Header bmp_header = {
        .type                     = {'B', 'M'},
        .file_size                = static_cast<uint32_t>(sizeof(BMP_Header) + sizeof(BMP_Info_Header) + size_framebuffer),
        .reserved                 = {0},
        .offset_start_framebuffer = sizeof(BMP_Header) + sizeof(BMP_Info_Header)
    };
    
    const BMP_Info_Header info_header = {
        .header_size          = sizeof(BMP_Info_Header),
        .width                = (uint32_t)width,
        .height               = (uint32_t)height,
        .n_color_planes       = 1,
        .bits_per_pixel       = 8 * sizeof(BMP_Color),
        .compression_method   = 0,
        .raw_size_framebuffer = size_framebuffer,
        .h_res                = 2835,
        .v_res                = 2835,
        .n_color_palettes     = 0,
        .n_important_colors   = 0
    };

    BMP_Color *framebuffer = new BMP_Color[width * height];

    for (int i = height - 1; i >= 0; i--) {
        for (int j = 0; j < width; j++) {
            framebuffer[(height - 1 - i) * width + j].r = *(outputImage + i * width * CHANNEL_RGB + (CHANNEL_RGB * j + 0));
            framebuffer[(height - 1 - i) * width + j].g = *(outputImage + i * width * CHANNEL_RGB + (CHANNEL_RGB * j + 1));
            framebuffer[(height - 1 - i) * width + j].b = *(outputImage + i * width * CHANNEL_RGB + (CHANNEL_RGB * j + 2));
        }
    }

    std::FILE * f_out = std::fopen(imgName, "wb");

    if (f_out != NULL) {

        std::fwrite(&bmp_header, sizeof(BMP_Header), 1, f_out);
        std::fwrite(&info_header, sizeof(BMP_Info_Header), 1, f_out);
        std::fwrite(framebuffer, sizeof(BMP_Color), width * height, f_out);

        std::fclose(f_out);
    }  

    delete[] framebuffer;

    free(cmyk_image);
    free(output);
    free(outputImage);
    stbi_image_free(input);

    return 0;
}
