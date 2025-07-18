#ifdef WASM
typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;

typedef char s8;
typedef short s16;
typedef int s32;

typedef u32 bool;
typedef u32 size_t;
#endif

#ifndef WASM
#include <stdint.h>
#include <stddef.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef unsigned long long u64;

typedef int8_t s8;
typedef int16_t s16;
typedef int32_t s32;
typedef long long s64;

typedef u32 bool;
#endif

typedef struct String
{
    u8 *data;
    size_t size;
} String;

#include <time.h>

#define NS_IN_SECOND (1000000000LL)

static u64
readTimer() {
    struct timespec instant;
    clock_gettime(CLOCK_MONOTONIC, &instant);

    return instant.tv_sec * NS_IN_SECOND + instant.tv_nsec;
}

#define LIT_TO_STR(x) ((String){ .data = (u8 *)x, .size = sizeof(x) - 1 })

const u32 Kilobyte = 1024;
const u32 Megabyte = Kilobyte * 1024;
const u32 Gigabyte = Megabyte * 1024;

#define true 1
#define false 0

#define MIN(a,b) (((a)<(b))?(a):(b))
#define MAX(a,b) (((a)>(b))?(a):(b))
#define ARRAY_LENGTH(a) (sizeof((a))/sizeof((a)[0]))

#define STMNT(s) do{ s }while(0)

#define SLL_STACK_PUSH_(H,N) N->next=H,H=N
#define SLL_STACK_POP_(H) H=H=H->next
#define SLL_QUEUE_PUSH_MULTIPLE_(F,L,FF,LL) if(LL){if(F){L->next=FF;}else{F=FF;}L=LL;L->next=0;}
#define SLL_QUEUE_PUSH_(F,L,N) SLL_QUEUE_PUSH_MULTIPLE_(F,L,N,N)
#define SLL_QUEUE_POP_(F,L) if (F==L) { F=L=0; } else { F=F->next; }

#define SLL_STACK_PUSH(H,N) (SLL_STACK_PUSH_((H),(N)))
#define SLL_STACK_POP(H) (SLL_STACK_POP_((H)))
#define SLL_QUEUE_PUSH_MULTIPLE(F,L,FF,LL) STMNT( SLL_QUEUE_PUSH_MULTIPLE_((F),(L),(FF),(LL)) )
#define SLL_QUEUE_PUSH(F,L,N) STMNT( SLL_QUEUE_PUSH_((F),(L),(N)) )
#define SLL_QUEUE_POP(F,L) STMNT( SLL_QUEUE_POP_((F),(L)) )

#define WASM_PAGE_SIZE 65536

#define assert(expression) __assert((void*)(expression), __LINE__)

#if defined(LINUX) || defined(WINDOWS)
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

void javascriptPrintStringPtr(void *s) {
    String *string = (String *)s;
    printf("%.*s\n", (int)string->size, string->data);
}
void javascriptPrintNumber(u32 n) {}
#endif

#ifdef WASM
extern unsigned char __heap_base;
unsigned int bumpPointerBase = 0;
unsigned int memoryUsed = 0;
unsigned int bumpPointerArenaTop = 0;
unsigned int bumpPointer = (unsigned int)(&__heap_base);

extern void javascriptPrintStringPtr(void *s);
extern void javascriptThrowErrorStringPtr(void *s);
extern void javascriptPrintNumber(u32 n);

static void unreachable() {
    __builtin_unreachable();
}

static void __assert(void *boolean, int line) {
    if (!boolean) {
        String message = LIT_TO_STR("Assertion failed");
        javascriptThrowErrorStringPtr(&message);
        __builtin_unreachable();
    }
}

void*
malloc(unsigned long n) {
    u32 memorySize = __builtin_wasm_memory_size(0) * WASM_PAGE_SIZE;
    u32 heapSize = memorySize - bumpPointerBase;
    u32 availableMemory = heapSize - (bumpPointer - bumpPointerBase);
    if(n > availableMemory) {
        __builtin_wasm_memory_grow(0, n / WASM_PAGE_SIZE + 1);
    }

    unsigned int r = bumpPointer;
    bumpPointer += n;
    return (void *)r;
}

