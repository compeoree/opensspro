/*
  Copyright (c) 2016 Louis McCarthy
  All rights reserved.

  Licensed under MIT License, see LICENSE for full license text

  parseRawImage.ccp - Loads the raw image binary file and spits out debug info
*/

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
    for (long i=0; i<result; i++)
    {
        if (data[i] == 0x00)
        {
            if (++zeroCount == 18)
            {
                thisZeroIndex = i - 17;
                printf("Found row %d at index 0x%X, which is %d bytes since previous row\n", rowCount++, thisZeroIndex, thisZeroIndex - lastZeroIndex);
                lastZeroIndex = thisZeroIndex;
            }
        }
        else
            zeroCount = 0;
    }
    free(data);
}
