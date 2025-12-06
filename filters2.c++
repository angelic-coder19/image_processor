#include <getopt.h>
#include <iostream>
#include <cstdlib>
#include <cmath>
#include <fstream>
#include <vector>
#include <memory>
#include <stdexcept>
#include <algorithm>
#include <cstdint>

// BMP-related data types based on Microsoft's own

// These data types are essentially aliases for C/C++ primitive data types.
// Adapted from http://msdn.microsoft.com/en-us/library/cc230309.aspx.
// See https://en.wikipedia.org/wiki/C_data_types#stdint.h for more on stdint.h.

typedef uint8_t  BYTE;
typedef uint32_t DWORD;
typedef int32_t  LONG;
typedef uint16_t WORD;

// The BITMAPFILEHEADER structure contains information about the type, size,
// and layout of a file that contains a DIB [device-independent bitmap].
// Adapted from http://msdn.microsoft.com/en-us/library/dd183374(VS.85).aspx.

typedef struct
{
    WORD   bfType;
    DWORD  bfSize;
    WORD   bfReserved1;
    WORD   bfReserved2;
    DWORD  bfOffBits;
} __attribute__((__packed__))
BITMAPFILEHEADER;

// The BITMAPINFOHEADER structure contains information about the
// dimensions and color format of a DIB [device-independent bitmap].
// Adapted from http://msdn.microsoft.com/en-us/library/dd183376(VS.85).aspx.

typedef struct
{
    DWORD  biSize;
    LONG   biWidth;
    LONG   biHeight;
    WORD   biPlanes;
    WORD   biBitCount;
    DWORD  biCompression;
    DWORD  biSizeImage;
    LONG   biXPelsPerMeter;
    LONG   biYPelsPerMeter;
    DWORD  biClrUsed;
    DWORD  biClrImportant;
} __attribute__((__packed__))
BITMAPINFOHEADER;

// The RGBTRIPLE structure describes a color consisting of relative intensities of
// red, green, and blue. Adapted from http://msdn.microsoft.com/en-us/library/aa922590.asp.

typedef struct
{
    BYTE  rgbtBlue;
    BYTE  rgbtGreen;
    BYTE  rgbtRed;
} __attribute__((__packed__))
RGBTRIPLE;

// Prototypes for funtions
void grayscale(int height, int width, std::vector<std::vector<RGBTRIPLE>>& image);

void sepia(int height, int width, std::vector<std::vector<RGBTRIPLE>>& image);

void reflect(int height, int width, std::vector<std::vector<RGBTRIPLE>>& image);

void blur(int height, int width, std::vector<std::vector<RGBTRIPLE>>& image);

void edges(int height, int width, std::vector<std::vector<RGBTRIPLE>>& image);

// Helper function for clamping values 
BYTE clamp(int value, int min, int max);

int main(int argc, char *argv[])
{
    // Define allowable filters
    const char* filters = "bgrse";

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

        // Edge detection
        case 'e':
            edges(height, width, image);
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

    // File automatically closed 
    return 0;
}


// Convert image to grayscale
void grayscale(int height, int width, std::vector<std::vector<RGBTRIPLE>>& image)
{
    for (int row = 0; row < height; ++row)
    {
        for (int col = 0; col < width; ++col)
        {
            // Calculate average of the three RGB values
            int average = static_cast<int>(std::round(
                (image[row][col].rgbtBlue + 
                 image[row][col].rgbtRed + 
                 image[row][col].rgbtGreen) / 3.0));
            
            image[row][col].rgbtBlue = static_cast<BYTE>(average);
            image[row][col].rgbtRed = static_cast<BYTE>(average);
            image[row][col].rgbtGreen = static_cast<BYTE>(average);
        }
    }
}

// Convert image to sepia
void sepia(int height, int width, std::vector<std::vector<RGBTRIPLE>>& image)
{
    for (int i = 0; i < height; ++i)
    {
        for (int j = 0; j < width; ++j)
        {
            // Calculate sepia values
            int sepiaRed = static_cast<int>(std::round(
                0.393 * image[i][j].rgbtRed + 
                0.769 * image[i][j].rgbtGreen + 
                0.189 * image[i][j].rgbtBlue));
            
            int sepiaGreen = static_cast<int>(std::round(
                0.349 * image[i][j].rgbtRed + 
                0.686 * image[i][j].rgbtGreen + 
                0.168 * image[i][j].rgbtBlue));
            
            int sepiaBlue = static_cast<int>(std::round(
                0.272 * image[i][j].rgbtRed + 
                0.534 * image[i][j].rgbtGreen + 
                0.131 * image[i][j].rgbtBlue));

            // Clamp values to 0-255 range
            sepiaRed = std::min(sepiaRed, 255);
            sepiaGreen = std::min(sepiaGreen, 255);
            sepiaBlue = std::min(sepiaBlue, 255);

            // Ensure non-negative values
            sepiaRed = std::max(sepiaRed, 0);
            sepiaGreen = std::max(sepiaGreen, 0);
            sepiaBlue = std::max(sepiaBlue, 0);

            image[i][j].rgbtRed = static_cast<BYTE>(sepiaRed);
            image[i][j].rgbtGreen = static_cast<BYTE>(sepiaGreen);
            image[i][j].rgbtBlue = static_cast<BYTE>(sepiaBlue);
        }
    }
}

