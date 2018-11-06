#ifndef MODULE_PREDOSIMPLEDOPPLER_H
#define MODULE_PREDOSIMPLEDOPPLER_H

#include "csi_packet.h"

class module_predoSimpleDoppler
{
private:
    uint8_t *data, *payload;
    uint8_t antenna_sel, Ntx, Nrx;
    uint16_t calc_len, len;

private:
    uint8_t perm[3], regu[3], *map;

private:
    bool read_bfee(csi_packet*, void*);
    bool read_bf_file(const char*, csi_packet* &, int32_t*);
    bool Module_Scaled(csi_packet*, const int);
    bool get_scaled_csi(csi_packet*);
    double get_total_rss(csi_packet*);
    double get_total_csi(csi_packet*);
    void print_csi_packet(csi_packet*);

public:
    module_predoSimpleDoppler();
    bool Module_predoSimpleDoppler_Shm(csi_packet*, void*);
    bool Module_predoSimpleDoppler_File(const char*, csi_packet* &, const int, int*);
};

#endif // MODULE_PREDOSIMPLEDOPPLER_H
