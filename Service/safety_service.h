#ifndef __SAFETY_SERVICE_H
#define __SAFETY_SERVICE_H
#include "typedefs.h"

system_state_t WarningProcess(system_data_t *data);
void SafetyService_HeartbeatTimeout(void);

#endif /* __SAFETY_SERVICE_H */
