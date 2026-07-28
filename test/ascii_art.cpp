#include "ascii_art.h"
#define _USE_MATH_DEFINES  //this is for M_PI
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#include <algorithm>
#include <math.h>
#include <stdio.h>
#include <stdexcept>
#include <map>

using namespace std;

// construction / loading

AsciiArt::AsciiArt(const string& image_filename, const string& texture_filename)
    : image_filename_(image_filename), texture_filename_(texture_filename) {

    int w, h, channels;
    unsigned char* raw = stbi_load(image_filename_.c_str(), &w, &h, &channels, 4); // force RGBA
    if (!raw) throw runtime_error("failed to load image: " + image_filename_);

    height_ = h;
    width_ = w;
    image_.assign(static_cast<size_t>(h) * w * 4, 0.0f); // keep RGBA for now; will be collapsed in getLuminance()
    for (int i = 0; i < h * w * 4; ++i) image_[i] = raw[i] / 255.0f;
    stbi_image_free(raw);

    int tw, th, tchannels;
    unsigned char* traw = stbi_load(texture_filename_.c_str(), &tw, &th, &tchannels, 4);
    if (!traw) throw runtime_error("failed to load texture: " + texture_filename_);

    texHeight_ = th;
    texWidth_ = tw;
    texture_.assign(static_cast<size_t>(th) * tw * 4, 0.0f);
    for (int i = 0; i < th * tw * 4; ++i) texture_[i] = traw[i] / 255.0f;
    stbi_image_free(traw);

    getLuminance();
    reduceTextureDim();

    charSize_ = texHeight_; // assumes square cells
}

// greyscale conversion

void AsciiArt::getLuminance() {
    // image_ currently holds RGBA (4 floats/pixel, already normalized to [0,1].
    vector<float> grey(static_cast<size_t>(height_) * width_);
    for (int i = 0; i < height_ * width_; ++i) {
        float r = image_[i * 4 + 0];
        float g = image_[i * 4 + 1];
        float b = image_[i * 4 + 2];
        grey[i] = 0.2989f * r + 0.5870f * g + 0.1140f * b;
    }
    image_ = move(grey); // now it's a single-channel, height_ x width_ !
}

void AsciiArt::reduceTextureDim() {
    // keep just the red channel,
    vector<float> single(static_cast<size_t>(texHeight_) * texWidth_);
    for (int i = 0; i < texHeight_ * texWidth_; ++i) {
        single[i] = texture_[i * 4 + 0];
    }
    texture_ = move(single);
}


// downsample / quantize


void AsciiArt::downsample(int factor) {
    int newH = height_ / factor;
    int newW = width_ / factor;
    vector<float> buffer(static_cast<size_t>(newH) * newW, 0.0f);

    for (int i = 0; i < newH; ++i) {
        for (int j = 0; j < newW; ++j) {
            buffer[i * newW + j] = image_[(i * factor) * width_ + (j * factor)];
        }
    }
    image_ = move(buffer);
    height_ = newH;
    width_ = newW;
}

void AsciiArt::quantize(int count) {
    for (auto& v : image_) {
        v = floor(v * count) / static_cast<float>(count);
    }
}


// glyph lookup


vector<float> AsciiArt::getChar(int index) const {
    vector<float> block(static_cast<size_t>(charSize_) * charSize_);
    int startCol = index * charSize_;
    for (int r = 0; r < charSize_; ++r) {
        for (int c = 0; c < charSize_; ++c) {
            int col = startCol + c;
            block[r * charSize_ + c] =
                (col >= 0 && col < texWidth_) ? texture_[r * texWidth_ + col] : 0.0f;
        }
    }
    return block;
}

// edge detection


float AsciiArt::at(const vector<float>& buf, int h, int w, int i, int j) {
    // clamp-to-edge boundary. scipy.ndimage's default is 'reflect'
    // (mirrored, without repeating the edge pixel); clamping is simpler and
    // close enough for ASCII-art purposes, but if you need exact numerical
    // parity with the Python version, swap this for a reflect implementation.
    i = clamp(i, 0, h - 1);
    j = clamp(j, 0, w - 1);
    return buf[i * w + j];
}

void AsciiArt::sobel() {
    gx_.assign(static_cast<size_t>(height_) * width_, 0.0f);
    gy_.assign(static_cast<size_t>(height_) * width_, 0.0f);

    // standard 3x3 Sobel kernels
    // gx: horizontal derivative (axis 0 in scipy's ndimage.sobel(image, 0))
    // gy: vertical derivative   (axis 1)
    static const int kx[3][3] = {{-1, 0, 1}, {-2, 0, 2}, {-1, 0, 1}};
    static const int ky[3][3] = {{-1, -2, -1}, {0, 0, 0}, {1, 2, 1}};

    for (int i = 0; i < height_; ++i) {
        for (int j = 0; j < width_; ++j) {
            float sx = 0.0f, sy = 0.0f;
            for (int di = -1; di <= 1; ++di) {
                for (int dj = -1; dj <= 1; ++dj) {
                    float v = at(image_, height_, width_, i + di, j + dj);
                    sx += v * kx[di + 1][dj + 1];
                    sy += v * ky[di + 1][dj + 1];
                }
            }
            gx_[i * width_ + j] = sx;
            gy_[i * width_ + j] = sy;
        }
    }
}

