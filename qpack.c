/*
 * qpack.c
 *
 *  Created on: Mar 22, 2017
 *      Author: Jeroen van der Heijden <jeroen@transceptor.technology>
 */


#include <qpack.h>
#include <assert.h>
#include <stdlib.h>

#define QP__INITIAL_NEST_SIZE 4
#define QP__INITIAL_ALLOC_SZ 8
#define QP__CHKN if (n < 0) return QP_ERR; *pt += n;
#define QP__CHKT if (tp == QP_ERR) return QP_ERR;
#define QP__MINSZ 16
#define QP__FACTOR 1.4

#define QP__RETURN_INC_C                                                \
    if (packer->depth) packer->nesting[packer->depth - 1].n++;          \
    return 0;

#define QP_RESIZE(LEN)                                                  \
if (packer->len + LEN > packer->buffer_size)                            \
{                                                                       \
    unsigned char * tmp;                                                \
    packer->buffer_size =                                               \
            ((packer->len + LEN) / packer->alloc_size + 1)              \
            * packer->alloc_size;                                       \
    tmp = (unsigned char *) realloc(                                    \
            packer->buffer,                                             \
            packer->buffer_size);                                       \
    if (tmp == NULL)                                                    \
    {                                                                   \
        packer->buffer_size = packer->len;                              \
        return QP_ERR_ALLOC;                                            \
    }                                                                   \
    packer->buffer = tmp;                                               \
}

#define QP_PLAIN_OBJ(QP__TYPE)                                          \
{                                                                       \
    QP_RESIZE(1)                                                        \
    packer->buffer[packer->len++] = QP__TYPE;                           \
    QP__RETURN_INC_C                                                    \
}

#define QP_ADD_INT8(packer, integer) \
    QP_RESIZE(2)                                                        \
    if (integer >= 0 && integer < 64)                                   \
    {                                                                   \
        packer->buffer[packer->len++] = integer;                        \
    }                                                                   \
    else if (integer >= -60 && integer < 0)                             \
    {                                                                   \
        packer->buffer[packer->len++] = 63 - integer;                   \
    }                                                                   \
    else                                                                \
    {                                                                   \
        packer->buffer[packer->len++] = QP__INT8;                       \
        packer->buffer[packer->len++] = integer;                        \
    }

#define QP_ADD_INT16(packer, integer)                                   \
    QP_RESIZE(3)                                                        \
    packer->buffer[packer->len++] = QP__INT16;                          \
    memcpy(packer->buffer + packer->len, &integer, sizeof(int16_t));    \
    packer->len += sizeof(int16_t);

#define QP_ADD_INT32(packer, integer)                                   \
    QP_RESIZE(5)                                                        \
    packer->buffer[packer->len++] = QP__INT32;                          \
    memcpy(packer->buffer + packer->len, &integer, sizeof(int32_t));    \
    packer->len += sizeof(int32_t);

#define QP_UNPACK_RAW(uintx_t)                                          \
{                                                                       \
    QP_UNPACK_CHECK_SZ(sizeof(uintx_t))                                 \
    size_t sz = (size_t) *((uintx_t *) unpacker->pt);                   \
    unpacker->pt += sizeof(uintx_t);                                    \
    QP_UNPACK_CHECK_SZ(sz)                                              \
    if (qp_obj != NULL)                                                 \
    {                                                                   \
        qp_obj->tp = QP_RAW;                                            \
        qp_obj->via.raw = (char *) unpacker->pt;                        \
        qp_obj->len = sz;                                               \
    }                                                                   \
    unpacker->pt += sz;                                                 \
    return QP_RAW;                                                      \
}

#define QP_UNPACK_INT(intx_t)                                           \
{                                                                       \
    QP_UNPACK_CHECK_SZ(sizeof(intx_t))                                  \
    int64_t val = (int64_t) *((intx_t *) unpacker->pt);                 \
    if (qp_obj != NULL)                                                 \
    {                                                                   \
        qp_obj->tp = QP_INT64;                                          \
        qp_obj->via.int64 = val;                                        \
    }                                                                   \
    unpacker->pt += sizeof(intx_t);                                     \
    return QP_INT64;                                                    \
}

#define QP_UNPACK_CHECK_SZ(size)                                        \
if (unpacker->pt + size > unpacker->end)                                \
{                                                                       \
    if  (qp_obj != NULL)                                                \
    {                                                                   \
        qp_obj->tp = QP_ERR;                                            \
    }                                                                   \
    return QP_ERR;                                                      \
}

typedef int (*QP_f)(qp_unpacker_t * unpacker, qp_res_t * res, qp_obj_t * val);

static qp_types_t QP_print_unpacker(
        FILE * stream,
        qp_types_t tp,
        qp_unpacker_t * unpacker,
        qp_obj_t * qp_obj);
static void QP_fprint_raw(FILE * stream, const char * s, size_t n);
static int QP_res_str(qp_unpacker_t * unpacker, qp_res_t * res, qp_obj_t * val);
static int QP_res(qp_unpacker_t * unpacker, qp_res_t * res, qp_obj_t * val);
static void QP_res_destroy(qp_res_t * res);
static qp_types_t QP_sprint_unpacker(
        char ** s,
        size_t * sz,
        char ** pt,
        qp_types_t tp,
        qp_unpacker_t * unpacker,
        qp_obj_t * qp_obj);
static int QP_sprint_raw(
        char ** s,
        size_t * sz,
        size_t p,
        const char * d,
        size_t n);

