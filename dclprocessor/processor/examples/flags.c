#include <stdio.h>

// 128 bit = 3 bit dproc (<= 8 states: ERROR, UNKNOWN, INIT(=STOP), INIT->LIVE(=STARTING), LIVE->INIT(STOPPING), LIVE(=START)) + dproc->pipeline (3 bit = states + NOT_MPLEMENTED) + 4*3bit dproc->collector (included: n_sources <= 16, i.e. n_sources = 4bit * (3bit = states + NOT_IMPLEMENTED)) + 8 bit dproc->msg
// server: 3bit dproc + 3bit dproc->pipeline + (dproc->collector: state = NOT_IMPLEMENTED) +  8 bit dproc->msg (proxy: server for cmd_mngr + client for)
// first: 3bit dproc + (dproc->pipeline: state = NOT_IMPLEMENTED) + 8(?) bit dproc->collector (n_sources <= 16 * dproc->collector->source) +  bit8(?) bit dproc->msg (proxy_client + (n_sources <= 16 = 4bit) * dproc->colle)
// middle: 3bit dproc + 

#define DPF_INIT 1
#define

int main() {
    
    return 0;
}
