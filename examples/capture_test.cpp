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
    camera->Capture(120000);
    camera->Disconnect();
}
