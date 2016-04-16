#ifndef _LJB_PROXYKMD_IOCTL_H_
#define _LJB_PROXYKMD_IOCTL_H_


#define LJB_PROXYKMD_IOCTL_BASE        0x0000

#define IOCTL_PROXYKMD_PLUGIN_FAKE_MONITOR          \
    CTL_CODE(FILE_DEVICE_UNKNOWN,                   \
    LJB_PROXYKMD_IOCTL_BASE,                        \
    METHOD_BUFFERED,                                \
    FILE_ANY_ACCESS)

#define IOCTL_PROXYKMD_UNPLUG_FAKE_MONITOR          \
    CTL_CODE(FILE_DEVICE_UNKNOWN,                   \
    (LJB_PROXYKMD_IOCTL_BASE + 1),                  \
    METHOD_BUFFERED,                                \
    FILE_ANY_ACCESS)

#endif /* _LJB_PROXYKMD_IOCTL_H_ */