static void free(void* p) {}

void *
memcpy(void *dst, const void *src, unsigned long n) {
    char *d = dst;
    const char *s = src;
    for (int i = 0; i < n; i++) {
        d[i] = s[i];
    }
    return dst;
}

static void
memset(void *dst, u8 value, int n) {
    u8 *d = (u8 *)dst;
    for (int i = 0; i < n; i++) {
        d[i] = value;
    }
}

void resetBumpPointer() {
    if(bumpPointerBase == 0) {
        bumpPointerBase = bumpPointer;
    }

    u32 size = MIN(bumpPointer - bumpPointerBase, bumpPointerArenaTop - bumpPointerBase);
    memset((void *)bumpPointerBase, 0, size);
    bumpPointer = bumpPointerBase;
}

typedef __builtin_va_list va_list;
#define va_start(v,l)   __builtin_va_start(v,l)
#define va_end(v)       __builtin_va_end(v)
#define va_arg(v,l)     __builtin_va_arg(v,l)
#define va_copy(d,s)    __builtin_va_copy(d,s)

#define countLeadingZeros(x)  __builtin_clz(x)
#define countTrailingZeros(x) __builtin_ctz(x)
#else

#include <stdarg.h>

#endif

typedef struct MemoryCursor {
    u8* basePointer;
    u8* cursorPointer;
    size_t size;
} MemoryCursor;

typedef struct MemoryCursorNode {
    struct MemoryCursorNode *next;
    MemoryCursor cursor;
} MemoryCursorNode;

typedef struct Arena {
    MemoryCursorNode *cursorNode;
    size_t chunkSize;
    size_t alignment;
} Arena;

static MemoryCursorNode *
arenaNewNode(Arena *arena, size_t size) {
    MemoryCursorNode *result = 0x0;
    size = MAX(arena->chunkSize, size);

#ifdef WASM
    void *memory = (u8 *)malloc(size + sizeof(MemoryCursorNode));
#else
    void *memory = (u8 *)calloc(1, size + sizeof(MemoryCursorNode));
#endif
    assert(memory);

    result = (MemoryCursorNode *)memory;

    result->cursor.basePointer = (u8 *)memory + sizeof(MemoryCursorNode);
    result->cursor.cursorPointer = result->cursor.basePointer;
    result->cursor.size = size;
    SLL_STACK_PUSH(arena->cursorNode, result);

    return result;
}

static void
cursorDestroy(MemoryCursor *cursor) {
    if(cursor && cursor->basePointer) {
        void *pointer = (u8 *)cursor->basePointer - sizeof(MemoryCursorNode);
        free(pointer);
    }
}

static size_t
cursorFreeBytes(MemoryCursor *cursor) {
    size_t result = cursor->size - (size_t)(cursor->cursorPointer - cursor->basePointer);

    return result;
}

static size_t
cursorTakenBytes(MemoryCursor *cursor) {
    size_t result = (size_t)(cursor->cursorPointer - cursor->basePointer);

    return result;
}

static u32
arenaTakenBytes(Arena *arena) {
    u32 result = 0;

    if(arena) {
        for(MemoryCursorNode *cursorNode = arena->cursorNode;
            cursorNode != 0x0;
            cursorNode = cursorNode->next) {
            result += cursorTakenBytes(&cursorNode->cursor);
        }
    }

    return result;
}

static u32
arenaFreeBytes(Arena *arena) {
    u32 result = 0;

    if(arena) {
        for(MemoryCursorNode *cursorNode = arena->cursorNode;
            cursorNode != 0x0;
            cursorNode = cursorNode->next) {
            result += cursorFreeBytes(&cursorNode->cursor);
        }
    }

    return result;
}

