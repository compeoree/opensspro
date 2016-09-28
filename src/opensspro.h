/*
  Copyright (c) 2016 Louis McCarthy
  All rights reserved.

  Licensed under MIT License, see LICENSE for full License text
*/

#ifndef __OPEN_SSPRO_H__ 
#define __OPEN_SSPRO_H__ 

#ifdef VERBOSE
    #define DEBUG(...) printf(__VA_ARGS__)
#else
    #define DEBUG(...)
#endif

#define ERROR(...) printf(__VA_ARGS__)

#define SSPRO_VENDOR_ID 0x1856  // Imaginova
#define SSPRO_PRODUCT_ID 0x001E // Starshoot Pro V2.0

typedef struct libusb_device_handle libusb_device_handle;

namespace OpenSSPRO
{
    enum ReadOutSpeed
    {
        READOUT_FASTEST = 0,
        READOUT_FASTER = 1,
        READOUT_FAST = 2,
        READOUT_MED_FAST = 3,
        READOUT_MED_SLOW = 4,
        READOUT_SLOW = 5,
        READOUT_SLOWER = 6,
        READOUT_SLOWEST = 7
    };

    struct rawImage {
        unsigned int width;
        unsigned int height;
        unsigned char* data;
    };

    struct deviceInfo {
        char serialNum[256];
        struct deviceInfo* next;
    };

    class SSPRO
    {
    private:
        libusb_device_handle* device;
        rawImage lastImage;
        ReadOutSpeed readoutSpeed;
        bool fanHigh;
        bool coolerOn;
        bool capturing;
        bool frameReady;

        void Init();
        void SetupFrame();
        void DownloadFrame();
        void SetDIO();
        bool GetCMDResult(unsigned char cmd);

    public:
        SSPRO();

        bool Connect();
        void Disconnect();
        bool IsConnected();

        void GetStatus();
        struct rawImage* Capture(int ms); // Blocking call

        bool StartCapture(int ms); // Asynchronous call
        void CancelCapture();
        unsigned char* GetLastImage();
    };
}

#endif /* __OPEN_SSPRO_H__ */
