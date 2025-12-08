// BMP-related data types based on Microsoft's own
#include <cstdint>
#include <cstring>

// Use C++ type aliases
using BYTE = uint8_t;
using DWORD = uint32_t;
using LONG = int32_t;
using WORD = uint16_t;

// The BITMAPFILEHEADER structure contains information about the type, size,
// and layout of a file that contains a DIB [device-independent bitmap].
// Adapted from http://msdn.microsoft.com/en-us/library/dd183374(VS.85).aspx.
#pragma pack(push, 1)  // Ensure no padding between members
struct BITMAPFILEHEADER
{
    WORD   bfType;
    DWORD  bfSize;
    WORD   bfReserved1;
    WORD   bfReserved2;
    DWORD  bfOffBits;
    
    // Optional: Add constructor for initialization
    BITMAPFILEHEADER() : bfType(0), bfSize(0), bfReserved1(0), 
                         bfReserved2(0), bfOffBits(0) {}
};
#pragma pack(pop)

// The BITMAPINFOHEADER structure contains information about the
// dimensions and color format of a DIB [device-independent bitmap].
// Adapted from http://msdn.microsoft.com/en-us/library/dd183376(VS.85).aspx.
#pragma pack(push, 1)  // Ensure no padding between members
struct BITMAPINFOHEADER
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
    
    // Optional: Add constructor for initialization
    BITMAPINFOHEADER() : biSize(0), biWidth(0), biHeight(0), biPlanes(0),
                         biBitCount(0), biCompression(0), biSizeImage(0),
                         biXPelsPerMeter(0), biYPelsPerMeter(0), biClrUsed(0),
                         biClrImportant(0) {}
};
#pragma pack(pop)

// The RGBTRIPLE structure describes a color consisting of relative intensities of
// red, green, and blue. Adapted from http://msdn.microsoft.com/en-us/library/aa922590.aspx.
#pragma pack(push, 1)  // Ensure no padding between members
struct RGBTRIPLE
{
    BYTE  rgbtBlue;
    BYTE  rgbtGreen;
    BYTE  rgbtRed;
    
    // Optional: Add constructors for convenience
    RGBTRIPLE() : rgbtBlue(0), rgbtGreen(0), rgbtRed(0) {}
    
    RGBTRIPLE(BYTE red, BYTE green, BYTE blue) 
        : rgbtBlue(blue), rgbtGreen(green), rgbtRed(red) {}
};
#pragma pack(pop)

// Alternative version using C++11/14 features:
namespace bmp {
    
    // Type aliases in a namespace to avoid potential conflicts
    using byte = uint8_t;
    using dword = uint32_t;
    using long32 = int32_t;
    using word = uint16_t;

    struct BitmapFileHeader {
        word   type;
        dword  size;
        word   reserved1;
        word   reserved2;
        dword  offBits;
        
        // Check if the file is a valid BMP
        bool isValid() const {
            // 'BM' in little-endian
            return type == 0x4D42; 
        }
    } __attribute__((packed));

    struct BitmapInfoHeader {
        dword  size;
        long32 width;
        long32 height;
        word   planes;
        word   bitCount;
        dword  compression;
        dword  sizeImage;
        long32 xPelsPerMeter;
        long32 yPelsPerMeter;
        dword  clrUsed;
        dword  clrImportant;
        
        // Helper methods
        bool is24Bit() const {
            return bitCount == 24;
        }
        
        bool isUncompressed() const {
            return compression == 0;
        }
    } __attribute__((packed));

    struct RgbTriple {
        byte blue;
        byte green;
        byte red;
        
        // Operator overloads for convenience
        bool operator==(const RgbTriple& other) const {
            return red == other.red && 
                   green == other.green && 
                   blue == other.blue;
        }
        
        // Conversion to grayscale (optional)
        byte toGrayscale() const {
            return static_cast<byte>(0.299 * red + 0.587 * green + 0.114 * blue);
        }
    } __attribute__((packed));
}
