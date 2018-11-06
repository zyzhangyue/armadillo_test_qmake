#ifndef MODULE_CSIRAWDATAFETCHER
#define MODULE_CSIRAWDATAFETCHER

bool raw_csi_driver_init(void);
bool get_raw_csi_from_driver(void*);
void clean_on_exit(void);

#endif
