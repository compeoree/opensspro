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
#include <time.h>
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

#define IMAGE_WIDTH  3040
#define IMAGE_HEIGHT 2024
#define BUFFER_SIZE  1024

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

void SSPRO::GetStatus()
{
    DEBUG("Requesting camera status...");
    int txCount;
    unsigned char data[6] = { START_BYTE, USB_REQ_STATUS, 0x00, 0x00, 0x00, 0x00 };
    int result = libusb_bulk_transfer(this->device, USB_CMD_ENDPOINT, data, 6, &txCount, USB_TIMEOUT);
    if (result < 0)
        ERROR("Failed to get status, result = %d", result);
    else
        DEBUG("Done\n");

    this->GetCMDResult(USB_REQ_STATUS);
}

void SSPRO::SetupFrame()
{
    DEBUG("Setting up frame...");
    int txCount;
    unsigned char data[6] = { START_BYTE, USB_REQ_SET_FRAME, 0x00, 0x00, 0x03, 0xF9 };
    int result = libusb_bulk_transfer(this->device, USB_CMD_ENDPOINT, data, 6, &txCount, USB_TIMEOUT);
    if (result < 0)
        ERROR("Failed to setup frame, result = %d", result);
    else
        DEBUG("Done\n");

    this->GetCMDResult(USB_REQ_SET_FRAME);
}

bool SSPRO::IsConnected()
{
    return (this->device != NULL);
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

    if (rxData[1] != cmd)
    {
        ERROR("Mismatched command type, RX:%d vs CMD:%d", rxData[1], cmd);
        return false;
    }

    switch (rxData[1])
    {
        case USB_REQ_STATUS:
            DEBUG("Found Status Result\n");
            DEBUG("Data: %02x, %02x, %02x, %02x, %02x, %02x, %02x, %02x\n",
                    rxData[0], rxData[1], rxData[2], rxData[3], rxData[4], rxData[5], rxData[6], rxData[7]);
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

    // Wait capture time + 100 ms
    sleep(ms+100);

    // while status == busy
    while (this->capturing)
    {
        this->GetStatus();
        sleep(500);
    }

    this->DownloadFrame();
    return &lastImage;
}

bool SSPRO::StartCapture(int ms)
{
    this->SetupFrame();

    DEBUG("Starting capture...");
    int txCount;
    unsigned char data[6] = { START_BYTE, USB_REQ_CAPTURE, 0x01, 0x04, 0x92, 0x02 }; // 120s Color 1x1 binning
    int result = libusb_bulk_transfer(this->device, USB_CMD_ENDPOINT, data, 6, &txCount, USB_TIMEOUT);
    if (result < 0)
    {
        ERROR("Failed to capture, result = %d", result);
        return false;
    }
    DEBUG("Done\n");

    return this->GetCMDResult(USB_REQ_CAPTURE);
}

void SSPRO::CancelCapture()
{
    DEBUG("Attempting to cancel capture...");
    int txCount;
    unsigned char data[6] = { START_BYTE, USB_REQ_ABORT, 0x00, 0x00, 0x00, 0x00 };
    int result = libusb_bulk_transfer(this->device, USB_CMD_ENDPOINT, data, 6, &txCount, USB_TIMEOUT);
    if (result < 0)
        ERROR("Failed to cancel capture, result = %d", result);
    else
        DEBUG("Done\n");

    this->GetCMDResult(USB_REQ_ABORT);
}

void SSPRO::DownloadFrame()
{ // TODO: Check camera status before trying to download
    DEBUG("Requesting frame download...");
    int txCount;
    unsigned char data[6] = { START_BYTE, USB_REQ_DOWNLOAD, 0x00, 0x00, 0x00, 0x00 };
    int result = libusb_bulk_transfer(this->device, USB_CMD_ENDPOINT, data, 6, &txCount, USB_TIMEOUT);
    if (result < 0)
    {
        ERROR("Failed to start download, result = %d", result);
        return;
    }
    DEBUG("Done\n");

    this->GetCMDResult(USB_REQ_DOWNLOAD);

    DEBUG("Waiting for data from camera...");
    int rxCount;
    unsigned char* rxData = (unsigned char*)malloc(BUFFER_SIZE);
    unsigned char* newImage = (unsigned char*)malloc(IMAGE_WIDTH * IMAGE_HEIGHT);
    unsigned char* pointer = newImage;
    // Loop through until we get a partial buffer of data
    do
    {
        result = libusb_bulk_transfer(this->device, USB_RX_ENDPOINT, rxData, BUFFER_SIZE, &rxCount, USB_TIMEOUT);
        if (result < 0)
        {
            ERROR("Failed to download image, result = %d", result);
            free(rxData);
            return;
        }

        memcpy(pointer, rxData, rxCount);
        pointer += BUFFER_SIZE;
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
}

void SSPRO::SetDIO()
{
    DEBUG("Setting DIO...");
    int txCount;
    unsigned char dio =0x00;
    if (fanHigh)
        dio |= 0x02;
    if (coolerOn)
        dio |= 0x01;
    unsigned char data[6] = { START_BYTE, USB_REQ_SET_DIO, dio, 0x00, 0x00, 0x00 };
    int result = libusb_bulk_transfer(this->device, USB_CMD_ENDPOINT, data, 6, &txCount, USB_TIMEOUT);
    if (result < 0)
        ERROR("Failed to set DIO, result = %d", result);
    else
        DEBUG("Done\n");

    this->GetCMDResult(USB_REQ_SET_DIO);
}

void SSPRO::Init()
{
    DEBUG("Initializing camera...");
    fanHigh = false;
    coolerOn = true;
    readoutSpeed = READOUT_FASTEST;
    DEBUG("Done\n");

    this->SetDIO();
}
