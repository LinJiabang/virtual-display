/*!
    \file       usbav_class_100.h
    \brief      USB AV class 1.0 definitions
    \details    This files defines the USB Audio/Video class 1.0 structures,
                and constants.
    \authors    lucaslin
    \version    0.01a
    \date       May 23, 2013
    \todo       (Optional)
    \bug        (Optional)
    \warning    (Optional)
    \copyright  (c) 2013 Luminon Core Incorporated. All Rights Reserved.

    Revision Log
    + 0.01a;    May 23, 2013;   lucaslin
     - Created.

 */

/*
 USB AV class spec 1.0 definition
 */
#ifndef _USBAV_CLASS_100_H_
#define _USBAV_CLASS_100_H_

/*
 IAD for AV
 */
#define USBAV_IAD_CLASS                             0x10
#define USBAV_IAD_SUBCLASS                          0x00
#define USBAV_IAD_PROTOCOL                          0x00

/*
 standard USB interface Class/SubClass/Protocol
 refer to AV Function spec A.3 to A.6
 */
#define USBAV_INTERFACE_CLASS                       0x10

#define USBAV_INTERFACE_SUBCLASS_UNDEFINED          0x00
#define USBAV_INTERFACE_SUBCLASS_AVCONTROL          0x01
#define USBAV_INTERFACE_SUBCLASS_VIDEO_STREAMING    0x02
#define USBAV_INTERFACE_SUBCLASS_AUDIO_STREAMING    0x03

#define USBAV_INTERFACE_PROTOCOL_UNDEFINED          0x00
#define USBAV_INTERFACE_PROTOCOL_IP_VERSION_0100    0x10

/*
 AV request codes (A.9)
 */
#define USBAV_REQUEST_UNDEFINED                     0x00
#define USBAV_REQUEST_SETR                          0x01
#define USBAV_REQUEST_SET                           0x02
#define USBAV_REQUEST_GET                           0x03
#define USBAV_REQUEST_NOTIF                         0x04

/*
 AV property codes (A.10)
 */
#define USBAV_PROPERTY_RESERVED                     0x00
#define USBAV_PROPERTY_CUR                          0x01
#define USBAV_PROPERTY_NEXT                         0x02

/*
 AVControl Interface Control selectors(A11.1)
 */
#define USBAV_AC_CONTROL_UNDEFINED                  0x0000
#define USBAV_AC_AVDD_INFO                          0x0001
#define USBAV_AC_AVDD_CONTENT                       0x0002
#define USBAV_AC_COMMIT                             0x0003
#define USBAV_AC_POWER_LINE_FREQ                    0x0004
#define USBAV_AC_REMOTE_ONLY                        0x0005

/*
 Terminal Control Selectors (A11.2)
 */
#define USBAV_TE_CONTROL_UNDEFINED                  0x0000
#define USBAV_TE_CLUSTER                            0x0001

/*
 Mixer Unit Control Selectors (A11.3)
 */
#define USBAV_MU_CONTROL_UNDEFINED                  0x0000
#define USBAV_MU_AUDIOTRACK_SELECTOR                0x0001
#define USBAV_MU_LEVEL                              0x0002

/*
 Selector Unit Control Selectors (A11.5)
 */
#define USBAV_SU_CONTROL_UNDEFINED                  0x0000
#define USBAV_SU_SELECTOR                           0x0001

/*
 Feature Unit Control Selectors (A11.6)
 */
#define USBAV_FU_CONTROL_UNDEFINED                  0x0000
#define USBAV_FU_VIDEOTRACK_SELECTOR                0x0001
#define USBAV_FU_BRIGHTNESS                         0x0002
#define USBAV_FU_CONTRAST                           0x0003
#define USBAV_FU_AUDIOTRACK_SELECTOR                0x0004
#define USBAV_FU_MUTE                               0x0005
#define USBAV_FU_VOLUME                             0x0006
#define USBAV_FU_BASS                               0x0007
#define USBAV_FU_MID                                0x0008
#define USBAV_FU_TREBLE                             0x0009
#define USBAV_FU_GRAPHIC_EQ                         0x000A
#define USBAV_FU_DELAY                              0x000B
#define USBAV_FU_BASS_BOOT                          0x000C
#define USBAV_FU_LOUDNESS                           0x000D
#define USBAV_FU_INPUT_GAIN                         0x000E
#define USBAV_FU_AUDIO_INPUT_GAIN                   0x000F
#define USBAV_FU_INPUT_GAIN_PAD                     0x0010
#define USBAV_FU_PHASE_INVERTER                     0x0011

