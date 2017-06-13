# QPack-C
Fast and efficient data serializer for the C program language.

---------------------------------------
  * [Installation](#installation)
  * [Usage](#usage)

---------------------------------------

## Installation
Install debug or release version, in this example we will install the release version.
```
$ cd Release
```

Compile qpack
```
$ make all
```

Install qpack
```
$ sudo make install
```

> Note: run `sudo make uninstall` for removal.

## Usage
The following data structure will be packed and unpacked to illustrate how
qpack can be used:

```json
{
    "What is the answer to life the universe and everything?": 42
}
```
### Packer
This example shows how to use the packer.
```c
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
```

### Unpacker
```c
void print_qa(const unsigned char * data, size_t len)
{
    qp_unpacker_t unpacker;
    qp_res_t * res;
    inr rc;

    /* Initialize the unpacker */
    qp_unpacker_init(&unpacker, data, len);

    /* We can get the data from the unpacker in two ways. One is to step over
     * the data using qp_next() and the other is to unpack all to a qp_res_t
     * object. The last option is probably easier and is what we will use in
     * this example but note that it consumes more memory and is potentially
     * slower compared to qp_next() for some use cases. */

    res = qp_unpacker_res(&unpacker, &rc);
    if (rc) {
        fprintf(stderr, "error: %s\n", qp_strrerror(rc));
        abort();
    }

    /* cleanup res */
    qp_res_destroy(res);
}

/* Call the function from main (see packer example for full code) */
int main(void)
{
    // ... see packer example for full code

    /* Call print_qa() before destroying the packer using member property
     * qp_packer_t.buffer which contains the packer data and qp_packer_t.len
     * for the size of the data */
    print_qa(packer->buffer, packer->len);

    /* cleanup the packer */
    qp_packer_destroy(packer);

    return 0;
}
```
