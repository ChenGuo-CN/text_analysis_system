#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define STB_IMAGE_IMPLEMENTATION
#define STBI_NO_THREAD_LOCALS
#define STBI_ONLY_JPEG
#define STBI_ONLY_PNG
#include "stb_image.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#include "image_utils.h"

int read_image(const char* path, image_buffer_t* image)
{
    int width, height, channels;
    unsigned char* data = stbi_load(path, &width, &height, &channels, 3);
    if (data == NULL) {
        printf("Error: stbi_load failed for %s\n", path);
        return -1;
    }

    image->width = width;
    image->height = height;
    image->width_stride = width;
    image->height_stride = height;
    image->format = IMAGE_FORMAT_RGB888;
    image->size = width * height * 3;
    image->virt_addr = data;
    image->fd = -1;

    return 0;
}

int write_image(const char* path, const image_buffer_t* image)
{
    if (image->format != IMAGE_FORMAT_RGB888) {
        printf("Error: write_image only supports IMAGE_FORMAT_RGB888\n");
        return -1;
    }

    int ret = stbi_write_jpg(path, image->width, image->height, 3, image->virt_addr, 95);
    if (ret == 0) {
        printf("Error: stbi_write_jpg failed\n");
        return -1;
    }

    return 0;
}

int convert_image(image_buffer_t* src_image, image_buffer_t* dst_image, image_rect_t* src_box, image_rect_t* dst_box, char color)
{
    // 简单的图像缩放和格式转换实现
    // 这里假设源和目标都是RGB888格式
    
    if (src_image->format != IMAGE_FORMAT_RGB888 || dst_image->format != IMAGE_FORMAT_RGB888) {
        printf("Error: convert_image only supports IMAGE_FORMAT_RGB888\n");
        return -1;
    }

    int src_w = src_image->width;
    int src_h = src_image->height;
    int dst_w = dst_image->width;
    int dst_h = dst_image->height;

    unsigned char* src_data = src_image->virt_addr;
    unsigned char* dst_data = dst_image->virt_addr;

    // 简单的双线性插值缩放
    for (int y = 0; y < dst_h; y++) {
        for (int x = 0; x < dst_w; x++) {
            float src_x = (float)x * src_w / dst_w;
            float src_y = (float)y * src_h / dst_h;
            
            int x0 = (int)src_x;
            int y0 = (int)src_y;
            int x1 = x0 + 1;
            int y1 = y0 + 1;
            
            if (x1 >= src_w) x1 = src_w - 1;
            if (y1 >= src_h) y1 = src_h - 1;
            
            float fx = src_x - x0;
            float fy = src_y - y0;
            
            for (int c = 0; c < 3; c++) {
                float v00 = src_data[(y0 * src_w + x0) * 3 + c];
                float v01 = src_data[(y0 * src_w + x1) * 3 + c];
                float v10 = src_data[(y1 * src_w + x0) * 3 + c];
                float v11 = src_data[(y1 * src_w + x1) * 3 + c];
                
                float v0 = v00 * (1 - fx) + v01 * fx;
                float v1 = v10 * (1 - fx) + v11 * fx;
                float v = v0 * (1 - fy) + v1 * fy;
                
                dst_data[(y * dst_w + x) * 3 + c] = (unsigned char)v;
            }
        }
    }

    return 0;
}

int convert_image_with_letterbox(image_buffer_t* src_image, image_buffer_t* dst_image, letterbox_t* letterbox, char color)
{
    // 简化的letterbox实现
    int src_w = src_image->width;
    int src_h = src_image->height;
    int dst_w = dst_image->width;
    int dst_h = dst_image->height;

    float scale = (float)dst_w / src_w < (float)dst_h / src_h ? 
                  (float)dst_w / src_w : (float)dst_h / src_h;
    
    int new_w = (int)(src_w * scale);
    int new_h = (int)(src_h * scale);
    
    int x_pad = (dst_w - new_w) / 2;
    int y_pad = (dst_h - new_h) / 2;

    // 填充背景色
    memset(dst_image->virt_addr, color, dst_w * dst_h * 3);

    // 缩放图像到中心位置
    for (int y = 0; y < new_h; y++) {
        for (int x = 0; x < new_w; x++) {
            int src_x = (int)(x / scale);
            int src_y = (int)(y / scale);
            
            for (int c = 0; c < 3; c++) {
                dst_image->virt_addr[((y + y_pad) * dst_w + (x + x_pad)) * 3 + c] = 
                    src_image->virt_addr[(src_y * src_w + src_x) * 3 + c];
            }
        }
    }

    letterbox->x_pad = x_pad;
    letterbox->y_pad = y_pad;
    letterbox->scale = scale;

    return 0;
}

int get_image_size(image_buffer_t* image)
{
    int size = 0;
    switch (image->format) {
    case IMAGE_FORMAT_GRAY8:
        size = image->width * image->height;
        break;
    case IMAGE_FORMAT_RGB888:
        size = image->width * image->height * 3;
        break;
    case IMAGE_FORMAT_RGBA8888:
        size = image->width * image->height * 4;
        break;
    default:
        printf("Error: unsupported image format %d\n", image->format);
        break;
    }
    return size;
}
