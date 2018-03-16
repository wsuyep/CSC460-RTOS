//
// Created by Willy on 2018-03-14.
//
#include "kernel.h"
#include "util.c"

#ifndef CSC460_UTIL_H
#define CSC460_UTIL_H

void enqueue(struct Queue *queue, struct ProcessDescriptor *p);

static PD *dequeue(struct Queue *queue);

static void RemoveQ(struct Queue *queue, struct ProcessDescriptor *p);

static BOOL InQueue(struct Queue *queue, struct ProcessDescriptor *p);

static void InitQueue(struct Queue *queue);

#endif //CSC460_UTIL_H
