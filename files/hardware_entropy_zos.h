#pragma once
#include <clock_gettime_zos.h>
int mbedtls_hardware_poll (void *data, unsigned char *output, size_t len, size_t *olen);