// Reflect image horizontally
void reflect(int height, int width, std::vector<std::vector<RGBTRIPLE>>& image)
{
    for (int i = 0; i < height; ++i)
    {
        // Swap each pixel with the corresponding pixel on the other side
        for (int j = 0; j < width / 2; ++j)
        {
            std::swap(image[i][j], image[i][width - j - 1]);
        }
    }
}

// Blur image using box blur algorithm
void blur(int height, int width, std::vector<std::vector<RGBTRIPLE>>& image)
{
    // Create a copy of the image using vector
    std::vector<std::vector<RGBTRIPLE>> copy(height, std::vector<RGBTRIPLE>(width));
    
    for (int i = 0; i < height; ++i)
    {
        for (int j = 0; j < width; ++j)
        {
            copy[i][j] = image[i][j];
        }
    }

    // Iterate over each pixel
    for (int i = 0; i < height; ++i)
    {
        for (int j = 0; j < width; ++j)
        {
            int totalRed = 0, totalGreen = 0, totalBlue = 0;
            float pixelCount = 0.0f;

            // 3x3 kernel for box blur
            for (int dx = -1; dx <= 1; ++dx)
            {
                for (int dy = -1; dy <= 1; ++dy)
                {
                    int neighborX = i + dx;
                    int neighborY = j + dy;
                    
                    // Check bounds
                    if (neighborX >= 0 && neighborX < height && 
                        neighborY >= 0 && neighborY < width)
                    {
                        totalRed += image[neighborX][neighborY].rgbtRed;
                        totalGreen += image[neighborX][neighborY].rgbtGreen;
                        totalBlue += image[neighborX][neighborY].rgbtBlue;
                        pixelCount += 1.0f;
                    }
                }
            }

            // Calculate averages and update copy
            copy[i][j].rgbtRed = static_cast<BYTE>(std::round(totalRed / pixelCount));
            copy[i][j].rgbtGreen = static_cast<BYTE>(std::round(totalGreen / pixelCount));
            copy[i][j].rgbtBlue = static_cast<BYTE>(std::round(totalBlue / pixelCount));
        }
    }

    // Copy results back to original image
    for (int i = 0; i < height; ++i)
    {
        for (int j = 0; j < width; ++j)
        {
            image[i][j] = copy[i][j];
        }
    }
}

// Detect edges
void edges(int height, int width, std::vector<std::vector<RGBTRIPLE>>& image)
{
    // Matrices to represent the Kernels for the Sobel Algorithm
    int Gx[] = {-1, 0, 1, -2, 0, 2, -1, 0, 1};

    int Gy[] = {-1, -2, -1, 0, 0, 0, 1, 2, 1};

    // Create copy of image to work calculations on
    std::vector<std::vector<RGBTRIPLE>> copy(height, std::vector<RGBTRIPLE>(width));

    for (int i = 0; i < height; i++)
    {
        for (int j = 0; j < width; j++)
        {
            copy[i][j] = image[i][j];
        }
    }

    // Iterate over each of the pixels in the copy image
    for (int i = 0; i < height; i++)
    {
        for (int j = 0; j < width; j++)
        {
            int GxRed = 0; int GxBlue = 0; int GxGreen = 0;
            int GyRed = 0; int GyBlue = 0; int GyGreen = 0;


            // To determine the current row in kernel matrix
            int kernel_position = 0;

            // Iterate over a 3 by 3 box around each pixel
            for (int y = i - 1; y <= i + 1; y++)
            {
                for (int x = j - 1; x <= j + 1; x++)
                {
                    // If a pixel is on the edge assume it is a black pixel
                    if (!(x < 0 || x >= width || y < 0 || y >= height))
                    {
                        // Multiply each pixels values by its corresponding value in Gx kernel matrix
                        GxRed += image[y][x].rgbtRed * Gx[kernel_position];
                        GxBlue += image[y][x].rgbtBlue * Gx[kernel_position];
                        GxGreen += image[y][x].rgbtGreen * Gx[kernel_position];

                        // The same in the Gy Kernel matrix
                        GyRed += image[y][x].rgbtRed * Gy[kernel_position];
                        GyBlue += image[y][x].rgbtBlue * Gy[kernel_position];
                        GyGreen += image[y][x].rgbtGreen * Gy[kernel_position];
                    }

                    kernel_position++;
                }
            }

            // Find unique value for each pixel from the
            int newRed =  static_cast<int>(std::round(std::sqrt(std::pow(GxRed, 2) + std::pow(GyRed, 2))));
            if (newRed > 255) {// Cap each value to a 255 if it exceeds it
                newRed = 255; }

            int newBlue =  static_cast<int>(std::round(std::sqrt(std::pow(GxBlue, 2) + std::pow(GyBlue, 2))));
            if (newBlue > 255) {
                newBlue = 255; }

            int newGreen =  static_cast<int>(std::round(std::sqrt(std::pow(GxGreen, 2) + std::pow(GyGreen, 2))));
            if (newGreen > 255) {
                newGreen = 255; }

            copy[i][j].rgbtRed = static_cast<BYTE>(newRed);
            copy[i][j].rgbtBlue = static_cast<BYTE>(newBlue);
            copy[i][j].rgbtGreen = static_cast<BYTE>(newGreen);
        }
    }

    // Reassign each of the copy image pixels to the original image
    for (int i = 0; i < height; i++)
    {
        for (int j = 0; j < width; j++)
        {
            image[i][j] = copy[i][j];
        }
    }
}

