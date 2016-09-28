/*
  Copyright (c) 2016 Louis McCarthy
  All rights reserved.

  Licensed under MIT License, see LICENSE for full License text

  opensspro.cpp - Basic USB driver for the Starshoot Pro V2.0 Color Imager
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libusb-1.0/libusb.h>
#include <unistd.h>

#include "opensspro.h"

#define USB_REQ_STATUS    0x02
#define USB_REQ_SET_FRAME 0x0B
#define USB_REQ_CAPTURE   0x03
#define USB_REQ_DOWNLOAD  0x04
#define USB_REQ_ABORT     0x05
#define USB_REQ_SET_DIO   0x0C
#define USB_REQ_UNKNOWN   0x09

#define USB_TIMEOUT      1000
#define USB_RX_ENDPOINT  0x82
#define USB_CMD_ENDPOINT 0x08
#define USB_CONTROL_TYPE 0x03
#define START_BYTE       0xA5

#define IMAGE_WIDTH       3040
#define IMAGE_HEIGHT      2024
#define BUFFER_SIZE       1024
#define MAX_TRANSFER_SIZE 12677612

using namespace OpenSSPRO;

libusb_context* usb = NULL;

SSPRO::SSPRO()
{
    lastImage.width = IMAGE_WIDTH;
    lastImage.height = IMAGE_HEIGHT;
    lastImage.data = NULL;

    DEBUG("Searching for USB root...");
    if (usb == NULL)
    {
        int result = libusb_init(&usb);
        if (result < 0)
            ERROR("Failed to init libusb, result = %d", result);
        else
            DEBUG("Ready\n");
    }
}

bool SSPRO::Connect()
{
    DEBUG("Looking for camera...");
    if ((this->device = libusb_open_device_with_vid_pid(usb, SSPRO_VENDOR_ID, SSPRO_PRODUCT_ID)) == NULL)
    {
        DEBUG("NOT found!\n");
        return false;
    }
    DEBUG("Found it!\n");

    DEBUG("Checking for kernel driver...");
    int result;
    if (libusb_kernel_driver_active(this->device, 0) == 1)
    {
        result = libusb_detach_kernel_driver(this->device, 0);
        if (result < 0)
            ERROR("Failed to detach kernel driver, result = %d", result);
    }
    DEBUG("Done\n");

    DEBUG("Setting USB configuration...");
    result = libusb_set_configuration(this->device, 1); // Only one configuration available
    if (result < 0)
        ERROR("Failed to set configuration, result = %d", result);
    DEBUG("Done\n");

    DEBUG("Claiming USB interface...");
    result = libusb_claim_interface(this->device, 0); // Only one interface available
    if (result < 0)
        ERROR("Failed to claim interface, result = %d", result);
    DEBUG("Done\n");

    this->Init();
    return true;
}

void SSPRO::Disconnect()
{
    DEBUG("Disconnecting camera...");
    if (this->device)
        libusb_close(this->device);
    this->device = NULL;
    DEBUG("Done\n");
}

bool SSPRO::IsConnected()
{
    return (this->device != NULL);
}

void SSPRO::GetStatus()
{
    DEBUG("Requesting camera status...");
    if (!this->SendCMD( USB_REQ_STATUS, 0x00, 0x00, 0x00, 0x00 ))
        ERROR("Failed to get status");
    else
        DEBUG("Done\n");
}

void SSPRO::SetupFrame()
{
    DEBUG("Setting up frame...");
    if (!this->SendCMD( USB_REQ_SET_FRAME, 0x00, 0x00, 0x03, 0xF9 ))
        ERROR("Failed to setup frame");
    else
        DEBUG("Done\n");
}

bool SSPRO::SendCMD(unsigned char cmd, unsigned char data0, unsigned char data1, unsigned char data2, unsigned char data3)
{
    int txCount;
    unsigned char data[6] = { START_BYTE, cmd, data0, data1, data2, data3 };
    int result = libusb_bulk_transfer(this->device, USB_CMD_ENDPOINT, data, 6, &txCount, USB_TIMEOUT);
    if (result < 0)
        return false;

    if (txCount != 6)
        return false;

    return this->GetCMDResult(cmd);
}

bool SSPRO::GetCMDResult(unsigned char cmd)
{
    DEBUG("Getting command result...");
    int rxCount;
    unsigned char rxData[8];
    int result = libusb_bulk_transfer(this->device, USB_RX_ENDPOINT, rxData, sizeof(rxData), &rxCount, USB_TIMEOUT);
    if (result < 0)
    {
        ERROR("Failed to read command result, returned %d", result);
        return false;
    }

    if (rxCount != 8)
    {
        ERROR("Invalid packet size, %d", rxCount);
        return false;
    }

    if (rxData[0] != 0xA5)
    {
        ERROR("Invalid header, %d", rxData[0]);
        return false;
    }

    if (rxData[2] != cmd)
    {
        ERROR("Mismatched command type, RX:%d vs CMD:%d", rxData[1], cmd);
        return false;
    }

    switch (rxData[2])
    {
        case USB_REQ_STATUS:
            DEBUG("Found Status Result\n");
            DEBUG("Data: %02x, %02x, %02x, %02x, %02x, %02x, %02x, %02x\n",
                    rxData[0], rxData[1], rxData[2], rxData[3], rxData[4], rxData[5], rxData[6], rxData[7]);
            capturing = (rxData[4] & 0x01) == 0x01;
            frameReady = (rxData[4] & 0x02) == 0x02;
            DEBUG("Capturing = %d, FrameReady = %d\n", capturing, frameReady);
            return true;
            break;
        case USB_REQ_SET_FRAME:
            DEBUG("Found Frame Result\n");
            DEBUG("Data: %02x, %02x, %02x, %02x, %02x, %02x, %02x, %02x\n",
                    rxData[0], rxData[1], rxData[2], rxData[3], rxData[4], rxData[5], rxData[6], rxData[7]);
            return true;
            break;
        case USB_REQ_CAPTURE:
            DEBUG("Found Capture Result\n");
            DEBUG("Data: %02x, %02x, %02x, %02x, %02x, %02x, %02x, %02x\n",
                    rxData[0], rxData[1], rxData[2], rxData[3], rxData[4], rxData[5], rxData[6], rxData[7]);
            return true;
            break;
        case USB_REQ_DOWNLOAD:
            DEBUG("Found Download Result\n");
            DEBUG("Data: %02x, %02x, %02x, %02x, %02x, %02x, %02x, %02x\n",
                    rxData[0], rxData[1], rxData[2], rxData[3], rxData[4], rxData[5], rxData[6], rxData[7]);
            return true;
            break;
        case USB_REQ_ABORT:
            DEBUG("Found Abort Result\n");
            DEBUG("Data: %02x, %02x, %02x, %02x, %02x, %02x, %02x, %02x\n",
                    rxData[0], rxData[1], rxData[2], rxData[3], rxData[4], rxData[5], rxData[6], rxData[7]);
            return true;
            break;
        case USB_REQ_SET_DIO:
            DEBUG("Found DIO Result\n");
            DEBUG("Data: %02x, %02x, %02x, %02x, %02x, %02x, %02x, %02x\n",
                    rxData[0], rxData[1], rxData[2], rxData[3], rxData[4], rxData[5], rxData[6], rxData[7]);
            return true;
            break;
        case USB_REQ_UNKNOWN:
            DEBUG("Found 0x09 Result\n");
            DEBUG("Data: %02x, %02x, %02x, %02x, %02x, %02x, %02x, %02x\n",
                    rxData[0], rxData[1], rxData[2], rxData[3], rxData[4], rxData[5], rxData[6], rxData[7]);
            return true;
            break;
        default:
            ERROR("Unknown command result, %d", rxData[1]);
            break;
    }

    return false;
}

struct rawImage* SSPRO::Capture(int ms)
{
    DEBUG("Performing a blocking capture...\n");
    if (!this->StartCapture(ms))
        return NULL;

    usleep((ms+100)*1000); // Wait capture time + 100 ms

    int count = 0;
    while (!this->frameReady && count++ < 10)
    {
        this->GetStatus();
        usleep(500*1000); // Wait 500 ms
    }

    if (count >= 10)
        return NULL;

    if (!this->DownloadFrame())
        return NULL;

    return &lastImage;
}

bool SSPRO::StartCapture(int ms)
{
    this->SetupFrame();

    DEBUG("Starting capture...");
    if (!this->SendCMD( USB_REQ_CAPTURE, 0x00, 0x26, 0x39, 0x02 )) // 10s Color 1x1 binning
    {
        ERROR("Failed to capture");
        return false;
    }
    DEBUG("Done\n");

    return true;
}

void SSPRO::CancelCapture()
{
    DEBUG("Attempting to cancel capture...");
    if (!this->SendCMD( USB_REQ_ABORT, 0x00, 0x00, 0x00, 0x00 ))
        ERROR("Failed to cancel capture");
    else
        DEBUG("Done\n");
}

bool SSPRO::DownloadFrame()
{
    if (!frameReady)
        return false;

    DEBUG("Requesting frame download...");
    if (!this->SendCMD( USB_REQ_DOWNLOAD, 0x00, 0x00, 0x00, 0x00 ))
    {
        ERROR("Failed to start download");
        return false;
    }
    DEBUG("Done\n");

    DEBUG("Waiting for data from camera...");
    int rxCount;
    unsigned char* rxData = (unsigned char*)malloc(BUFFER_SIZE);
    unsigned char* newImage = (unsigned char*)malloc(MAX_TRANSFER_SIZE);
    unsigned char* pointer = newImage;
    // Loop through until we get a partial buffer of data
    do
    {
        int result = libusb_bulk_transfer(this->device, USB_RX_ENDPOINT, rxData, BUFFER_SIZE, &rxCount, USB_TIMEOUT);
        if (result < 0)
        {
            ERROR("Failed to download image, result = %d", result);
            free(rxData);
            return false;
        }

        DEBUG(".");
        memcpy(pointer, rxData, rxCount);
        pointer += rxCount; // Using BUFFER_SIZE here will push it off the end of the object on the last packet
    } while (rxCount == BUFFER_SIZE);
    free(rxData);
    DEBUG("Done\n");

    DEBUG("Updating lastImage...");
    if (lastImage.data)
        free(lastImage.data);
    lastImage.data = newImage;
    lastImage.width = IMAGE_WIDTH;
    lastImage.height = IMAGE_HEIGHT;
    DEBUG("Done\n");

    return true;
}

void SSPRO::SetDIO()
{
    DEBUG("Setting DIO...");
    unsigned char dio =0x00;
    if (fanHigh)
        dio |= 0x02;
    if (coolerOn)
        dio |= 0x01;
    if (!this->SendCMD( USB_REQ_SET_DIO, dio, 0x00, 0x00, 0x00 ))
        ERROR("Failed to set DIO");
    else
        DEBUG("Done\n");
}

void SSPRO::Init()
{
    DEBUG("Initializing camera...");
    fanHigh = false;
    coolerOn = true;
    frameReady = false;
    readoutSpeed = READOUT_FASTEST;
    DEBUG("Done\n");

    this->SetDIO();
}
