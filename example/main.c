/*
 * main.c - Example used in the README.md
 *
 *  Created on: Jun 14, 2017
 *      Author: Jeroen van der Heijden <jeroen@transceptor.technology>
 */
#include <qpack.h>
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <inttypes.h>

const char * q = "What is the answer to life the universe and everything?";
const int a = 42;

void print_qa(const unsigned char * data, size_t len)
{
    qp_unpacker_t unpacker;
    qp_res_t * res;
    int rc;

    /* Initialize the unpacker */
    qp_unpacker_init(&unpacker, data, len);

    /* We can get the data from the unpacker in two ways. One is to step over
     * the data using qp_next() and the other is to unpack all to a qp_res_t
     * object. The last option is probably easier and is what we will use in
     * this example but note that it consumes more memory and is potentially
     * slower compared to qp_next() for some use cases. An advantage might be
     * that the original data/unpacker is no longer required by qp_res_t.
     *
     * One more important difference is that qp_res_t contains null terminated
     * copies for raw data which mean that this method cannot be used in case
     * raw data contains null characters by themself. */

    res = qp_unpacker_res(&unpacker, &rc);
    if (rc) {
        fprintf(stderr, "error: %s\n", qp_strerror(rc));
    } else {
        /* Usually you should check your response not with assertions */
        assert (res->tp == QP_RES_MAP);
        assert (res->via.map->n == 1); /* one key/value pair */
        assert (res->via.map->keys[0].tp == QP_RES_STR);   /* key */
        assert (res->via.map->values[0].tp == QP_RES_INT64); /* val */

        printf(
            "Unpacked:\n"
            "  Question: %s\n"
            "  Answer  : %" PRId64 "\n",
            res->via.map->keys[0].via.str,
            res->via.map->values[0].via.int64);

        /* cleanup res */
        qp_res_destroy(res);
    }
}



int main(void)
{
    qp_packer_t * packer = qp_packer_create(512);
    if (packer == NULL) {
        abort();
    }

    /* Open a map. */
    qp_add_map(&packer);

    /* Add a key. We will add the question. Note that the question will be
     * packed without the teminator (null) character. */
    qp_add_raw(packer, q, strlen(q)); /* add question as key */

    /* Add a value. QPack only supports signed intergers. */
    qp_add_int64(packer, (int64_t) a);

    /* Close the map. Note that in a map an even number of items must be placed
     * which represent key/value pairs. */
    qp_close_map(packer);

    /* Print the packed data */
    qp_packer_print(packer);

    /* Unpack example */
    print_qa(packer->buffer, packer->len);

    /* cleanup the packer */
    qp_packer_destroy(packer);

    return 0;
}
