typedef struct Walker Walker;

static void walkerEnqueueFile(Walker *w, String parent, String path);
static void walkerPushDirectory(Walker *w, String parent, String path);

enum FileType_Enum {
    FileType_Unknown,
    FileType_Directory,
    FileType_File,
};

typedef u32 FileType;

typedef struct Entry {
    String name;
    FileType type;
    u32 entryLength;
} Entry;

#if __linux__
#include <sys/stat.h>
#include <pthread.h>
typedef void * ThreadReturnType;
typedef ThreadReturnType (*ThreadFunction)(void *);
typedef pthread_t ThreadHandle;
typedef pthread_mutex_t Mutex;

typedef struct stat stat64_t;

static stat64_t
fileStat(const char *path) {
    struct stat out;
    stat(path, &out);
    return out;
}

static ThreadHandle
createThread(ThreadFunction fn, void *user_pointer) {
    ThreadHandle handle;
    pthread_create(&handle, NULL, fn, user_pointer);
    return handle;
}

static void
joinAndDestroyThread(ThreadHandle handle) {
    pthread_join(handle, NULL);
}

static Mutex
createMutex() {
    Mutex mutex;
    pthread_mutex_init(&mutex, NULL);
    return mutex;
}

static void
destroyMutex(Mutex *mutex) {
    pthread_mutex_destroy(mutex);
}

static void
lockMutex(Mutex *mutex) {
    pthread_mutex_lock(mutex);
}

static void
unlockMutex(Mutex *mutex) {
    pthread_mutex_unlock(mutex);
}

static u64
getCurrentThreadId() {
    return (u64)pthread_self();
}

#include <sys/sysinfo.h>

static u32
getProcessorCount() {
    return get_nprocs();
}

#include <dirent.h>

typedef struct KernelEntry {
    u64 inode;
    u64 offset;
    u16 entryLength;
    u8 fileType;
    u8 name[];
} KernelEntry;

long getdents64(unsigned int fd, void *dirp, unsigned int count);

static Entry
decodeEntry(void *data) {
    KernelEntry *kernelEntry = (KernelEntry *)data;

    Entry entry = { 0 };

    entry.entryLength = kernelEntry->entryLength;
    u32 nameLength = strlen((char *)kernelEntry->name);

    entry.name = (String){
        .data = kernelEntry->name,
        .size = nameLength,
    };

    switch(kernelEntry->fileType) {
        case DT_DIR: entry.type = FileType_Directory; break;
        case DT_REG: entry.type = FileType_File; break;
        default: entry.type = FileType_Unknown; break;
    }

    return entry;
}

static void
walkDirectory(Walker *w, Arena *arena, String path) {
    u32 dirDataCapacity = 64 * Kilobyte;
    u8 *dirData = arrayPushZero(arena, u8, dirDataCapacity);

    int fd = open((char *)path.data, O_RDONLY);
    if(fd == -1) {
        printf("Failed to open dir: %s\n", path.data);
        return;
    }

    int returned = -1;
    long base = 0;
    do {
        returned = getdents64(fd, dirData, dirDataCapacity);
        void *head = dirData;
        void *end = head + returned;
        while(head < end) {
            Entry entry = decodeEntry(head);
            head += entry.entryLength;

            if(stringMatch(entry.name, LIT_TO_STR(".")) || stringMatch(entry.name, LIT_TO_STR(".."))) {
                continue;
            }

            switch(entry.type) {
                case FileType_File: {
                    walkerEnqueueFile(w, path, entry.name);
                } break;
                case FileType_Directory: {
                    walkerPushDirectory(w, path, entry.name);
                } break;
            }
        }
    } while(returned > 0);
    close(fd);
}

#elif __APPLE__
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/sysctl.h>

typedef int FileHandle;
typedef struct stat stat64_t;

static u32
getProcessorCount() {
    int mib[] = { CTL_HW, HW_NCPU };

    int numCPU;
    size_t len = sizeof(numCPU);
    sysctl(mib, 2, &numCPU, &len, NULL, 0);

    return numCPU;
}

static stat64_t
fileStat(const char *path) {
    struct stat out;
    stat(path, &out);
    return out;
}

static FileHandle
openFile(const char *filepath) {
    return open(filepath, O_RDWR | O_CREAT);
}

/*static void*/
/*readFile(FileHandle handle, u8 *out, u64 length) {*/
/*    read(handle, out, length);*/
/*}*/

