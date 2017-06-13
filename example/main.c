#include <qpack.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>

const char * q = "What is the answer to life the universe and everything?";
const int a = 42;

int main(void)
{
    qp_packer_t * packer = qp_packer_create(QP_SUGGESTED_SIZE);
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

    /* cleanup the packer */
    qp_packer_destroy(packer);

    return 0;
}