static void
arenaDestroy(Arena *arena) {
    if(arena) {
        MemoryCursorNode *toDestroy = 0x0;
        for(MemoryCursorNode *cursorNode = arena->cursorNode;
            cursorNode != 0x0;
            cursorNode = cursorNode->next) {
            if(toDestroy) {
                cursorDestroy(&toDestroy->cursor);
            }

            toDestroy = cursorNode;
        }

        if(toDestroy) {
            cursorDestroy(&toDestroy->cursor);
        }

        arena->cursorNode = 0x0;
    }
}

static Arena
arenaCreate(size_t size, size_t chunkSize, size_t alignment) {
    Arena result = {};

    result.chunkSize = chunkSize;
    result.alignment = alignment;

    if(size > 0) {
    	arenaNewNode(&result, size);
    }

    return result;
}

static void
cursorClear(MemoryCursor *cursor, u8 clearTo) {
    memset(cursor->basePointer, clearTo, cursor->size);
    cursor->cursorPointer = cursor->basePointer;
}

static Arena
arenaCreateZeros(size_t size, size_t chunkSize, size_t alignment) {
    Arena result = {};

    result = arenaCreate(size, chunkSize, alignment);
    cursorClear(&result.cursorNode->cursor, 0);

    return result;
}

static void *
arenaPush(Arena *arena, size_t size) {
    void *result = 0x0;

    assert(arena);

    if(size) {
        MemoryCursorNode *cursorNode = arena->cursorNode;
        if(!cursorNode) {
            cursorNode = arenaNewNode(arena, size);
        }

        MemoryCursor *cursor = &cursorNode->cursor;
        size_t bytesLeft = cursorFreeBytes(cursor);
        // Calculates how many bytes we need to add to be aligned on the 16 bytes.
        size_t paddingNeeded = (0x10 - ((size_t)cursor->cursorPointer & 0xf)) & 0xf;

        if(size + paddingNeeded > bytesLeft) {
            cursorNode = arenaNewNode(arena, size + paddingNeeded);
            cursor = &cursorNode->cursor;
        }

        cursor->cursorPointer += paddingNeeded;
        result = cursor->cursorPointer;
        cursor->cursorPointer += size;
    }

    return result;
}

static u64
arenaPos(Arena *arena) {
    u64 result = 0;

    if(arena) {
        MemoryCursorNode *toCount = 0x0;
        for(MemoryCursorNode *cursorNode = arena->cursorNode;
            cursorNode != 0x0;
            cursorNode = cursorNode->next) {
            if(toCount) {
                result += cursorTakenBytes(&toCount->cursor);
            }

            toCount = cursorNode;
        }

        if(toCount) {
            result += cursorTakenBytes(&toCount->cursor);
        }
    }

    return result;
}

static void
arenaPop(Arena *arena, size_t size) {
    assert(arena);

    if(size) {
        MemoryCursorNode *cursorNode = arena->cursorNode;
        if(!cursorNode) {
            return;
        }

        while(size > 0) {
            MemoryCursor *cursor = &cursorNode->cursor;

            u64 takenBytes = cursorTakenBytes(cursor);
            u64 toSubtract = MIN(size, takenBytes);
            u64 newOffset = takenBytes - toSubtract;

            size -= toSubtract;
            cursor->cursorPointer = cursor->basePointer + newOffset;
            cursorNode = cursorNode->next;
        }
    }
}

static void
arenaPopZero(Arena *arena, size_t size) {
    assert(arena);

    if(size) {
        MemoryCursorNode *cursorNode = arena->cursorNode;
        if(!cursorNode) {
            return;
        }

        while(size > 0) {
            MemoryCursor *cursor = &cursorNode->cursor;

            u64 takenBytes = cursorTakenBytes(cursor);
            u64 toSubtract = MIN(size, takenBytes);
            u64 newOffset = takenBytes - toSubtract;

            memset(cursor->basePointer + newOffset, 0, toSubtract);

            size -= toSubtract;
            cursor->cursorPointer = cursor->basePointer + newOffset;
            cursorNode = cursorNode->next;
        }
    }
}

