/*
  Copyright (c) 2016 Louis McCarthy
  All rights reserved.

  Licensed under MIT License, see LICENSE for full license text

  parseRawImage.ccp - Loads the raw image binary file and spits out debug info
*/

#include <CCfits>
#include <cmath>

// The library is enclosed in a namespace.
using namespace CCfits;
using std::valarray;

#include <stdio.h>
#include <stdlib.h>

#define MAX_TRANSFER_SIZE 12677612
#define MAX_HEIGHT 2028
#define MAX_WIDTH  3040
#define ROW_LENGTH_BYTES 6220

int main()
{
    FILE* newFile = fopen("raw.image", "r");
    if (newFile == NULL)
    {
        printf("Failed to open file\n");
        return -1;
    }

    unsigned char* data = (unsigned char*)malloc(MAX_TRANSFER_SIZE);
    size_t result = fread(data, 1, MAX_TRANSFER_SIZE, newFile);
    fclose(newFile);

    printf("Read %lu bytes\n", result);

    int rowCount = 0;
    unsigned char lastByte;


    // FITS Variables
    long naxis = 2;
    long naxes[naxis] = { MAX_WIDTH, MAX_HEIGHT };

    std::auto_ptr<FITS> pFits(0);

    try
    {
        const std::string fileName("!sspro_mxdl.fit");
        pFits.reset( new FITS(fileName , USHORT_IMG , naxis , naxes ) );
    }
    catch (FITS::CantCreate)
    {
        return -1;
    }

    long& width = naxes[0];
    long nelements = std::accumulate(&naxes[0],&naxes[naxis],1,std::multiplies<long>());
    std::valarray<unsigned short> row(width);
    std::valarray<unsigned short> image(nelements);

    printf("Parsing rows...");
    int zeroCount = 0;
    unsigned int lastIndex = 0;
    unsigned int rowIndexes[MAX_HEIGHT];

    // Find and validate row starts
    for (unsigned int i=0; i<result; i++)
    {
        if (data[i] == 0x00)
        {
            if (++zeroCount==18)
            {
                int rowSize = i - lastIndex;

                // Is this a valid row?
                if (rowSize == ROW_LENGTH_BYTES || lastIndex == 0)
                {
                    rowIndexes[rowCount++] = i - 17; // Set start of row to beginning of zeros
                    printf("\r\nRow %d: starts at location 0x%x", rowCount, i - 17);
                }
                else
                {
                    printf("\r\nToo many bytes (%d) in row....", rowSize);
                    if (rowSize % ROW_LENGTH_BYTES == 0)
                    {
                        printf("good, byte count is a multiple of rows...parsing\r\n");
                        while (lastIndex < i)
                        {
                            rowIndexes[rowCount++] = lastIndex - 17 + ROW_LENGTH_BYTES;
                            printf("\r\nRow %d: starts at location 0x%x", rowCount, lastIndex - 17 + ROW_LENGTH_BYTES);
                            lastIndex += ROW_LENGTH_BYTES;
                        }
                    }
                    else
                    {
                        printf("\r\nInvalid byte count...giving up\r\n");
                    }
                }

                lastIndex = i;
                zeroCount = 0;
            }
        }
        else
        {
            //if (zeroCount > 4)
            //    printf("\r\nFound %d, zero's in a row....0x%x\r\n", zeroCount, i);
            zeroCount=0;
        }
    }

    for (int fitsRows=3;fitsRows<rowCount;fitsRows++)
    {
        int start = 0;

        // Add first rows (3-1010, raw frame is 0-1016)
        if (fitsRows <= 1010)
        {
            const short frontPorch = 60 * 2; // Pixels * Bytes per pixel
            const short backPorch = 10 * 2;  // Pixels * Bytes per pixel

            start = rowIndexes[fitsRows] + frontPorch;

            for (int pixelBytes=0;pixelBytes<(ROW_LENGTH_BYTES - (frontPorch + backPorch));pixelBytes++)
            {
                // create 16 bit values from 2 bytes
                if (pixelBytes % 2 != 0)
                    row[(pixelBytes-1)/2] = ((unsigned short)data[start + pixelBytes] << 8) + lastByte;

                lastByte = data[start + pixelBytes];
            }

            //image[std::slice(width*static_cast<unsigned short>(fitsRows*2+1),width,1)] = row; // Odds first
            image[std::slice(width*static_cast<unsigned short>(fitsRows*2),width,1)] = row; // Evens first
        }

        // Add last rows (1021-1010, raw frame is 1017-2032)
        if (fitsRows >= 1021 && fitsRows < 2032)
        {
            const short frontPorch = 70 * 2; // Pixels * Bytes per pixel
            const short backPorch = 0 * 2;   // Pixels * Bytes per pixel

            start = rowIndexes[fitsRows] + frontPorch;

            for (int pixelBytes=0;pixelBytes<(ROW_LENGTH_BYTES - (frontPorch + backPorch));pixelBytes++)
            {
                // create 16 bit values from 2 bytes
                if (pixelBytes % 2 != 0)
                    row[(pixelBytes-1)/2] = ((unsigned short)data[start + pixelBytes] << 8) + lastByte;

                lastByte = data[start + pixelBytes];
            }

            //image[std::slice(width*static_cast<unsigned short>((fitsRows-1017)*2),width,1)] = row; // Odds first
            image[std::slice(width*static_cast<unsigned short>((fitsRows-1016)*2-1),width,1)] = row; // Evens first
        }
    }

    printf("\r\nDone parsing\r\n\r\nGenerating FITS file:\r\n");

    // Save image
    long firstPixel(1);
    long exposure(1500);
    pFits->pHDU().addKey("EXPOSURE", exposure,"Total Exposure Time");

    pFits->pHDU().write(firstPixel,nelements,image);
    std::cout << pFits->pHDU() << std::endl;

    free(data);
}
