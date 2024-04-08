package ShareMemory;

import java.io.*;
import java.nio.*;
import java.nio.channels.FileChannel;

public class SharedMemoryExample {

    private static final String MEMORY_FILE = "shared_memory.dat";
    private static final String LOG_POINTER_FILE = "log_pointer.dat";
    private static final int MEMORY_SIZE = 1024; // 假设共享内存大小为1KB

    public static void main(String[] args) {
        try {
            // 初始化或恢复log指针
            int logPointer = initializeOrRecoverLogPointer();

            // 映射共享内存
            MappedByteBuffer sharedMemory = mapSharedMemory(MEMORY_FILE, MEMORY_SIZE);

            // 使用共享内存
            // 示例：写入一些数据
            String data = "Hello, Shared Memory!";
            sharedMemory.position(logPointer);
            sharedMemory.put(data.getBytes());

            // 更新log指针
            updateLogPointer(data.length());

            // 读取共享内存中的数据
            sharedMemory.position(0);
            byte[] buffer = new byte[data.length()];
            sharedMemory.get(buffer);
            System.out.println("Read from shared memory: " + new String(buffer));

        } catch (IOException e) {
            e.printStackTrace();
        }
    }

    private static int initializeOrRecoverLogPointer() throws IOException {
        File logPointerFile = new File(LOG_POINTER_FILE);
        if (logPointerFile.exists()) {
            try (DataInputStream dis = new DataInputStream(new FileInputStream(logPointerFile))) {
                return dis.readInt();
            }
        } else {
            return 0;
        }
    }

    private static MappedByteBuffer mapSharedMemory(String filename, int size) throws IOException {
        RandomAccessFile memoryFile = new RandomAccessFile(filename, "rw");
        FileChannel channel = memoryFile.getChannel();
        return channel.map(FileChannel.MapMode.READ_WRITE, 0, size);
    }

    private static void updateLogPointer(int dataLength) throws IOException {
        try (RandomAccessFile logFile = new RandomAccessFile(LOG_POINTER_FILE, "rw");
             FileChannel logChannel = logFile.getChannel()) {
            MappedByteBuffer logBuffer = logChannel.map(FileChannel.MapMode.READ_WRITE, 0, 4);
            int currentPointer = logBuffer.getInt(0);
            logBuffer.putInt(0, currentPointer + dataLength);
        }
    }
}