qp_packer_t * qp_packer_create(size_t alloc_size)
{
    assert (alloc_size);  /* alloc_size should not be 0 */

    qp_packer_t * packer;
    packer = (qp_packer_t *) malloc(
            sizeof(qp_packer_t) +
            QP__INITIAL_NEST_SIZE * sizeof(struct qp__nest_s));
    if (packer != NULL)
    {
        packer->alloc_size = alloc_size;
        packer->buffer_size = alloc_size;
        packer->len = 0;
        packer->depth = 0;
        packer->nest_sz = QP__INITIAL_NEST_SIZE;
        packer->buffer = (unsigned char *) malloc(
                sizeof(unsigned char) * alloc_size);

        if (packer->buffer == NULL)
        {
            free(packer);
            packer = NULL;
        }
    }
    return packer;
}

void qp_packer_destroy(qp_packer_t * packer)
{
    free(packer->buffer);
    free(packer);
}

const char * qp_strerror(int err_code)
{
    switch (err_code)
    {
    case 0:                     return "success";
    case QP_ERR_ALLOC:          return "memory allocation error";
    case QP_ERR_WRITE_STREAM:   return "cannot write to stream";
    case QP_ERR_CORRUPT:        return "data is invalid or corrupt";
    case QP_ERR_NO_OPEN_ARRAY:  return "no array found to close";
    case QP_ERR_NO_OPEN_MAP:    return "no map found to close";
    case QP_ERR_MISSING_VALUE:  return "value is missing for the last key";
    default:                    return "unknown error code";
    }
}

const char * qp_version(void)
{
    return QP_VERSION;
}

int qp_add_raw(qp_packer_t * packer, const unsigned char * raw, size_t len)
{
    size_t required_size = len + 9;

    QP_RESIZE(required_size)

    if (len < 100)
    {
        packer->buffer[packer->len++] = 128 + len;
    }
    else if (len <= UINT8_MAX)
    {
        uint8_t length = (uint8_t) len;
        packer->buffer[packer->len++] = QP__RAW8;
        packer->buffer[packer->len++] = length;
    }
    else if (len <= UINT16_MAX)
    {
        uint16_t length = (uint16_t) len;
        packer->buffer[packer->len++] = QP__RAW16;
        memcpy(packer->buffer + packer->len, &length, 2);
        packer->len += 2;
    }
    else if (len <= UINT32_MAX)
    {
        uint32_t length = (uint32_t) len;
        packer->buffer[packer->len++] = QP__RAW32;
        memcpy(packer->buffer + packer->len, &length, 4);
        packer->len += 4;
    }
    else
    {
        uint64_t length = (uint64_t) len;
        packer->buffer[packer->len++] = QP__RAW64;
        memcpy(packer->buffer + packer->len, &length, 8);
        packer->len += 8;
    }
    memcpy(packer->buffer + packer->len, raw, len);
    packer->len += len;

    QP__RETURN_INC_C
}

int qp_fadd_raw(FILE * f, const unsigned char * raw, size_t len)
{
    if (len < 100)
    {
        if (fputc(128 + len, f) == EOF) return EOF;
    }
    else if (len <= UINT8_MAX)
    {
        if (fputc(QP__RAW8, f) == EOF || fputc(len, f) == EOF) return EOF;
    }
    else if (len <= UINT16_MAX)
    {
        if (fputc(QP__RAW16, f) == EOF ||
            fwrite(&len, sizeof(uint16_t), 1, f) != 1) return EOF;
    }
    else if (len <= UINT32_MAX)
    {
        if (fputc(QP__RAW32, f) == EOF ||
            fwrite(&len, sizeof(uint32_t), 1, f) != 1) return EOF;
    }
    else
    {
        if (fputc(QP__RAW64, f) == EOF ||
            fwrite(&len, sizeof(uint64_t), 1, f) != 1) return EOF;

    }
    return -(fwrite(raw, len, 1, f) != 1);
}

int qp_add_array(qp_packer_t ** packaddr)
{
    qp_packer_t * packer = *packaddr;
    QP_RESIZE(1)

    size_t i = packer->depth;

    packer->depth++;

    if (packer->depth == packer->nest_sz)
    {
        qp_packer_t * tmp;
        packer->nest_sz *= 2;
        tmp = (qp_packer_t *) realloc(
                packer,
                sizeof(qp_packer_t) +
                packer->nest_sz * (sizeof(struct qp__nest_s)));
        if (tmp == NULL)
        {
            packer->nest_sz /= 2;
            return QP_ERR_ALLOC;
        }

        (*packaddr) = packer = tmp;
    }
    packer->nesting[i].n = 0;
    packer->nesting[i].pos = packer->len;
    packer->nesting[i].tp = NEST_ARRAY;

    packer->buffer[packer->len++] = QP__ARRAY_OPEN;

    if (i)
    {
        packer->nesting[i - 1].n++;
    }
    return 0;
}

int qp_close_array(qp_packer_t * packer)
{
    struct qp__nest_s nesting;
    if (    !packer->depth ||
            (nesting = packer->nesting[packer->depth - 1]).tp != NEST_ARRAY)
    {
        return QP_ERR_NO_OPEN_ARRAY;
    }

    if (nesting.n > 5)
    {
        QP_RESIZE(1)
        packer->buffer[packer->len++] = QP__ARRAY_CLOSE;
    }
    else
    {
        packer->buffer[nesting.pos] = QP__ARRAY0 + nesting.n;
    }
    packer->depth--;
    return 0;
}

int qp_add_map(qp_packer_t ** packaddr)
{
    qp_packer_t * packer = *packaddr;
    QP_RESIZE(1)

    size_t i = packer->depth;

    packer->depth++;

    if (packer->depth == packer->nest_sz)
    {
        qp_packer_t * tmp;
        packer->nest_sz *= 2;
        tmp = (qp_packer_t *) realloc(
                packer,
                sizeof(qp_packer_t) +
                packer->nest_sz * (sizeof(struct qp__nest_s)));
        if (tmp == NULL)
        {
            packer->nest_sz /= 2;
            return QP_ERR_ALLOC;
        }

        (*packaddr) = packer = tmp;
    }
    packer->nesting[i].n = 0;
    packer->nesting[i].pos = packer->len;
    packer->nesting[i].tp = NEST_MAP;

    packer->buffer[packer->len++] = QP__MAP_OPEN;

    if (i)
    {
        packer->nesting[i - 1].n++;
    }
    return 0;
}