static void
writeFile(FileHandle handle, u8 *data, u64 length) {
    write(handle, data, length);
}

static void
closeFile(FileHandle handle) {
    close(handle);
}

#include <sys/attr.h>

static Entry
decodeAttr(void *data) {
    Entry entry = { 0 };

    void *head = data;
    entry.entryLength = ((u32 *)head)[0];
    head += sizeof(u32);

    // Just skiping
    head += sizeof(attribute_set_t);

    attrreference_t *name = (attrreference_t *)head;
    entry.name = (String){
        .data = head + name->attr_dataoffset,
        .size = name->attr_length - 1,
    };
    head += sizeof(attrreference_t);

    u32 *fileType = (u32 *)head;
    head += sizeof(u32);

    switch(fileType[0]) {
        case 0x1: entry.type = FileType_File; break;
        case 0x2: entry.type = FileType_Directory; break;
        default: entry.type = FileType_Unknown; break;
    }

    return entry;
}

static void
macosWalkDirectoryPublic(Walker *w, Arena *arena, String path) {
    u32 dirDataCapacity = 128 * Kilobyte;
    u8 *dirData = arrayPushZero(arena, u8, dirDataCapacity);

    struct attrlist attrList = {
        .bitmapcount = ATTR_BIT_MAP_COUNT,
        .reserved    = 0,
        .commonattr  = ATTR_CMN_RETURNED_ATTRS | ATTR_CMN_NAME | ATTR_CMN_OBJTYPE,
    };

    int fd = open((char *)path.data, O_RDONLY);
    if(fd == -1) {
        printf("Failed to open dir: %s\n", path.data);
        return;
    }

    int returned = -1;
    do {
        returned = getattrlistbulk(fd, &attrList, dirData, dirDataCapacity, 0);
        void *entryHead = dirData;
        for(int i = 0; i < returned; i++) {
            Entry entry = decodeAttr(entryHead);
            entryHead += entry.entryLength;
            switch(entry.type) {
                case FileType_File: {
                    walkerEnqueueFile(w, path, entry.name);
                    break;
                }
                case FileType_Directory: {
                    walkerPushDirectory(w, path, entry.name);
                    break;
                }
            }
        }
    } while(returned > 0);
    close(fd);
}

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
static int
getdirentries64(int fd, void *buf, u32 nbytes, long *basep) {
    return syscall(344, fd, buf, nbytes, basep);
}

typedef struct KernelEntry {
    u64 inode;
    u64 seekOffset;
    u16 recordLength;
    u16 nameLength;
    u8 fileType;
    u8 name[256 + 1];
} KernelEntry;

static Entry
decodeEntry(void *data) {
    KernelEntry *kernelEntry = (KernelEntry *)data;

    Entry entry = { 0 };

    entry.entryLength = kernelEntry->recordLength;
    entry.name = (String){
        .data = kernelEntry->name,
        .size = kernelEntry->nameLength,
    };

    switch(kernelEntry->fileType) {
        case 4: entry.type = FileType_Directory; break;
        case 8: entry.type = FileType_File; break;
        default: entry.type = FileType_Unknown; break;
    }

    return entry;
}

static void
macosWalkDirectoryPrivate(Walker *w, Arena *arena, String path) {
    u32 dirDataCapacity = 64 * Kilobyte;
    u8 *dirData = arrayPushZero(arena, u8, dirDataCapacity);

    int fd = open((char *)path.data, O_RDONLY);
    if(fd == -1) {
        printf("Failed to open dir: %s\n", path.data);
        return;
    }

    int returned = -1;
    long base = 0;
    do {
        returned = getdirentries64(fd, dirData, dirDataCapacity, &base);
        void *head = dirData;
        void *end = head + returned;
        while(head < end) {
            Entry entry = decodeEntry(head);
            head += entry.entryLength;

            switch(entry.type) {
                case FileType_File: {
                    walkerEnqueueFile(w, path, entry.name);
                } break;
                case FileType_Directory: {
                    if(stringMatch(entry.name, LIT_TO_STR(".")) || stringMatch(entry.name, LIT_TO_STR(".."))) {
                        continue;
                    }

                    walkerPushDirectory(w, path, entry.name);
                } break;
            }
        }
    } while(returned > 0);
    close(fd);
}

static void
walkDirectory(Walker *w, Arena *arena, String path) {
    macosWalkDirectoryPrivate(w, arena, path);
}

#endif
