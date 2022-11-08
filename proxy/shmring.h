#ifndef SHMRING_H_
#define SHMRING_H_

#include <stddef.h>

struct ring_header {
  int write_pos;
  int read_pos;
  int full;
  size_t ring_size;
};

struct ring_buffer {
  void *hdr_addr;
  void *buf_addr;
  size_t size;
};

struct ring_buffer* shmring_init(void *base_addr, size_t size);
int shmring_pop(struct ring_buffer *rx_ring, void *buf, size_t size);
int shmring_push(struct ring_buffer *tx_ring, void *buf, size_t size);

#endif /* ndef SHMRING_H_ */