/*
 * qpack.h
 */
#ifndef QPACK_H_
#define QPACK_H_

/* Version numbers are configured with CMake. */
#define QP_VERSION_MAJOR 0
#define QP_VERSION_MINOR 11
#define QP_VERSION_PATCH 3

#define QP_VERSION "0.11.3-alpha-0"

enum
{
    QP_UNPACK_FLAG_RAW      =1,
    QP_UNPACK_FLAG_KEY_STR  =2,
};

#include <inttypes.h>
#include <stddef.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>

typedef union qp_via_u qp_via_t;
typedef union qp_res_u qp_res_via_t;

typedef struct qp_obj_s qp_obj_t;
typedef struct qp_packer_s qp_packer_t;
typedef struct qp_unpacker_s qp_unpacker_t;
typedef struct qp_res_s qp_res_t;
typedef struct qp_map_s qp_map_t;
typedef struct qp_array_s qp_array_t;
typedef struct qp_raw_s qp_raw_t;

/* private typemap */
enum
{
    QP__END,             /* at the end while unpacking */
    QP__ERR,             /* error */
    QP__RAW,             /* raw string */
    /*
     * Both END and RAW are never actually packed but 0 and 1 are reserved
     * for positive signed integers.
     *
     * Fixed positive integers from 0 till 63       [  0...63  ]
     *
     * Fixed negative integers from -60 till -1     [ 64...123 ]
     *
     */
    QP__HOOK=124,        /* Hook is not used by SiriDB */
    QP__DOUBLE_N1=125,   /* ## double value -1.0 */
    QP__DOUBLE_0,        /* ## double value 0.0 */
    QP__DOUBLE_1,        /* ## double value 1.0 */
    /*
     * Fixed raw strings lengths from 0 till 99     [ 128...227 ]
     */
    QP__RAW8=228,        /* ## raw string with length < 1 byte */
    QP__RAW16,           /* ## raw string with length < 1 byte */
    QP__RAW32,           /* ## raw string with length < 1 byte */
    QP__RAW64,           /* ## raw string with length < 1 byte */
    QP__INT8,            /* ## 1 byte signed integer */
    QP__INT16,           /* ## 2 byte signed integer */
    QP__INT32,           /* ## 4 byte signed integer */
    QP__INT64,           /* 8 bytes signed integer */
    QP__DOUBLE,          /* 8 bytes double */
    QP__ARRAY0,          /* empty array */
    QP__ARRAY1,          /* array with 1 item */
    QP__ARRAY2,          /* array with 2 items */
    QP__ARRAY3,          /* array with 3 items */
    QP__ARRAY4,          /* array with 4 items */
    QP__ARRAY5,          /* array with 5 items */
    QP__MAP0,            /* empty map */
    QP__MAP1,            /* map with 1 item */
    QP__MAP2,            /* map with 2 items */
    QP__MAP3,            /* map with 3 items */
    QP__MAP4,            /* map with 4 items */
    QP__MAP5,            /* map with 5 items */
    QP__TRUE,            /* boolean true */
    QP__FALSE,           /* boolean false */
    QP__NULL,            /* null (none, nil) */
    QP__ARRAY_OPEN,      /* open a new array */
    QP__MAP_OPEN,        /* open a new map */
    QP__ARRAY_CLOSE,     /* close array */
    QP__MAP_CLOSE,       /* close map */
};

/* default initial nest size */
#define QP__INITIAL_NEST_SIZE 8

/* enums */
typedef enum qp_err_e
{
    QP_ERR_KEY_STR=-7,
    QP_ERR_WRITE_STREAM,
    QP_ERR_NO_OPEN_ARRAY,
    QP_ERR_NO_OPEN_MAP,
    QP_ERR_MISSING_VALUE,
    QP_ERR_CORRUPT,
    QP_ERR_ALLOC,
} qp_err_t;

typedef enum qp_res_e
{
    QP_RES_MAP,
    QP_RES_ARRAY,
    QP_RES_INT64,
    QP_RES_REAL,
    QP_RES_STR,
    QP_RES_RAW,
    QP_RES_BOOL,
    QP_RES_NULL,
} qp_res_tp;

