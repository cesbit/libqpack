# QPack-C
Fast and efficient data serializer for the C program language.

---------------------------------------
  * [Installation](#installation)
  * [Usage](#usage)
    * [Packer](#packer)
    * [Unpacker](#unpacker)
  * [API](#api)
    * [qp_packer_t](#qp_packer_t)
    * [qp_unpacker_t](#qp_unpacker_t)
    * [qp_res_t](#qp_res_t)
    * [qp_map_t](#qp_map_t)
    * [qp_array_t](#qp_array_t)
    * [Miscellaneous functions](#miscellaneous-functions)

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
First we start to pack the data.
```c
#include <qpack.h>
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <inttypes.h>

const char * q = "What is the answer to life the universe and everything?";
const int a = 42;

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
    qp_add_raw(packer, q, strlen(q));

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
Now we create some code to unpack the data.
```c
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
     * slower compared to qp_next() for some use cases. One more important
     * difference is that qp_res_t contains null terminated copies for raw data
     * which mean that this function cannot be used in case raw data contains
     * null characters by themself. */

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

## API

### `qp_packer_t`
Object which is used to pack data.

*Public members*
- `unsigned char * qp_packer_t.buffer`: contains the packed data (readonly)
- `size_t qp_packer_t.len`: the length of the data (readonly)

#### `qp_packer_t * qp_packer_create(size_t alloc_size)`
Create and return a new packer instance. Argument `alloc_size` should be at
least greather than 0 since this value is used to allocate memory. When more
memory is required while packing data the size will grow in steps of
`alloc_size`.

#### `void qp_packer_destroy(qp_packer_t * packer)`
Cleaunup packer instance.

#### `int qp_add_raw(qp_packer_t * packer, const char * raw, size_t len)`
Add raw data to the packer.

Returns 0 if successful or `QP_ERR_ALLOC` in case more memory is required which
cannot be allocated.

Example:
```c
int rc;
const char * s = "some string";
/* We use strlen(s) so the string will be packed without the terminator (null)
 * character. */
if ((rc = qp_add_raw(packer, s, strlen(s)))) {
    fprintf(stderr, "error: %s\n", qp_strerror(rc));
}
```

#### `int qp_add_int64(qp_packer_t * packer, int64_t i)`
Add an integer value to the packer. Note that only signed integers are
supported by QPack so unsigned integers should be casted. This function
accepts only `int64_t` but tries to pack the value as small as possible.

Returns 0 if successful or `QP_ERR_ALLOC` in case more memory is required which
cannot be allocated.

#### `int qp_add_double(qp_packer_t * packer, double d)`
Add a double value to the packer.

Returns 0 if successful or `QP_ERR_ALLOC` in case more memory is required which
cannot be allocated.

#### `int qp_add_true(qp_packer_t * packer)`
Add boolean value `TRUE` to the packer.

Returns 0 if successful or `QP_ERR_ALLOC` in case more memory is required which
cannot be allocated.

#### `int qp_add_false(qp_packer_t * packer)`
Add boolean value `FALSE` to the packer.

Returns 0 if successful or `QP_ERR_ALLOC` in case more memory is required which
cannot be allocated.

#### `int qp_add_null(qp_packer_t * packer)`
Add `NULL` to the packer.

Returns 0 if successful or `QP_ERR_ALLOC` in case more memory is required which
cannot be allocated.

#### `int qp_add_array(qp_packer_t ** packaddr)`
Add an `ARRAY` to the packer. Do not forget to close the array using
`qp_close_array()` although closing is not required by qpack in case
no other values need to be added after the array.

>Note: this function needs a pointer-pointer to a packer since the function
>might re-allocate memory for the packer.

Returns 0 if successful or `QP_ERR_ALLOC` in case more memory is required which
cannot be allocated.

#### `int qp_add_map(qp_packer_t ** packaddr)`
Add an `MAP` to the packer for storing key/value pairs. Do not forget to close
the map using `qp_close_map()` although closing is not required by qpack in case
no other values need to be added after the map. Always add an even number of
items which represent key/value pairs.

>Note: this function needs a pointer-pointer to a packer since the function
>might re-allocate memory for the packer.

Returns 0 if successful or `QP_ERR_ALLOC` in case more memory is required which
cannot be allocated.

#### `int qp_close_array(qp_packer_t * packer)`
Close an array.

Returns 0 if successful or `QP_ERR_ALLOC` in case more memory is required which
cannot be allocated. `QP_ERR_NO_OPEN_ARRAY` can be returned in case no *open*
array was found.

#### `int qp_close_map(qp_packer_t * packer)`
Close a map.

Returns 0 if successful or `QP_ERR_ALLOC` in case more memory is required which
cannot be allocated. `QP_ERR_NO_OPEN_MAP` can be returned in case no *open*
map was found or `QP_ERR_MISSING_VALUE` is returned (and the map will *not* be
closed) if the map contains a key without value.

#### `void qp_packer_print(qp_packer_t * packer)`
Macro function for printing packer content to stdout.

### `qp_unpacker_t`
Object which is used to unpack data.

*Public members*
- `const unsigned char * qp_unpacker_t.pt`:
  pointer to the current position in the data
- `const unsigned char * qp_unpacker_t.start`:
  pointer to data start (readonly)
- `const unsigned char * qp_unpacker_t.end`:
  pointer to data end (readonly)

#### `void qp_unpacker_init(qp_unpacker_t * unpacker, const unsigned char * pt, size_t len)`
Initialize an `qp_unpacker_t` instance. No additional clean is required.

#### `qp_types_t qp_next(qp_unpacker_t * unpacker, qp_obj_t * qp_obj)`
Used walk over an unpacker instance step-by-step. After calling the function,
`qp_obj` points to the current item in the unpacker. `qp_obj.tp` is always equal
to the return value of this function but note that `qp_obj` is allowed to be
`NULL` in which case you only have the return value. Because a `qp_obj_t`
instance *can* point to the raw data in the unpacker, the unpacker/data must
not be destroyed as long as the `qp_obj_t` is in use.

Example (this code can replace the `print_qa()` code in
the [unpacker example](#unpacker):
```c
qp_obj_t question, answer;

assert (qp_is_map(qp_next(&unpacker, NULL)));
assert (qp_is_raw(qp_next(&unpacker, &question)));
assert (qp_is_int(qp_next(&unpacker, &answer)));

printf(
    "Unpacked:\n"
    "  Question: %.*s\n"
    "  Answer  : %" PRId64 "\n",
    (int) question.len, question.via.raw,
    answer.via.int64);
```
#### `qp_res_t * qp_unpacker_res(qp_unpacker_t * unpacker, int * rc)`
Returns a pointer to a [qp_res_t](#qp_res_t) instance or `NULL` in case of an
error. When successful the original data (unpacker) is no longer required and
can be destroyed.

Argument `rc` is 0 when successful or `QP_ERR_ALLOC` in case required memory
cannot be allocated. The value of `rc` is `QP_ERR_CORRUPT` when the unpacker
contains data which cannot be unpacked. `rc` is not required and is allowed to
be `NULL`.

>Note: Each raw value will be copied to a NULL terminated string which means
>that this function cannot be used in case your raw data contains NULL
>characters. In the latter case you need to use `qp_next()` to unpack the data.

#### `void qp_unpacker_print(qp_unpacker_t * unpacker)`
Macro function for printing unpacker content to stdout.

### `qp_res_t`
Object which contains unpacked data. An instance of `qp_res_t` is created with
the [qp_unpacker_t](#qp_unpacker_t) function `qp_unpacker_res()`.

*Public members*
- `qp_res_tp qp_res_t.tp`: Type of the result (readonly)
  - `QP_RES_MAP`
  - `QP_RES_ARRAY`
  - `QP_RES_INT64`
  - `QP_RES_REAL`
  - `QP_RES_STR`
  - `QP_RES_BOOL`
  - `QP_RES_NULL`
- `qp_res_via_t qp_res_t.via`: Union containing the actual data (readonly)
  - `qp_map_t * map`
  - `qp_array_t * array`
  - `char * str` (null terminated string)
  - `int64_t int64`
  - `double real`
  - `int bool` (0=FALSE or 1=TRUE)
  - `void * null` (set to the actual `NULL` pointer)

#### `void qp_res_destroy(qp_res_t * res)`
Cleanup result instance.

### `qp_map_t`
Object is accesible via [qp_res_t.via.map](#qp_res_t).

*Public members*
- `size_t qp_map_t.n`: Number of keys and values in the map (readonly)
- `qp_res_t * qp_map_t.keys`: Keys (readonly)
- `qp_res_t * qp_map_t.values`: Values (readonly)

### `qp_array_t`
Object is accesible via [qp_res_t.via.array](#qp_res_t).

*Public members*
- `size_t qp_map_t.n`: Number of values in the array (readonly)
- `qp_res_t * qp_map_t.values`: Values (readonly)

### `qp_obj_t`
Object is used together with the `qp_next()` function and *can* point to
data from an unpacker object.

*Public members*
- `uint8_t qp_obj_t.tp`: Type for the object (readonly)
  - `QP_END         ` *(equal to 0 and indicates end of unpacker)*
  - `QP_ERR         ` *(error while reading from unpacker)*
  - `QP_RAW         ` *(raw data)*
  - `QP_HOOK        ` *(hook for custom implementations)*
  - `QP_INT64       ` *( signed 64bit interger)*
  - `QP_DOUBLE      ` *(double value)*
  - `QP_ARRAY0      ` *(empty array)*
  - `QP_ARRAY1      ` *(array with 1 item)*
  - `QP_ARRAY2      ` *(array with 2 items)*
  - `QP_ARRAY3      ` *(array with 3 items)*
  - `QP_ARRAY4      ` *(array with 4 items)*
  - `QP_ARRAY5      ` *(array with 5 items)*
  - `QP_MAP0        ` *(empty map)*
  - `QP_MAP1        ` *(map with 1 item)*
  - `QP_MAP2        ` *(map with 2 items)*
  - `QP_MAP3        ` *(map with 3 items)*
  - `QP_MAP4        ` *(map with 4 items)*
  - `QP_MAP5        ` *(map with 5 items)*
  - `QP_TRUE        ` *(boolean true)*
  - `QP_FALSE       ` *(boolean false)*
  - `QP_NULL        ` *(null)*
  - `QP_ARRAY_OPEN  ` *(open a new array)*
  - `QP_MAP_OPEN    ` *(open a new map)*
  - `QP_ARRAY_CLOSE ` *(close array)*
  - `QP_MAP_CLOSE   ` *(close map)*
- `size_t qp_obj_t.len`: Length of data. (readonly, only used when tp=QP_RAW)
- `qp_via_t qp_obj_t.via`: Union type (readonly, only used when tp=QP_RAW,
 QP_INT64 or QP_DOUBLE)
  - `int64_t int64`
  - `double real`
  - `const char * raw`

### Miscellaneous functions
#### `const char * qp_strerror(int err_code)`
Returns the error message for a given error code.

#### `const char * qp_version(void)`
Returns the version of qpack.
