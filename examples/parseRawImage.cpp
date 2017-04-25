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

    int rowCount = 0;
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
    long nelements = std::accumulate(&naxes[0],&naxes[naxis],1,std::multiplies<long>());
    std::valarray<unsigned short> row(width);
    std::valarray<unsigned short> image(nelements);

    printf("Parsing rows...");
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

            /* Add row to image (frames a+b, even's first)
            if (rowCount > 1016)
                image[std::slice(width*static_cast<unsigned short>((rowCount-1016)*2-1),width,1)] = row; // + (unsigned short)rowCount;
            else
                image[std::slice(width*static_cast<unsigned short>(rowCount*2),width,1)] = row;
            rowCount++;*/

            // Add row to image (swapped frames, b+a, odd's first)
            if (rowCount > 1016)
                image[std::slice(width*static_cast<unsigned short>((rowCount-1017)*2),width,1)] = row; // + (unsigned short)rowCount;
            else
                image[std::slice(width*static_cast<unsigned short>(rowCount*2+1),width,1)] = row;
            rowCount++;
        }
    }

    printf("Done\r\n");
    // Save image
    long firstPixel(1);
    long exposure(1500);
    pFits->pHDU().addKey("EXPOSURE", exposure,"Total Exposure Time");

    pFits->pHDU().write(firstPixel,nelements,image);
    std::cout << pFits->pHDU() << std::endl;

    free(data);
}
