#ifndef __IPC_H__
#define __IPC_H__

#include <types.h>

struct _ioctl{
         void *data;
         u32 len;
};

struct _ipcreq
{                  //ipc struct size: 32
   u32 cmd;         //0
   s32 result;         //4
   union {            //8
      s32 fd;
      u32 req_cmd;
   };
   union {
      struct {
         char *filepath;
         u32 mode;
      } open;
      struct {
         void *data;
         u32 len;
      } read, write;
      struct {
         s32 where;
         s32 whence;
      } seek;
      struct {
         u32 ioctl;
         void *buffer_in;
         u32 len_in;
         void *buffer_io;
         u32 len_io;
      } ioctl;
      struct {
         u32 ioctl;
         u32 argcin;
         u32 argcio;
         struct _ioctl *argv;
      } ioctlv;
      u32 args[5];
   };

   
} ATTRIBUTE_PACKED;

#endif
