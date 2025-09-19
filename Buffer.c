#define _GNU_SOURCE
#include "Buffer.h"
#include <string.h>
#include <strings.h>

struct Buffer *bufferInit(int size)
{
    struct Buffer *buffer = (struct Buffer *)malloc(sizeof(struct Buffer));
    if (buffer != NULL)
    {
        buffer->data = (char *)malloc(size);
        buffer->capacity = size;
        buffer->readPos = buffer->writePos = 0;
        memset(buffer->data, 0, size);
    }
    return buffer;
}
void bufferDestory(struct Buffer *buf)
{
    if (buf != NULL)
    {
        if (buf->data != NULL)
        {
            free(buf->data);
        }
    }
    free(buf);
}
void bufferExtendRoom(struct Buffer *buffer, int size)
{
    // 1.内存够用----》不需要扩容
    if (bufferWriteableSize(buffer) >= size)
    {
        return;
    }
    // 2.内存需要合并才够用，不需要扩容
    // 剩余可写的内存+已读的内存>size
    if (buffer->readPos + bufferWriteableSize(buffer) >= size)
    {
        // 得到未读的内存大小
        int readable = bufferReadableSize(buffer);
        // 移动内存
        memcpy(buffer->data, buffer->data + buffer->readPos, readable);
        // 更新位置
        buffer->readPos = 0;
        buffer->writePos = readable;
    }
    // 3.内存不足，需要扩容
    else
    {
        void *temp = realloc(buffer->data, buffer->capacity + size);
        if (temp == NULL)
        {
            return;
        }
        memset(temp + buffer->capacity, 0, size);
        // 更新数据
        buffer->data = temp;
        buffer->capacity = buffer->capacity + size;
    }
}
int bufferReadableSize(struct Buffer *buffer)
{
    return buffer->writePos - buffer->readPos;
}
int bufferWriteableSize(struct Buffer *buffer)
{
    return buffer->capacity - buffer->writePos;
}
int bufferAppendData(struct Buffer *buffer, const char *data, int size)
{
    if (buffer == NULL || data == NULL || size <= 0)
    {
        return -1;
    }
    // 扩容
    bufferExtendRoom(buffer, size);
    memcpy(buffer->data + buffer->writePos, data, size);
    buffer->writePos += size;
    return 0;
}
int bufferAppendString(struct Buffer *buffer, const char *data)
{
    int size = strlen(data);
    int ret = bufferAppendData(buffer, data, size);
    return ret;
}
int bufferSocketRead(struct Buffer *buffer, int fd)
{
    // read / recv / readv readv比较高级，可以指定多个缓冲区

    struct iovec vec[2];
    // 初始化数组元素
    int writeable = bufferWriteableSize(buffer);
    vec[0].iov_base = buffer->data + buffer->writePos;
    vec[0].iov_len = writeable;
    char *tmpbuf = (char *)malloc(40960);
    vec[1].iov_base = tmpbuf;
    vec[1].iov_len = 40960;
    int result = readv(fd, vec, 2);
    if (result == -1)
    {
        return -1;
    }
    else if (result <= writeable)
    {
        buffer->writePos += result;
    }
    else
    {
        buffer->writePos = buffer->capacity;
        bufferAppendData(buffer, tmpbuf, result - writeable);
    }
    free(tmpbuf);
    return result;
}
char *bufferFindCRLF(struct Buffer *buffer)
{
    // strstr -->大字符串匹配子字符串(遇到\0结束)
    // memmem-->大数据块中匹配子数据块（需要指定数据块大小）
    char *ptr = memmem(buffer->data + buffer->readPos, bufferReadableSize(buffer), "\r\n", 2);
    return ptr;
}
int bufferSendData(struct Buffer *buffer, int socket)
{
    // 判断该buffer里面有多少未读的数据，未读的数据就说明我们还有多少未发送的数据
    int readable = bufferReadableSize(buffer);
    if (readable > 0)
    {
        int count = send(socket, buffer->data + buffer->readPos, readable,MSG_NOSIGNAL);
        if (count > 0)
        {
            buffer->readPos += count;
            usleep(1);
        }
        return count;
    }
    return 0;
}