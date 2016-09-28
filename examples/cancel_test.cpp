/*
  Copyright (c) 2016 Louis McCarthy
  All rights reserved.

  Licensed under MIT License, see LICENSE for full license text

  cancel_test.ccp - Sends the command to terminate the last capture request
                    and zeros the frame ready flag
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
    camera->CancelCapture();
    camera->GetStatus();
    camera->Disconnect();
}
