#pragma once

#ifndef COREMEM

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <unistd.h>

#define MEMIOCTMAPC _IOR('M', 0x2, int)
#define MEMIOCTMAP  _IOWR('M', 0x3, void *)

#define __core(call, size, args...) \
    call(##args); \
    if (size == 0)

#define DEV_FILE_NAME "/dev/dvmem"

namespace coremem
{

struct mem_process_rw {
    pid_t pid;
    size_t virt_addr;
    void *base;
    size_t len;
};

struct vm_area_process {
    unsigned long start;
    unsigned long end;
    char perms[5];
    unsigned long offset;
    unsigned int major;
    unsigned int minor;
    unsigned long inode;
    char path[128];
};

extern int fd;

extern int pid; // emm ...

extern int vm_area_count;

extern int useless_len;

extern vm_area_process *vm_area;

extern thread_local mem_process_rw core_clint[1];

int connect_core();

int judge_process_bit(int pid);

int get_process_maps(int pid);

template <class T, class T1>
size_t readv(T1 addr, T *data, size_t len)
{
    core_clint->pid = pid;
    core_clint->base = data;
    core_clint->virt_addr = (size_t)addr;
    core_clint->len = len;
    return read(fd, core_clint, sizeof(mem_process_rw));
}

template <typename T, typename T1>
T readv(T1 addr)
{
    T temp;
    readv(addr, &temp, sizeof(T));
    return temp;
}

template <class T, class T1>
T readv(T1 addr, size_t *size)
{
    T temp;
    *size = readv(addr, &temp, sizeof(T));
    return temp;
}

template <class T, class T1>
void readv(T1 addr, T *data)
{
    readv(addr, data, sizeof(T));
}

template <class T, class T1>
void readv(T1 addr, T *data, size_t len, size_t *size)
{
    *size = readv(addr, data, len);
}

template <class T, class T1>
void readv(T1 addr, T *data, size_t *size)
{
    *size = readv(addr, data, sizeof(T));
}

template <class T, class T1>
size_t writev(T1 addr, T *data, size_t len)
{
    core_clint->pid = pid;
    core_clint->base = data;
    core_clint->virt_addr = (size_t)addr;
    core_clint->len = len;
    return write(fd, core_clint, sizeof(mem_process_rw));
}

template <typename T, typename T1>
void writev(T1 addr, T &&data)
{
    writev(addr, &data, sizeof(T));
}

template <class T, class T1>
void writev(T1 addr, T &&data, size_t *size)
{
    *size = writev(addr, &data, sizeof(T));
}

template <class T, class... Args>
T read_pointer_safe(T start, size_t &size, Args &&...args)
{
    T address;
    T offset[] = {args...};

    address = start + offset[0];
    for (int i = 1; i < sizeof...(args); ++i) {
        readv(address, &address, size);
        if (size != sizeof(T))
            return 0;

        address += offset[i];
    }
    return address;
}

template <typename T, typename... Args>
T read_pointer(T start, Args &&...args)
{
    return read_pointer_safe(start, useless_len, args...);
}

} // namespace coremem

#endif