int qp_close_map(qp_packer_t * packer)
{
    struct qp__nest_s nesting;
    if (    !packer->depth ||
            (nesting = packer->nesting[packer->depth - 1]).tp != NEST_MAP)
    {
        return QP_ERR_NO_OPEN_MAP;
    }

    if (nesting.n % 2)
    {
        return QP_ERR_MISSING_VALUE;
    }

    if (nesting.n > 10)
    {
        QP_RESIZE(1)
        packer->buffer[packer->len++] = QP__MAP_CLOSE;
    }
    else
    {
        packer->buffer[nesting.pos] = QP__MAP0 + nesting.n / 2;
    }
    packer->depth--;
    return 0;
}

int qp_add_int64(qp_packer_t * packer, int64_t i)
{
    int8_t i8;
    int16_t i16;
    int32_t i32;

    if ((i8 = (int8_t) i) == i)
    {
        QP_ADD_INT8(packer, i8)
    }
    else if ((i16 = (int16_t) i) == i)
    {
        QP_ADD_INT16(packer, i16)
    }
    else if ((i32 = (int32_t) i) == i)
    {
        QP_ADD_INT32(packer, i32)
    }
    else
    {
        QP_RESIZE(9)
        packer->buffer[packer->len++] = QP__INT64;
        memcpy(packer->buffer + packer->len, &i, sizeof(int64_t));
        packer->len += sizeof(int64_t);
    }
    QP__RETURN_INC_C
}

int qp_fadd_int64(FILE * f, int64_t i)
{
    int8_t i8;
    if ((i8 = (int8_t) i) == i)
    {
        return (i >= 0 && i < 64) ? -(fputc(i, f) == EOF) :
            (i >= -60 && i < 0) ? -(fputc(63 - i, f) == EOF) :
            -(fputc(QP__INT8, f) == EOF || fputc(i, f) == EOF);
    }

    int16_t i16;
    if ((i16 = (int16_t) i) == i)
    {
        return -(fputc(QP__INT16, f) == EOF ||
                fwrite(&i, sizeof(int16_t), 1, f) != 1);
    }

    int32_t i32;
    if ((i32 = (int32_t) i) == i)
    {
        return -(fputc(QP__INT32, f) == EOF ||
                fwrite(&i, sizeof(int32_t), 1, f) != 1);
    }

    return -(fputc(QP__INT64, f) == EOF ||
            fwrite(&i, sizeof(int64_t), 1, f) != 1);
}

int qp_add_double(qp_packer_t * packer, double d)
{
    QP_RESIZE(9)
    if (d == 0.0)
    {
        packer->buffer[packer->len++] = QP__DOUBLE_0;
    }
    else if (d == 1.0)
    {
        packer->buffer[packer->len++] = QP__DOUBLE_1;
    }
    else if (d == -1.0)
    {
        packer->buffer[packer->len++] = QP__DOUBLE_N1;
    }
    else
    {
        packer->buffer[packer->len++] = QP__DOUBLE;
        memcpy(packer->buffer + packer->len, &d, sizeof(double));
        packer->len += sizeof(double);
    }
    QP__RETURN_INC_C
}



int qp_add_true(qp_packer_t * packer) QP_PLAIN_OBJ(QP__TRUE)
int qp_add_false(qp_packer_t * packer) QP_PLAIN_OBJ(QP__FALSE)
int qp_add_null(qp_packer_t * packer) QP_PLAIN_OBJ(QP__NULL)

void qp_unpacker_init(
        qp_unpacker_t * unpacker,
        const unsigned char * pt,
        size_t len)
{
    unpacker->start = pt;
    unpacker->pt = pt;
    unpacker->end = pt + len;
    unpacker->flags = 0;
}