static void
arenaPopTo(Arena *arena, size_t pos) {
    size_t currentPos = arenaPos(arena);
    if(currentPos > pos) {
        arenaPop(arena, currentPos - pos);
    }
}

static void
arenaPopToZero(Arena *arena, size_t pos) {
    size_t currentPos = arenaPos(arena);
    if(currentPos > pos) {
        arenaPopZero(arena, currentPos - pos);
    }
}

static void *
arenaPushZero(Arena *arena, size_t size) {
    void *memory = arenaPush(arena, size);
    memset(memory, 0, size);
    return memory;
}

#define arrayPush(a,T,c) ((T *)arenaPush((a), sizeof(T)*(c)))
#define arrayPushZero(a,T,c) ((T *)arenaPushZero((a), sizeof(T)*(c)))
#define structPush(a, T) ((T *)arenaPush((a), sizeof(T)))
#define bytesPush(a, c) (arenaPush((a), (c)))

#define javascriptPrintString(s) _javascriptPrintString((s), sizeof(s) - 1)

void _javascriptPrintString(char *s, u32 size) {
    String str = { .data = (u8 *)s, .size = size };
    javascriptPrintStringPtr(&str);
}

static String stringPushfv(Arena *arena, const char *format, va_list args);

static void logInfo(Arena *arena, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    String output = stringPushfv(arena, fmt, args);
    va_end(args);

    javascriptPrintStringPtr(&output);
}

static bool
isWhitespace(char c) {
    return (c == 0x20) | (c >= 0x09 & c <= 0x0d);
}

static char
toUpper(char c) {
    return c & ~(1 << 5);
}

static bool
isAlphabet(char c) {
    c = toUpper(c);
    return (c >= 'A' & c <= 'Z');
}

static bool
isDigit(char c) {
    return (c >= '0' & c <= '9');
}

static bool
isHexDigit(char c) {
    char upper = toUpper(c);
    return (c >= '0' & c <= '9') | (upper >= 'A' & upper <= 'F');
}

typedef struct SplitIterator
{
    char delim;
    u32 strLength;
    char *string;
    char *head;
} SplitIterator;

static String
stringPush(Arena *arena, size_t size) {
    String string = {};
    string.data = arrayPush(arena, u8, size);
    string.size = size;
    return string;
}

static String
stringCopy(Arena *arena, String v) {
    String string = {};
    string.data = arrayPush(arena, u8, v.size);
    string.size = v.size;
    memcpy(string.data, v.data, v.size);
    return string;
}

static s32
stringCompare(String a, String b){
    s32 result = 0;
    if(a.size != b.size) {
        return a.size < b.size ? -1 : 1;
    }

    for (size_t i = 0; i < a.size || i < b.size; i += 1){
        u8 ca = (i < a.size)?a.data[i]:0;
        u8 cb = (i < b.size)?b.data[i]:0;
        s32 dif = ((ca) - (cb));
        if (dif != 0){
            result = (dif > 0)?1:-1;
            break;
        }
    }
    return(result);
}

static String
stringTrimLeft(String str) {
    while(str.size > 0 && isWhitespace(str.data[0])) {
        str.data++;
        str.size--;
    }

    return str;
}

static String
stringTrim(String str) {
    while(str.size > 0 && isWhitespace(str.data[0])) {
        str.data++;
        str.size--;
    }

    while(str.size > 0 && isWhitespace(str.data[str.size - 1])) {
        str.size--;
    }

    return str;
}

static String
stringUnixLines(String a){
    size_t size = 0;

    for (size_t i = 0; i < a.size; i += 1) {
        if(a.data[i] != '\r') {
            a.data[size++] = a.data[i];
        }
    }

    a.size = size;
    return a;
}

static SplitIterator
stringSplit(String string, char delim) {
    SplitIterator it = { };

    it.string = (char *)string.data;
    it.strLength = string.size;
    it.head = (char *)string.data;
    it.delim = delim;

    return it;
}

