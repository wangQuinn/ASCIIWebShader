    
// It WILL compile without the required assests (edgesASCII.ong, fillASCII.png, sample.png); it will throw a runtime_error and exit!!

#include "ascii_art.h"
#include <iostream>
#include <filesystem>
#include <stdio.h>

using namespace std; 

static string stemOf(const string& path) {
    return filesystem::path(path).stem().string();
}

int main() {
    const float sigma = 2.0f;
    const float scale = 1.6f;
    const float tau = 1.0f;
    const float threshold = 0.3f;
    const float magnitude_threshold = 0.2f;
    const int char_size = 8;              // must match the glyph strips' cell size
    const int downsample_threshold = 12;
    const float bloom_threshold = 0.8f;
    const float bloom_sigma = 50.0f;
    const int quantize_size = 10;

    const string image_path = "images/e.png";
    const string image_name = stemOf(image_path);
    const string edges_texture = "res/edgesASCII.png";
    const string fill_texture = "res/fillASCII.png";

    try {
        // --- edge detection pass ---
        AsciiArt edges(image_path, edges_texture);

        // preprocessor for the sobel filter, to reduce noise
        // reference: https://users.cs.northwestern.edu/~sco590/winnemoeller-cag2012.pdf
        edges.differenceOfGaussian(sigma, scale, tau, threshold);

        // apply sobel filter and find angle based on gradient
        edges.sobel();
        edges.getMagnitude(magnitude_threshold);
        edges.findAngle();

        // quantize image into edge-direction buckets
        edges.edgeQuantize();
        edges.controlledDownsample(char_size, downsample_threshold);

        // convert edges to characters (slashes/dashes/pipes)
        edges.toAsciiArt(/*edge_mode=*/true);

        // store edge data
        edges.storeImage("output/" + image_name + "_edges.png");

        // fill pass
        AsciiArt fill(image_path, fill_texture);

        // bloom
        fill.getBloomData(bloom_threshold, bloom_sigma);

        // downsample image
        fill.downsample(char_size);

        // quantize into `quantize_size` brightness levels (10 ASCII chars)
        fill.quantize(quantize_size);

        // convert image to ASCII art
        fill.toAsciiArt(false);

        // combine edge glyphs on top of fill glyphs
        fill.combine(edges);

        // add optional bloom data
        fill.addBloomData();

        // store output
        fill.storeImage("output/" + image_name + "_final_with_bloom.png");

        cout << "done: output/" << image_name << "_final_with_bloom.png\n";
    } catch (const exception& e) {
        cerr << "error: " << e.what() << "\n";
        return 1;
    }

    return 0;
}