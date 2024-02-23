#ifndef SKE_SECURE_PORT_H
#define SKE_SECURE_PORT_H

#include <ske.h>

#ifdef __cplusplus
extern "C" {
#endif

void ske_sec_enable_secure_port(u16 sp_key_idx);
void ske_sec_disable_secure_port(void);

#ifdef __cplusplus
}
#endif

#endif

