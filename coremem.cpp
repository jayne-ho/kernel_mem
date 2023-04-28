#include "coremem.hh"

#ifndef COREMEM
#define COREMEM

 // i think u better set process by u self

int coremem::fd = -1;

int coremem::pid = -1;

int coremem::vm_area_count = 0;

coremem::vm_area_process *coremem::vm_area = nullptr;

thread_local coremem::mem_process_rw coremem::core_clint[1];

int coremem::connect_core()
{
    return (fd = open(DEV_FILE_NAME, O_RDWR));
}

int coremem::get_process_maps(int pid)
{
    int count;

    free(vm_area);
    vm_area = nullptr;
    vm_area_count = 0;

    count = ioctl(fd, MEMIOCTMAPC, pid);
    if (count == 0)
        return 0;

    vm_area = (vm_area_process *)malloc(sizeof(vm_area_process) * count);
    core_clint->pid = pid;
    core_clint->base = vm_area;
    core_clint->len = count;
    vm_area_count = ioctl(fd, MEMIOCTMAP, core_clint);

    return vm_area_count;
}

int coremem::judge_process_bit(pid_t pid)
{

#if (__aarch64__) || (__x86_64__)

    char path[128];
    struct stat st_64, st_buf;

    sprintf(path, "/proc/%d/exe", pid);
    if ((stat("/system/bin/app_process64", &st_64) == -1) || (stat(path, &st_buf)) == -1)
        return -1; // perror has been set

    return (st_buf.st_ino != st_64.st_ino);

#endif

    return 1;
}

#endif
