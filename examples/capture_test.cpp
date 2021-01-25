/*
  Copyright (c) 2016 Louis McCarthy
  All rights reserved.

  Licensed under MIT License, see LICENSE for full license text

  capture_test.ccp - Performs a simple capture test and exits
*/

#include <stdio.h>

#include "../src/opensspro.h"

int main()
{
    OpenSSPRO::SSPRO* camera = new OpenSSPRO::SSPRO();

    if (!camera->Connect())
    {
        printf("Failed to connect to camera\n");
        return -1;
    }

    camera->GetStatus();
    OpenSSPRO::rawImage* newImage = camera->Capture(30000);
    FILE* newFile = fopen("raw10s.image", "w");
    fwrite(newImage->data, 1, newImage->dataSize, newFile);
    fclose(newFile);
    camera->Disconnect();
}
