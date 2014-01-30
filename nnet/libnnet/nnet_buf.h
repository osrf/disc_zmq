#ifndef NNET_BUF_H
#define NNET_BUF_H

#include <stdint.h>

#define NNET_BUF_TX_SIZE 512
extern uint8_t nnet_buf_tx[NNET_BUF_TX_SIZE];

#define NNET_BUF_RX_SIZE 512
extern uint8_t nnet_buf_rx[NNET_BUF_RX_SIZE];

#endif

