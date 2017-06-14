/*
 * qpack.h
 *
 *  Created on: Mar 22, 2017
 *      Author: Jeroen van der Heijden <jeroen@transceptor.technology>
 */

#ifndef QPACK_H_
#define QPACK_H_

#define QP_VERSION "1.0.0"

#include <inttypes.h>
#include <stddef.h>

typedef union qp_via_u qp_via_t;
typedef union qp_res_u qp_res_via_t;

typedef struct qp_obj_s qp_obj_t;
typedef struct qp_packer_s qp_packer_t;
typedef struct qp_unpacker_s qp_unpacker_t;
typedef struct qp_res_s qp_res_t;
typedef struct qp_map_s qp_map_t;
typedef struct qp_array_s qp_array_t;

typedef enum qp_res_e qp_res_tp;
typedef enum qp_err_e qp_err_t;
typedef enum qp_types_e qp_types_t;

/* private typemap */
enum
{
    QP__END,             // at the end while unpacking
    QP__ERR,             // error
    QP__RAW,             // raw string
    /*
     * Both END and RAW are never actually packed but 0 and 1 are reserved
     * for positive signed integers.
     *
     * Fixed positive integers from 0 till 63       [  0...63  ]
     *
     * Fixed negative integers from -60 till -1     [ 64...123 ]
     *
     */
    QP__HOOK=124,        // Hook is not used by SiriDB
    QP__DOUBLE_N1=125,   // ## double value -1.0
    QP__DOUBLE_0,        // ## double value 0.0
    QP__DOUBLE_1,        // ## double value 1.0
    /*
     * Fixed raw strings lengths from 0 till 99     [ 128...227 ]
     */
    QP__RAW8=228,        // ## raw string with length < 1 byte
    QP__RAW16,           // ## raw string with length < 1 byte
    QP__RAW32,           // ## raw string with length < 1 byte
    QP__RAW64,           // ## raw string with length < 1 byte
    QP__INT8,            // ## 1 byte signed integer
    QP__INT16,           // ## 2 byte signed integer
    QP__INT32,           // ## 4 byte signed integer
    QP__INT64,           // 8 bytes signed integer
    QP__DOUBLE,          // 8 bytes double
    QP__ARRAY0,          // empty array
    QP__ARRAY1,          // array with 1 item
    QP__ARRAY2,          // array with 2 items
    QP__ARRAY3,          // array with 3 items
    QP__ARRAY4,          // array with 4 items
    QP__ARRAY5,          // array with 5 items
    QP__MAP0,            // empty map
    QP__MAP1,            // map with 1 item
    QP__MAP2,            // map with 2 items
    QP__MAP3,            // map with 3 items
    QP__MAP4,            // map with 4 items
    QP__MAP5,            // map with 5 items
    QP__TRUE,            // boolean true
    QP__FALSE,           // boolean false
    QP__NULL,            // null (none, nil)
    QP__ARRAY_OPEN,      // open a new array
    QP__MAP_OPEN,        // open a new map
    QP__ARRAY_CLOSE,     // close array
    QP__MAP_CLOSE,       // close map
};

/* enums */
enum qp_err_e
{
    QP_ERR_NO_OPEN_ARRAY=-5,
    QP_ERR_NO_OPEN_MAP,
    QP_ERR_MISSING_VALUE,
    QP_ERR_CORRUPT,
    QP_ERR_ALLOC,
};

enum qp_res_e
{
    QP_RES_MAP,
    QP_RES_ARRAY,
    QP_RES_INT64,
    QP_RES_REAL,
    QP_RES_STR,
    QP_RES_BOOL,
    QP_RES_NULL,
};

