/*
 * main.c
 *
 *  Created on: Mar 22, 2017
 *      Author: joente
 */
#ifdef TEST_

#include <qpack.h>
#include <stdio.h>

int main(void)
{
    qp_packer_t * packer;

    puts("Start QPack library test...");

    packer = qp_packer_create(QP_SUGGESTED_SIZE);
    qp_packer_destroy(packer);

    puts("Finished QPack library test!");

    return 0;
}

#endif /* TEST_ */
