
#ifndef __READ_BMP_H__
#define __READ_BMP_H__

typedef struct {                    // Offset   Size
  short   bfType;                 //      0      2
  long    bfSize;                 //      2      4
  short   bfReserved1;            //      6      2
  short   bfReserved2;            //      8      2
  long    bfOffBits;              //     10      4
} BITMAPFILEHEADER;                 // Total size: 14

typedef struct {  // Offset   Size
    long    biSize;                 //      0      4
    long    biWidth;                //      4      4
    long    biHeight;               //      8      4
    short   biPlanes;               //     12      2
    short   biBitCount;             //     14      2
    long    biCompression;          //     16      4
    long    biSizeImage;            //     20      4
    long    biXPelsPerMeter;        //     24      4
    long    biYPelsPerMeter;        //     28      4
    long    biClrUsed;              //     32      4
    long    biClrImportant;         //     36      4
} BITMAPINFOHEADER;                 // Total size: 40

#endif /* __READ_BMP_H__ */