static bool
stringMatch(String a, String b) {
    if(a.size == b.size) {
        for(u32 k = 0; k < a.size; k++) {
            if(a.data[k] != b.data[k]) {
                return false;
            }
        }
        return true;
    } else {
        return false;
    }
}

static bool
stringStartsWith(String string, String prefix) {
    if(string.size < prefix.size) {
        return false;
    } else {
        string.size = prefix.size;
        return stringMatch(string, prefix);
    }
}

static bool
stringEndsWith(String string, String suffix) {
    if(string.size < suffix.size) {
        return false;
    } else {
        string.data += (string.size - suffix.size);
        string.size = suffix.size;
        return stringMatch(string, suffix);
    }
}

static bool
stringIsInteger(String string) {
    for(u32 i = 0; i < string.size; i++) {
        if(!isDigit(string.data[i])) {
            return false;
        }
    }

    return true;
}

static u32
stringToInteger(String string) {
    u32 result = 0;

    assert(string.data[0] != '-');

    for(u32 i = 0; i < string.size; i++) {
        if(!isDigit(string.data[i])) {
            return result;
        }

        u32 digit = string.data[i] - '0';
        result = result * 10 + digit;
    }

    return result;
}

static String
stringNextInSplit(SplitIterator *it) {
    String result = { };

    size_t readLength = (size_t)(it->head - it->string);

    if(readLength < it->strLength) {
        char *head = it->head;
        result.data = (u8 *)head;
        size_t toRead = it->strLength - readLength;
        for(size_t i = 0; (i < toRead) && (head[0] != it->delim); i++) {
            head++;
            result.size += 1;
        }

        if(head[0] == it->delim) {
            head++;
        }
        it->head = head;
    } else {
        result.data = 0x0;
    }

    return result;
}

static String
stringConsumeIteratorIntoString(SplitIterator it)
{
    String result = { 0 };

    size_t readLength = (size_t)(it.head - it.string);
    size_t toRead = it.strLength - readLength;

    result.data = (u8 *)it.head;
    result.size = toRead;

    return result;
}

static String
subStringUntilDelimiter(String string, u32 startOffset, u32 endOffset, char delimiter)
{
    String result = { 0 };
    result.data = string.data + startOffset;
    result.size = endOffset - startOffset;

    while(((result.size-1) + startOffset) < string.size && result.data[result.size-1] != '\n') {
        result.size += 1;
    }

    return result;
}

static int
mr_vsnprintf(char *buffer, size_t size, const char *format, va_list args) {
    u32 written = 0;

    while(*format) {
        char c = *format++;

        if(c != '%') {
            if(written < size) {
                buffer[written] = c;
            }
            written += 1;
        } else {
            char next = *format++;
            if(next == 's') {
                char *s = va_arg(args, char *);
                while(*s) {
                    if(written < size) {
                        buffer[written] = *s;
                    }
                    written += 1;
                    s += 1;
                }
            } else if(next == 'S') {
                String s = va_arg(args, String);
                for(u32 i = 0; i < s.size; i++) {
                    if(written < size) {
                        buffer[written] = s.data[i];
                    }
                    written += 1;
                }
            } else if(next == 'd') {
                int n = va_arg(args, int);
                if(n < 0) {
                    if(written < size) {
                        buffer[written] = '-';
                    }
                    written += 1;
                    n = -n;
                }

                u32 divisor = 1;
                while(n / divisor >= 10) {
                    divisor *= 10;
                }

                while(divisor > 0) {
                    u32 digit = (n / divisor) % 10;
                    if(written < size) {
                        buffer[written] = '0' + digit;
                    }
                    written += 1;
                    divisor /= 10;
                }
            } else {
                assert(0);
            }
        }
    }

    return written;
}

static String
stringPushfv(Arena *arena, const char *format, va_list args) {
    va_list args2;
    va_copy(args2, args);

    u32 size = mr_vsnprintf(0x0, 0, format, args) + 1;
    String result = stringPush(arena, size);
    mr_vsnprintf((char *)result.data, size, format, args2);
    result.data[size - 1] = 0;
    return result;
}

