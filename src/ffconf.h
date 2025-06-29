#ifndef FFCONF_H
#define FFCONF_H

/*---------------------------------------------------------------------------*/
/* Basic Configuration                                                       */
/*---------------------------------------------------------------------------*/



#define FFCONF_DEF       80286

#define FF_FS_READONLY   0       // RW
#define FF_FS_MINIMIZE   0
#define FF_USE_LABEL     1
#define FF_USE_FORWARD   0

#define FF_CODE_PAGE     437 
#define FF_USE_LFN       1       // LFN enabled
#define FF_MAX_LFN       255
#define FF_LFN_UNICODE   0       // 0=ASCII - 1=UTF-8
#define FF_LFN_BUF       255
#define FF_SFN_BUF       12

#define FF_USE_FIND        1     // for f_findfirst/f_findnext
#define FF_USE_MKFS        0     // no format needed
#define FF_USE_FASTSEEK    1     // Fast seek

#define FF_FS_RPATH      0       // NO CURRENT DIR
#define FF_VOLUMES       1
#define FF_STR_VOLUME_ID 0
#define FF_VOLUME_STRS   ""
#define FF_MULTI_PARTITION 0
#define FF_MIN_SS        512
#define FF_MAX_SS        512
#define FF_USE_TRIM      0
#define FF_FS_NOFSINFO   0

#define FF_FS_TINY       0
#define FF_FS_EXFAT      0       // ONLY FAT16 / FAT32 FOR NOW

#define FF_FS_NORTC      1       // NO RTC
#define FF_NORTC_MON     1
#define FF_NORTC_MDAY    1
#define FF_NORTC_YEAR    2025

#define FF_FS_LOCK       0       // NO LOCK
#define FF_FS_REENTRANT  0       // NO MULTITASK
#define FF_FS_TIMEOUT    1000
#define FF_SYNC_t        HANDLE

#endif /* FFCONF_H */