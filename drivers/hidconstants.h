#ifndef _HIDCONSTANTS_H_
#define _HIDCONSTANTS_H_

#define HID_SIZE_ZERO                       0x00
#define HID_SIZE_BYTE                       0x01
#define HID_SIZE_WORD                       0x02
#define HID_SIZE_DWORD                      0x03

#define HID_MAIN_INPUT                      0x80
#define HID_MAIN_OUTPUT                     0x90
#define HID_MAIN_BEGIN_COLLECTION           0xA0
#define HID_MAIN_FEATURE                    0xB0
#define HID_MAIN_END_COLLECTION             0xC0

#define HID_GLOBAL_USAGE_PAGE               0x04
#define HID_GLOBAL_LOGICAL_MINIMUM          0x14
#define HID_GLOBAL_LOGICAL_MAXIMUM          0x24
#define HID_GLOBAL_PHYSICAL_MINIMUM         0x34
#define HID_GLOBAL_PHYSICAL_MAXIMUM         0x44
#define HID_GLOBAL_UNIT_EXPONENT            0x54
#define HID_GLOBAL_UNIT                     0x64
#define HID_GLOBAL_REPORT_SIZE              0x74
#define HID_GLOBAL_REPORT_ID                0x84
#define HID_GLOBAL_REPORT_COUNT             0x94
#define HID_GLOBAL_PUSH                     0xA4
#define HID_GLOBAL_POP                      0xB4

#define HID_LOCAL_USAGE                     0x08
#define HID_LOCAL_USAGE_MINIMUM             0x18
#define HID_LOCAL_USAGE_MAXIMUM             0x28
#define HID_LOCAL_DESIGNATOR_INDEX          0x38
#define HID_LOCAL_DESIGNATOR_MINIMUM        0x48
#define HID_LOCAL_DESIGNATOR_MAXIMUM        0x58
#define HID_LOCAL_STRING_INDEX              0x78
#define HID_LOCAL_STRING_MINIMUM            0x88
#define HID_LOCAL_STRING_MAXIMUM            0x98
#define HID_LOCAL_DELIMITER                 0xA8

#define HID_MAIN_DATA                       0x00
#define HID_MAIN_ARRAY                      0x00
#define HID_MAIN_ABSOLUTE                   0X00
#define HID_MAIN_NO_WRAP                    0X00
#define HID_MAIN_LINEAR                     0X00
#define HID_MAIN_PREFERRED_STATE            0X00
#define HID_MAIN_NO_NULL                    0X00
#define HID_MAIN_NON_VOLATILE               0x00
#define HID_MAIN_BIT_FIELD                  0X00

#define HID_MAIN_CONSTANT                   0x01
#define HID_MAIN_VARIABLE                   0x02
#define HID_MAIN_RELATIVE                   0x04
#define HID_MAIN_WRAP                       0x08
#define HID_MAIN_NON_LINEAR                 0x10
#define HID_MAIN_NO_PREFERRED               0x20
#define HID_MAIN_NULL_STATE                 0x40
#define HID_MAIN_VOLATILE                   0x80
//#define HID_MAIN_BUFFERED_BYTES       0x0100

#define HID_DATA_SIZE_UINT8                 0x08
#define HID_DATA_SIZE_UINT16                0x10
#define HID_DATA_SIZE_UINT32                0x20
#define HID_DATA_SIZE_UINT64                0x40

#define HID_COLLECTION_PHYSICAL             0x00
#define HID_COLLECTION_APPLICATION          0x01
#define HID_COLLECTION_LOGICAL              0x02
#define HID_COLLECTION_REPORT               0x03
#define HID_COLLECTION_NAMEDARRAY           0x04
#define HID_COLLECTION_USAGESWITCH          0x05
#define HID_COLLECTION_USAGEMODIFIER        0x06

#define HID_COLLECTION_ENDCOLLECTION        0x00


#define HID_USAGE_PAGE(usagePage)               (HID_GLOBAL_USAGE_PAGE | HID_SIZE_BYTE), usagePage
#define HID_USAGE(usage)                        (HID_LOCAL_USAGE  | HID_SIZE_BYTE), usage
#define HID_USAGE_WORD(usage)                   (HID_LOCAL_USAGE  | HID_SIZE_WORD), (usage & 0xFF), ((usage << 8) & 0xFF)

