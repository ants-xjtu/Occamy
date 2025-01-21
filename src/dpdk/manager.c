#include "main.h"

uint64_t get_buffer_size() { return app.buffer_size; }

inline uint64_t get_occu_size(uint32_t port_id, uint32_t queue_id) {
  if (unlikely(app.occu_size_in[port_id][queue_id] <
               app.occu_size_out[port_id][queue_id] +
                   app.occu_size_drop[port_id][queue_id])) {
    return 0;
  }
  return app.occu_size_in[port_id][queue_id] -
         (app.occu_size_out[port_id][queue_id] +
          app.occu_size_drop[port_id][queue_id]);
}

void update_buffer_info(uint32_t port_id, uint32_t queue_id, uint32_t pkt_len,
                        bool add, bool drop) {
  if (add) {
    if (unlikely(drop)) {
      rte_panic("error in update buffer !");
    }
    app.occu_size_in[port_id][queue_id] += pkt_len;
  } else {

    if (drop) {
      app.occu_size_drop[port_id][queue_id] += pkt_len;
    } else {
      app.occu_size_out[port_id][queue_id] += pkt_len;
    }
  }
}
