#include <iostream>
#include <fstream>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <cstring>

const char* SHARED_MEMORY_OBJECT_NAME = "/my_shared_memory";
const char* LOG_POINTER_FILE_NAME = "log_pointer.dat";
const int SHARED_MEMORY_SIZE = 1024; // 共享内存大小为1KB

// 初始化或恢复log指针
int initializeOrRecoverLogPointer() {
    std::fstream logPointerFile;
    logPointerFile.open(LOG_POINTER_FILE_NAME, std::ios::in | std::ios::out | std::ios::binary);
    
    int logPointer = 0;
    if (logPointerFile.is_open()) {
        // 文件存在，读取log指针
        logPointerFile.read(reinterpret_cast<char*>(&logPointer), sizeof(logPointer));
    } else {
        // 文件不存在，创建文件并初始化log指针为0
        logPointerFile.open(LOG_POINTER_FILE_NAME, std::ios::out | std::ios::binary);
        logPointerFile.write(reinterpret_cast<char*>(&logPointer), sizeof(logPointer));
    }
    logPointerFile.close();
    return logPointer;
}

// 更新log指针
void updateLogPointer(int logPointer) {
    std::ofstream logPointerFile;
    logPointerFile.open(LOG_POINTER_FILE_NAME, std::ios::out | std::ios::binary | std::ios::trunc);
    logPointerFile.write(reinterpret_cast<char*>(&logPointer), sizeof(logPointer));
    logPointerFile.close();
}

int main() {
    // 初始化或恢复log指针
    int logPointer = initializeOrRecoverLogPointer();

    // 创建或打开共享内存对象
    int shm_fd = shm_open(SHARED_MEMORY_OBJECT_NAME, O_CREAT | O_RDWR, 0666);
    if (shm_fd == -1) {
        std::cerr << "Error opening shared memory object." << std::endl;
        return 1;
    }

    // 调整共享内存对象的大小
    if (ftruncate(shm_fd, SHARED_MEMORY_SIZE) == -1) {
        std::cerr << "Error setting size of shared memory object." << std::endl;
        return 1;
    }

    // 映射共享内存对象
    void* shared_memory = mmap(0, SHARED_MEMORY_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (shared_memory == MAP_FAILED) {
        std::cerr << "Error mapping shared memory object." << std::endl;
        return 1;
    }

    // 使用共享内存
    // 示例：写入一些数据
    const char* data = "Hello, Shared Memory!";
    memcpy(static_cast<char*>(shared_memory) + logPointer, data, strlen(data));

    // 更新log指针
    logPointer += strlen(data);
    updateLogPointer(logPointer);

    // 读取共享内存中的数据
    std::cout << "Read from shared memory: " << static_cast<char*>(shared_memory) << std::endl;

    // 解除映射
    if (munmap(shared_memory, SHARED_MEMORY_SIZE) == -1) {
        std::cerr << "Error unmapping shared memory object." << std::endl;
        return 1;
    }

    // 关闭共享内存对象
    close(shm_fd);

    return 0;
}
