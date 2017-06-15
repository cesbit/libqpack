/*
 * main.c
 *
 *  Created on: Mar 22, 2017
 *      Author: joente
 */

#include <qpack.h>
#include <stdio.h>
#include <string.h>

int main(void)
{
    qp_packer_t * packer;
    qp_unpacker_t unpacker;
    qp_res_t * res = NULL;
    int rc;

    puts("Start QPack library test...");

    packer = qp_packer_create(512);
    qp_add_map(&packer);
    qp_add_raw(packer, "Hi Qpack", strlen("Hi Qpack"));
    qp_add_array(&packer);
    qp_add_array(&packer);
    qp_add_array(&packer);
    qp_add_int64(packer, 9);
    qp_add_int64(packer, -79);
    qp_add_int64(packer, -1);
    qp_add_int64(packer, 123456789);
    qp_add_double(packer, 123.456);
    qp_close_array(packer);
    qp_close_array(packer);
    qp_close_array(packer);
    qp_close_map(packer);
    qp_packer_print(packer);

    qp_unpacker_init(&unpacker, packer->buffer, packer->len);

    res = qp_unpacker_res(&unpacker, &rc);

    if (rc == 0)
    {
        qp_res_fprint(res, stdout);
        qp_res_destroy(res);
    }

    qp_packer_destroy(packer);

    puts("\n\nFinished QPack library test!");

    return 0;
}
