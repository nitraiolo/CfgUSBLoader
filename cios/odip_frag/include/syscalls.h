#ifndef _IOS_SYSCALLS_H_
#define _IOS_SYSCALLS_H_

#include "types.h"
#include "ipc.h"

/* Prototypes */
s32   os_thread_create(u32 (*entry)(void *arg), void *arg, void *stack, u32 stacksize, u32 priority, s32 autostart);
void  os_thread_set_priority(u32 priority);
s32   os_thread_get_priority(void);
s32   os_get_thread_id(void);
s32   os_get_parent_thread_id(void);
s32   os_thread_continue(s32 id);
s32   os_thread_stop(s32 id);
s32   os_message_queue_create(void *ptr, u32 id);
s32   os_message_queue_receive(s32 queue, u32 *message, u32 flags);
s32   os_message_queue_send(s32 queue, u32 message, s32 flags);
s32   os_message_queue_now(s32 queue, u32 message, s32 flags);
s32   os_heap_create(void *ptr, s32 size);
s32   os_heap_destroy(s32 heap);
void *os_heap_alloc(s32 heap, u32 size);
void *os_heap_alloc_aligned(s32 heap, s32 size, s32 align);
void  os_heap_free(s32 heap, void *ptr);
s32   os_device_register(const char *devicename, s32 queuehandle);
void  os_message_queue_ack(void *message, s32 result);
void  os_sync_before_read(void *ptr, s32 size);
void  os_sync_after_write(void *ptr, s32 size);
void  os_syscall_50(u32 unknown);
void  os_puts(char *str);
s32   os_open(char *device, s32 mode);
s32   os_close(s32 fd);
s32   os_read(s32 fd, void *d, s32 len);
s32   os_write(s32 fd, void *s, s32 len);
s32   os_seek(s32 fd, s32 offset, s32 mode);
s32   os_ioctlv(s32 fd, s32 request, s32 bytes_in, s32 bytes_out, struct _ioctl *vector);
s32   os_ioctl(s32 fd, s32 request, void *in,  s32 bytes_in, void *out, s32 bytes_out);
s32   os_create_timer(s32 time_us, s32 repeat_time_us, s32 message_queue, s32 message);
s32   os_destroy_timer(s32 time_id);
s32   os_stop_timer(s32 timer_id);
s32   os_restart_timer(s32 timer_id, s32 time_us);
s32   os_timer_now(s32 time_id); 
s32   os_register_event_handler(s32 device, s32 queue, s32 message);
s32   os_unregister_event_handler(s32 device);
s32   os_software_IRQ(s32 dev);
void  os_set_uid(u32 pid, u32 uid);
void  os_set_gid(u32 pid, u32 gid);
s32   os_ppc_boot(const char *filepath);
s32   os_ios_boot(const char *filepath, u32 flag, u32 version);
void  os_get_key(s32 keyid, void *out);
void *os_virt_to_phys(void *ptr);

/* Prototypes of functions exported from the hacked IOS */
void dip_free(void *);
void dip_doReadHashEncryptedState(void);
u32 *dip_memcpy(u8 *, const u8 *, u32);
void *dip_alloc_aligned(u32, u32);
#endif