qp_types_t qp_next(qp_unpacker_t * unpacker, qp_obj_t * qp_obj)
{
    uint8_t tp;

    if (unpacker->pt >= unpacker->end)
    {
        if (qp_obj != NULL)
        {
            qp_obj->tp = QP_END;
        }
        return QP_END;
    }

    tp = *unpacker->pt;
    unpacker->pt++;

    switch(tp)
    {
    case 0:
    case 1:
    case 2:
    case 3:
    case 4:
    case 5:
    case 6:
    case 7:
    case 8:
    case 9:
    case 10:
    case 11:
    case 12:
    case 13:
    case 14:
    case 15:
    case 16:
    case 17:
    case 18:
    case 19:
    case 20:
    case 21:
    case 22:
    case 23:
    case 24:
    case 25:
    case 26:
    case 27:
    case 28:
    case 29:
    case 30:
    case 31:
    case 32:
    case 33:
    case 34:
    case 35:
    case 36:
    case 37:
    case 38:
    case 39:
    case 40:
    case 41:
    case 42:
    case 43:
    case 44:
    case 45:
    case 46:
    case 47:
    case 48:
    case 49:
    case 50:
    case 51:
    case 52:
    case 53:
    case 54:
    case 55:
    case 56:
    case 57:
    case 58:
    case 59:
    case 60:
    case 61:
    case 62:
    case 63:
        if (qp_obj != NULL)
        {
            qp_obj->tp = QP_INT64;
            qp_obj->via.int64 = (int64_t) tp;
        }
        return QP_INT64;

    case 64:
    case 65:
    case 66:
    case 67:
    case 68:
    case 69:
    case 70:
    case 71:
    case 72:
    case 73:
    case 74:
    case 75:
    case 76:
    case 77:
    case 78:
    case 79:
    case 80:
    case 81:
    case 82:
    case 83:
    case 84:
    case 85:
    case 86:
    case 87:
    case 88:
    case 89:
    case 90:
    case 91:
    case 92:
    case 93:
    case 94:
    case 95:
    case 96:
    case 97:
    case 98:
    case 99:
    case 100:
    case 101:
    case 102:
    case 103:
    case 104:
    case 105:
    case 106:
    case 107:
    case 108:
    case 109:
    case 110:
    case 111:
    case 112:
    case 113:
    case 114:
    case 115:
    case 116:
    case 117:
    case 118:
    case 119:
    case 120:
    case 121:
    case 122:
    case 123:
        if (qp_obj != NULL)
        {
            qp_obj->tp = QP_INT64;
            qp_obj->via.int64 = (int64_t) 63 - tp;
        }
        return QP_INT64;

    case 124:
        if (qp_obj != NULL)
        {
            qp_obj->tp = QP_HOOK;
        }
        return QP_HOOK;

    case 125:
    case 126:
    case 127:
        /* unpack fixed doubles -1.0, 0.0 or 1.0 */
        if (qp_obj != NULL)
        {
            qp_obj->tp = QP_DOUBLE;
            qp_obj->via.real = (double) (tp - 126);
        }
        return QP_DOUBLE;

    case 128:
    case 129:
    case 130:
    case 131:
    case 132:
    case 133:
    case 134:
    case 135:
    case 136:
    case 137:
    case 138:
    case 139:
    case 140:
    case 141:
    case 142:
    case 143:
    case 144:
    case 145:
    case 146:
    case 147:
    case 148:
    case 149:
    case 150:
    case 151:
    case 152:
    case 153:
    case 154:
    case 155:
    case 156:
    case 157:
    case 158:
    case 159:
    case 160:
    case 161:
    case 162:
    case 163:
    case 164:
    case 165:
    case 166:
    case 167:
    case 168:
    case 169:
    case 170:
    case 171:
    case 172:
    case 173:
    case 174:
    case 175:
    case 176:
    case 177:
    case 178:
    case 179:
    case 180:
    case 181:
    case 182:
    case 183:
    case 184:
    case 185:
    case 186:
    case 187:
    case 188:
    case 189:
    case 190:
    case 191:
    case 192:
    case 193:
    case 194:
    case 195:
    case 196:
    case 197:
    case 198:
    case 199:
    case 200:
    case 201:
    case 202:
    case 203:
    case 204:
    case 205:
    case 206:
    case 207:
    case 208:
    case 209:
    case 210:
    case 211:
    case 212:
    case 213:
    case 214:
    case 215:
    case 216:
    case 217:
    case 218:
    case 219:
    case 220:
    case 221:
    case 222:
    case 223:
    case 224:
    case 225:
    case 226:
    case 227:
        /* unpack fixed sized raw strings */
        {
            size_t size = tp - 128;
            QP_UNPACK_CHECK_SZ(size)

            if (qp_obj != NULL)
            {
                qp_obj->tp = QP_RAW;
                qp_obj->via.raw = (char *) unpacker->pt;
                qp_obj->len = size;
            }
            unpacker->pt += size;
            return QP_RAW;
        }
    case 228:
        QP_UNPACK_RAW(uint8_t)
    case 229:
        QP_UNPACK_RAW(uint16_t)
    case 230:
        QP_UNPACK_RAW(uint32_t)
    case 231:
        QP_UNPACK_RAW(uint64_t)

    case 232:
        QP_UNPACK_INT(int8_t)
    case 233:
        QP_UNPACK_INT(int16_t)
    case 234:
        QP_UNPACK_INT(int32_t)
    case 235:
        QP_UNPACK_INT(int64_t)

    case 236:
        QP_UNPACK_CHECK_SZ(sizeof(double))
        if (qp_obj != NULL)
        {
            qp_obj->tp = QP_DOUBLE;
            memcpy(&qp_obj->via.real, unpacker->pt, sizeof(double));
        }
        unpacker->pt += sizeof(double);
        return QP_DOUBLE;

    case 237:
    case 238:
    case 239:
    case 240:
    case 241:
    case 242:
    case 243:
    case 244:
    case 245:
    case 246:
    case 247:
    case 248:
    case 249:
    case 250:
    case 251:
    case 252:
    case 253:
    case 254:
    case 255:
        if (qp_obj != NULL)
        {
            qp_obj->tp = tp;
        }
        return tp;
    }

    return QP_ERR;
}

int qp_raw_is_equal(qp_obj_t * obj, const char * str)
{
    return
        strlen(str) == obj->len &&
        strncmp(obj->via.raw, str, obj->len) == 0;
}


void qp_res_destroy(qp_res_t * res)
{
    QP_res_destroy(res);
    free(res);
}

