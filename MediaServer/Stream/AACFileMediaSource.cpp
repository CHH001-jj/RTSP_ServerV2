﻿#include <string.h>
#include "AACFileMediaSource.h"
#include "../Utils/Log.h"
#include <iostream>

AACFileMeidaSource* AACFileMeidaSource::createNew(UsageEnvironment* env, const std::string& file)
{
    return new AACFileMeidaSource(env, file);
}
double getSampleRate(const std::string& filePath) {
    // Construct the ffprobe command to extract the sample rate of the audio stream
    std::string command = "ffprobe -select_streams a:0 -show_entries stream=sample_rate -of default=noprint_wrappers=1:nokey=1 " + filePath + " 2>/dev/null";
    std::array<char, 128> buffer;
    std::string result;
    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(command.c_str(), "r"), pclose);

    if (!pipe) {
        std::cerr << "Failed to run ffprobe command." << std::endl;
        return 0;
    }

    // Read the output from ffprobe
    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
        result += buffer.data();
    }

    // Parse the output (e.g., "44100")
    if (!result.empty()) {
        try {
            double sampleRate = std::stod(result);
            double frameRate = sampleRate / 1024.0; // Calculate frame rate
            return frameRate;
        } catch (const std::exception& e) {
            std::cerr << "Failed to parse sample rate: " << e.what() << std::endl;
        }
    }

    return 0;
}
AACFileMeidaSource::AACFileMeidaSource(UsageEnvironment* env, const std::string& file) :
    MediaSource(env){

    mSourceName = file;
    mFile = fopen(file.c_str(), "rb");

    setFps(getSampleRate(file));

    for(int i = 0; i < DEFAULT_FRAME_NUM; ++i){
        mEnv->threadPool()->addTask(mTask);
    }
}

AACFileMeidaSource::~AACFileMeidaSource()
{
    fclose(mFile);
}

void AACFileMeidaSource::handleTask()
{
    std::lock_guard <std::mutex> lck(mMtx);

    if(mFrameInputQueue.empty())
        return;
    MediaFrame* frame = mFrameInputQueue.front();

    frame->mSize = getFrameFromAACFile(frame->temp, FRAME_MAX_SIZE);
    if (frame->mSize < 0) {
        return;
    }
    frame->mBuf = frame->temp;

    mFrameInputQueue.pop();
    mFrameOutputQueue.push(frame);
}

bool AACFileMeidaSource::parseAdtsHeader(uint8_t* in, struct AdtsHeader* res)
{
    memset(res,0,sizeof(*res));

    if ((in[0] == 0xFF)&&((in[1] & 0xF0) == 0xF0))
    {
        res->id = ((unsigned int) in[1] & 0x08) >> 3;
        res->layer = ((unsigned int) in[1] & 0x06) >> 1;
        res->protectionAbsent = (unsigned int) in[1] & 0x01;
        res->profile = ((unsigned int) in[2] & 0xc0) >> 6;
        res->samplingFreqIndex = ((unsigned int) in[2] & 0x3c) >> 2;
        res->privateBit = ((unsigned int) in[2] & 0x02) >> 1;
        res->channelCfg = ((((unsigned int) in[2] & 0x01) << 2) | (((unsigned int) in[3] & 0xc0) >> 6));
        res->originalCopy = ((unsigned int) in[3] & 0x20) >> 5;
        res->home = ((unsigned int) in[3] & 0x10) >> 4;
        res->copyrightIdentificationBit = ((unsigned int) in[3] & 0x08) >> 3;
        res->copyrightIdentificationStart = (unsigned int) in[3] & 0x04 >> 2;
        res->aacFrameLength = (((((unsigned int) in[3]) & 0x03) << 11) |
                                (((unsigned int)in[4] & 0xFF) << 3) |
                                    ((unsigned int)in[5] & 0xE0) >> 5) ;
        res->adtsBufferFullness = (((unsigned int) in[5] & 0x1f) << 6 |
                                        ((unsigned int) in[6] & 0xfc) >> 2);
        res->numberOfRawDataBlockInFrame = ((unsigned int) in[6] & 0x03);

        return true;
    }
    else
    {
        LOGE("failed to parse adts header");
        return false;
    }
}

int AACFileMeidaSource::getFrameFromAACFile(uint8_t* buf, int size)
{
    if (!mFile) {
        return -1;
    }

    uint8_t tmpBuf[7];
    int ret;
    ret = fread(tmpBuf,1,7,mFile);
    if(ret <= 0)
    {
        fseek(mFile,0,SEEK_SET);
        ret = fread(tmpBuf,1,7,mFile);
        if(ret <= 0)
        {
            return -1;
        }
    }

    if(!parseAdtsHeader(tmpBuf, &mAdtsHeader))
    {
        return -1;
    }

    if(static_cast<int>(mAdtsHeader.aacFrameLength) > size)
        return -1;

    memcpy(buf, tmpBuf, 7);
    ret = fread(buf+7,1,mAdtsHeader.aacFrameLength-7,mFile);
    if(ret < 0)
    {
        LOGE("read error");
        return -1;
    }

    return mAdtsHeader.aacFrameLength;
}