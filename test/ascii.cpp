#include "ascii.h"
unsigned char* toGrayscale(unsigned char* image, int width, int height, int channels){
        unsigned char* greyscale = new unsigned char[width * height];
        for(int i = 0; i < (width * height); i++){
            int index = i * channels;
            //when converting from color to greyscale, you don't average them! because of human perception, very interesting
            //0.2989, 0.5870, 0.1140. are the weights for it. 
            float luminance = 0.2989 * image[index] + 0.5870 * image[index + 1] + 0.1140 * image[index+2];
            greyscale[i] = luminance;
        }
        return greyscale;
}
void deleteBuffer(unsigned char* grayscale) {
    delete[] grayscale;
};
unsigned char* downsample(unsigned char* greyscale, int width, int height, int factor, int* outWidth, int* outHeight){
    int newWidth = width / factor;
    int newHeight = height / factor;
    *outWidth = newWidth; 
    *outHeight = newHeight;
    unsigned char* downsample_buffer = new unsigned char [newWidth * newHeight];
    for(int i = 0; i < newWidth * newHeight; i++){
        downsample_buffer[i] = greyscale[i * factor];
    }
    return downsample_buffer;  
}

unsigned char* upsample(unsigned char* downsample, int width, int height, int factor, int* outWidth, int* outHeight){
    
}
