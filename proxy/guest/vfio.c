#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/epoll.h>
#include <sys/ioctl.h>
#include <sys/eventfd.h>
#include <linux/vfio.h>
#include <linux/pci.h>

#include "../proxy.h"
#include "internal.h"
#include "vfio.h"

int vfio_set_irq(struct guest_proxy *pxy);
int vfio_subscribe_irqs(struct guest_proxy *pxy);
int vfio_map_all_regions(struct guest_proxy *pxy);
int vfio_map_region(int dev, int idx, void **addr, size_t *len, size_t *off);
int vfio_get_region_info(int dev, int i, struct vfio_region_info *reg);

int vfio_init(struct guest_proxy *pxy)
{
  struct vfio_group_status g_status = { .argsz = sizeof(g_status) };

  /* Create vfio container */
  if ((pxy->cont = open("/dev/vfio/vfio", O_RDWR)) < 0)
  {
    fprintf(stderr, "vfio_init: failed to open vfio container");
    goto error_cont;
  }

  /* Check API version of container */
  if (ioctl(pxy->cont, VFIO_GET_API_VERSION) != VFIO_API_VERSION)
  {
    fprintf(stderr, "vfio_init: api version doesn't match.\n");
    goto error_cont;
  }

  /* Open the vfio group */
  if((pxy->group = open(VFIO_GROUP, O_RDWR)) < 0)
  {
    fprintf(stderr, "vfio_init: failed to open vfio group.\n");
    goto error_cont;
  }

  /* Test if group is viable and available */
  ioctl(pxy->group, VFIO_GROUP_GET_STATUS, &g_status);
  if (!(g_status.flags & VFIO_GROUP_FLAGS_VIABLE))
  {
    fprintf(stderr, "vfio_init: group is not viable or avail.\n");
    goto error_group;
  }

  /* Add group to container */
  if (ioctl(pxy->group, VFIO_GROUP_SET_CONTAINER, &pxy->cont) < 0)
  {
    fprintf(stderr, "vfio_init: failed to add group to container.\n");
    goto error_group;
  }

  /* Enable desired IOMMU model */
  if (ioctl(pxy->cont, VFIO_SET_IOMMU, VFIO_NOIOMMU_IOMMU) < 0)
  {
    fprintf(stderr, "vfio_init: failed to set IOMMU model.\n");
    goto error_group;
  }

  /* Get file descriptor for device */
  if ((pxy->dev = ioctl(pxy->group, 
      VFIO_GROUP_GET_DEVICE_FD, VFIO_PCI_DEV)) < 0)
  {
    fprintf(stderr, "vfio_init: failed to get fd for VFIO device.\n");
    goto error_group;
  }

  /* Device reset and go */
  if (ioctl(pxy->dev, VFIO_DEVICE_RESET) < 0)
  {
    fprintf(stderr, "vfio_init: dev reset failed.\n");
    goto error_dev;
  }

  /* Set interrupt request fd */
  if (vfio_set_irq(pxy) < 0)
  {
    fprintf(stderr, "vfio_init: failed to set interrupt requests.\n");
    goto error_dev;
  }

  /* Map shared memory regions */
  if (vfio_map_all_regions(pxy) < 0)
  {
    fprintf(stderr, "vfio_init: failed to map mem regions.\n");
    goto error_dev;

  }

  /* Add interrupt request fd to interest list */
  if (vfio_subscribe_irqs(pxy) < 0)
  {
    fprintf(stderr, "vfio_init: failed to subscrive to interrupts.\n");
    goto error_dev;
  }

  return 0;

error_dev:
  close(pxy->dev);
error_group:
  close(pxy->group);
error_cont:
  close(pxy->cont);

  return -1;
}

int vfio_set_irq(struct guest_proxy *pxy)
{
  struct vfio_irq_set irq;

  if ((pxy->irq_fd = eventfd(0, EFD_NONBLOCK)) < 0)
  {
    fprintf(stderr, "vfio_set_irq: failed to create event fd.\n");
    return -1;
  }

  irq.flags = VFIO_IRQ_SET_DATA_EVENTFD | VFIO_IRQ_SET_ACTION_TRIGGER;
  irq.index = 2;
  irq.start = 0;
  irq.count = 1;
  irq.data[0] = pxy->irq_fd;
  irq.argsz = sizeof(struct vfio_irq_set) + sizeof(int);

  if (ioctl(pxy->irq_fd, VFIO_DEVICE_SET_IRQS, &irq) < 0)
  {
    fprintf(stderr, "vfio_set_irq: failed to set interrupt request.\n");
    goto error_close;
  }

  return 0;

error_close:
  close(pxy->irq_fd);

return -1;

}

int vfio_subscribe_irqs(struct guest_proxy *pxy)
{
  struct epoll_event ev;

  ev.events = EPOLLIN;
  ev.data.u32 = EP_NOTIFY;
  ev.data.fd = pxy->irq_fd;
  
  if (epoll_ctl(pxy->epfd, EPOLL_CTL_ADD, pxy->irq_fd, &ev) != 0)
  {
    fprintf(stderr, "vfio_subscribe_irqs: addint irq_fd to epoll failed.\n");
    return -1;
  }

  return 0;
}

int vfio_map_all_regions(struct guest_proxy *pxy)
{
  /* Map BAR 0 to receive interrupts */
  if (vfio_map_region(pxy->dev, 0, pxy->sgm, 
      &pxy->sgm_size, &pxy->sgm_off) != 0)
  {
    fprintf(stderr, "vfio_map_all_regions: failed to map sgm region.\n");    
    return -1;
  }

  /* Map BAR 2 for shm between TAS and guest */
  if (vfio_map_region(pxy->dev, 2, pxy->shm, 
      &pxy->shm_size, &pxy->shm_off) != 0)
  {
    fprintf(stderr, "vfio_map_all_regions: failed to map shm region.\n");    
    return -1;
  }

  return 0;
}

int vfio_map_region(int dev, int idx, void **addr, size_t *len, size_t *off)
{
  void *ret;
  int prot, flags;
  struct vfio_region_info reg = { .argsz = sizeof(reg) };

  if (vfio_get_region_info(dev, idx, &reg) != 0) 
  {
    fprintf(stderr, "vfio_map_region: failed to get region info.\n");  
    return -1;
  }

  prot = PROT_READ | PROT_WRITE;
  flags = MAP_SHARED | MAP_POPULATE;
  ret = mmap(NULL, reg.size, prot, flags, dev, reg.offset);
  if (ret == MAP_FAILED)
  {
    fprintf(stderr, "vfio_map_region: mmap failed.\n");
    return -1;
  }

  *addr = ret;
  *len = reg.size;
  *off = reg.offset;

  return 0;
}

int vfio_get_region_info(int dev, int i, struct vfio_region_info *reg)
{
  reg->index = i;

  if (ioctl(dev, VFIO_DEVICE_GET_REGION_INFO, reg))
  {
    fprintf(stderr, "vfio_get_region_info: failed to get info.\n");
    return -1;
  }

  return 0;
}