/*
 Converter Unit Control Selectors (A11.9)
 */
#define USBAV_CU_CONTROL_UNDEFINED                  0x0000
#define USBAV_CU_VIDEOTRACK_SELECTOR                0x0001
#define USBAV_CU_AUDIOTRACK_SELECTOR                0x0002
#define USBAV_CU_VIDEO_MODE_SELECTOR                0x0003
#define USBAV_CU_AUDIO_MODE_SELECTOR                0x0004
#define USBAV_CU_CLUSTER                            0x0005

/*
 Router Unit Control Selectors (A11.10)
 */
#define USBAV_RU_CONTROL_UNDEFINED                  0x0000
#define USBAV_RU_VIDEOTRACK_SELECTOR                0x0001
#define USBAV_RU_AUDIOTRACK_SELECTOR                0x0002
#define USBAV_RU_METADATATRACK_SELECTOR             0x0003
#define USBAV_RU_VIDEO_INPUT_PIN_SELECTOR           0x0004
#define USBAV_RU_AUDIO_INPUT_PIN_SELECTOR           0x0005
#define USBAV_RU_METADATA_INPUT_PIN_SELECTOR        0x0006
#define USBAV_RU_CLUSTER                            0x0007

/*
 AVData Control Selectors (A11.11)
 */
#define USBAV_AD_CONTROL_UNDEFINED                  0x0000
#define USBAV_AD_CONNECTOR                          0x0001
#define USBAV_AD_OVERLOAD                           0x0002
#define USBAV_AD_ACT_ALT_SETTING                    0x0003
#define USBAV_AD_TUNNEL                             0x0004
#define USBAV_AD_AUDIO_LANGUAGE                     0x0005
#define USBAV_AD_CEC_WRITE                          0x0006
#define USBAV_AD_CEC_WRITE_STATUS                   0x0007
#define USBAV_AD_CEC_READ                           0x0008
#define USBAV_AD_SOURCEDATA                         0x0009
#define USBAV_AD_SINKDATA                           0x000A
#define USBAV_AD_STREAM_SELECTOR                    0x000B
#define USBAV_AD_EDID                               0x000C
#define USBAV_AD_REFERENECE_CLOCK                   0x000D
#define USBAV_AD_CLOCK_CONNECTOR                    0x000E
#define USBAV_AD_CLOCK_VALID                        0x000F
#define USBAV_AD_PITCH                              0x0010

/*
 AV Device Class General Constants (A12)
 */
#define USBAV_MESSAGE_GRANULARITY                   32 /* 32 bytes */
#define USBAV_MESSAGE_INVAR_SIZE                    16 /* 16 bytes */
#define USBAV_MESSAGE_GRANULARITY_SHIFT             5

/*
 Legacy View Descriptor Constants A13
 */
/*
 Descriptor Type Codes (A13.1)
 */
#define USBAV_DESC_TYPE_AVCONTROL_IF                0x21
#define USBAV_DESC_TYPE_TERMINAL                    0x22
#define USBAV_DESC_TYPE_UNIT                        0x23
#define USBAV_DESC_TYPE_AVDATA                      0x24
#define USBAV_DESC_TYPE_AVCONTROL                   0x25
#define USBAV_DESC_TYPE_RANGES                      0x26
#define USBAV_DESC_TYPE_VIDEOBULK                   0x27
#define USBAV_DESC_TYPE_VIDEOISO                    0x28
#define USBAV_DESC_TYPE_AUDIOISO                    0x29

/*
 Terminal Type Codes (A13.2)
 */
