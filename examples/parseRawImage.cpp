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

    int zeroCount = 0;
    int rowCount = 0;
    int thisZeroIndex = 0;
    int lastZeroIndex = 0;
    int byteCount = 0;
    unsigned char lastByte;


    // FITS Variables
    long naxis = 2;
    long naxes[naxis] = { 3110, 2034 };

    std::auto_ptr<FITS> pFits(0);

    try
    {
        const std::string fileName("!sspro.fit");
        pFits.reset( new FITS(fileName , USHORT_IMG , naxis , naxes ) );
    }
    catch (FITS::CantCreate)
    {
          return -1;
    }

    long& width = naxes[0];
    //long& numberOfRows = naxes[1];
    long nelements = std::accumulate(&naxes[0],&naxes[naxis],1,std::multiplies<long>());
    std::valarray<unsigned short> row(width);
    std::valarray<unsigned short> image(nelements);

    for (unsigned long i=0; i<result; i++)
    {
        // create 16 bit values from 2 bytes
        if (byteCount % 2 != 0)
        {
            //printf("Adding bytes 0x%X and 0x%X to get short 0x%X for array index %d\n", data[i], lastByte, ((unsigned short)data[i] << 8) + lastByte, (byteCount-1)/2);
            row[(byteCount-1)/2] = ((unsigned short)data[i] << 8) + lastByte;
        }

        lastByte = data[i];

        if (++byteCount == 6220)
        {
            byteCount = 0;
            zeroCount = 0;
            printf("...good row\n");

            // Add row to image
            image[std::slice(width*static_cast<unsigned short>(rowCount),width,1)] = row + (unsigned short)rowCount;

            continue;
        }

        if (data[i] == 0x00)
        {
            if (++zeroCount == 18)
            {
                thisZeroIndex = i - 17;
                if (byteCount == zeroCount)
                {
                    printf("Found row %d at index 0x%X, which is %d bytes since previous row", rowCount++, thisZeroIndex, thisZeroIndex - lastZeroIndex);
                    lastZeroIndex = thisZeroIndex;
                }
                else
                {
                    //byteCount -= 18;
                    printf("...glitch...");
                }
                zeroCount = 0; // reset counter so a start byte of value 0x00 doesn't trip this again...
            }
        }
        else
        {
            if (zeroCount > 2 && zeroCount != 18)
                printf("Found %d zeros in a row at location 0x%lX\n", zeroCount, (unsigned long)i);
            zeroCount = 0;
        }
    }

    // Save image
    long firstPixel(1);
    long exposure(1500);
    pFits->pHDU().addKey("EXPOSURE", exposure,"Total Exposure Time");

    pFits->pHDU().write(firstPixel,nelements,image);
    std::cout << pFits->pHDU() << std::endl;

    free(data);
}
