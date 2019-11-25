#ifndef KLBVFS_H
#define KLBVFS_H
#include "sqlite3.h"
#include "stdio.h"
#include "stdlib.h"
#include "unistd.h"
#include "limits.h"
#include "string.h"
#include "assert.h"

int klbvfs_register();
const int SEED_1 = 214013;
const int SEED_2 = 2531011;

#endif
