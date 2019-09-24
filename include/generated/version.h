#ifndef _KERNEL_VERSION_H_
#define _KERNEL_VERSION_H_

#define ZEPHYR_VERSION_CODE 67841
#define ZEPHYR_VERSION(a,b,c) (((a) << 16) + ((b) << 8) + (c))

#define KERNELVERSION \
0x01090100
#define KERNEL_VERSION_NUMBER     0x010901
#define KERNEL_VERSION_MAJOR      1
#define KERNEL_VERSION_MINOR      9
#define KERNEL_PATCHLEVEL         1
#define KERNEL_VERSION_STRING     "1.9.1"

#endif /* _KERNEL_VERSION_H_ */
