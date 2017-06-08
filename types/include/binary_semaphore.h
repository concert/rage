#pragma once
#include <stdbool.h>

typedef struct bin_sem bin_sem;

bin_sem * bin_sem_new(bool available);
void bin_sem_free(bin_sem * s);

void bin_sem_wait(bin_sem * s);
void bin_sem_post(bin_sem * s);
