/*
 * qpack.h
 *
 *  Created on: Mar 22, 2017
 *      Author: joente
 */

#ifndef QPACK_H_
#define QPACK_H_

#include <inttypes.h>
#include <stddef.h>

#define QP_SUGGESTED_SIZE 65536

typedef union qp_via_u qp_via_t;
typedef struct qp_obj_s qp_obj_t;
typedef struct qp_packer_s qp_packer_t;

union qp_via_u
{
    int64_t int64;
    double real;
    char * raw;
};

struct qp_obj_s
{
    uint8_t tp;
    size_t len;
    qp_via_t via;
};

enum qp__nest_types
{
    NEST_ARRAY,
    NEST_MAP
};

struct qp__nest_s
{
    enum qp__nest_types tp;
    size_t n;
    char * pos;
};

struct qp_packer_s
{
    unsigned char * buffer;
    size_t len;
    size_t buffer_size;
    size_t alloc_size;
    size_t depth;
    size_t nest_sz;
    struct qp__nest_s nesting[];
};

qp_packer_t * qp_packer_create(size_t alloc_size);
void qp_packer_destroy(qp_packer_t * packer);


#endif /* QPACK_H_ */