int qp_res_fprint(qp_res_t * res, FILE * stream)
{
    size_t i;
    int rc = 0;
    switch(res->tp)
    {
    case QP_RES_ARRAY:
        if (fprintf(stream, "[") < 0)
        {
            return QP_ERR_WRITE_STREAM;
        }
        for (i = 0; i < res->via.array->n; i++)
        {
            if ((i && fprintf(stream, ",") < 0) ||
                (rc = qp_res_fprint(res->via.array->values + i, stream)))
            {
                return (rc) ? rc : QP_ERR_WRITE_STREAM;
            }
        }
        if (fprintf(stream, "]") < 0)
        {
            return QP_ERR_WRITE_STREAM;
        }
        break;
    case QP_RES_STR:
        if (fprintf(stream, "%s", res->via.str) < 0)
        {
            return QP_ERR_WRITE_STREAM;
        }
        break;
    case QP_RES_RAW:
        if (fprintf(
                stream, "%.*s",
                (int) res->via.raw->n, res->via.raw->data) < 0)
        {
            return QP_ERR_WRITE_STREAM;
        }
        break;
    case QP_RES_BOOL:
        if (fprintf(stream, "%s", (res->via.boolean) ? "TRUE": "FALSE") < 0)
        {
            return QP_ERR_WRITE_STREAM;
        }
        break;
    case QP_RES_NULL:
        if (fprintf(stream, "NULL") < 0)
        {
            return QP_ERR_WRITE_STREAM;
        }
        break;
    case QP_RES_INT64:
        if (fprintf(stream, "%" PRId64, res->via.int64) < 0)
        {
            return QP_ERR_WRITE_STREAM;
        }
        break;
    case QP_RES_REAL:
        if (fprintf(stream, "%f", res->via.real) < 0)
        {
            return QP_ERR_WRITE_STREAM;
        }
        break;
    case QP_RES_MAP:
        if (fprintf(stream, "{") < 0)
        {
            return QP_ERR_WRITE_STREAM;
        }

        for (i = 0; i < res->via.map->n; i++)
        {
            if ((i && fprintf(stream, ",") < 0) || (
                (rc = qp_res_fprint(res->via.map->keys + i, stream)) ||
                fprintf(stream, ":") < 0 ||
                (rc = qp_res_fprint(res->via.map->values + i, stream))))
            {
                return (rc) ? rc : QP_ERR_WRITE_STREAM;
            }
        }
        if (fprintf(stream, "}") < 0)
        {
            return QP_ERR_WRITE_STREAM;
        }
        break;
    default:
        return QP_ERR_CORRUPT;
    }
    return rc;
}

qp_res_t * qp_unpacker_res(qp_unpacker_t * unpacker, int * rc)
{
    int fallb;
    int * rcode = (rc == NULL) ? &fallb : rc;
    qp_res_t * res;
    qp_types_t tp;
    qp_obj_t val;

    tp = qp_next(unpacker, &val);
    if (tp == QP_END ||
        tp == QP_ERR ||
        tp == QP_HOOK ||
        tp == QP_ARRAY_CLOSE ||
        tp == QP_MAP_CLOSE)
    {
        *rcode = QP_ERR_CORRUPT;
        return NULL;
    }

    res = (qp_res_t *) malloc(sizeof(qp_res_t));

    if (res == NULL)
    {
        *rcode = QP_ERR_ALLOC;
    }
    else
    {
        *rcode = QP_res(unpacker, res, &val);

        if (*rcode)
        {
            qp_res_destroy(res);
            res = NULL;
        }
    }
    return res;
}

void qp_print(const unsigned char * data, size_t len)
{
    qp_fprint(stdout, data, len);
    printf("\n");
}

void qp_fprint(FILE * stream, const unsigned char * data, size_t len)
{
    qp_obj_t qp_obj;
    qp_unpacker_t unpacker;
    qp_unpacker_init(&unpacker, data, len);
    QP_print_unpacker(stream, qp_next(&unpacker, &qp_obj), &unpacker, &qp_obj);
}

char * qp_sprint(const unsigned char * data, size_t len)
{
    if (!len)
    {
        return NULL;
    }
    size_t sz = QP__MINSZ + len * QP__FACTOR;
    char * s = (char *) malloc(sz);
    if (s != NULL)
    {
        char * pt;
        qp_obj_t qp_obj;
        qp_unpacker_t unpacker;
        qp_unpacker_init(&unpacker, data, len);
        pt = s;
        if (QP_sprint_unpacker(
                &s,
                &sz,
                &pt,
                qp_next(&unpacker, &qp_obj),
                &unpacker, &qp_obj) != QP_ERR)
        {
            *pt = '\0';
            sz = pt - s + 1;
            char * tmp = (char *) realloc(s, sz);
            if (tmp == NULL)
            {
                free(s);
            }
            s = tmp;
        }
        else
        {
            free(s);
            s = NULL;
        }
    }
    return s;
}

static void QP_fprint_raw(FILE * stream, const char * s, size_t n)
{
    size_t i;
    fputc('"', stream);
    for (i = 0; i < n; i++)
    {
        char c = s[i];
        switch (c)
        {
        case '"':
        case '\\':
            fputc('\\', stream);
        }
        fputc(c, stream);
    }
    fputc('"', stream);
}