void AsciiArt::getMagnitude(float threshold) {
    magnitude_.assign(static_cast<size_t>(height_) * width_, 0.0f);
    for (int i = 0; i < height_ * width_; ++i) {
        float m = sqrt(gx_[i] * gx_[i] + gy_[i] * gy_[i]);
        magnitude_[i] = (m < threshold) ? 0.0f : m;
    }
}

void AsciiArt::findAngle() {
    theta_.assign(static_cast<size_t>(height_) * width_, 0.0f);
    absTheta_.assign(static_cast<size_t>(height_) * width_, 0.0f);
    for (int i = 0; i < height_ * width_; ++i) {
        float t = atan2(gy_[i], gx_[i]);
        theta_[i] = t;
        absTheta_[i] = fabs(t) / static_cast<float>(M_PI);
    }
}

void AsciiArt::edgeQuantize() {
    vector<float> buffer(static_cast<size_t>(height_) * width_, -1.0f);

    for (int i = 0; i < height_; ++i) {
        for (int j = 0; j < width_; ++j) {
            int idx = i * width_ + j;
            float at_ = absTheta_[idx];
            float th = theta_[idx];

            if (at_ <= 0.2f) {
                at_ = 0.0f;
                th = 0.0f;
            }

            int direction = -1;
            bool zeroGrad = (gx_[idx] == 0.0f && gy_[idx] == 0.0f);
            if (!zeroGrad) {
                if (at_ >= 0.0f && at_ < 0.05f) {
                    direction = 1;
                } else if (at_ > 0.9f && at_ <= 1.0f) {
                    direction = 1;
                } else if (at_ > 0.45f && at_ < 0.55f) {
                    direction = 0;
                } else if (at_ > 0.05f && at_ < 0.45f) {
                    direction = (th > 0.0f) ? 2 : 3;
                } else if (at_ > 0.55f && at_ < 0.9f) {
                    direction = (th > 0.0f) ? 3 : 2;
                }
            }
            buffer[idx] = static_cast<float>(direction);
        }
    }
    image_ = move(buffer);
}


// ascii rendering


void AsciiArt::toAsciiArt(bool edge_mode) {
    int outH = height_ * charSize_;
    int outW = width_ * charSize_;
    vector<float> buffer(static_cast<size_t>(outH) * outW, 0.0f);

    for (int i = 0; i < height_; ++i) {
        for (int j = 0; j < width_; ++j) {
            int glyphIndex;
            if (edge_mode) {
                glyphIndex = static_cast<int>(image_[i * width_ + j]) + 1;
            } else {
                glyphIndex = static_cast<int>(image_[i * width_ + j] * 10.0f);
            }
            if (glyphIndex < 0 || (glyphIndex + 1) * charSize_ > texWidth_) continue; ///out of bounds skipped!

            vector<float> glyph = getChar(glyphIndex);
            for (int r = 0; r < charSize_; ++r) {
                for (int c = 0; c < charSize_; ++c) {
                    buffer[(i * charSize_ + r) * outW + (j * charSize_ + c)] =
                        glyph[r * charSize_ + c];
                }
            }
        }
    }

    image_ = move(buffer);
    height_ = outH;
    width_ = outW;
}


// majority-vote downsample of a categorical (edge-direction) image??
pair<float, int> AsciiArt::maxFreq(const vector<float>& values) {
    map<float, int> counts;
    for (float v : values) {
        if (v != -1.0f) counts[v]++;
    }
    if (counts.empty()) return {-1.0f, 0};

    float best = counts.begin()->first;
    int bestCount = 0;
    for (auto& [val, cnt] : counts) {
        if (cnt > bestCount) {
            bestCount = cnt;
            best = val;
        }
    }
    return {best, bestCount};
}

void AsciiArt::controlledDownsample(int factor, int threshold) {
    int newH = height_ / factor;
    int newW = width_ / factor;
    vector<float> buffer(static_cast<size_t>(newH) * newW, 0.0f);

    for (int i = 0; i < height_; i += factor) {
        for (int j = 0; j < width_; j += factor) {
            int oi = i / charSize_;
            int oj = j / charSize_;
            if (oi < 0 || oi >= newH || oj < 0 || oj >= newW) continue;

            int rEnd = min(i + charSize_, height_);
            int cEnd = min(j + charSize_, width_);
            vector<float> block;
            block.reserve(static_cast<size_t>(rEnd - i) * (cEnd - j));
            for (int r = i; r < rEnd; ++r) {
                for (int c = j; c < cEnd; ++c) {
                    block.push_back(image_[r * width_ + c]);
                }
            }

            auto [val, freq] = maxFreq(block);
            buffer[oi * newW + oj] = (freq > threshold) ? val : -1.0f;
        }
    }

    image_ = move(buffer);
    height_ = newH;
    width_ = newW;
}