#define HID_BEGIN_COLLECTION(type)              (HID_MAIN_BEGIN_COLLECTION | HID_SIZE_BYTE), type
#define HID_BEGIN_PHYSICAL_COLLECTION()         HID_BEGIN_COLLECTION(HID_COLLECTION_PHYSICAL)
#define HID_BEGIN_LOGICAL_COLLECTION()          HID_BEGIN_COLLECTION(HID_COLLECTION_LOGICAL)
#define HID_BEGIN_APPLICATION_COLLECTION()      HID_BEGIN_COLLECTION(HID_COLLECTION_APPLICATION)
#define HID_END_COLLECTION()                    HID_MAIN_END_COLLECTION

#define HID_REPORT_ID(reportId)                 (HID_GLOBAL_REPORT_ID | HID_SIZE_BYTE), reportId
#define HID_REPORT_COUNT(count)                 (HID_GLOBAL_REPORT_COUNT | HID_SIZE_BYTE), count
#define HID_REPORT_SIZE(size)                   (HID_GLOBAL_REPORT_SIZE | HID_SIZE_BYTE), size
#define HID_REPORT_SIZE_UINT8()                 HID_REPORT_SIZE(8)
#define HID_REPORT_SIZE_UINT16()                HID_REPORT_SIZE(16)
#define HID_REPORT_SIZE_UINT32()                HID_REPORT_SIZE(32)

#define HID_INPUT_STATIC_VALUE()                (HID_MAIN_INPUT | HID_SIZE_BYTE), (HID_MAIN_CONSTANT | HID_MAIN_VARIABLE | HID_MAIN_ABSOLUTE)
#define HID_INPUT_DYNAMIC_VALUE()               (HID_MAIN_INPUT | HID_SIZE_BYTE), (HID_MAIN_DATA | HID_MAIN_VARIABLE | HID_MAIN_ABSOLUTE)
#define HID_FEATURE_STATIC_VALUE()              (HID_MAIN_FEATURE | HID_SIZE_BYTE), (HID_MAIN_CONSTANT | HID_MAIN_VARIABLE | HID_MAIN_ABSOLUTE)
#define HID_FEATURE_DYNAMIC_VALUE()             (HID_MAIN_FEATURE | HID_SIZE_BYTE), (HID_MAIN_DATA | HID_MAIN_VARIABLE | HID_MAIN_ABSOLUTE)
#define HID_OUTPUT_STATIC_VALUE()               (HID_MAIN_OUTPUT | HID_SIZE_BYTE), (HID_MAIN_CONSTANT | HID_MAIN_VARIABLE | HID_MAIN_ABSOLUTE)
#define HID_OUTPUT_DYNAMIC_VALUE()              (HID_MAIN_OUTPUT | HID_SIZE_BYTE), (HID_MAIN_DATA | HID_MAIN_VARIABLE | HID_MAIN_ABSOLUTE)


#define HID_LOGICAL_MINIMUM(val)                (HID_GLOBAL_LOGICAL_MINIMUM | HID_SIZE_BYTE), val
#define HID_LOGICAL_MAXIMUM(val)                (HID_GLOBAL_LOGICAL_MAXIMUM | HID_SIZE_BYTE), val
#define HID_LOGICAL_MINIMUM_WORD(val)           (HID_GLOBAL_LOGICAL_MINIMUM | HID_SIZE_WORD), (val & 0xFF), ((val << 8) & 0xFF)
#define HID_LOGICAL_MAXIMUM_WORD(val)           (HID_GLOBAL_LOGICAL_MAXIMUM | HID_SIZE_WORD), (val & 0xFF), ((val << 8) & 0xFF)
#define HID_LOGICAL_MINIMUM_DWORD(val)          (HID_GLOBAL_LOGICAL_MINIMUM | HID_SIZE_DWORD), (val & 0xFF), ((val << 8) & 0xFF), ((val << 16) & 0xFF), ((val << 24) & 0xFF)
#define HID_LOGICAL_MAXIMUM_DWORD(val)          (HID_GLOBAL_LOGICAL_MINIMUM | HID_SIZE_DWORD), (val & 0xFF), ((val << 8) & 0xFF), ((val << 16) & 0xFF), ((val << 24) & 0xFF)


#endif //_HIDCONSTANTS_H_

