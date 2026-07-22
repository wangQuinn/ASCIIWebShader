//declaration

#include <string>
#include <vector> 
#include <utility>

using namespace std; 

unsigned char* toGrayscale(unsigned char* image, int width, int height, int channels);
void deleteBuffer(unsigned char* grayscale);
unsigned char* downsample(unsigned char* greyscale, int width, int height, int factor, int* outWidth, int* outHeight);
unsigned char* upsample(unsigned char* greyscale, int width, int height, int factor, int* outWidth, int* outHeight);