static qp_types_t QP_print_unpacker(
        FILE * stream,
        qp_types_t tp,
        qp_unpacker_t * unpacker,
        qp_obj_t * qp_obj)
{
    int count;
    int found;
    switch (tp)
    {
    case QP_INT64:
        fprintf(stream, "%" PRId64, qp_obj->via.int64);
        break;
    case QP_DOUBLE:
        fprintf(stream, "%f", qp_obj->via.real);
        break;
    case QP_RAW:
        QP_fprint_raw(stream, qp_obj->via.raw, qp_obj->len);
        break;
    case QP_TRUE:
        fprintf(stream, "true");
        break;
    case QP_FALSE:
        fprintf(stream, "false");
        break;
    case QP_NULL:
        fprintf(stream, "null");
        break;
    case QP_ARRAY0:
    case QP_ARRAY1:
    case QP_ARRAY2:
    case QP_ARRAY3:
    case QP_ARRAY4:
    case QP_ARRAY5:
        fprintf(stream, "[");
        count = tp - QP_ARRAY0;
        tp = qp_next(unpacker, qp_obj);
        for (found = 0; count-- && tp; found = 1)
        {
            if (found )
            {
                fprintf(stream, ",");
            }
            tp = QP_print_unpacker(stream, tp, unpacker, qp_obj);
        }
        fprintf(stream, "]");
        return tp;
    case QP_MAP0:
    case QP_MAP1:
    case QP_MAP2:
    case QP_MAP3:
    case QP_MAP4:
    case QP_MAP5:
        fprintf(stream, "{");
        count = tp - QP_MAP0;
        tp = qp_next(unpacker, qp_obj);
        for (found = 0; count-- && tp; found = 1)
        {
            if (found )
            {
                fprintf(stream, ",");
            }
            tp = QP_print_unpacker(stream, tp, unpacker, qp_obj);
            fprintf(stream, ":");
            tp = QP_print_unpacker(stream, tp, unpacker, qp_obj);
        }
        fprintf(stream, "}");
        return tp;
    case QP_ARRAY_OPEN:
        fprintf(stream, "[");
        tp = qp_next(unpacker, qp_obj);
        for (count = 0; tp && tp != QP_ARRAY_CLOSE; count = 1)
        {
            if (count)
            {
                fprintf(stream, ",");
            }
            tp = QP_print_unpacker(stream, tp, unpacker, qp_obj);
        }
        fprintf(stream, "]");
        break;
    case QP_MAP_OPEN:
        fprintf(stream, "{");
        tp = qp_next(unpacker, qp_obj);
        for (count = 0; tp && tp != QP_MAP_CLOSE; count = 1)
        {
            if (count)
            {
                fprintf(stream, ",");
            }
            tp = QP_print_unpacker(stream, tp, unpacker, qp_obj);
            fprintf(stream, ":");
            tp = QP_print_unpacker(stream, tp, unpacker, qp_obj);
        }
        fprintf(stream, "}");
        break;
    default:
        break;
    }
    return qp_next(unpacker, qp_obj);
}

static int QP_sprint_raw(
        char ** s,
        size_t * sz,
        size_t p,
        const char * d,
        size_t n)
{
    size_t i;
    size_t pos = p;
    (*s)[pos++] = '"';
    for (i = 0; i < n; i++)
    {
        char c = d[i];

        if (pos >= (*sz) - 3)
        {
            char * tmp;
            *sz += QP__MINSZ + (n - i) * QP__FACTOR;
            tmp = (char *) realloc(*s, *sz);
            if (tmp == NULL)
            {
                return -1;
            }
            *s = tmp;
        }

        switch (c)
        {
        case '"':
        case '\\':
            (*s)[pos++] = '\\';
        }
        (*s)[pos++] = c;
    }
    (*s)[pos++] = '"';
    return pos - p;
}

static qp_types_t QP_sprint_unpacker(
        char ** s,
        size_t * sz,
        char ** pt,
        qp_types_t tp,
        qp_unpacker_t * unpacker,
        qp_obj_t * qp_obj)
{
    int n, count, found;
    size_t pos = (*pt) - (*s);
    if ((*sz) - pos < QP__MINSZ)
    {
        char * tmp;
        *sz += QP__MINSZ + (unpacker->end - unpacker->pt) * QP__FACTOR;
        tmp = (char *) realloc(*s, *sz);
        if (tmp == NULL)
        {
            return QP_ERR;
        }
        *s = tmp;
        *pt = (*s) + pos;
    }
    switch (tp)
    {
    case QP_INT64:
        n = sprintf(*pt, "%" PRId64, qp_obj->via.int64); QP__CHKN
        break;
    case QP_DOUBLE:
        n = sprintf(*pt, "%f", qp_obj->via.real); QP__CHKN
        break;
    case QP_RAW:
        n = QP_sprint_raw(s, sz, pos, qp_obj->via.raw, qp_obj->len); QP__CHKN
        break;
    case QP_TRUE:
        n = sprintf(*pt, "true"); QP__CHKN
        break;
    case QP_FALSE:
        n = sprintf(*pt, "false"); QP__CHKN
        break;
    case QP_NULL:
        n = sprintf(*pt, "null"); QP__CHKN
        break;
    case QP_ARRAY0:
    case QP_ARRAY1:
    case QP_ARRAY2:
    case QP_ARRAY3:
    case QP_ARRAY4:
    case QP_ARRAY5:
        n = sprintf(*pt, "["); QP__CHKN
        count = tp - QP_ARRAY0;
        tp = qp_next(unpacker, qp_obj); QP__CHKT
        for (found = 0; count-- && tp; found = 1)
        {
            if (found )
            {
                n = sprintf(*pt, ","); QP__CHKN
            }
            tp = QP_sprint_unpacker(s, sz, pt, tp, unpacker, qp_obj); QP__CHKT
        }
        n = sprintf(*pt, "]"); QP__CHKN
        return tp;
    case QP_MAP0:
    case QP_MAP1:
    case QP_MAP2:
    case QP_MAP3:
    case QP_MAP4:
    case QP_MAP5:
        n = sprintf(*pt, "{"); QP__CHKN
        count = tp - QP_MAP0;
        tp = qp_next(unpacker, qp_obj); QP__CHKT
        for (found = 0; count-- && tp; found = 1)
        {
            if (found )
            {
                n = sprintf(*pt, ","); QP__CHKN
            }
            tp = QP_sprint_unpacker(s, sz, pt, tp, unpacker, qp_obj); QP__CHKT
            n = sprintf(*pt, ":"); QP__CHKN
            tp = QP_sprint_unpacker(s, sz, pt, tp, unpacker, qp_obj); QP__CHKT
        }
        n = sprintf(*pt, "}"); QP__CHKN
        return tp;
    case QP_ARRAY_OPEN:
        n = sprintf(*pt, "["); QP__CHKN
        tp = qp_next(unpacker, qp_obj); QP__CHKT
        for (count = 0; tp && tp != QP_ARRAY_CLOSE; count = 1)
        {
            if (count)
            {
                n = sprintf(*pt, ","); QP__CHKN
            }
            tp = QP_sprint_unpacker(s, sz, pt, tp, unpacker, qp_obj); QP__CHKT
        }
        n = sprintf(*pt, "]"); QP__CHKN
        break;
    case QP_MAP_OPEN:
        n = sprintf(*pt, "{"); QP__CHKN
        tp = qp_next(unpacker, qp_obj); QP__CHKT
        for (count = 0; tp && tp != QP_MAP_CLOSE; count = 1)
        {
            if (count)
            {
                n = sprintf(*pt, ","); QP__CHKN
            }
            tp = QP_sprint_unpacker(s, sz, pt, tp, unpacker, qp_obj); QP__CHKT
            n = sprintf(*pt, ":"); QP__CHKN
            tp = QP_sprint_unpacker(s, sz, pt, tp, unpacker, qp_obj); QP__CHKT
        }
        n = sprintf(*pt, "}"); QP__CHKN
        break;
    default:
        break;
    }

    return qp_next(unpacker, qp_obj);
}