namespace image_filters {    
    // Grayscale 
    void grayscale(std::vector<std::vector<RGBTRIPLE>>& image)
    {
        int height = image.size();
        if (height == 0) return;
        int width = image[0].size();
        
        for (auto& row : image)
        {
            for (auto& pixel : row)
            {
                BYTE gray = clamp(static_cast<int>(std::round(
                    (pixel.rgbtRed + pixel.rgbtGreen + pixel.rgbtBlue) / 3.0)), 0, 255);
                pixel.rgbtRed = pixel.rgbtGreen = pixel.rgbtBlue = gray;
            }
        }
    }
    
    // Sepia
    void sepia(std::vector<std::vector<RGBTRIPLE>>& image)
    {
        for (auto& row : image)
        {
            for (auto& pixel : row)
            {
                int sepiaRed = static_cast<int>(std::round(0.393 * pixel.rgbtRed + 
                                          0.769 * pixel.rgbtGreen + 
                                          0.189 * pixel.rgbtBlue));
                
                int sepiaGreen = static_cast<int>(std::round(0.349 * pixel.rgbtRed + 
                                            0.686 * pixel.rgbtGreen + 
                                            0.168 * pixel.rgbtBlue));
                
                int sepiaBlue = static_cast<int>(std::round(0.272 * pixel.rgbtRed + 
                                           0.534 * pixel.rgbtGreen + 
                                           0.131 * pixel.rgbtBlue));
                
                pixel.rgbtRed = clamp(sepiaRed, 0, 255);
                pixel.rgbtGreen = clamp(sepiaGreen, 0, 255);
                pixel.rgbtBlue = clamp(sepiaBlue, 0, 255);
            }
        }
    }
    
    // Reflect 
    void reflect(std::vector<std::vector<RGBTRIPLE>>& image)
    {
        for (auto& row : image)
        {
            std::reverse(row.begin(), row.end());
        }
    }
    
    // Blur with bounds checking helper
    bool inBounds(int x, int y, int height, int width)
    {
        return x >= 0 && x < height && y >= 0 && y < width;
    }
    
    void blur(std::vector<std::vector<RGBTRIPLE>>& image)
    {
        int height = image.size();
        if (height == 0) return;
        int width = image[0].size();
        
        auto copy = image; // Create copy
        
        for (int i = 0; i < height; ++i)
        {
            for (int j = 0; j < width; ++j)
            {
                int totalRed = 0, totalGreen = 0, totalBlue = 0;
                int count = 0;
                
                // Iterate over 3x3 neighborhood
                for (int dx = -1; dx <= 1; ++dx)
                {
                    for (int dy = -1; dy <= 1; ++dy)
                    {
                        int nx = i + dx;
                        int ny = j + dy;
                        
                        if (inBounds(nx, ny, height, width))
                        {
                            totalRed += image[nx][ny].rgbtRed;
                            totalGreen += image[nx][ny].rgbtGreen;
                            totalBlue += image[nx][ny].rgbtBlue;
                            ++count;
                        }
                    }
                }
                
                copy[i][j].rgbtRed = clamp(static_cast<int>(std::round(static_cast<float>(totalRed) / count)), 0, 255);
                copy[i][j].rgbtGreen = clamp(static_cast<int>(std::round(static_cast<float>(totalGreen) / count)), 0, 255);
                copy[i][j].rgbtBlue = clamp(static_cast<int>(std::round(static_cast<float>(totalBlue) / count)), 0, 255);
            }
        }
        
        // Assign the new image t0 the 0riginal image
        image = copy;
    }
}

// Helper function for clamping values
BYTE clamp(int value, int min, int max)
{
    if (value < min) return static_cast<BYTE>(min);
    if (value > max) return static_cast<BYTE>(max);
    return static_cast<BYTE>(value);
}