static String
stringPushf(Arena *arena, const char *format, ...) {
    va_list args;
    va_start(args, format);
    String result = stringPushfv(arena, format, args);
    va_end(args);

    return result;
}

typedef struct ByteConsumer {
    u8 *data;
    u8 *head;
    u32 length;
} ByteConsumer;

static ByteConsumer
createByteConsumer(u8 *data, u32 length) {
    ByteConsumer result = {
        .data = data,
        .head = data,
        .length = length,
    };

    return result;
}

static u32
consumerGood(ByteConsumer *c) {
    return (c->head - c->data) < c->length;
}

static u32
consumerGoodN(ByteConsumer *c, u32 size) {
    return ((c->head - c->data) + size - 1) < c->length;
}

static void
advanceN(ByteConsumer *c, u32 size) {
    c->head += size;
}

static u8
consumeByte(ByteConsumer *c) {
    assert(consumerGood(c));
    return *(c->head++);
}

static u8
peekByte(ByteConsumer *c) {
    assert(consumerGood(c));
    return *(c->head);
}

static u16
consumeWord(ByteConsumer *c) {
    assert(consumerGoodN(c, 2));
    u16 result = *(u16 *)c->head;
    c->head += 2;
    return result;
}

static u16
peekWord(ByteConsumer *c) {
    assert(consumerGoodN(c, 2));
    return *(u16 *)c->head;
}

static u32
consumeDWord(ByteConsumer *c) {
    assert(consumerGoodN(c, 4));
    u32 result = *(u32 *)c->head;
    c->head += 4;
    return result;
}

static u32
peekDWord(ByteConsumer *c) {
    assert(consumerGoodN(c, 4));
    return *(u32 *)c->head;
}

static String
peekString(ByteConsumer *c, u32 size) {
    return (String){
        .data = c->head,
        .size = size,
    };
}

typedef struct StringNode {
    String string;
    struct StringNode *next;
} StringNode;

typedef struct StringList {
    StringNode *head;
    StringNode *last;
} StringList;

#ifdef WASM
#include <wasm_simd128.h>
#endif

static bool
isMultiplePO2(u32 a, u32 b) {
    if(a == 0 || b == 0) {
        return false;
    }
    return (a % b) == 0;
}

static void
processUtf8Byte(const char *string, s32 len, String *result, s32 *indexPointer) {
    static u8 followingByteCount[] = {
        0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0,
        1, 1, 1, 1, 2, 2, 3, 0xff,
    };

    s32 i = *indexPointer;

    if(string[i] & 0x80) {
        u8 c = string[i++];
        u8 first5Bits = (c >> 3) & 0x1f;
        u8 bytesToFollow = followingByteCount[first5Bits];
        u32 codePoint = c & (0x7f >> (bytesToFollow + 1));

        assert(i + bytesToFollow < len);
        for(int j = 0; j < bytesToFollow; j++) {
            u8 b = string[i++];
            assert((b & 0xc0) == 0x80);
            codePoint = (codePoint << 6) | (b & 0x3f);
        }

        if(codePoint > 0xffff) {
            result->data[result->size++] = 'u';
        }
        result->data[result->size++] = 'u';
    } else {
        result->data[result->size++] = string[i++] == '\r' ? '\n' : string[i - 1];
    }

    *indexPointer = i;
}

