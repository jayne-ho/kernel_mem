#include "coremem.hh"
#include "coremem.cpp"

int main()
{
    if (coremem::connect_core() < 0)
        return 1;

    coremem::pid = getpid();
    int j = 1234;
    int l = -1;
    size_t size = 0;
    int c = coremem::get_process_maps(coremem::pid);
    for (int i = 0; i < c; ++i)
        printf("%lx %lx %s %s\n", coremem::vm_area[i].start, coremem::vm_area[i].end, coremem::vm_area[i].perms, coremem::vm_area[i].path);

    printf("count: %d\n", c);
    int o = coremem::readv<int>(&j, &size);
    printf("real: %ld val: %d\n", size, o);

    coremem::writev(&j, c, &size);
    printf("real: %ld val: %d\n", size, j);
    return 0;
}