static int QP_res_str(qp_unpacker_t * unpacker, qp_res_t * res, qp_obj_t * val)
{
    if (val->tp != QP_RAW) return QP_ERR_KEY_STR;
    res->tp = QP_RES_STR;
    res->via.str = strndup(val->via.raw, val->len);
    return (res->via.str == NULL) ? QP_ERR_ALLOC : 0;
}

static int QP_res(qp_unpacker_t * unpacker, qp_res_t * res, qp_obj_t * val)
{
    QP_f f;
    int rc;
    size_t i, n, m;
    qp_types_t tp;
    qp_res_t * tmp;

    switch((qp_types_t) val->tp)
    {
    case QP_RAW:
        if (unpacker->flags & QP_UNPACK_FLAG_KEY_STR)
        if (unpacker->flags & QP_UNPACK_FLAG_RAW)
        {
            res->tp = QP_RES_RAW;
            res->via.raw = (qp_raw_t *) malloc(sizeof(qp_raw_t) + val->len);
            if (res->via.raw == NULL)
            {
                return QP_ERR_ALLOC;
            }
            res->via.raw->n = val->len;
            memcpy(res->via.raw->data, val->via.raw, val->len);
            return 0;
        }

        res->tp = QP_RES_STR;
        res->via.str = strndup(val->via.raw, val->len);
        return (res->via.str == NULL) ? QP_ERR_ALLOC : 0;
    case QP_HOOK:
        /* hooks are not implemented yet */
        return QP_ERR_CORRUPT;
    case QP_INT64:
        res->tp = QP_RES_INT64;
        res->via.int64 = val->via.int64;
        return 0;
    case QP_DOUBLE:
        res->tp = QP_RES_REAL;
        res->via.real = val->via.real;
        return 0;
    case QP_ARRAY0:
    case QP_ARRAY1:
    case QP_ARRAY2:
    case QP_ARRAY3:
    case QP_ARRAY4:
    case QP_ARRAY5:
        res->tp = QP_RES_ARRAY;
        n = val->tp - QP_ARRAY0;
        res->via.array = (qp_array_t *) malloc(sizeof(qp_array_t));
        if (res->via.array == NULL)
        {
            return QP_ERR_ALLOC;
        }
        res->via.array->n = n;
        res->via.array->values = (qp_res_t *) malloc(sizeof(qp_res_t) * n);

        if (!n)
        {
            return 0;
        }

        if (res->via.array->values == NULL)
        {
            res->via.array->n = 0;
            return QP_ERR_ALLOC;
        }

        for (i = 0; i < n; i++)
        {
            tp = qp_next(unpacker, val);

            if (tp == QP_END ||
                tp == QP_ERR ||
                tp == QP_HOOK ||
                tp == QP_ARRAY_CLOSE ||
                tp == QP_MAP_CLOSE)
            {
                res->via.array->n = i;
                return QP_ERR_CORRUPT;
            }

            if ((rc = QP_res(unpacker, res->via.array->values + i, val)))
            {
                res->via.array->n = i + 1;
                return rc;
            }
        }
        return 0;
    case QP_MAP0:
    case QP_MAP1:
    case QP_MAP2:
    case QP_MAP3:
    case QP_MAP4:
    case QP_MAP5:
        res->tp = QP_RES_MAP;
        n = val->tp - QP_MAP0;
        res->via.map = (qp_map_t *) malloc(sizeof(qp_map_t));
        if (res->via.map == NULL)
        {
            return QP_ERR_ALLOC;
        }
        res->via.map->n = n;
        res->via.map->keys = (qp_res_t *) malloc(sizeof(qp_res_t) * n);
        res->via.map->values = (qp_res_t *) malloc(sizeof(qp_res_t) * n);

        if (!n)
        {
            return 0;
        }

        if (res->via.map->keys == NULL || res->via.map->values == NULL)
        {
            res->via.map->n = 0;
            return QP_ERR_ALLOC;
        }

        for (i = 0; i < n; i++)
        {
            int kv = 2;

            while (kv--)
            {
                tp = qp_next(unpacker, val);

                f = ((unpacker->flags & QP_UNPACK_FLAG_KEY_STR) && kv) ?
                    QP_res_str : QP_res;

                if (tp == QP_END ||
                    tp == QP_ERR ||
                    tp == QP_HOOK ||
                    tp == QP_ARRAY_CLOSE ||
                    tp == QP_MAP_CLOSE ||
                    (rc = f(
                        unpacker,
                        (kv) ? res->via.map->keys + i : res->via.map->values + i,
                        val)))
                {
                    res->via.map->n = i;
                    if (!kv)
                    {
                        QP_res_destroy(res->via.map->keys + i);
                    }
                    return QP_ERR_CORRUPT;
                }
            }
        }
        return 0;
    case QP_TRUE:
    case QP_FALSE:
        res->tp = QP_RES_BOOL;
        res->via.boolean = (val->tp == QP_TRUE);
        return 0;
    case QP_NULL:
        res->tp = QP_RES_NULL;
        res->via.null = NULL;
        return 0;
    case QP_ARRAY_OPEN:
        res->tp = QP_RES_ARRAY;

        n = 0;

        res->via.array = (qp_array_t *) malloc(sizeof(qp_array_t));
        if (res->via.array == NULL)
        {
            return QP_ERR_ALLOC;
        }

        res->via.array->n = 0;
        res->via.array->values = NULL;

        for (i = 0;; i++)
        {
            tp = qp_next(unpacker, val);

            if (tp == QP_END || tp == QP_ARRAY_CLOSE)
            {
                break;
            }

            if (tp == QP_ERR || tp == QP_HOOK || tp == QP_MAP_CLOSE)
            {
                return QP_ERR_CORRUPT;
            }

            res->via.array->n++;

            if (res->via.array->n > n)
            {
                n = (n) ? n * 2: QP__INITIAL_ALLOC_SZ;

                tmp = (qp_res_t *) realloc(
                        res->via.array->values,
                        sizeof(qp_res_t) * n);

                if (tmp == NULL)
                {
                    res->via.array->n--;
                    return QP_ERR_ALLOC;
                }

                res->via.array->values = tmp;
            }

            if ((rc = QP_res(unpacker, res->via.array->values + i, val)))
            {
                return rc;
            }
        }

        if (n > res->via.array->n)
        {
            tmp = (qp_res_t *) realloc(
                    res->via.array->values,
                    sizeof(qp_res_t) * res->via.array->n);

            if (tmp == NULL)
            {
                return QP_ERR_ALLOC;
            }

            res->via.array->values = tmp;
        }

        return 0;
    case QP_MAP_OPEN:
        res->tp = QP_RES_MAP;

        tp = QP_MAP_OPEN;
        n = 0;
        m = 0;
        res->via.map = (qp_map_t *) malloc(sizeof(qp_map_t));
        if (res->via.map == NULL)
        {
            return QP_ERR_ALLOC;
        }

        res->via.map->n = 0;
        res->via.map->keys = NULL;
        res->via.map->values = NULL;

        for (i = 0; tp != QP_END && tp != QP_MAP_CLOSE; i++)
        {
            int kv = 2;

            while (kv--)
            {
                qp_res_t ** rs;
                size_t * sz;
                tp = qp_next(unpacker, val);

                if (kv)
                {
                    f = ((unpacker->flags & QP_UNPACK_FLAG_KEY_STR)) ?
                        QP_res_str : QP_res;
                    if (tp == QP_END || tp == QP_MAP_CLOSE)
                    {
                        break;
                    }

                    if (tp == QP_ERR ||
                        tp == QP_HOOK ||
                        tp == QP_ARRAY_CLOSE)
                    {
                        return QP_ERR_CORRUPT;
                    }
                    res->via.map->n++;
                    rs = &res->via.map->keys;
                    sz = &n;
                }
                else
                {
                    f = QP_res;
                    if (tp == QP_END ||
                        tp == QP_ERR ||
                        tp == QP_HOOK ||
                        tp == QP_ARRAY_CLOSE ||
                        tp == QP_MAP_CLOSE)
                    {
                        res->via.map->n--;
                        QP_res_destroy(res->via.map->keys + i);
                        return QP_ERR_CORRUPT;
                    }
                    rs = &res->via.map->values;
                    sz = &m;
                }

                if (res->via.map->n > *sz)
                {
                    *sz = (*sz) ? (*sz) * 2: QP__INITIAL_ALLOC_SZ;

                    tmp = (qp_res_t *) realloc(*rs, sizeof(qp_res_t) * (*sz));

                    if (tmp == NULL)
                    {
                        res->via.map->n--;
                        if (!kv)
                        {
                            QP_res_destroy(res->via.map->keys + i);
                        }
                        return QP_ERR_ALLOC;
                    }

                    *rs = tmp;
                }

                if ((rc = f(unpacker, (*rs) + i, val)))
                {
                    if (!kv)
                    {
                        printf("\n\nHERE!!!\n\n");
                        QP_res_destroy(res->via.map->keys + i);
                    }
                    res->via.map->n--;
                    return rc;
                }
            }
        }

        if (n > res->via.map->n)
        {
            tmp = (qp_res_t *) realloc(
                    res->via.map->keys,
                    sizeof(qp_res_t) * res->via.map->n);
            if (tmp == NULL)
            {
                return QP_ERR_ALLOC;
            }
            res->via.map->keys = tmp;

            tmp = (qp_res_t *) realloc(
                    res->via.map->values,
                    sizeof(qp_res_t) * res->via.map->n);
            if (tmp == NULL)
            {
                return QP_ERR_ALLOC;
            }
            res->via.map->values = tmp;
        }

        return 0;
    default:
        res->tp = QP_RES_NULL;
        return QP_ERR_CORRUPT;
    }
}

static void QP_res_destroy(qp_res_t * res)
{
    size_t i;
    switch(res->tp)
    {
    case QP_RES_ARRAY:
        for (i = 0; i < res->via.array->n; i++)
        {
            QP_res_destroy(res->via.array->values + i);
        }
        free(res->via.array->values);
        free(res->via.array);
        break;
    case QP_RES_RAW:
        free(res->via.raw);
        break;
    case QP_RES_STR:
        free(res->via.str);
        break;
    case QP_RES_MAP:
        for (i = 0; i < res->via.map->n; i++)
        {
            QP_res_destroy(res->via.map->keys + i);
            QP_res_destroy(res->via.map->values + i);
        }
        free(res->via.map->keys);
        free(res->via.map->values);
        free(res->via.map);
        break;
    default:
        break;
    }
}