typedef enum qp_types_e
{
    QP_END=QP__END,
    QP_ERR,
    QP_RAW,
    QP_HOOK=QP__HOOK,
    QP_INT64=QP__INT64, /* 8 bytes signed integer */
    QP_DOUBLE,          /* 8 bytes double */
    QP_ARRAY0,          /* empty array */
    QP_ARRAY1,          /* array with 1 item */
    QP_ARRAY2,          /* array with 2 items */
    QP_ARRAY3,          /* array with 3 items */
    QP_ARRAY4,          /* array with 4 items */
    QP_ARRAY5,          /* array with 5 items */
    QP_MAP0,            /* empty map */
    QP_MAP1,            /* map with 1 item */
    QP_MAP2,            /* map with 2 items */
    QP_MAP3,            /* map with 3 items */
    QP_MAP4,            /* map with 4 items */
    QP_MAP5,            /* map with 5 items */
    QP_TRUE,            /* boolean true */
    QP_FALSE,           /* boolean false */
    QP_NULL,            /* null (none, nil) */
    QP_ARRAY_OPEN,      /* open a new array */
    QP_MAP_OPEN,        /* open a new map */
    QP_ARRAY_CLOSE,     /* close array */
    QP_MAP_CLOSE,       /* close map */
} qp_types_t;

enum qp__nest_types
{
    NEST_ARRAY,
    NEST_MAP
};

