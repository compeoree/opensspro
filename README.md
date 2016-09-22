Linux Driver for the Orion Starshoot Pro V2.0 Deep Space Color Imager.

## Background
I wanted a way to capture images, at a remote observatory, without lugging my laptop (running Windows) out into the field. And being remote, it doesn't have line power, so I am stuck running everything off of a finite supply. 

It is anticipated that this driver will allow embedded systems the ability to capture images using this very capable camera.

&nbsp;
## Building
### Folder Structure
  * src      - C++ Source code
  * usb-logs - Wireshark and USBPcap capture files
  * examples - Simple demos to test the camera connection

&nbsp;
### Dependencies
  * libusb-1.0

&nbsp;
### Indilib Support
[indilib](http://www.indilib.org/) support is being worked on. Current progress can be found in the [sspro branch](https://github.com/compeoree/indi/tree/sspro) of my indilib fork.

&nbsp;
## Technical Information
Basic information about the sensor and associated hardware can be found on the product page on [Orion Telescopes & Binoculars Website](http://www.telescope.com/Orion-StarShoot-Pro-V20-Deep-Space-Color-CCD-Imaging-Camera/p/52085.uts).

Information below was gathered by capturing data on the USB port.

&nbsp;

### USB Capture Software Configuration
  * Wireshark and its bundled version of USBPcap
  * Windows 10 (or similar)
  * Bundled version of MaximDL

&nbsp;

### USB Endpoints

All Commands from the PC being sent to camera use the following USB endpoint and parameters:

```
URB Function 0x0009 (URB_FUNCTION_BULK_OR_INTERRUPT_TRANSFER)
Endpoint     0x08 
Type         0x03 (URB_BULK)
Data length  6
```

&nbsp;

All Responses from the camera use the following USB endpoint and parameters:

```
URB Function 0x0009 (URB_FUNCTION_BULK_OR_INTERRUPT_TRANSFER)
Endpoint     0x82 
Type         0x03 (URB_BULK)
Data length  8 (data length may be up to 1024 for image data packets)
```

&nbsp;

### Packet Structure

All packets originating from the PC use the following format:

| ID  | Command | Data |
|:---:|:-------:|:----:|
| 0xA5 | 1 Byte | 4 Bytes |

&nbsp;

Standard result packets from the camera use the following format:

| ID  | Data 0 | Command ACK | Bulk Data |
|:---:|:------:|:-----------:|:---------:|
| 0xA5 | 1 Byte |   1 Byte    | 5 Bytes |

&nbsp;

Image transfer packets use the following format (no ID or ACK):

| ID  | Data 0 | Command ACK | Bulk Data |
|:---:|:------:|:-----------:|:---------:|
| None | None | None    | <= 1024 Bytes |

&nbsp;

## Command Reference

#### Get Camera Status
_Return status byte and other data_

**Command:** ```0x02```
<br>
**TX Data:** ```0x00 0x00 0x00 0x00```
<br>
**RX Data:** ```0xA5 Data0 0x02 Data1 Data2 Data3 Data4 Data5```

_Data0, Data1, Data3, and Data5 = Usually 0x00._

_Data2 = Status Byte_

_Data4 = Usually 0xBE_

**Examples:**

| TX | RX | Result |
| --- | --- | --- |
| 0xA5 0x02 0x00 0x00 0x00 0x00 | 0xA5 0x00 0x02 0x00 **0x00** 0x00 0xBE 0x00 | Camera Idle |
| 0xA5 0x02 0x00 0x00 0x00 0x00 | 0xA5 0x00 0x02 0x00 **0x01** 0x00 0xBE 0x00 | Camera Exposing |
| 0xA5 0x02 0x00 0x00 0x00 0x00 | 0xA5 0x00 0x02 0x00 **0x02** 0x00 0xBE 0x00 | Frame Ready |

&nbsp;

#### Set Capture Mode and Frame Size
_Define the subframe (may also set a binning flag, not sure yet)_

**Command:** ```0x0B```
<br>
**TX Data:** ```Cmd0 Cmd1 Cmd2 Cmd3```
<br>
**RX Data:** ```0xA5 Data0 0x0B Data1 Data2 Data3 Data4 Data5```

Cmd0-3 = Unknown definition of capture frame and possibly binning mode

_Data0, Data1, Data3, and Data5 = Usually same as Cmd3 byte._

_Data2 = Usually 0x01_

_Data4 = Usually 0x00_

**Examples:**

| TX | RX | Description |
| --- | --- | --- |
| 0xA5 0x0B 0x00 0x00 0x03 0xF9 | 0xA5 0xF9 0x0B 0xF9 0x01 0xF9 0x00 0xF9 | Light Frame Raw/Color 1x1 Binning |
| 0xA5 0x0B 0x00 0x00 0x03 0xFA | 0xA5 0xFA 0x0B 0xFA 0x01 0xFA 0x00 0xFA | Light Frame Mono 2x2 Binning |
| 0xA5 0x0B 0x01 0x52 0x00 0x70 | 0xA5 0x70 0x0B 0x70 0x01 0x70 0x00 0x70 | Light Frame Mono 2x2 Binning, subframe 95,338 to 706,444 |

&nbsp;

#### Start Capture
_Set Readout speed, Shutter time, and other data_

**Command:** ```0x03```
<br>
**TX Data:** ```Cmd0 Cmd1 Cmd2 Cmd3```

| Cmd0 | Cmd1 & Cmd2 | Cmd3 |
| ------------------- |:----------------:|:-------------------:|
| Bits 7-4 = Always 0 | 0x0001 to 0xFFFF | 0x00=Time <= 8.000s |
| Bits 3-1 = Readout Speed (Fast=0...Slow=7) | Time value | 0x02=Time >= 8.001s |
| Bit 0 = Time Units (0=ms, 1=0.1s) |                  | 0x03=2x2 Binning?   |
<br>
**RX Data:** ```0xA5 Data0 0x0B Data1 Data2 Data3 Data4 Data5```

_Data0, Data1, Data3, and Data5 = Usually same as Cmd3 byte._

_Data2 = Usually 0x01_

_Data4 = Usually 0x00_

**Description:**

Units are milliseconds when time is less than 10s.<br>
Time value seems offset by some random value, which seems to change after a restart of MaximDL. One example uses a setpoint of 6696.9s in MaximDL which produces a packet value of 0xfffe.<br>

**Example:**

| TX | RX | Description |
| --- | --- | --- |
| 0xA5 0x03 0x01 0x04 0x92 0x02 | 0xA5 0x02 0x03 0x02 0x01 0x02 0x00 0x02 | 120s Light Frame Raw/Color 1x1 Binning |
