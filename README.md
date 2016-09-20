# opensspro
Linux Driver for the Orion Starshoot Pro V2.0 Deep Space Color Imager

# Folder Structure
  * src      - C++ Source code
  * usb-logs - Wireshark and USBPcap capture files
  * examples - Simple demos to test the camera connection

# Dependencies
  * libusb-1.0

# Indilib
[indilib](http://www.indilib.org/) support is being worked on and current progress can be found in the [sspro branch](https://github.com/compeoree/indi/tree/sspro) of my indilib fork

# Technical Information
Basic information about the sensor and associated hardware can be found on the product page on [Orion Telescopes & Binoculars Website](http://www.telescope.com/Orion-StarShoot-Pro-V20-Deep-Space-Color-CCD-Imaging-Camera/p/52085.uts).

Information below was gathered by capturing data on the USB port.

**USB Capture Configuration**

  * Wireshark and its bundled version of USBPcap
  * Windows 10 (or similar)
  * Bundled version of MaximDL

**Endpoints**

All Commands from the PC being sent to camera use the following USB endpoint and parameters:
`URB Function 0x0009 (URB_FUNCTION_BULK_OR_INTERRUPT_TRANSFER)
Endpoint     0x08 
Type         0x03 (URB_BULK)
Data length  6
`

All Responses from the camera use the following USB endpoint and parameters:
`URB Function 0x0009 (URB_FUNCTION_BULK_OR_INTERRUPT_TRANSFER)
Endpoint     0x82 
Type         0x03 (URB_BULK)
Data length  8 (data length may be up to 1024 for image data packets)
`

**Packet Structure**

All packets originating from the PC use the following format:
` ID | Command | Data
0xA5 | 1 Byte  | 4 Bytes
`

Standard command result packets from the camera use the following format:
` ID  | Data 0 | Command ACK | Bulk Data
0xA5 | 1 Byte |   1 Byte    | 5 Bytes
`

Image transfer packets use the following format (no ID or ACK):
`  Bulk Data
<=1024 Bytes
`

# Command Reference

**Get Camera Status**
Return capturing flag and other data
`Command    0x02
Data       0x00 0x00 0x00 0x00
Example 1  0xA5 0x00 0x02 0x00 0x00 0x00 0xBE 0x00 (Camera Idle)
Example 2  0xA5 0x00 0x02 0x00 0x01 0x00 0xBE 0x00 (Camera Exposing)
Example 3  0xA5 0x00 0x02 0x00 0x02 0x00 0xBE 0x00 (Frame Ready)
`

**Set Capture Mode and Frame Size**
Define the subframe (may also set a binning flag, not sure yet)
`Command    0x0B
Data       0xXX 0xXX 0xXX 0xXX
TX Data Bytes are not fully decoded yet
Example 1 (Light Frame Raw/Color 1x1 Binning)
 TX: 0xA5 0x0B 0x00 0x00 0x03 0xF9
 RX: 0xA5 0xF9 0x0B 0xF9 0x01 0xF9 0x00 0xF9
Example 2 (Light Frame Mono 2x2 Binning)
 TX: 0xA5 0x0B 0x00 0x00 0x03 0xFA
 RX: 0xA5 0xFA 0x0B 0xFA 0x01 0xFA 0x00 0xFA
Example 3 (Light Frame Mono 2x2 Binning, subframe 95,338 to 706,444)
 TX: 0xA5 0x0B 0x01 0x52 0x00 0x70
 RX: 0xA5 0x70 0x0B 0x70 0x01 0x70 0x00 0x70
RX Data Bytes are not fully decoded yet
`

**Start Capture**
Set Readout speed, Shutter time, and other data
`Command    0x03
Data       0x0X 0xXX 0xXX 0x0X
Bits|                 Byte 1              | Byte 2 & Byte 3  |       Byte 4        |
------------------------------------------------------------------------------------
Bit |    7-4   |       3-1       |   0    | 0x0001 to 0xFFFF | 0x00=Time <= 8.000s |
    | Always 0 |  Readout Speed  | Units  |    Time value    | 0x02=Time >= 8.001s |
    |          | 0,2,4,6,8,A,C,E | 1=0.1s |                  | 0x03=2x2 Binning    |
    |          | Fast ..... Slow | 0=ms   |                  |   More??            |

Units are milliseconds when time is less than 10s.
Time value seems offset by some random value, MaximDL setpoint is 6696.9s and packet value is 0xfffe.
Example 1 (120s Light Frame Raw/Color 1x1 Binning)
 TX: 0xA5 0x03 0x01 0x04 0x92 0x02
 RX: 0xA5 0x02 0x03 0x02 0x01 0x02 0x00 0x02
RX Data Bytes are not fully decoded yet
`
