#include <stdio.h>
#include <signal.h>
#include "csi_packet.h"
#include "module_predosimpledoppler.h"
#include "module_csisharedmemory.h"
#include "module_csirawdatafetcher.h"


int main()
{
    int count = 0;
    module_predoSimpleDoppler module_pre;

    csi_packet *packet;
    // packet points to the csi_packet struct array returned
    if (!module_pre.Module_predoSimpleDoppler_File("./amaoa2_1.dat", packet, 1, &count)) {
        return 0;
    }
    std::cout << "Total Count: " << count << endl;
    // free it in case of memory leak
    free(packet);
    return 0;
}
