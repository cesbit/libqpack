/*
 * qpack.c
 *
 *  Created on: Mar 22, 2017
 *      Author: joente
 */


#include <qpack.h>
#include <stdlib.h>

#define QP__INITIAL_NEST_SIZE 2

#define QP_RESIZE(LEN)                                                  \
if (packer->len + LEN > packer->buffer_size)                            \
{                                                                       \
    packer->buffer_size =                                               \
            ((packer->len + LEN) / packer->alloc_size + 1)              \
            * packer->alloc_size;                                       \
    char * tmp = (char *) realloc(packer->buffer, packer->buffer_size); \
    if (tmp == NULL)                                                    \
    {                                                                   \
        ERR_ALLOC                                                       \
        packer->buffer_size = packer->len;                              \
        return -1;                                                      \
    }                                                                   \
    packer->buffer = tmp;                                               \
}



qp_packer_t * qp_packer_create(size_t alloc_size)
{
    qp_packer_t * packer;
    packer = (qp_packer_t *) malloc(
            sizeof(qp_packer_t) +
            QP__INITIAL_NEST_SIZE * sizeof(struct qp__nest_s));
    if (packer != NULL)
    {
        packer->alloc_size = alloc_size;
        packer->buffer_size = alloc_size;
        packer->len = 0;
        packer->depth = 0;
        packer->nest_sz = QP__INITIAL_NEST_SIZE;
        packer->buffer = (unsigned char *) malloc(
                sizeof(unsigned char) * alloc_size);

        if (packer->buffer == NULL)
        {
            free(packer);
            packer = NULL;
        }
    }
    return packer;
}

void qp_packer_destroy(qp_packer_t * packer)
{
    free(packer->buffer);
    free(packer);
}