static String
neutralizeUnicode(const char *string, s32 len, Arena *arena) {
#ifdef WASM
    String result = {
        .data = arrayPush(arena, u8, len),
        .size = 0,
    };

    s32 i = 0;
    s32 SIMD_LANE_SIZE = sizeof(v128_t);

    for(; i < len && !isMultiplePO2(len - i, 16);) {
        processUtf8Byte(string, len, &result, &i);
    }

    v128_t utfMask = wasm_i8x16_splat(0x80);
    v128_t crMask = wasm_i8x16_splat('\r');
    v128_t nlMask = wasm_i8x16_splat('\n');
    for(; i < len - SIMD_LANE_SIZE;) {
        v128_t bytes = wasm_v128_load(&string[i]);
        v128_t utf8 = wasm_v128_and(bytes, utfMask);
        v128_t utf8cmp = wasm_i8x16_eq(utf8, utfMask);
        v128_t crCmp = wasm_i8x16_eq(bytes, crMask);
        u32 hasUtf8HighBit = wasm_v128_any_true(utf8cmp);
        v128_t picked = wasm_v128_bitselect(nlMask, bytes, crCmp);

        if(!hasUtf8HighBit) {
            wasm_v128_store(&result.data[result.size], picked);
            result.size += SIMD_LANE_SIZE;
            i += SIMD_LANE_SIZE;
        } else {
            processUtf8Byte(string, len, &result, &i);
        }
    }

    for(; i < len;) {
        processUtf8Byte(string, len, &result, &i);
    }

    return result;
#else
    static u8 followingByteCount[] = {
        0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0,
        1, 1, 1, 1, 2, 2, 3, 0xff,
    };

    String result = {
        .data = arrayPush(arena, u8, len),
        .size = 0,
    };

    for(int i = 0; i < len; i++) {
        if(string[i] & 0x80) {
            u8 c = string[i];
            u8 first5Bits = (c >> 3) & 0x1f;
            u8 bytesToFollow = followingByteCount[first5Bits];
            u32 codePoint = c & (0x7f >> (bytesToFollow + 1));

            assert(i + bytesToFollow < len);
            for(int j = 0; j < bytesToFollow; j++) {
                u8 b = string[++i];
                assert((b & 0xc0) == 0x80);
                codePoint = (codePoint << 6) | (b & 0x3f);
            }

            if(codePoint > 0xffff) {
                result.data[result.size++] = 'u';
            }
            result.data[result.size++] = 'u';
        } else {
            result.data[result.size++] = string[i];
        }
    }

    return result;
#endif
}

typedef struct U16Bucket {
    u16 values[256];
    u32 count;
    struct U16Bucket *next;
} U16Bucket;

typedef struct U16List {
    U16Bucket *first;
    U16Bucket *last;
    u32 count;
} U16List;

static U16Bucket *
listPushU16(U16List *list, u16 value, Arena *arena) {
    U16Bucket *bucket = 0x0;
    if(list->count == 0) {
        assert(list->first == 0x0 && list->last == 0x0);
        bucket = structPush(arena, U16Bucket);
        SLL_QUEUE_PUSH(list->first, list->last, bucket);
    } else {
        bucket = list->last;
    }

    if(bucket->count >= ARRAY_LENGTH(bucket->values)) {
        bucket = structPush(arena, U16Bucket);
        SLL_QUEUE_PUSH(list->first, list->last, bucket);
    }

    bucket->values[bucket->count++] = value;
    list->count += 1;

    return bucket;
}

static u16
listGetU16(U16List *list, u32 index) {
    assert(index < list->count);
    U16Bucket *bucket = list->first;
    for(u32 i = 0; i < index / ARRAY_LENGTH(bucket->values); i++) {
        bucket = bucket->next;
    }

    return bucket->values[index % ARRAY_LENGTH(bucket->values)];
}

static String
readFile(Arena *arena, const char *path) {
    int fd = open(path, O_RDONLY);
    if (fd < 0) {
        return (String){0};
    }

    struct stat st;
    if (fstat(fd, &st) != 0) {
        return (String){0};
    }

    u64 len = (u64)st.st_size;
    u8 *content = arrayPush(arena, u8, len + 1);

    ssize_t n = read(fd, content, len);
    assert(n == len);
    close(fd);

    content[len] = 0;
    return (String){ .data = content, .size = (u32)(n >= 0 ? n : 0) };
}

#define ANSI_RESET   "\x1b[0m"
#define ANSI_RED     "\x1b[31m"
#define ANSI_CYAN    "\x1b[36m"
#define ANSI_GREEN   "\x1b[32m"
#define ANSI_GREY    "\x1b[90m"
#define ANSI_MAGENTA "\x1b[35m"
#define ANSI_YELLOW  "\x1b[33m"
#define ANSI_BLUE    "\x1b[34m"

