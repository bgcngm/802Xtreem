#ifndef KGSL_HTC_H
#define KGSL_HTC_H

#include "kgsl_device.h"

/* Dump pid informations of all contexts *
 * caller need to hold context_lock      */
void kgsl_dump_contextpid_locked(struct idr *context_idr);

#endif