union qp_via_u
{
    int64_t int64;
    double real;
    const unsigned char * raw;
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

struct qp_raw_s
{
    size_t n;
    unsigned char data[];
};

union qp_res_u
{
    qp_map_t * map;
    qp_array_t * array;
    qp_raw_t * raw;
    char * str;
    int64_t int64;
    double real;
    _Bool boolean;
    void * null;
};

struct qp_res_s
{
    qp_res_tp tp;
    qp_res_via_t via;
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
    unsigned char flags;
};

#ifdef __cplusplus
extern "C" {
#endif

/* create and destroy functions */
static inline qp_packer_t * qp_packer_create(size_t alloc_size);
qp_packer_t * qp_packer_create2(size_t alloc_size, size_t init_nest_size);
void qp_packer_destroy(qp_packer_t * packer);

/* add to packer functions */
int qp_add_raw(qp_packer_t * packer, const unsigned char * raw, size_t len);
int qp_add_int(qp_packer_t * packer, int64_t i);
int qp_add_double(qp_packer_t * packer, double d);
int qp_add_bool(qp_packer_t * packer, _Bool b);
int qp_add_true(qp_packer_t * packer);
int qp_add_false(qp_packer_t * packer);
int qp_add_null(qp_packer_t * packer);
int qp_add_array(qp_packer_t ** packaddr);
int qp_add_map(qp_packer_t ** packaddr);
/* qp_add_raw_from_fmt() will strip off the terminiator char */
int qp_add_raw_from_fmt(qp_packer_t * packer, const char * fmt, ...);
/* qp_add_raw_from_str() will strip off the terminiator char */
static inline int qp_add_raw_from_str(qp_packer_t * packer, const char * str);
/* raw must be formatted qpack data which fits at the insert point */
int qp_add_qp(qp_packer_t * packer, const unsigned char * raw, size_t len);
/* alias for backward compatibility */
static inline int qp_add_int64(qp_packer_t * packer, int64_t i);


/* Add to file-packer functions */
int qp_fadd_raw(FILE * f, const unsigned char * raw, size_t len);
int qp_fadd_int(FILE * f, int64_t i);
static inline int qp_fadd_double(FILE * f, double d);
static inline int qp_fadd_bool(FILE * f, _Bool b);
static inline int qp_fadd_true(FILE * f);
static inline int qp_fadd_false(FILE * f);
static inline int qp_fadd_null(FILE * f);
static inline int qp_fadd_type(FILE * f, qp_types_t tp);
/* qp_fadd_raw_from_str() will strip off the terminiator char */
static inline int qp_fadd_raw_from_str(FILE * f, const char * str);
static inline int qp_fadd_int64(FILE * f, int64_t i);


/* close array/map functions */
int qp_close_array(qp_packer_t * packer);
int qp_close_map(qp_packer_t * packer);

/* initialize unpacker */
void qp_unpacker_init2(
        qp_unpacker_t * unpacker,
        const unsigned char * pt,
        const size_t len,
        const unsigned char flags);
static inline void qp_unpacker_init(
        qp_unpacker_t * unpacker,
        const unsigned char * pt,
        size_t len);

/* step over an unpacker */
qp_types_t qp_next(qp_unpacker_t * unpacker, qp_obj_t * qp_obj);

/* if the current position in the unpacker is an array or map, then the
 * total collecion is skipped and the position is set at the end so a call
 * to `qp_next` will return the `obect` after the collection. If the
 * position is at anything else, this function does nothing.
 */
void qp_skip(qp_unpacker_t * unpacker);

/* step over a file */
qp_types_t qp_fnext(FILE * f, qp_res_t * qp_res);

/* unpack all */
qp_res_t * qp_unpacker_res(qp_unpacker_t * unpacker, int * rc);
void qp_res_destroy(qp_res_t * res);
void qp_res_clear(qp_res_t * res);
int qp_res_fprint(qp_res_t * res, FILE * stream);

/* test functions for tp and qp_obj_t */
static inline int qp_is_array(qp_types_t tp);
static inline int qp_is_map(qp_types_t tp);
static inline int qp_is_close(qp_types_t tp);
static inline int qp_is_int(qp_types_t tp);
static inline int qp_is_double(qp_types_t tp);
static inline int qp_is_bool(qp_types_t tp);
static inline int qp_is_true(qp_types_t tp);
static inline int qp_is_false(qp_types_t tp);
static inline int qp_is_null(qp_types_t tp);
static inline int qp_is_raw(qp_types_t tp);
static inline int qp_is_raw_term(qp_obj_t * obj);
static inline int qp_is_raw_equal(qp_obj_t * obj, const char * raw, size_t n);
/* returns true if equal to str without the terminator char */
static inline int qp_is_raw_equal_str(qp_obj_t * obj, const char * str);
/* DEPRECATED: use qp_is_raw_equal_str */
static inline int qp_raw_is_equal(qp_obj_t * obj, const char * str);

/* print */
void qp_print(const unsigned char * data, size_t len);
void qp_fprint(FILE * stream, const unsigned char * data, size_t len);
char * qp_sprint(const unsigned char * data, size_t len);
static inline void qp_packer_print(qp_packer_t * packer);
static inline void qp_packer_fprint(FILE * f, qp_packer_t * packer);
static inline char * qp_packer_sprint(qp_packer_t * packer);
static void qp_unpacker_print(qp_unpacker_t * unpacker);
static void qp_unpacker_fprint(FILE * f, qp_unpacker_t * unpacker);
static inline char * qp_unpacker_sprint(qp_unpacker_t * unpacker);

/* misc functions */
const char * qp_strerror(int err_code);
const char * qp_version(void);

/*
 * definitions of static inline functions
 */
static inline int qp_add_int64(qp_packer_t * packer, int64_t i)
{
    return qp_add_int(packer, i);
}
static inline int qp_add_raw_from_str(qp_packer_t * packer, const char * str)
{
    return qp_add_raw(packer, (const unsigned char *) str, strlen(str));
}
static inline int qp_fadd_true(FILE * f)
{
    return -(fputc(QP__TRUE, f) == EOF);
}
static inline int qp_fadd_false(FILE * f)
{
    return -(fputc(QP__FALSE, f) == EOF);
}
static inline int qp_fadd_null(FILE * f)
{
    return -(fputc(QP__NULL, f) == EOF);
}
static inline int qp_fadd_type(FILE * f, qp_types_t tp)
{
    return -(fputc(tp, f) == EOF);
}
static inline int qp_fadd_raw_from_str(FILE * f, const char * str)
{
    return qp_fadd_raw(f, (const unsigned char *) str, strlen(str));
}
static inline int qp_fadd_int64(FILE * f, int64_t i)
{
    return qp_fadd_int(f, i);
}
static inline int qp_fadd_double(FILE * f, double d)
{
    return (d == 0.0) ? -(fputc(QP__DOUBLE_0, f) == EOF) :
         (d == 1.0) ? -(fputc(QP__DOUBLE_1, f) == EOF) :
         (d == -1.0) ? -(fputc(QP__DOUBLE_N1, f) == EOF) :
         -(fputc(QP__DOUBLE, f) == EOF ||
            fwrite(&d, sizeof(double), 1, f) != 1);
}
static inline int qp_fadd_bool(FILE * f, _Bool b)
{
    return -(fputc(b ? QP__TRUE: QP__FALSE, f) == EOF);
}
static inline void qp_packer_print(qp_packer_t * packer)
{
    qp_print(packer->buffer, packer->len);
}
static inline void qp_packer_fprint(FILE * f, qp_packer_t * packer)
{
    qp_fprint(f, packer->buffer, packer->len);
}
static inline char * qp_packer_sprint(qp_packer_t * packer)
{
    return qp_sprint(packer->buffer, packer->len);
}
__attribute__((unused))
static void qp_unpacker_print(qp_unpacker_t * unpacker)
{
    qp_print(unpacker->start, unpacker->end - unpacker->start);
}
__attribute__((unused))
static void qp_unpacker_fprint(FILE * f, qp_unpacker_t * unpacker)
{
    qp_fprint(f, unpacker->start, unpacker->end - unpacker->start);
}
static inline char * qp_unpacker_sprint(qp_unpacker_t * unpacker)
{
    return qp_sprint(unpacker->start, unpacker->end - unpacker->start);
}
static inline qp_packer_t * qp_packer_create(size_t alloc_size)
{
    return qp_packer_create2(alloc_size, QP__INITIAL_NEST_SIZE);
}
static inline void qp_unpacker_init(
        qp_unpacker_t * unpacker,
        const unsigned char * pt,
        size_t len)
{
    return qp_unpacker_init2(unpacker, pt, len, 0);
};
static inline int qp_is_array(qp_types_t tp)
{
    return tp == QP_ARRAY_OPEN || (tp >= QP_ARRAY0 && tp <= QP_ARRAY5);
}
static inline int qp_is_map(qp_types_t tp)
{
    return tp == QP_MAP_OPEN || (tp >= QP_MAP0 && tp <= QP_MAP5);
}
static inline int qp_is_close(qp_types_t tp)
{
    return tp == QP_ARRAY_CLOSE || tp == QP_MAP_CLOSE || tp == QP_END;
}
static inline int qp_is_int(qp_types_t tp)
{
    return tp == QP_INT64;
}
static inline int qp_is_double(qp_types_t tp)
{
    return tp == QP_DOUBLE;
}
static inline int qp_is_bool(qp_types_t tp)
{
    return tp == QP_TRUE || tp == QP_FALSE;
}
static inline int qp_is_true(qp_types_t tp)
{
    return tp == QP_TRUE;
}
static inline int qp_is_false(qp_types_t tp)
{
    return tp == QP_FALSE;
}
static inline int qp_is_null(qp_types_t tp)
{
    return tp == QP_NULL;
}
static inline int qp_is_raw(qp_types_t tp)
{
    return tp == QP_RAW;
}
static inline int qp_is_raw_term(qp_obj_t * obj)
{
    return (obj->tp == QP_RAW &&
            obj->len &&
            obj->via.raw[obj->len - 1] == '\0');
}
static inline int qp_is_raw_equal(qp_obj_t * obj, const char * raw, size_t n)
{
    return (obj->tp == QP_RAW &&
            n == obj->len &&
            memcmp(obj->via.raw, raw, obj->len) == 0);
}
static inline int qp_is_raw_equal_str(qp_obj_t * obj, const char * str)
{
    return qp_is_raw_equal(obj, str, strlen(str));
}
/* DEPRECATED: use qp_is_raw_equal_str */
static inline int qp_raw_is_equal(qp_obj_t * obj, const char * str)
{
    return qp_is_raw_equal_str(obj, str);
}

#ifdef __cplusplus
}
#endif

#endif /* QPACK_H_ */