#define USBAV_TERM_TYPE_INPUT                       0x0001
#define USBAV_TERM_TYPE_OUTPUT                      0x0002

/*
 Unit Type Codes (A13.3)
 */
#define USBAV_UNIT_TYPE_MIXER                       0x0001
#define USBAV_UNIT_TYPE_SELECTOR                    0x0002
#define USBAV_UNIT_TYPE_FEATURE                     0x0003
#define USBAV_UNIT_TYPE_EFFECT                      0x0004
#define USBAV_UNIT_TYPE_PROCESSING                  0x0005
#define USBAV_UNIT_TYPE_CONVERTER                   0x0006
#define USBAV_UNIT_TYPE_ROUTER                      0x0007

/*
 Data Entity Codes (A13.4)
 */
#define USBAV_DATA_ENTITY_GENERIC                   0x0001
#define USBAV_DATA_ENTITY_FRAMEBUFFER               0x0002
#define USBAV_DATA_ENTITY_VIDEOSTREAMING            0x0003
#define USBAV_DATA_ENTITY_AUDIOSTREAMING            0x0004
#define USBAV_DATA_ENTITY_HDMI                      0x0005

/*
 Ranges Codes (A13.5)
 */
#define USBAV_RANGES_RANGE                          0x0001
#define USBAV_RANGES_VALUELIST                      0x0002

#define USBAV_DEFAULT_DATA_SIZE                     4

/*
 Descriptor structures. The USB descriptors are little-endian by nature.
 */
#pragma pack(push, 1)

/*
 8.4.1  AVControl Interface Descriptor
 The AVControl Interface Descriptor is the top-level Descriptor for all the
 class-specific aspects of the AVFunction. It includes information that applies
 globally to the AVFunction. This Descriptor is returned after the standard USB
 Interface Descriptor that corresponds to the AVControl Interface and is
 followed by a number of associated Descriptors that describe further detailed
 aspects of the AVControl Interface.
 */
typedef struct _USBAV_CTRL_IFC_DESC
    {
    UCHAR       bLength;
    UCHAR       bDescriptorType;    /* always USBAV_DESC_TYPE_AVCONTROL_IF */
    USHORT      wBCDAVFunction;
    } USBAV_CTRL_IFC_DESC;

/*
 8.4.2  Terminal Descriptor
 The Terminal Descriptor describes a Terminal (Input or Output Terminal).
 Whenever necessary, the Terminal Descriptor is followed by one or more
 AVControl Descriptors describing the AVControls supported by this Terminal.
 */
typedef struct _USBAV_TERM_DESC
    {
    UCHAR       bLength;
    UCHAR       bDescriptorType;    /* always USBAV_DESC_TYPE_TERMINAL */
    USHORT      wTerminalType;      /* USBAV_TERM_TYPE_[INPUT | OUTPUT] */
    USHORT      wEntityID;
    USHORT      wAssocEntityID;
    USHORT      wSourceID;
    } USBAV_TERM_DESC;

/*
 8.4.3  Unit Descriptor
 The Unit Descriptor describes a Unit. Whenever necessary, the Unit Descriptor
 is followed by one or more AVControl Descriptors describing the AVControls
 supported by this Unit.

 Note that Legacy View restricts the number of advertised Input Pins to two
 (although more may be present ¡V only exposed through the AVDD).
 */
typedef struct _USBAV_UNIT_DESC
    {
    UCHAR       bLength;
    UCHAR       bDescriptorType;    /* always USBAV_DESC_TYPE_UNIT */
    USHORT      wUnitType;          /* USBAV_UNIT_TYPE_xxx */
    USHORT      wEntityID;
    USHORT      wSourceID1;
    USHORT      wSourceID2;
    } USBAV_UNIT_DESC;

/*
 8.4.4  AVControl Descriptor
 The AVControl Descriptor describes an AVControl. Whenever necessary, the
 AVControl Descriptor is followed by one or more Ranges Descriptors describing
 the ranges supported by this AVControl.
 */
