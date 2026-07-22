
#include <iostream>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include "ascii.h"

using namespace std;

int main(int argc, char** argv) {
    if (argc < 2) {
        cerr << "usage: " << argv[0] << " <image.png>\n";
        return 1;
    }

    const char*  image_file = "sample.png";
    int img_width;
    int img_height;
    int channels;
    unsigned char *image = stbi_load(image_file, &img_width, &img_height, &channels, 0);

    if(nullptr == image){
        cerr << "image loading: " << image_file << " failed." << endl;
        return 1;
    }
    cout << "width: " << img_width << " height: " << img_height << " channels: " << channels << endl;

    unsigned char* grayscale_buffer = toGrayscale(image, img_width, img_height,channels);
    int i = grayscale_buffer[0];
    cout<< "first index of grayscale_buffer: " <<i  << endl;
    int factor = 3;
    int downscale_width, downscale_height;
    unsigned char* downscale_buffer = downsample(grayscale_buffer, img_width, img_height, 3, &downscale_width, &downscale_height);
    cout<< "factor: " << factor << " with new height and width: " << downscale_width << ", " << downscale_height  << endl;
    //memory clearing. 
    deleteBuffer(grayscale_buffer);
    deleteBuffer(downscale_buffer);
    stbi_image_free(image); 
    return 0;
}