// difference of gaussian


vector<float> AsciiArt::gaussianFilter(const vector<float>& src, int h, int w, float sigma) {
    if (sigma <= 0.0f) return src;

    // separable Gaussian: build a 1D array, convolve rows then columns.
    int radius = static_cast<int>(ceil(3.0f * sigma));
    vector<float> kernel(2 * radius + 1);
    float sum = 0.0f;
    for (int k = -radius; k <= radius; ++k) {
        float val = exp(-(k * k) / (2.0f * sigma * sigma));
        kernel[k + radius] = val;
        sum += val;
    }
    for (auto& v : kernel) v /= sum;

    vector<float> temp(static_cast<size_t>(h) * w, 0.0f);
    // horizontal pass
    for (int i = 0; i < h; ++i) {
        for (int j = 0; j < w; ++j) {
            float acc = 0.0f;
            for (int k = -radius; k <= radius; ++k) {
                acc += at(src, h, w, i, j + k) * kernel[k + radius];
            }
            temp[i * w + j] = acc;
        }
    }
    // vertical pass
    vector<float> out(static_cast<size_t>(h) * w, 0.0f);
    for (int i = 0; i < h; ++i) {
        for (int j = 0; j < w; ++j) {
            float acc = 0.0f;
            for (int k = -radius; k <= radius; ++k) {
                acc += at(temp, h, w, i + k, j) * kernel[k + radius];
            }
            out[i * w + j] = acc;
        }
    }
    return out;
}

void AsciiArt::differenceOfGaussian(float sigma, float scale, float tau, float threshold) {
    vector<float> g1 = gaussianFilter(image_, height_, width_, sigma);
    vector<float> g2 = gaussianFilter(image_, height_, width_, scale * sigma);

    for (int i = 0; i < height_ * width_; ++i) {
        float v = (1.0f + tau) * g1[i] - tau * g2[i];
        image_[i] = (v >= threshold) ? 1.0f : 0.0f;
    }
}

// combine

void AsciiArt::combine(const AsciiArt& edge_data) {
    // assumes both images share dimensions, as in the Python version
    for (int i = 0; i < height_; i += charSize_) {
        for (int j = 0; j < width_; j += charSize_) {
            float chunkSum = 0.0f;
            int rEnd = min(i + charSize_, height_);
            int cEnd = min(j + charSize_, width_);
            for (int r = i; r < rEnd; ++r)
                for (int c = j; c < cEnd; ++c)
                    chunkSum += edge_data.image_[r * edge_data.width_ + c];

            if (chunkSum != 0.0f) {
                for (int r = i; r < rEnd; ++r)
                    for (int c = j; c < cEnd; ++c)
                        image_[r * width_ + c] = edge_data.image_[r * edge_data.width_ + c];
            }
        }
    }
}

// bloom

void AsciiArt::getBloomData(float threshold, float stdev) {
    bloomHeight_ = height_;
    bloomWidth_ = width_;
    bloomData_.assign(static_cast<size_t>(height_) * width_, 0.0f);
    for (int i = 0; i < height_ * width_; ++i) {
        bloomData_[i] = (image_[i] > threshold) ? image_[i] : 0.0f;
    }
    bloomData_ = gaussianFilter(bloomData_, height_, width_, stdev);

    // NOTE: like the Python original, bloom is computed at the
    // pre-toAsciiArt() resolution and never upscaled to match the
    // charSize_-times-larger output of toAsciiArt(). addBloomData() below
    // only overlaps the top-left corner correctly -- this reproduces a real
    // bug in the source rather than silently fixing it. If you want correct
    // bloom, upscale bloomData_ by charSize_ (e.g. nearest-neighbor repeat)
    // before calling addBloomData().
}

void AsciiArt::addBloomData() {
    int h = min(bloomHeight_, height_);
    int w = min(bloomWidth_, width_);
    for (int i = 0; i < h; ++i) {
        for (int j = 0; j < w; ++j) {
            image_[i * width_ + j] += bloomData_[i * bloomWidth_ + j];
        }
    }
}

// output

void AsciiArt::storeImage(const string& outputfile) const {
    vector<unsigned char> out(static_cast<size_t>(height_) * width_);
    for (int i = 0; i < height_ * width_; ++i) {
        float v = clamp(image_[i], 0.0f, 1.0f); // plt.imsave clips for you; do it explicitly here
        out[i] = static_cast<unsigned char>(v * 255.0f);
    }
    stbi_write_png(outputfile.c_str(), width_, height_, 1, out.data(), width_);
}