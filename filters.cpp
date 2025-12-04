#include <getopt.h>
#include <iostream>
#include <cstdlib>
#include <cmath>
#include <fstream>
#include <vector>
#include <memory>
#include <stdexcept>

// Assuming these structures are defined in helpers.h
#include "helpers.h"

int main(int argc, char *argv[])
{
    // Define allowable filters
    const char* filters = "bgrs";

    // Get filter flag and check validity
    char filter = getopt(argc, argv, filters);
    if (filter == '?')
    {
        std::cerr << "Invalid filter.\n";
        return 1;
    }

    // Ensure only one filter
    if (getopt(argc, argv, filters) != -1)
    {
        std::cerr << "Only one filter allowed.\n";
        return 2;
    }

    // Ensure proper usage
    if (argc != optind + 2)
    {
        std::cerr << "Usage: ./filter [flag] infile outfile\n";
        return 3;
    }

    // Remember filenames
    const char* infile = argv[optind];
    const char* outfile = argv[optind + 1];

    // Open input file
    std::ifstream inptr(infile, std::ios::binary);
    if (!inptr.is_open())
    {
        std::cerr << "Could not open " << infile << ".\n";
        return 4;
    }

    // Open output file
    std::ofstream outptr(outfile, std::ios::binary);
    if (!outptr.is_open())
    {
        std::cerr << "Could not create " << outfile << ".\n";
        return 5;
    }

    // Read infile's BITMAPFILEHEADER
    BITMAPFILEHEADER bf;
    inptr.read(reinterpret_cast<char*>(&bf), sizeof(BITMAPFILEHEADER));

    // Read infile's BITMAPINFOHEADER
    BITMAPINFOHEADER bi;
    inptr.read(reinterpret_cast<char*>(&bi), sizeof(BITMAPINFOHEADER));

    // Ensure infile is (likely) a 24-bit uncompressed BMP 4.0
    if (bf.bfType != 0x4d42 || bf.bfOffBits != 54 || bi.biSize != 40 ||
        bi.biBitCount != 24 || bi.biCompression != 0)
    {
        std::cerr << "Unsupported file format.\n";
        return 6;
    }

    // Get image's dimensions
    int height = abs(bi.biHeight);
    int width = bi.biWidth;

    // Allocate memory for image using vector
    std::vector<std::vector<RGBTRIPLE>> image(height, std::vector<RGBTRIPLE>(width));

    // Determine padding for scanlines
    int padding = (4 - (width * sizeof(RGBTRIPLE)) % 4) % 4;

    // Iterate over infile's scanlines
    for (int i = 0; i < height; i++)
    {
        // Read row into pixel array
        inptr.read(reinterpret_cast<char*>(image[i].data()), width * sizeof(RGBTRIPLE));
        
        // Skip over padding
        inptr.seekg(padding, std::ios::cur);
    }

    // Filter image
    switch (filter)
    {
        // Blur
        case 'b':
            blur(height, width, image);
            break;

        // Grayscale
        case 'g':
            grayscale(height, width, image);
            break;

        // Reflection
        case 'r':
            reflect(height, width, image);
            break;

        // Sepia
        case 's':
            sepia(height, width, image);
            break;
    }

    // Write outfile's BITMAPFILEHEADER
    outptr.write(reinterpret_cast<const char*>(&bf), sizeof(BITMAPFILEHEADER));

    // Write outfile's BITMAPINFOHEADER
    outptr.write(reinterpret_cast<const char*>(&bi), sizeof(BITMAPINFOHEADER));

    // Write new pixels to outfile
    for (int i = 0; i < height; i++)
    {
        // Write row to outfile
        outptr.write(reinterpret_cast<const char*>(image[i].data()), width * sizeof(RGBTRIPLE));

        // Write padding at end of row
        for (int k = 0; k < padding; k++)
        {
            outptr.put(0x00);
        }
    }

    // Files are automatically closed when objects go out of scope
    return 0;
}