typedef struct _USBAV_CTRL_DESC
    {
    UCHAR       bLength;
    UCHAR       bDescriptorType;    /* always USBAV_DESC_TYPE_AVCONTROL */
    USHORT      wControlSelector;
    USHORT      wRWN;
    } USBAV_CTRL_DESC;

/*
 8.4.4.1    Ranges Descriptor
 The Ranges Descriptor describes a range of values or a list of values that an
 AVControl supports. There are two variations of the Ranges Descriptor:
    RANGE       Ranges Descriptor
    VALUELIST   Ranges Descriptor

 One or more of these Descriptors may be returned after the AVControl Descriptor
 of the AVControl that requires its ranges to be expressed. The rules for
 expressing ranges are exactly the same as those used in AVControl Descriptions.
 See Section 9.4, ¡§Control Properties¡¨ for details.

 When the wRangeType field is set to RANGE, then the Ranges Descriptor shall be
 in the format shown in Table 8-5. The RANGE Ranges Descriptor describes one
 range for the AVControl.
 */
typedef struct _USBAV_RANGES_DESC
    {
    UCHAR       bLength;
    UCHAR       bDescriptorType;    /* always USBAV_DESC_TYPE_RANGES */
    USHORT      wRangeType;         /* RANGE or VALUELIST */
    } USBAV_RANGES_DESC;

typedef struct _USBAV_RANGE_RANGES_DESC
    {
    UCHAR       bLength;
    UCHAR       bDescriptorType;    /* always USBAV_DESC_TYPE_RANGES */
    USHORT      wRangeType;         /* USBAV_RANGES_RANGE */
    ULONG       dwMin;
    ULONG       dwMax;
    } USBAV_RANGE_RANGES_DESC;

typedef struct _USBAV_VALUELIST_RANGES_DESC
    {
    UCHAR       bLength;
    UCHAR       bDescriptorType;    /* always USBAV_DESC_TYPE_RANGES */
    USHORT      wRangeType;         /* USBAV_RANGES_VALUELIST */
    ULONG       dwValue[1];
    } USBAV_VALUELIST_RANGES_DESC;

/*
 8.4.5  AVData Entity Descriptor
 The AVData Entity Descriptor describes an AVData Entity. The AVData Entity may
 represent a Source or Sink for AV content.

 Whenever necessary, the AVData Entity Descriptor is followed by one or more
 AVControl Descriptors describing the AVControls supported by this AVData Entity.

 If the AVData Entity is an AVData FrameBuffer entity and therefore able to
 interact with VideoBulkStreams, then the AVData Entity Descriptor is followed
 by one or more VideoBulkStreamConfig Descriptors.

 If the AVData Entity is an AVData Video Streaming Interface and therefore able
 to interact with VideoIsoStreams, then the AVData Entity Descriptor is followed
 by one or more VideoIsoStreamConfig Descriptors.

 If the AVData Entity is an AVData Audio Streaming Interface and therefore able
 to interact with AudioStreams, then the AVData Entity Descriptor is followed by
 one or more AudioStreamConfig Descriptors.

 In Legacy View, an AVData Entity that is not an AVData Streaming Interface
 shall only support the (mandatory) Alternate Setting 0 and one Active
 Alternate Setting 1. An AVData Streaming Interface may support one or more
 Active Alternate Settings. However, since there is no class-specific
 information to convey for the Active Alternate Setting(s), there is no need for
 an Alternate Setting Descriptor.
 */
typedef struct _USBAV_AVDATA_ENTITY_DESC
    {
    UCHAR       bLength;
    UCHAR       bDescriptorType;    /* always USBAV_DESC_TYPE_AVDATA */
    USHORT      wEntityType;        /* eg. USBAV_DATA_ENTITY_GENERIC */
    USHORT      wEntityID;
    USHORT      wClockDomain;       /* 0: no clock, 1: source, 2: sink */
    USHORT      wClockDomainID;
    } USBAV_AVDATA_ENTITY_DESC;

/*
 8.4.6  VideoBulkStreamConfig Descriptor
 The VideoBulkStreamConfig Descriptor describes one VideoBulkStream
 Configuration. For detailed information on VideoBulkStream Configurations,
 refer to [AVFORMAT_1].
 */
