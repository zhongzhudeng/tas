#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <errno.h>
#include <numaif.h>
#include <stdint.h>

#define MAX_NODES 32

void *util_create_shmsiszed(const char *name, size_t size, void *addr, 
    int *pfd, uint64_t node)
{
  int fd;
  void *p;
  unsigned long nmask[MAX_NODES];

  if ((fd = shm_open(name, O_CREAT | O_RDWR, 0666)) == -1) {
    perror("shm_open failed");
    goto error_out;
  }
  if (ftruncate(fd, size) != 0) {
    perror("ftruncate failed");
    goto error_remove;
  }

  if ((p = mmap(addr, size, PROT_READ | PROT_WRITE,
      MAP_SHARED | (addr == NULL ? 0 : MAP_FIXED) | MAP_POPULATE, fd, 0)) ==
      (void *) -1)
  {
    perror("mmap failed");
    goto error_remove;
  }

  memset(p, 0, size);

  if (node != UINT64_MAX)
  {
    nmask[0] = 1UL << node;
    if (mbind(p, size, MPOL_BIND, nmask, 
        MAX_NODES, MPOL_MF_MOVE_ALL | MPOL_MF_STRICT) < 0)
    {
      perror("util_create_shmsiszed: mbind failed");
      goto error_remove;
    }
  }

  if (pfd != NULL)
    *pfd = fd;
  else
    close(fd);

  return p;

error_remove:
  close(fd);
  shm_unlink(name);
error_out:
  return NULL;
}

void util_destroy_shm(const char *name, size_t size, void *addr)
{
  if (munmap(addr, size) != 0) {
    fprintf(stderr, "Warning: munmap failed (%s)\n", strerror(errno));
  }
  shm_unlink(name);
}

void *util_create_shmsiszed_huge(const char *name, size_t size,
    void *addr, int *pfd, char *flexnic_huge_prefix, uint64_t node)
{
  int fd;
  void *p;
  char path[128];
  unsigned long nmask[MAX_NODES];

  snprintf(path, sizeof(path), "%s/%s", flexnic_huge_prefix, name);

  if ((fd = open(path, O_CREAT | O_RDWR, 0666)) == -1) {
    perror("util_create_shmsiszed: open failed");
    goto error_out;
  }
  if (ftruncate(fd, size) != 0) {
    perror("util_create_shmsiszed: ftruncate failed");
    goto error_remove;
  }

  if ((p = mmap(addr, size, PROT_READ | PROT_WRITE,
      MAP_SHARED | (addr == NULL ? 0 : MAP_FIXED) | MAP_POPULATE, fd, 0)) ==
      (void *) -1)
  {
    perror("util_create_shmsiszed: mmap failed");
    goto error_remove;
  }

  memset(p, 0, size);
  
  if (node != UINT64_MAX)
  {
    nmask[0] = 1UL << node;
    if (mbind(p, size, MPOL_BIND, nmask, 
        MAX_NODES, MPOL_MF_MOVE_ALL | MPOL_MF_STRICT) < 0)
    {
      perror("util_create_shmsiszed: mbind failed");
      goto error_remove;
    }
  } 
  
    if (pfd != NULL)
    *pfd = fd;
  else
    close(fd);

  return p;

error_remove:
  close(fd);
  shm_unlink(name);
error_out:
  return NULL;
}

void util_destroy_shm_huge(const char *name, size_t size, void *addr,
    char *flexnic_huge_prefix)
{
  char path[128];

  snprintf(path, sizeof(path), "%s/%s", flexnic_huge_prefix, name);

  if (munmap(addr, size) != 0) {
    fprintf(stderr, "Warning: munmap failed (%s)\n", strerror(errno));
  }
  unlink(path);
}
