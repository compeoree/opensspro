/*
  Copyright (c) 2016 Louis McCarthy
  All rights reserved.

  Licensed under MIT License, see LICENSE for full license text

  status_test.ccp - A simple test program that queries the camera status and exits
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
    camera->Disconnect();
}
