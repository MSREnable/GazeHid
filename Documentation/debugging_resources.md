# Debugging Resources

Driver development can be...tricky. Along the way we picked up a few tricks
that may help other developers.

## Kernel Debugging

Kernel debugging is very powerful, but there are a few caveats we ran into. Please see 
official documentation on [setting up Kernel-mode debugging](https://docs.microsoft.com/en-us/windows-hardware/drivers/debugger/setting-up-kernel-mode-debugging-in-windbg--cdb--or-ntsd). 
Our experience showed that having a second physical machine with a built-in hardwired ethernet
connection was best.

### Remote Kernel Debugging

While many devices should just work, be aware that not all network adapters are supported
by kernel debugging. This is specifically true for our experience with wifi and USB ethernet
adapters. It is best to debug on a machine with a build-in ethernet port such as a laptop
or desktop device. Tablets or ultra portable devices may be tricky due to their lack of built-in
hardwired ethernet.

### Local Kernel Debugging

Local debugging was not sufficient to allow `devcon.exe` to succeed. It was sufficient
for allowing the use of the [HID debugger extensions](https://docs.microsoft.com/en-us/windows-hardware/drivers/debugger/hid-extensions). This modality also
makes driver development more difficult since stepping through code and many other functions
are not available.

### Virtual Machine Debugging and Remote Desktop

This modality is very attractive for driver development due to the opportunity to use snapshots,
have multiple configurations, etc. However, "enhanced sessions" and similar can make driver 
interactions work differently than you may expect. Further, we found that certain events in Windows
weren't called using Virtual Machine or Remote Desktop, which made validation tricky.

## Debugger Tips
### Debugger Extensions

The WinDbg Debugger supports extensions which can add useful functionality. One extension we 
discovered was the [HID Extensions](https://docs.microsoft.com/en-us/windows-hardware/drivers/debugger/hid-extensions). 
Specifically, the `!hidtree`, `!hidrd`, and `!hidppd` commands. Below is a sample capture from 
[Windbg Preview](https://www.microsoft.com/en-us/p/windbg-preview/9pgjgd53tn86).

```
1: kd> !hidkd.load
1: kd> .reload
Connected to Windows 10 19041 x64 target at (Tue Sep 15 08:31:07.109 2020 (UTC - 7:00)), ptr64 TRUE
Loading Kernel Symbols
...............................................................
................................................................
................................................................
...............................................................
Loading User Symbols

Loading unloaded module list
....................

1: kd> !hidtree
HID Device Tree
================================================================================
FDO  VendorID:0xDEED ProductID:0xFEED Version:0x0001
!hidfdo 0xffff8c0ab2a1d060
PowerStates: S0/D0   |  D0
dt FDO_EXTENSION 0xffff8c0ab2a1d1d0
!devnode 0xffff8c0ab26d4cd0  |  DeviceNodeStarted (0n776)
InstancePath: VHF\HID_DEVICE_SYSTEM_VHF\1&14adfd20&3&{ede8b365-11ba-5682-a346-5fce54b083a1}
IFR Log: !rcdrlogdump HIDCLASS -a 0xFFFF8C0A95871000

    PDO  Reserved (0x12) | 0x01
    !hidpdo 0xffff8c0ab2a8b060
    Power States: S0/D0  |  COLLECTION_STATE_RUNNING (0n3)
    dt PDO_EXTENSION 0xffff8c0ab2a8b1d0
    !devnode 0xffff8c0a9705dca0  |  DeviceNodeStarted (0n776)
    Instance Path:HID\HID_DEVICE_SYSTEM_VHF\2&192ee871&0&0000
================================================================================
1: kd> !hidfdo 0xffff8c0aa6be9060
FDO 0xffff8c0aa6be9060  (!devobj/!devstack)
==============================================
  Name              : \Device\_HID00000006
  Vendor ID         : 0xDEED
  Product ID        : 0xFEED
  Version Number    : 0x0001
  Is Present?       : Y
  Report Descriptor : !hidrd 0xffff8c0ab1b93a30 0x154
  Per-FDO IFR Log(s): !rcdrlogdump HIDCLASS -a 0xFFFF8C0AA72CF000
                      !rcdrlogdump HIDCLASS -a 0xFFFF8C0AB286E000 (Device Info)
                      !rcdrlogdump HIDCLASS -a 0xFFFF8C0AB38D5000 (INT Reports)

  Position in HID tree

  dt FDO_EXTENSION 0xffff8c0aa6be91d0

Device States
--------------------------
  Power States..........: S0/D0
  State Machine State...: D0
  Idle IRP..............: !irp 0xffff8c0a9812cdc0 (completed with status code 0x0)
  Idle PDOs.............: 0
  WaitWake IRP..........: none
  Power-delayed IRPs....: 0 
  PDO WaitWake IRPs.....: 0 
  Open Count............: 2
  Last INT Report Status: 0x0
  Last INT Report Time..: 09/15/2020-08:30:36.364 (Pacific Daylight Time)

Device Capabilities
--------------------------
  Support D1        : N
  Support D2        : N
  Removable         : N
  SurpriseRemovalOK : Y
  Wake from D0      : N
  Wake from D1      : N
  Wake from D2      : N
  Wake from D3      : N
  Device states     : S0=>D0, S1=>D3, S2=>D3, S3=>D3, S4=>D3 S5=>D3
  SystemWake        : S-1
  DeviceWake        : D-1

PingPong IRPs (2 Total)
--------------------------
  #0: dt HIDCLASS_PINGPONG 0xffff8c0a973b8430
      !irp 0xffff8c0ab2c5c500 (pending on \Driver\vhf)

  #1: dt HIDCLASS_PINGPONG 0xffff8c0a973b8510
      !irp 0xffff8c0a92c0c3b0 (pending on \Driver\vhf)

Collections (1 Total)
--------------------------
  Collection Num..............: 1
  Collection..................: dt HIDCLASS_COLLECTION 0xffff8c0a7eaa7d50
  Collection PDO..............: !hidpdo 0xffff8c0ab38d4060
  UsagePage...................: Reserved (0x12)
  Usage.......................: 0x01
  Report Lengths..............: 0x41(Input) 0x0(Output) 0x16(Feature)
  Preparsed Data..............: !hidppd 0xffff8c0aa72ea010
  Open Count..................: 1 (Read:1|Write:1 Restriction:[])
  Pending Reads...............: 1
  Cumulative # of INT Reports.: 0
  Last INT Report Time........: 01/01/1601-16:00:00.000 (Pacific Daylight Time)

!devnode 0xffff8c0ab2ee5cd0
---------------------------
  State         : DeviceNodeStarted (0n776)
  Instance Path : VHF\HID_DEVICE_SYSTEM_VHF\1&1f44c7b&c&{ede8b365-11ba-5682-a346-5fce54b083a1}

State Machine Information
---------------------------
Current Device State: D0

Device State History: <Event>      New-State

  [ 2]: <OperationSuccess>.........D0
  [ 1]: <OperationSuccess>.........SettingIoStateToPassOnStart
  [ 0]: <PnPStart>.................InitializingDevice

Device Event History:

  [ 0]: PnPStart

1: kd> !hidrd 0xffff8c0ab1b93a30 0x154
Report Descriptor at 0xffff8c0ab1b93a30

Raw Data
-------------------------------------------------------
0x0000: 05 12 09 01 A1 01 09 10-A1 02 85 10 15 00 26 FF
0x0010: 00 75 08 09 20 66 01 10-55 0A 95 08 81 02 95 01
0x0020: 75 20 65 11 55 0C 17 80-7B E1 FF 27 80 84 1E 00
0x0030: 09 24 A1 00 95 02 09 21-09 22 81 03 C0 09 25 A1
0x0040: 00 95 03 09 21 09 22 09-23 81 03 C0 09 26 A1 00
0x0050: 95 03 09 21 09 22 09 23-81 03 C0 95 06 81 03 C0
0x0060: 09 11 A1 02 85 11 75 08-95 01 0A 00 01 15 00 25
0x0070: 01 65 00 55 00 B1 03 A1-00 95 05 75 20 17 80 7B
0x0080: E1 FF 27 80 84 1E 00 65-11 55 0C 0A 01 01 0A 02
0x0090: 01 0A 03 01 0A 04 01 0A-05 01 B1 03 C0 C0 09 12
0x00A0: A1 02 85 12 75 08 15 00-26 FF 00 95 01 75 10 27
0x00B0: FF FF 00 00 65 00 55 00-0A 00 02 B1 03 0A 01 02
0x00C0: B1 03 75 20 27 FF FF FF-7F 0A 02 02 B1 03 75 10
0x00D0: 15 00 27 FF FF 00 00 0A-03 02 B1 03 A1 00 65 11
0x00E0: 55 0C 27 FF FF FF 7F 75-20 0A 04 02 B1 03 0A 05
0x00F0: 02 B1 03 C0 C0 09 13 A1-02 85 13 75 08 65 00 55
0x0100: 00 25 04 0A 01 03 B1 02-75 10 27 FF FF 00 00 66
0x0110: 01 F0 55 00 0A 00 03 B1-02 C0 09 13 A1 02 85 13
0x0120: 75 08 65 00 55 00 25 04-0A 01 03 81 02 75 10 27
0x0130: FF FF 00 00 66 01 F0 55-00 0A 00 03 81 02 C0 09
0x0140: 14 A1 02 85 14 75 08 25-07 65 00 55 00 0A 00 04
0x0150: B1 02 C0 C0

Parsed
----------------------------------------------------
Usage Page (Reserved)...............0x0000: 05 12
Usage (0x01)........................0x0002: 09 01
Collection (Application)............0x0004: A1 01
..Usage (0x10)......................0x0006: 09 10
..Collection (Logical)..............0x0008: A1 02
....Report ID (16)..................0x000A: 85 10
....Logical Minimum (0).............0x000C: 15 00
....Logical Maximum (255)...........0x000E: 26 FF 00
....Report Size (8).................0x0011: 75 08
....Usage (0x20)....................0x0013: 09 20
....Unit (SI Linear)................0x0015: 66 01 10
....Unit Exponent (-6)..............0x0018: 55 0A
....Report Count (8)................0x001A: 95 08
....Input (Data,Var,Abs)............0x001C: 81 02
....Report Count (1)................0x001E: 95 01
....Report Size (32)................0x0020: 75 20
....Unit (Centimeter)...............0x0022: 65 11
....Unit Exponent (-4)..............0x0024: 55 0C
....Logical Minimum (-2000000)......0x0026: 17 80 7B E1 FF
....Logical Maximum (2000000).......0x002B: 27 80 84 1E 00
....Usage (0x24)....................0x0030: 09 24
....Collection (Physical)...........0x0032: A1 00
......Report Count (2)..............0x0034: 95 02
......Usage (0x21)..................0x0036: 09 21
......Usage (0x22)..................0x0038: 09 22
......Input (Cnst,Var,Abs)..........0x003A: 81 03
....End Collection ()...............0x003C: C0
....Usage (0x25)....................0x003D: 09 25
....Collection (Physical)...........0x003F: A1 00
......Report Count (3)..............0x0041: 95 03
......Usage (0x21)..................0x0043: 09 21
......Usage (0x22)..................0x0045: 09 22
......Usage (0x23)..................0x0047: 09 23
......Input (Cnst,Var,Abs)..........0x0049: 81 03
....End Collection ()...............0x004B: C0
....Usage (0x26)....................0x004C: 09 26
....Collection (Physical)...........0x004E: A1 00
......Report Count (3)..............0x0050: 95 03
......Usage (0x21)..................0x0052: 09 21
......Usage (0x22)..................0x0054: 09 22
......Usage (0x23)..................0x0056: 09 23
......Input (Cnst,Var,Abs)..........0x0058: 81 03
....End Collection ()...............0x005A: C0
....Report Count (6)................0x005B: 95 06
....Input (Cnst,Var,Abs)............0x005D: 81 03
..End Collection ().................0x005F: C0
..Usage (0x11)......................0x0060: 09 11
..Collection (Logical)..............0x0062: A1 02
....Report ID (17)..................0x0064: 85 11
....Report Size (8).................0x0066: 75 08
....Report Count (1)................0x0068: 95 01
....Usage (0x100)...................0x006A: 0A 00 01
....Logical Minimum (0).............0x006D: 15 00
....Logical Maximum (1).............0x006F: 25 01
....Unit (None).....................0x0071: 65 00
....Unit Exponent (0)...............0x0073: 55 00
....Feature (Cnst,Var,Abs)..........0x0075: B1 03
....Collection (Physical)...........0x0077: A1 00
......Report Count (5)..............0x0079: 95 05
......Report Size (32)..............0x007B: 75 20
......Logical Minimum (-2000000)....0x007D: 17 80 7B E1 FF
......Logical Maximum (2000000).....0x0082: 27 80 84 1E 00
......Unit (Centimeter).............0x0087: 65 11
......Unit Exponent (-4)............0x0089: 55 0C
......Usage (0x101).................0x008B: 0A 01 01
......Usage (0x102).................0x008E: 0A 02 01
......Usage (0x103).................0x0091: 0A 03 01
......Usage (0x104).................0x0094: 0A 04 01
......Usage (0x105).................0x0097: 0A 05 01
......Feature (Cnst,Var,Abs)........0x009A: B1 03
....End Collection ()...............0x009C: C0
..End Collection ().................0x009D: C0
..Usage (0x12)......................0x009E: 09 12
..Collection (Logical)..............0x00A0: A1 02
....Report ID (18)..................0x00A2: 85 12
....Report Size (8).................0x00A4: 75 08
....Logical Minimum (0).............0x00A6: 15 00
....Logical Maximum (255)...........0x00A8: 26 FF 00
....Report Count (1)................0x00AB: 95 01
....Report Size (16)................0x00AD: 75 10
....Logical Maximum (65535).........0x00AF: 27 FF FF 00 00
....Unit (None).....................0x00B4: 65 00
....Unit Exponent (0)...............0x00B6: 55 00
....Usage (0x200)...................0x00B8: 0A 00 02
....Feature (Cnst,Var,Abs)..........0x00BB: B1 03
....Usage (0x201)...................0x00BD: 0A 01 02
....Feature (Cnst,Var,Abs)..........0x00C0: B1 03
....Report Size (32)................0x00C2: 75 20
....Logical Maximum (2147483647)....0x00C4: 27 FF FF FF 7F
....Usage (0x202)...................0x00C9: 0A 02 02
....Feature (Cnst,Var,Abs)..........0x00CC: B1 03
....Report Size (16)................0x00CE: 75 10
....Logical Minimum (0).............0x00D0: 15 00
....Logical Maximum (65535).........0x00D2: 27 FF FF 00 00
....Usage (0x203)...................0x00D7: 0A 03 02
....Feature (Cnst,Var,Abs)..........0x00DA: B1 03
....Collection (Physical)...........0x00DC: A1 00
......Unit (Centimeter).............0x00DE: 65 11
......Unit Exponent (-4)............0x00E0: 55 0C
......Logical Maximum (2147483647)..0x00E2: 27 FF FF FF 7F
......Report Size (32)..............0x00E7: 75 20
......Usage (0x204).................0x00E9: 0A 04 02
......Feature (Cnst,Var,Abs)........0x00EC: B1 03
......Usage (0x205).................0x00EE: 0A 05 02
......Feature (Cnst,Var,Abs)........0x00F1: B1 03
....End Collection ()...............0x00F3: C0
..End Collection ().................0x00F4: C0
..Usage (0x13)......................0x00F5: 09 13
..Collection (Logical)..............0x00F7: A1 02
....Report ID (19)..................0x00F9: 85 13
....Report Size (8).................0x00FB: 75 08
....Unit (None).....................0x00FD: 65 00
....Unit Exponent (0)...............0x00FF: 55 00
....Logical Maximum (4).............0x0101: 25 04
....Usage (0x301)...................0x0103: 0A 01 03
....Feature (Data,Var,Abs)..........0x0106: B1 02
....Report Size (16)................0x0108: 75 10
....Logical Maximum (65535).........0x010A: 27 FF FF 00 00
....Unit (SI Linear)................0x010F: 66 01 F0
....Unit Exponent (0)...............0x0112: 55 00
....Usage (0x300)...................0x0114: 0A 00 03
....Feature (Data,Var,Abs)..........0x0117: B1 02
..End Collection ().................0x0119: C0
..Usage (0x13)......................0x011A: 09 13
..Collection (Logical)..............0x011C: A1 02
....Report ID (19)..................0x011E: 85 13
....Report Size (8).................0x0120: 75 08
....Unit (None).....................0x0122: 65 00
....Unit Exponent (0)...............0x0124: 55 00
....Logical Maximum (4).............0x0126: 25 04
....Usage (0x301)...................0x0128: 0A 01 03
....Input (Data,Var,Abs)............0x012B: 81 02
....Report Size (16)................0x012D: 75 10
....Logical Maximum (65535).........0x012F: 27 FF FF 00 00
....Unit (SI Linear)................0x0134: 66 01 F0
....Unit Exponent (0)...............0x0137: 55 00
....Usage (0x300)...................0x0139: 0A 00 03
....Input (Data,Var,Abs)............0x013C: 81 02
..End Collection ().................0x013E: C0
..Usage (0x14)......................0x013F: 09 14
..Collection (Logical)..............0x0141: A1 02
....Report ID (20)..................0x0143: 85 14
....Report Size (8).................0x0145: 75 08
....Logical Maximum (7).............0x0147: 25 07
....Unit (None).....................0x0149: 65 00
....Unit Exponent (0)...............0x014B: 55 00
....Usage (0x400)...................0x014D: 0A 00 04
....Feature (Data,Var,Abs)..........0x0150: B1 02
..End Collection ().................0x0152: C0
End Collection ()...................0x0153: C0

1: kd> !hidppd 0xffff8c0aa72ea010
SYMSRV:  BYINDEX: 0x5
         http://symweb
         hidparse.pdb
         67CEF75772ECE8DE23F86229A62D51951
SYMSRV:  PATH: C:\ProgramData\Dbg\sym\hidparse.pdb\67CEF75772ECE8DE23F86229A62D51951\hidparse.pdb
SYMSRV:  RESULT: 0x00000000
DBGHELP: HIDPARSE - private symbols & lines 
        C:\ProgramData\Dbg\sym\hidparse.pdb\67CEF75772ECE8DE23F86229A62D51951\hidparse.pdb
Reading preparsed data...
Preparsed Data at 0xffff8c0aa72ea010  

Summary
-----------------------------------------------------
  UsagePage             : Reserved (0x12)
  Usage                 : 0x01
  Report Lengths        : 0x41(Input) 0x0(Output) 0x16(Feature)
  Link Collection Nodes : 12
  Button Caps           : 0(Input) 0(Output) 0(Feature)
  Value Caps            : 11(Input) 0(Output) 15(Feature)
  Data Indices          : 11(Input) 0(Output) 15(Feature)

Input Value Capability #0
-----------------------------------------------------
  Report ID         : 0x10
  Usage Page        : Reserved (0x12)
  Bit Size          : 0x8
  Report Count      : 0x8
  Units Exponent    : 0xA
  Has Null          : No
  Alias             : No
  Usage Range       : 0x20 (min)  0x20 (max)
  Data Index Range  : 0x0 (min) 0x0 (max)
  Physical Range    : 0x0 (min) 0x0 (max)
  Logical Range     : 0x0 (min) 0xFF (max)
  String Index Range: 0x0 (min) 0x0 (max)
  Designator Range  : 0x0 (min) 0x0 (max)
  Is Absolute       : Yes


Input Value Capability #1
-----------------------------------------------------
  Report ID         : 0x10
  Usage Page        : Reserved (0x12)
  Bit Size          : 0x20
  Report Count      : 0x1
  Units Exponent    : 0xC
  Has Null          : No
  Alias             : No
  Usage Range       : 0x22 (min)  0x22 (max)
  Data Index Range  : 0x1 (min) 0x1 (max)
  Physical Range    : 0x0 (min) 0x0 (max)
  Logical Range     : 0xFFE17B80 (min) 0x1E8480 (max)
  String Index Range: 0x0 (min) 0x0 (max)
  Designator Range  : 0x0 (min) 0x0 (max)
  Is Absolute       : Yes


Input Value Capability #2
-----------------------------------------------------
  Report ID         : 0x10
  Usage Page        : Reserved (0x12)
  Bit Size          : 0x20
  Report Count      : 0x1
  Units Exponent    : 0xC
  Has Null          : No
  Alias             : No
  Usage Range       : 0x21 (min)  0x21 (max)
  Data Index Range  : 0x2 (min) 0x2 (max)
  Physical Range    : 0x0 (min) 0x0 (max)
  Logical Range     : 0xFFE17B80 (min) 0x1E8480 (max)
  String Index Range: 0x0 (min) 0x0 (max)
  Designator Range  : 0x0 (min) 0x0 (max)
  Is Absolute       : Yes


Input Value Capability #3
-----------------------------------------------------
  Report ID         : 0x10
  Usage Page        : Reserved (0x12)
  Bit Size          : 0x20
  Report Count      : 0x1
  Units Exponent    : 0xC
  Has Null          : No
  Alias             : No
  Usage Range       : 0x23 (min)  0x23 (max)
  Data Index Range  : 0x3 (min) 0x3 (max)
  Physical Range    : 0x0 (min) 0x0 (max)
  Logical Range     : 0xFFE17B80 (min) 0x1E8480 (max)
  String Index Range: 0x0 (min) 0x0 (max)
  Designator Range  : 0x0 (min) 0x0 (max)
  Is Absolute       : Yes


Input Value Capability #4
-----------------------------------------------------
  Report ID         : 0x10
  Usage Page        : Reserved (0x12)
  Bit Size          : 0x20
  Report Count      : 0x1
  Units Exponent    : 0xC
  Has Null          : No
  Alias             : No
  Usage Range       : 0x22 (min)  0x22 (max)
  Data Index Range  : 0x4 (min) 0x4 (max)
  Physical Range    : 0x0 (min) 0x0 (max)
  Logical Range     : 0xFFE17B80 (min) 0x1E8480 (max)
  String Index Range: 0x0 (min) 0x0 (max)
  Designator Range  : 0x0 (min) 0x0 (max)
  Is Absolute       : Yes


Input Value Capability #5
-----------------------------------------------------
  Report ID         : 0x10
  Usage Page        : Reserved (0x12)
  Bit Size          : 0x20
  Report Count      : 0x1
  Units Exponent    : 0xC
  Has Null          : No
  Alias             : No
  Usage Range       : 0x21 (min)  0x21 (max)
  Data Index Range  : 0x5 (min) 0x5 (max)
  Physical Range    : 0x0 (min) 0x0 (max)
  Logical Range     : 0xFFE17B80 (min) 0x1E8480 (max)
  String Index Range: 0x0 (min) 0x0 (max)
  Designator Range  : 0x0 (min) 0x0 (max)
  Is Absolute       : Yes


Input Value Capability #6
-----------------------------------------------------
  Report ID         : 0x10
  Usage Page        : Reserved (0x12)
  Bit Size          : 0x20
  Report Count      : 0x1
  Units Exponent    : 0xC
  Has Null          : No
  Alias             : No
  Usage Range       : 0x23 (min)  0x23 (max)
  Data Index Range  : 0x6 (min) 0x6 (max)
  Physical Range    : 0x0 (min) 0x0 (max)
  Logical Range     : 0xFFE17B80 (min) 0x1E8480 (max)
  String Index Range: 0x0 (min) 0x0 (max)
  Designator Range  : 0x0 (min) 0x0 (max)
  Is Absolute       : Yes


Input Value Capability #7
-----------------------------------------------------
  Report ID         : 0x10
  Usage Page        : Reserved (0x12)
  Bit Size          : 0x20
  Report Count      : 0x1
  Units Exponent    : 0xC
  Has Null          : No
  Alias             : No
  Usage Range       : 0x22 (min)  0x22 (max)
  Data Index Range  : 0x7 (min) 0x7 (max)
  Physical Range    : 0x0 (min) 0x0 (max)
  Logical Range     : 0xFFE17B80 (min) 0x1E8480 (max)
  String Index Range: 0x0 (min) 0x0 (max)
  Designator Range  : 0x0 (min) 0x0 (max)
  Is Absolute       : Yes


Input Value Capability #8
-----------------------------------------------------
  Report ID         : 0x10
  Usage Page        : Reserved (0x12)
  Bit Size          : 0x20
  Report Count      : 0x1
  Units Exponent    : 0xC
  Has Null          : No
  Alias             : No
  Usage Range       : 0x21 (min)  0x21 (max)
  Data Index Range  : 0x8 (min) 0x8 (max)
  Physical Range    : 0x0 (min) 0x0 (max)
  Logical Range     : 0xFFE17B80 (min) 0x1E8480 (max)
  String Index Range: 0x0 (min) 0x0 (max)
  Designator Range  : 0x0 (min) 0x0 (max)
  Is Absolute       : Yes


Input Value Capability #9
-----------------------------------------------------
  Report ID         : 0x13
  Usage Page        : Reserved (0x12)
  Bit Size          : 0x8
  Report Count      : 0x1
  Units Exponent    : 0x0
  Has Null          : No
  Alias             : No
  Usage Range       : 0x301 (min)  0x301 (max)
  Data Index Range  : 0x9 (min) 0x9 (max)
  Physical Range    : 0x0 (min) 0x0 (max)
  Logical Range     : 0x0 (min) 0x4 (max)
  String Index Range: 0x0 (min) 0x0 (max)
  Designator Range  : 0x0 (min) 0x0 (max)
  Is Absolute       : Yes


Input Value Capability #10
-----------------------------------------------------
  Report ID         : 0x13
  Usage Page        : Reserved (0x12)
  Bit Size          : 0x10
  Report Count      : 0x1
  Units Exponent    : 0x0
  Has Null          : No
  Alias             : No
  Usage Range       : 0x300 (min)  0x300 (max)
  Data Index Range  : 0xA (min) 0xA (max)
  Physical Range    : 0x0 (min) 0x0 (max)
  Logical Range     : 0x0 (min) 0xFFFF (max)
  String Index Range: 0x0 (min) 0x0 (max)
  Designator Range  : 0x0 (min) 0x0 (max)
  Is Absolute       : Yes


Feature Value Capability #0
-----------------------------------------------------
  Report ID         : 0x11
  Usage Page        : Reserved (0x12)
  Bit Size          : 0x8
  Report Count      : 0x1
  Units Exponent    : 0x0
  Has Null          : No
  Alias             : No
  Usage Range       : 0x100 (min)  0x100 (max)
  Data Index Range  : 0x0 (min) 0x0 (max)
  Physical Range    : 0x0 (min) 0x0 (max)
  Logical Range     : 0x0 (min) 0x1 (max)
  String Index Range: 0x0 (min) 0x0 (max)
  Designator Range  : 0x0 (min) 0x0 (max)
  Is Absolute       : Yes


Feature Value Capability #1
-----------------------------------------------------
  Report ID         : 0x11
  Usage Page        : Reserved (0x12)
  Bit Size          : 0x20
  Report Count      : 0x1
  Units Exponent    : 0xC
  Has Null          : No
  Alias             : No
  Usage Range       : 0x105 (min)  0x105 (max)
  Data Index Range  : 0x1 (min) 0x1 (max)
  Physical Range    : 0x0 (min) 0x0 (max)
  Logical Range     : 0xFFE17B80 (min) 0x1E8480 (max)
  String Index Range: 0x0 (min) 0x0 (max)
  Designator Range  : 0x0 (min) 0x0 (max)
  Is Absolute       : Yes


Feature Value Capability #2
-----------------------------------------------------
  Report ID         : 0x11
  Usage Page        : Reserved (0x12)
  Bit Size          : 0x20
  Report Count      : 0x1
  Units Exponent    : 0xC
  Has Null          : No
  Alias             : No
  Usage Range       : 0x104 (min)  0x104 (max)
  Data Index Range  : 0x2 (min) 0x2 (max)
  Physical Range    : 0x0 (min) 0x0 (max)
  Logical Range     : 0xFFE17B80 (min) 0x1E8480 (max)
  String Index Range: 0x0 (min) 0x0 (max)
  Designator Range  : 0x0 (min) 0x0 (max)
  Is Absolute       : Yes


Feature Value Capability #3
-----------------------------------------------------
  Report ID         : 0x11
  Usage Page        : Reserved (0x12)
  Bit Size          : 0x20
  Report Count      : 0x1
  Units Exponent    : 0xC
  Has Null          : No
  Alias             : No
  Usage Range       : 0x103 (min)  0x103 (max)
  Data Index Range  : 0x3 (min) 0x3 (max)
  Physical Range    : 0x0 (min) 0x0 (max)
  Logical Range     : 0xFFE17B80 (min) 0x1E8480 (max)
  String Index Range: 0x0 (min) 0x0 (max)
  Designator Range  : 0x0 (min) 0x0 (max)
  Is Absolute       : Yes


Feature Value Capability #4
-----------------------------------------------------
  Report ID         : 0x11
  Usage Page        : Reserved (0x12)
  Bit Size          : 0x20
  Report Count      : 0x1
  Units Exponent    : 0xC
  Has Null          : No
  Alias             : No
  Usage Range       : 0x102 (min)  0x102 (max)
  Data Index Range  : 0x4 (min) 0x4 (max)
  Physical Range    : 0x0 (min) 0x0 (max)
  Logical Range     : 0xFFE17B80 (min) 0x1E8480 (max)
  String Index Range: 0x0 (min) 0x0 (max)
  Designator Range  : 0x0 (min) 0x0 (max)
  Is Absolute       : Yes


Feature Value Capability #5
-----------------------------------------------------
  Report ID         : 0x11
  Usage Page        : Reserved (0x12)
  Bit Size          : 0x20
  Report Count      : 0x1
  Units Exponent    : 0xC
  Has Null          : No
  Alias             : No
  Usage Range       : 0x101 (min)  0x101 (max)
  Data Index Range  : 0x5 (min) 0x5 (max)
  Physical Range    : 0x0 (min) 0x0 (max)
  Logical Range     : 0xFFE17B80 (min) 0x1E8480 (max)
  String Index Range: 0x0 (min) 0x0 (max)
  Designator Range  : 0x0 (min) 0x0 (max)
  Is Absolute       : Yes


Feature Value Capability #6
-----------------------------------------------------
  Report ID         : 0x12
  Usage Page        : Reserved (0x12)
  Bit Size          : 0x10
  Report Count      : 0x1
  Units Exponent    : 0x0
  Has Null          : No
  Alias             : No
  Usage Range       : 0x200 (min)  0x200 (max)
  Data Index Range  : 0x6 (min) 0x6 (max)
  Physical Range    : 0x0 (min) 0x0 (max)
  Logical Range     : 0x0 (min) 0xFFFF (max)
  String Index Range: 0x0 (min) 0x0 (max)
  Designator Range  : 0x0 (min) 0x0 (max)
  Is Absolute       : Yes


Feature Value Capability #7
-----------------------------------------------------
  Report ID         : 0x12
  Usage Page        : Reserved (0x12)
  Bit Size          : 0x10
  Report Count      : 0x1
  Units Exponent    : 0x0
  Has Null          : No
  Alias             : No
  Usage Range       : 0x201 (min)  0x201 (max)
  Data Index Range  : 0x7 (min) 0x7 (max)
  Physical Range    : 0x0 (min) 0x0 (max)
  Logical Range     : 0x0 (min) 0xFFFF (max)
  String Index Range: 0x0 (min) 0x0 (max)
  Designator Range  : 0x0 (min) 0x0 (max)
  Is Absolute       : Yes


Feature Value Capability #8
-----------------------------------------------------
  Report ID         : 0x12
  Usage Page        : Reserved (0x12)
  Bit Size          : 0x20
  Report Count      : 0x1
  Units Exponent    : 0x0
  Has Null          : No
  Alias             : No
  Usage Range       : 0x202 (min)  0x202 (max)
  Data Index Range  : 0x8 (min) 0x8 (max)
  Physical Range    : 0x0 (min) 0x0 (max)
  Logical Range     : 0x0 (min) 0x7FFFFFFF (max)
  String Index Range: 0x0 (min) 0x0 (max)
  Designator Range  : 0x0 (min) 0x0 (max)
  Is Absolute       : Yes


Feature Value Capability #9
-----------------------------------------------------
  Report ID         : 0x12
  Usage Page        : Reserved (0x12)
  Bit Size          : 0x10
  Report Count      : 0x1
  Units Exponent    : 0x0
  Has Null          : No
  Alias             : No
  Usage Range       : 0x203 (min)  0x203 (max)
  Data Index Range  : 0x9 (min) 0x9 (max)
  Physical Range    : 0x0 (min) 0x0 (max)
  Logical Range     : 0x0 (min) 0xFFFF (max)
  String Index Range: 0x0 (min) 0x0 (max)
  Designator Range  : 0x0 (min) 0x0 (max)
  Is Absolute       : Yes


Feature Value Capability #10
-----------------------------------------------------
  Report ID         : 0x12
  Usage Page        : Reserved (0x12)
  Bit Size          : 0x20
  Report Count      : 0x1
  Units Exponent    : 0xC
  Has Null          : No
  Alias             : No
  Usage Range       : 0x204 (min)  0x204 (max)
  Data Index Range  : 0xA (min) 0xA (max)
  Physical Range    : 0x0 (min) 0x0 (max)
  Logical Range     : 0x0 (min) 0x7FFFFFFF (max)
  String Index Range: 0x0 (min) 0x0 (max)
  Designator Range  : 0x0 (min) 0x0 (max)
  Is Absolute       : Yes


Feature Value Capability #11
-----------------------------------------------------
  Report ID         : 0x12
  Usage Page        : Reserved (0x12)
  Bit Size          : 0x20
  Report Count      : 0x1
  Units Exponent    : 0xC
  Has Null          : No
  Alias             : No
  Usage Range       : 0x205 (min)  0x205 (max)
  Data Index Range  : 0xB (min) 0xB (max)
  Physical Range    : 0x0 (min) 0x0 (max)
  Logical Range     : 0x0 (min) 0x7FFFFFFF (max)
  String Index Range: 0x0 (min) 0x0 (max)
  Designator Range  : 0x0 (min) 0x0 (max)
  Is Absolute       : Yes


Feature Value Capability #12
-----------------------------------------------------
  Report ID         : 0x13
  Usage Page        : Reserved (0x12)
  Bit Size          : 0x8
  Report Count      : 0x1
  Units Exponent    : 0x0
  Has Null          : No
  Alias             : No
  Usage Range       : 0x301 (min)  0x301 (max)
  Data Index Range  : 0xC (min) 0xC (max)
  Physical Range    : 0x0 (min) 0x0 (max)
  Logical Range     : 0x0 (min) 0x4 (max)
  String Index Range: 0x0 (min) 0x0 (max)
  Designator Range  : 0x0 (min) 0x0 (max)
  Is Absolute       : Yes


Feature Value Capability #13
-----------------------------------------------------
  Report ID         : 0x13
  Usage Page        : Reserved (0x12)
  Bit Size          : 0x10
  Report Count      : 0x1
  Units Exponent    : 0x0
  Has Null          : No
  Alias             : No
  Usage Range       : 0x300 (min)  0x300 (max)
  Data Index Range  : 0xD (min) 0xD (max)
  Physical Range    : 0x0 (min) 0x0 (max)
  Logical Range     : 0x0 (min) 0xFFFF (max)
  String Index Range: 0x0 (min) 0x0 (max)
  Designator Range  : 0x0 (min) 0x0 (max)
  Is Absolute       : Yes


Feature Value Capability #14
-----------------------------------------------------
  Report ID         : 0x14
  Usage Page        : Reserved (0x12)
  Bit Size          : 0x8
  Report Count      : 0x1
  Units Exponent    : 0x0
  Has Null          : No
  Alias             : No
  Usage Range       : 0x400 (min)  0x400 (max)
  Data Index Range  : 0xE (min) 0xE (max)
  Physical Range    : 0x0 (min) 0x0 (max)
  Logical Range     : 0x0 (min) 0x7 (max)
  String Index Range: 0x0 (min) 0x0 (max)
  Designator Range  : 0x0 (min) 0x0 (max)
  Is Absolute       : Yes



```

### Displaying KdPrint Calls

To allow the debugger to show the *KdPrint* calls used in the code, use the command `ed Kd_DEFAULT_Mask 0x8`.
There may be other masks which work, but we found this mask to be sufficient for our 
needs.

## Other Notes

### Devcon

We found that `devcon.exe` would fail consistently and couldn't find a clear explanation as to 
why that was the case. After discussing with an experienced driver developer, a common cause 
of this behavior is not having a kernel debugger attached to the machine. Also ensure that
you have turned on TESTSIGNING.

### HID Reports

When modifying HID reports, two found two primary souces of error:

1. As noted in the [HID documentation](https://usb.org/sites/default/files/hid1_11.pdf) section 6.22
a given entry in the report will apply until it has been overwritten by another value. For example, 
the Report_Count(1) will continue to apply until another Report_Count() command is used. 
2. Mismatches between the data sent to a report and the report size can cause issues. Carefully inspect 
the report descriptor and the actual data being sent. Below is example of comparing the report 
descriptor to the data being sent:
```
..Collection (Logical)...............0x0054: A1 02
....Report ID (17)...................0x0056: 85 11              // HID_USAGE_CAPABILITIES
....Report Size (8)..................0x0058: 75 08
....Report Count (1).................0x005A: 95 01
....Usage (0x100)....................0x005C: 0A 00 01           // HID_USAGE_TRACKER_QUALITY
....Logical Minimum (0)..............0x005F: 15 00
....Logical Maximum (1)..............0x0061: 25 01
....Feature (Cnst,Var,Abs)...........0x0063: B1 03              // 1 feature, 8 bits in size
....Report Count (1).................0x0065: 95 01
....Report Size (16).................0x0067: 75 10
....Logical Minimum (0)..............0x0069: 15 00
....Logical Maximum (-1).............0x006B: 26 FF FF
....Feature (Cnst,Var,Abs)...........0x006E: B1 03
....Collection (Physical)............0x0070: A1 00
......Report Count (5)...............0x0072: 95 05
......Report Size (32)...............0x0074: 75 20
......Logical Minimum (-2147483648)..0x0076: 17 00 00 00 80
......Logical Maximum (2147483647)...0x007B: 27 FF FF FF 7F
......Usage (0x101)..................0x0080: 0A 01 01           // HID_USAGE_MINIMUM_TRACKING_DISTANCE  
......Usage (0x102)..................0x0083: 0A 02 01           // HID_USAGE_OPTIMUM_TRACKING_DISTANCE  
......Usage (0x103)..................0x0086: 0A 03 01           // HID_USAGE_MAXIMUM_TRACKING_DISTANCE  
......Usage (0x104)..................0x0089: 0A 04 01           // HID_USAGE_MAXIMUM_SCREEN_PLANE_WIDTH 
......Usage (0x105)..................0x008C: 0A 05 01           // HID_USAGE_MAXIMUM_SCREEN_PLANE_HEIGHT
......Feature (Cnst,Var,Abs).........0x008F: B1 03              // 5 features, 32 bits in size
....End Collection ()................0x0091: C0
..End Collection ()..................0x0092: C0

11,             // ReportId                 = HID_USAGE_CAPABILITIES
01,             // Tracker Quality          = 0x01
50,C3,00,00,    // MinimumTrackingDistance  = 0x0000C350 = 50000
E8,FD,00,00,    // OptimumTrackingDistance  = 0x0000FDE8 = 65000
90,5F,01,00,    // MaximumTrackingDistance  = 0x00015F90 = 90000
00,00,00,00,    // MaximumScreenPlaneWidth  = 0x00000000 
00,00,00,00,    // MaximumScreenPlaneHeight = 0x00000000
```