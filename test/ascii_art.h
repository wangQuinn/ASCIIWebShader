
#include <string>
#include <vector>
#include <utility>

// A 2D float "image" stored row-major, mirroring the shape/semantics of the

using namespace std;
class AsciiArt {
public:
    AsciiArt(const string& image_filename, const string& texture_filename);

    void getLuminance();                 // RGB(A) -> greyscale luminance
    void reduceTextureDim();              // texture RGBA -> single channel

    void downsample(int factor);          // nearest-neighbor downsample of image_
    void quantize(int count);             // bucket luminance into `count` levels

    void sobel();                         // fills gx_, gy_
    void getMagnitude(float threshold);   // fills magnitude_, thresholds it
    void findAngle();                     // fills theta_, absTheta_
    void edgeQuantize();                  // compares gradient angle -> direction index

    void toAsciiArt(bool edge_mode);      // adds array into a full-res buffer
    
    void controlledDownsample(int factor, int threshold);

    void differenceOfGaussian(float sigma, float scale, float tau, float threshold);
    void combine(const AsciiArt& edge_data);

    void getBloomData(float threshold, float stdev);
    void addBloomData();

    void storeImage(const string& outputfile) const;

    // exposed for combine()/testing??
    const vector<float>& image() const { return image_; }
    int height() const { return height_; }
    int width() const { return width_; }

private:
    string image_filename_;
    string texture_filename_;

    vector<float> image_;   // height_ x width_, row-major, values in [0,1] (mostly)
    int height_ = 0;
    int width_ = 0;

    vector<float> texture_; // texHeight_ x texWidth_, single channel (texture) 
    int texHeight_ = 0;
    int texWidth_ = 0;
    int charSize_ = 0;

    vector<float> gx_, gy_, theta_, absTheta_, magnitude_;
    vector<float> bloomData_;
    int bloomHeight_ = 0, bloomWidth_ = 0;

    // --- helpers ---

    // read with edge-clamped ("nearest") boundary handling.
    static float at(const vector<float>& buf, int h, int w, int i, int j);

    vector<float> getChar(int index) const; // char_size x char_size 

    static pair<float, int> maxFreq(const vector<float>& values);

    // separable Gaussian blur 
    static vector<float> gaussianFilter(const vector<float>& src, int h, int w, float sigma);
};