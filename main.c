/*
 * main.c
 *
 *  Created on: Mar 22, 2017
 *      Author: joente
 */


#include <qpack.h>

int main(void)
{
    qp_packer_t * packer;

    packer = qp_packer_create(QP_SUGGESTED_SIZE);
    puts("This is a shared library test...");
    foo();
    return 0;
}