typedef struct _USBAV_VIDEO_BULK_STREAM_CFG_DESC
    {
    UCHAR       bLength;
    UCHAR       bDescriptorType;    /* always USBAV_DESC_TYPE_VIDEOBULK */
    USHORT      bmAttributes;
    USHORT      wChannels;
    USHORT      wVideoFrameRate;
    USHORT      wFrameOrganization;
    USHORT      wFrameFormat;
    USHORT      wVideoSampleFormat;
    USHORT      wSubSlotSize;
    USHORT      wBitResolution;
    USHORT      wVideoCompression;
    } USBAV_VIDEO_BULK_STREAM_CFG_DESC;

/*
 8.4.7  VideoIsoStreamConfig Descriptor
 The VideoIsoStreamConfig Descriptor describes one VideoIsoStream Configuration.
 For detailed information on VideoIsoStream Configurations, refer to
 [AVFORMAT_3].
 */
typedef struct _USBAV_VIDEO_ISO_STREAM_CFG_DESC
    {
    UCHAR       bLength;
    UCHAR       bDescriptorType;    /* always USBAV_DESC_TYPE_VIDEOISO */
    USHORT      bmAttributes;
    USHORT      wChannels;
    USHORT      wVideoFrameRate;
    USHORT      wFrameOrganization;
    USHORT      wFrameFormat;
    USHORT      wVideoSampleFormat;
    USHORT      wSubSlotSize;
    USHORT      wBitResolution;
    USHORT      wVideoSIPSize;
    } USBAV_VIDEO_ISO_STREAM_CFG_DESC;

/*
 8.4.8. AudioStreamConfig Descriptor
 The AudioStreamConfig Descriptor describes one AudioStream Configuration. For
 detailed information on AudioStream Configurations, refer to [AVFORMAT_2].
 */
typedef struct _USBAV_AUDIO_STREAM_CFG_DESC
    {
    UCHAR       bLength;
    UCHAR       bDescriptorType;    /* always USBAV_DESC_TYPE_AUDIOISO */
    USHORT      bmAttributes;
    USHORT      wChannels;
    ULONG       dAudioFrameRateMin;
    ULONG       dAudioFrameRateMax;
    USHORT      wAudioSampleFormat;
    USHORT      wSubSlotSize;
    USHORT      wBitResolution;
    } USBAV_AUDIO_STREAM_CFG_DESC;

/*
 9.3.1  Command Message
 We rely on Microsoft C compiler feature where bit fields are stored as
 little-endian where bit fields are stored from LSB to MSB.
 Do NOT USE this structure if you are not using MS C compiler.
 */
typedef struct _USBAV_CMD_MSG
    {
    UINT        Eid: 16;    /* DWORD0 bit [15:0] */
    UINT        Type: 4;    /* DWORD0 bit [19:16] */
    UINT        Prop: 4;    /* DWORD0 bit [23:20] */
    UINT        V: 1;       /* DWORD0 bit [24] */
    UINT        Res0: 7;    /* DWORD0 bit [31:25] */
    UINT        Cs: 16;     /* DWORD1 bit [15:0] */
    UINT        Ocn: 16;    /* DWORD1 bit [31:16] */
    UINT        Icn: 16;    /* DWORD2 bit [15:0] */
    UINT        Ipn: 16;    /* DWORD2 bit [31:16] */
    UINT        Datalen: 24;/* DWORD3 bit [15: 0] */
    UINT        Res1: 8;    /* DWORD3 bit [31: 16] */
    } USBAV_CMD_MSG;

/*
 9.3.2  Response Message
 We rely on Microsoft C compiler feature where bit fields are stored as
 little-endian where bit fields are stored from LSB to MSB.
 Do NOT USE this structure if you are not using MS C compiler.
 */