enum qp_types_e
{
    QP_END=QP__END,
    QP_ERR,
    QP_RAW,
    QP_HOOK=QP__HOOK,
    QP_INT64=QP__INT64, // 8 bytes signed integer
    QP_DOUBLE,          // 8 bytes double
    QP_ARRAY0,          // empty array
    QP_ARRAY1,          // array with 1 item
    QP_ARRAY2,          // array with 2 items
    QP_ARRAY3,          // array with 3 items
    QP_ARRAY4,          // array with 4 items
    QP_ARRAY5,          // array with 5 items
    QP_MAP0,            // empty map
    QP_MAP1,            // map with 1 item
    QP_MAP2,            // map with 2 items
    QP_MAP3,            // map with 3 items
    QP_MAP4,            // map with 4 items
    QP_MAP5,            // map with 5 items
    QP_TRUE,            // boolean true
    QP_FALSE,           // boolean false
    QP_NULL,            // null (none, nil)
    QP_ARRAY_OPEN,      // open a new array
    QP_MAP_OPEN,        // open a new map
    QP_ARRAY_CLOSE,     // close array
    QP_MAP_CLOSE,       // close map
};

union qp_via_u
{
    int64_t int64;
    double real;
    const char * raw;
};

struct qp_obj_s
{
    uint8_t tp;
    size_t len;
    qp_via_t via;
};

struct qp_map_s
{
    size_t n;
    qp_res_t * keys;
    qp_res_t * values;
};

struct qp_array_s
{
    size_t n;
    qp_res_t * values;
};

union qp_res_u
{
    qp_map_t * map;
    qp_array_t * array;
    char * str;
    int64_t int64;
    double real;
    int bool;
    void * null;
};

struct qp_res_s
{
    qp_res_tp tp;
    qp_res_via_t via;
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
    size_t pos;
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

struct qp_unpacker_s
{
    const unsigned char * start;
    const unsigned char * pt;
    const unsigned char * end;
};

/* create and destroy functions */
qp_packer_t * qp_packer_create(size_t alloc_size);
void qp_packer_destroy(qp_packer_t * packer);

/* add to packer functions */
int qp_add_raw(qp_packer_t * packer, const char * raw, size_t len);
int qp_add_int64(qp_packer_t * packer, int64_t i);
int qp_add_double(qp_packer_t * packer, double d);
int qp_add_true(qp_packer_t * packer);
int qp_add_false(qp_packer_t * packer);
int qp_add_null(qp_packer_t * packer);
int qp_add_array(qp_packer_t ** packaddr);
int qp_add_map(qp_packer_t ** packaddr);

/* close array/map functions */
int qp_close_array(qp_packer_t * packer);
int qp_close_map(qp_packer_t * packer);

/* initialize unpacker */
void qp_unpacker_init(
        qp_unpacker_t * unpacker,
        const unsigned char * pt,
        size_t len);

/* step over an unpacker */
qp_types_t qp_next(qp_unpacker_t * unpacker, qp_obj_t * qp_obj);

/* unpack all */
qp_res_t * qp_unpacker_res(qp_unpacker_t * unpacker, int * rc);
void qp_res_destroy(qp_res_t * res);

/* test functions for qp_obj_t */
extern int qp_is_array(qp_types_t tp);
extern int qp_is_map(qp_types_t tp);
extern int qp_is_close(qp_types_t tp);
extern int qp_is_int(qp_types_t tp);
extern int qp_is_double(qp_types_t tp);
extern int qp_is_bool(qp_types_t tp);
extern int qp_is_null(qp_types_t tp);
extern int qp_is_raw(qp_types_t tp);
extern int qp_is_raw_term(qp_obj_t * qp_obj);
extern int qp_is_raw_equal(qp_obj_t * obj, const char * str);

/* print */
void qp__print(const unsigned char * data, size_t len);
#define qp_packer_print(packer) qp__print(packer->buffer, packer->len)
#define qp_unpacker_print(unpacker) \
    qp__print(unpacker->start, unpacker->end - unpacker->start)

/* misc functions */
const char * qp_strerror(int err_code);
const char * qp_version(void);

#endif /* QPACK_H_ */
