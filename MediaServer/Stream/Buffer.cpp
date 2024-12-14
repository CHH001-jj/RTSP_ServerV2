#include "Buffer.h"
#include "../EventManager/SocketsOps.h"

const int Buffer::initialSize = 1024;
const char* Buffer::kCRLF = "\r\n";

Buffer::Buffer() :
    mBufferSize(initialSize),
    mReadIndex(0),
    mWriteIndex(0)
{
    mBuffer = (char*)malloc(mBufferSize);
}

Buffer::~Buffer()
{
    free(mBuffer);
}


int Buffer::read(int fd)
{
    char extrabuf[65536];
    const int writable = writableBytes();
    const int n = ::recv(fd, extrabuf, sizeof(extrabuf), 0);
    if (n <= 0) {
        return -1;
    }
    else if (n <= writable)
    {
        std::copy(extrabuf, extrabuf + n, beginWrite()); //拷贝数据
        mWriteIndex += n;

    }
    else
    {
        std::copy(extrabuf, extrabuf + writable, beginWrite()); //拷贝数据
        mWriteIndex += writable;
        append(extrabuf+ writable, n - writable);
    }
    return n;
}

int Buffer::write(int fd)
{
    return sockets::write(fd, peek(), readableBytes());
}