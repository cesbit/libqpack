/*
 * main.c
 *
 *  Created on: Mar 22, 2017
 *      Author: joente
 */
#ifdef TEST_

#include <qpack.h>
#include <stdio.h>
#include <string.h>

int main(void)
{
    qp_packer_t * packer;

    puts("Start QPack library test...");

    packer = qp_packer_create(QP_SUGGESTED_SIZE);
    qp_add_map(&packer);
    qp_add_raw(packer, "Hi Qpack", strlen("Hi Qpack"));
    qp_add_int64(packer, 9);
    qp_add_int64(packer, -79);
    qp_add_int64(packer, -1);
    qp_add_int64(packer, 123456789);
    qp_add_double(packer, 123.456);
    qp_close_map(packer);
    qp_packer_print(packer);
    qp_packer_destroy(packer);

    puts("Finished QPack library test!");

    return 0;
}

#endif /* TEST_ */