typedef struct _USBAV_RESP_MSG
    {
    UINT        Eid: 16;    /* DWORD0 bit [15:0] */
    UINT        Type: 4;    /* DWORD0 bit [19:16] */
    UINT        Prop: 4;    /* DWORD0 bit [23:20] */
    UINT        V: 1;       /* DWORD0 bit [24] */
    UINT        Res0: 6;    /* DWORD0 bit [30:25] */
    UINT        E: 1;       /* DWORD0 bit [31] */
    UINT        Cs: 16;     /* DWORD1 bit [15:0] */
    UINT        Ocn: 16;    /* DWORD1 bit [31:16] */
    UINT        Icn: 16;    /* DWORD2 bit [15:0] */
    UINT        Ipn: 16;    /* DWORD2 bit [31:16] */
    UINT        Datalen: 24;/* DWORD3 bit [15: 0] */
    UINT        Res1: 8;    /* DWORD3 bit [31: 16] */
    } USBAV_RESP_MSG;

/*
 9.3.3  Notify Message
 We rely on Microsoft C compiler feature where bit fields are stored as
 little-endian where bit fields are stored from LSB to MSB.
 Do NOT USE this structure if you are not using MS C compiler.
 */
typedef struct _USBAV_NOTIFY_MSG
    {
    UINT        Eid: 16;    /* DWORD0 bit [15:0] */
    UINT        Type: 4;    /* DWORD0 bit [19:16] */
    UINT        Prop: 4;    /* DWORD0 bit [23:20] */
    UINT        V: 1;       /* DWORD0 bit [24] */
    UINT        Res0: 7;    /* DWORD0 bit [31:25] */
    UINT        Cs: 16;     /* DWORD1 bit [15:0] */
    UINT        Ocn: 16;    /* DWORD1 bit [31:16] */
    UINT        Icn: 16;    /* DWORD2 bit [15:0] */
    UINT        Ipn: 16;    /* DWORD2 bit [31:16] */
    UINT        Datalen: 24;/* DWORD3 bit [15: 0] */
    UINT        Res1: 8;    /* DWORD3 bit [31: 16] */
    } USBAV_NOTIFY_MSG;

/*
 9.3.4  Null Message
 We rely on Microsoft C compiler feature where bit fields are stored as
 little-endian where bit fields are stored from LSB to MSB.
 Do NOT USE this structure if you are not using MS C compiler.
 */
typedef struct _USBAV_NULL_MSG
    {
    UINT        Eid: 16;    /* DWORD0 bit [15:0] */
    UINT        Res0: 16;   /* DWORD0 bit [31:16] */
    UINT        Res1: 32;   /* DWORD1 bit [31:0] */
    UINT        Res2: 32;   /* DWORD2 bit [31:0] */
    UINT        Res3: 32;   /* DWORD3 bit [31:0] */
    UINT        Zero[4];    /* DWORD4~ DWORD7 */
    } USBAV_NULL_MSG;

    
/*
 AVFormat1 Video Over Bulk v1.0.pdf , 4.1
 4.1    AV Header
 The AVHeader is exactly 32 bytes long and has the following layout:
 */
#define USBAV_FORMAT1_HEADER_SIZE           32

typedef struct _USBAV_FORMAT1_HEADER
    {
    USHORT      wFlags;         /* offset 0 */
    UCHAR       bVideoFrameID;  /* offset 2 */
    UCHAR       Reserved1;      /* offset 3 */
    ULONG       dStreamCtr;     /* offset 4 */
    ULONGLONG   qInputCtr;      /* offset 8 */
    ULONG       dPTSLow;        /* offset 16 */
    ULONG       dESCRBaseLow;   /* offset 20 */
    USHORT      wExtension;     /* offset 24 */
    UCHAR       Reserved6[6];   /* offset 26 */
    } USBAV_FORMAT1_HEADER;

#define USBAV_FORMAT1_INFO_BLOCK_SIZE       12
typedef struct _USBAV_FORMAT1_INFO_BLOCK
    {
    USHORT      X;
    USHORT      Y;
    USHORT      W;
    USHORT      H;
    ULONG       VideoDataLength;
    UCHAR       VideoData[1];
    } USBAV_FORMAT1_INFO_BLOCK;
#pragma pack(pop)
#endif