#define ANSI_CYAN_LIGHT    "\x1b[96m"
#define ANSI_GREEN_LIGHT   "\x1b[92m"
#define ANSI_MAGENTA_LIGHT "\x1b[95m"

typedef struct Buffer {
    u8 *data;
    size_t size;
    size_t capacity;
} Buffer;

typedef struct ByteConcatenator {
    Buffer buffers[32];
    u32 bufferCount;
    u32 activeBuffer;
    u32 capacity;
    Arena *arena;
} ByteConcatenator;

static ByteConcatenator
byteConcatenatorInit(Arena *arena, u32 capacity) {
    return (ByteConcatenator){
        .arena = arena,
        .capacity = capacity,
        .activeBuffer = ((u32)-1)
    };
}

static void
byteConcatenatorPush(ByteConcatenator *c) {
    u8 *data = arrayPush(c->arena, u8, c->capacity);

    c->buffers[c->bufferCount++] = (Buffer){ .data = data, .size = 0, .capacity = c->capacity };
    c->activeBuffer += 1;
}

static Buffer *
byteConcatenatorGetCurrentBuffer(ByteConcatenator *c) {
    if(c->activeBuffer == -1) {
        return 0x0;
    }

    return c->buffers + c->activeBuffer;
}

static void
byteConcatenatorPushBytes(ByteConcatenator *c, u8 *bytes, u32 count) {
    assert(count < c->capacity);

    Buffer *buffer = byteConcatenatorGetCurrentBuffer(c);
    if(buffer == 0x0 || buffer->size + count >= buffer->capacity) {
        byteConcatenatorPush(c);
        buffer = byteConcatenatorGetCurrentBuffer(c);
    }

    u8 *ptr = (buffer->data + buffer->size);
    memcpy(ptr, bytes, count);
    buffer->size += count;
}

static void
byteConcatenatorPushString(ByteConcatenator *c, String str) {
    byteConcatenatorPushBytes(c, str.data, str.size);
}

static Buffer
byteConcatenatorFinish(ByteConcatenator *c) {
    if(c->bufferCount == 1) {
        return byteConcatenatorGetCurrentBuffer(c)[0];
    }

    u32 totalMemoryUsed = 0;
    for(u32 i = 0; i < c->bufferCount; i++) {
        totalMemoryUsed += c->buffers[i].capacity;
    }

    u8 *data = arrayPush(c->arena, u8, totalMemoryUsed);
    u8 *head = data;
    u32 outputSize = 0;
    for(u32 i = 0; i < c->bufferCount; i++) {
        u32 count = c->buffers[i].size;
        outputSize += count;

        memcpy(head, c->buffers[i].data, count);
        head += count;
    }

    return (Buffer) { .data = data, .size = outputSize };
}

static u64
log10I(u64 v) {
    u64 result = 0;
    if (v >= 10000000000000000000ULL) return 19;
    if (v >= 1000000000000000000ULL) return 18;
    if (v >= 100000000000000000ULL) return 17;
    if (v >= 10000000000000000ULL) return 16;
    if (v >= 1000000000000000ULL) return 15;
    if (v >= 100000000000000ULL) return 14;
    if (v >= 10000000000000ULL) return 13;
    if (v >= 1000000000000ULL) return 12;
    if (v >= 100000000000ULL) return 11;
    if (v >= 10000000000ULL) return 10;
    if (v >= 1000000000ULL) return 9;
    if (v >= 100000000ULL) return 8;
    if (v >= 10000000ULL) return 7;
    if (v >= 1000000ULL) return 6;
    if (v >= 100000ULL) return 5;
    if (v >= 10000ULL) return 4;
    if (v >= 1000ULL) return 3;
    if (v >= 100ULL) return 2;
    if (v >= 10ULL) return 1;
    return 0;
}

static u64
base10DigitCount(u64 v) {
    return log10I(v) + 1;
}
