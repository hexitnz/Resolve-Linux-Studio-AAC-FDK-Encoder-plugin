#!/bin/bash
# AAC Encoder Plugin - Fixed version based on working implementation
# Uses the buffer allocation pattern from toxblh's working plugin

set -e

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

echo -e "${BLUE}============================================${NC}"
echo -e "${BLUE}AAC Encoder Plugin (WORKING VERSION)${NC}"
echo -e "${BLUE}============================================${NC}"
echo ""

if [ ! -d "/opt/resolve/Developer/CodecPlugin" ]; then
    echo -e "${RED}[✗]${NC} DaVinci Resolve SDK not found!"
    exit 1
fi

if ! pkg-config --exists fdk-aac; then
    echo -e "${RED}[✗]${NC} libfdk-aac not found!"
    exit 1
fi

echo -e "${GREEN}[✓]${NC} Prerequisites OK"

WORK_DIR="$HOME/aac_working_plugin"
rm -rf "$WORK_DIR"
mkdir -p "$WORK_DIR"
cd "$WORK_DIR"

cp -r /opt/resolve/Developer/CodecPlugin/Examples/x264_encoder_plugin/wrapper .
cp -r /opt/resolve/Developer/CodecPlugin/Examples/x264_encoder_plugin/include .

cat > plugin.h << 'EOF'
#pragma once
#include "wrapper/plugin_api.h"
EOF

cat > aac_encoder.h << 'EOF'
#pragma once
#include <memory>
#include <vector>
#include "wrapper/plugin_api.h"
#include <fdk-aac/aacenc_lib.h>

using namespace IOPlugin;

class UIAACSettingsController;

class AACEncoder : public IPluginCodecRef
{
public:
    static const uint8_t s_UUID[];
    
    AACEncoder();
    ~AACEncoder();
    
    static StatusCode s_RegisterCodecs(HostListRef* p_pList);
    static StatusCode s_GetEncoderSettings(HostPropertyCollectionRef* p_pValues, HostListRef* p_pSettingsList);

protected:
    virtual void DoFlush() override;
    virtual StatusCode DoInit(HostPropertyCollectionRef* p_pProps) override;
    virtual StatusCode DoOpen(HostBufferRef* p_pBuff) override;
    virtual StatusCode DoProcess(HostBufferRef* p_pBuff) override;

private:
    void AddPCMToRingBuffer(const float** planarPCM, size_t samples);
    bool IsRingBufferFull() const;
    void GetFrameFromRingBuffer(int16_t* out, size_t samples);
    void PadAndFlushRingBuffer();
    void ResetRingBuffer();
    
    std::unique_ptr<UIAACSettingsController> m_pSettings;
    HANDLE_AACENCODER m_hEncoder;
    
    uint32_t m_outputBitDepth;
    uint32_t m_SampleRate;
    uint32_t m_NumChannels;
    int m_EncoderFrameSize;
    int64_t m_OutputPTS;  // Track output PTS
    
    std::vector<std::vector<float>> m_pcmRingBuffer;
    size_t m_ringBufferFill;
    std::vector<uint8_t> m_OutputBuffer;
};
EOF

cat > aac_encoder.cpp << 'EOF'
#include "aac_encoder.h"
#include <cstring>
#include <algorithm>

const uint8_t AACEncoder::s_UUID[] = { 
    0x9f, 0xb2, 0xe4, 0x73, 0xda, 0x86, 0x5c, 0x4d, 
    0xb3, 0x5f, 0xe6, 0xa2, 0xff, 0xc3, 0xed, 0xe8 
};

class UIAACSettingsController
{
public:
    UIAACSettingsController() { m_BitRate = 192; }
    
    void Load(IPropertyProvider* p_pValues)
    {
        p_pValues->GetINT32("aac_enc_bitrate", m_BitRate);
    }

    StatusCode Render(HostListRef* p_pSettingsList)
    {
        HostUIConfigEntryRef item("aac_enc_bitrate");
        item.MakeSlider("Bit Rate", "kbps", m_BitRate, 128, 320, 192);
        item.SetTriggersUpdate(true);
        
        if (!item.IsSuccess() || !p_pSettingsList->Append(&item))
            return errFail;
        
        return errNone;
    }

    int32_t GetBitRate() const { return m_BitRate; }

private:
    int32_t m_BitRate;
};

StatusCode AACEncoder::s_RegisterCodecs(HostListRef* p_pList)
{
    HostPropertyCollectionRef codecInfo;
    if (!codecInfo.IsValid())
        return errAlloc;

    codecInfo.SetProperty(pIOPropUUID, propTypeUInt8, AACEncoder::s_UUID, 16);

    const char* pCodecName = "AAC (FDK-AAC)";
    codecInfo.SetProperty(pIOPropName, propTypeString, pCodecName, strlen(pCodecName));

    uint32_t val = 'aac ';
    codecInfo.SetProperty(pIOPropFourCC, propTypeUInt32, &val, 1);

    val = mediaAudio;
    codecInfo.SetProperty(pIOPropMediaType, propTypeUInt32, &val, 1);

    val = dirEncode;
    codecInfo.SetProperty(pIOPropCodecDirection, propTypeUInt32, &val, 1);

    std::vector<uint32_t> bitDepths({16, 24});
    codecInfo.SetProperty(pIOPropBitDepth, propTypeUInt32, bitDepths.data(), bitDepths.size());

    std::vector<uint32_t> samplingRates({44100, 48000});
    codecInfo.SetProperty(pIOPropSamplingRate, propTypeUInt32, samplingRates.data(), samplingRates.size());

    std::vector<std::string> containerVec{"mp4", "mov", "mkv"};
    std::string valStrings;
    for (size_t i = 0; i < containerVec.size(); ++i)
    {
        valStrings.append(containerVec[i]);
        if (i < containerVec.size() - 1)
            valStrings.append(1, '\0');
    }

    codecInfo.SetProperty(pIOPropContainerList, propTypeString, valStrings.c_str(), valStrings.size());

    if (!p_pList->Append(&codecInfo))
        return errFail;

    g_Log(logLevelInfo, "AAC (FDK-AAC) Plugin :: Registered");
    return errNone;
}

StatusCode AACEncoder::s_GetEncoderSettings(HostPropertyCollectionRef* p_pValues, HostListRef* p_pSettingsList)
{
    UIAACSettingsController settings;
    settings.Load(p_pValues);
    return settings.Render(p_pSettingsList);
}

AACEncoder::AACEncoder()
    : m_hEncoder(nullptr)
    , m_outputBitDepth(16)
    , m_SampleRate(48000)
    , m_NumChannels(2)
    , m_EncoderFrameSize(0)
    , m_OutputPTS(0)
    , m_ringBufferFill(0)
{
    g_Log(logLevelInfo, "AAC Encoder :: Constructor");
}

AACEncoder::~AACEncoder()
{
    if (m_hEncoder)
    {
        aacEncClose(&m_hEncoder);
        m_hEncoder = nullptr;
    }
    g_Log(logLevelInfo, "AAC Encoder :: Destructor");
}

StatusCode AACEncoder::DoInit(HostPropertyCollectionRef* p_pProps)
{
    p_pProps->GetUINT32(pIOPropBitDepth, m_outputBitDepth);
    p_pProps->GetUINT32(pIOPropSamplingRate, m_SampleRate);
    p_pProps->GetUINT32(pIOPropNumChannels, m_NumChannels);

    if (m_outputBitDepth != 16 && m_outputBitDepth != 24)
    {
        g_Log(logLevelError, "AAC Plugin :: Unsupported bit depth: %d", m_outputBitDepth);
        return errFail;
    }

    g_Log(logLevelInfo, "AAC Plugin :: Init - %d Hz, %d ch, %d-bit", 
          m_SampleRate, m_NumChannels, m_outputBitDepth);

    return errNone;
}

StatusCode AACEncoder::DoOpen(HostBufferRef* p_pBuff)
{
    m_pSettings.reset(new UIAACSettingsController());
    m_pSettings->Load(p_pBuff);

    // Open FDK-AAC
    if (aacEncOpen(&m_hEncoder, 0, m_NumChannels) != AACENC_OK)
    {
        g_Log(logLevelError, "AAC Plugin :: Failed to open encoder");
        return errFail;
    }

    aacEncoder_SetParam(m_hEncoder, AACENC_AOT, AOT_AAC_LC);
    aacEncoder_SetParam(m_hEncoder, AACENC_SAMPLERATE, m_SampleRate);
    aacEncoder_SetParam(m_hEncoder, AACENC_CHANNELMODE, m_NumChannels == 1 ? MODE_1 : MODE_2);
    aacEncoder_SetParam(m_hEncoder, AACENC_BITRATE, m_pSettings->GetBitRate() * 1000);
    aacEncoder_SetParam(m_hEncoder, AACENC_TRANSMUX, TT_MP4_RAW);

    if (aacEncEncode(m_hEncoder, nullptr, nullptr, nullptr, nullptr) != AACENC_OK)
    {
        g_Log(logLevelError, "AAC Plugin :: Failed to initialize encoder");
        return errFail;
    }

    AACENC_InfoStruct info;
    aacEncInfo(m_hEncoder, &info);
    
    m_EncoderFrameSize = info.frameLength;
    m_OutputBuffer.resize(info.maxOutBufBytes);
    m_OutputPTS = 0;  // Initialize PTS counter
    
    // Init ring buffer
    m_pcmRingBuffer.clear();
    m_pcmRingBuffer.resize(m_NumChannels);
    for (size_t ch = 0; ch < m_NumChannels; ++ch)
    {
        m_pcmRingBuffer[ch].resize(m_EncoderFrameSize, 0.0f);
    }
    m_ringBufferFill = 0;

    const uint64_t bitRate = static_cast<uint64_t>(m_pSettings->GetBitRate()) * 1000;
    p_pBuff->SetProperty(pIOPropBitRate, propTypeUInt64, &bitRate, 1);

    g_Log(logLevelInfo, "AAC Plugin :: Opened - %d kbps, frame size: %d", 
          m_pSettings->GetBitRate(), m_EncoderFrameSize);

    return errNone;
}

void AACEncoder::DoFlush()
{
    g_Log(logLevelInfo, "AAC Plugin :: Flush");
}

StatusCode AACEncoder::DoProcess(HostBufferRef* p_pBuff)
{
    if (!m_hEncoder) 
    {
        g_Log(logLevelError, "AAC Plugin :: DoProcess called but encoder not initialized");
        return errFail;
    }

    if (p_pBuff != nullptr)
    {
        g_Log(logLevelInfo, "AAC Plugin :: DoProcess called with buffer");
        
        char* pBuf = nullptr;
        size_t bufSize = 0;
        if (!p_pBuff->LockBuffer(&pBuf, &bufSize))
        {
            g_Log(logLevelWarn, "AAC Plugin :: Failed to lock input buffer");
            return errNone;
        }
        
        if (bufSize == 0)
        {
            g_Log(logLevelWarn, "AAC Plugin :: Input buffer size is 0");
            p_pBuff->UnlockBuffer();
            return errNone;
        }
        
        g_Log(logLevelInfo, "AAC Plugin :: Processing %zu bytes", bufSize);

        g_Log(logLevelInfo, "AAC Plugin :: Processing %zu bytes", bufSize);

        uint32_t inputBitDepth = m_outputBitDepth;
        p_pBuff->GetUINT32(pIOPropBitDepth, inputBitDepth);

        int bytesPerSample = (inputBitDepth == 16) ? 2 : 3;
        int totalSamples = bufSize / (m_NumChannels * bytesPerSample);
        
        // Convert to planar float
        std::vector<std::vector<float>> planarPCM(m_NumChannels, std::vector<float>(totalSamples, 0.0f));
        
        if (inputBitDepth == 16)
        {
            int16_t* src = (int16_t*)pBuf;
            for (int i = 0; i < totalSamples; ++i)
            {
                for (int ch = 0; ch < (int)m_NumChannels; ++ch)
                {
                    planarPCM[ch][i] = src[i * m_NumChannels + ch] / 32768.0f;
                }
            }
        }
        else if (inputBitDepth == 24)
        {
            unsigned char* src = (unsigned char*)pBuf;
            for (int i = 0; i < totalSamples; ++i)
            {
                for (int ch = 0; ch < (int)m_NumChannels; ++ch)
                {
                    int idx = (i * m_NumChannels + ch) * 3;
                    int32_t sample = (src[idx + 2] << 24) | (src[idx + 1] << 16) | (src[idx] << 8);
                    sample >>= 8;
                    planarPCM[ch][i] = sample / 8388608.0f;
                }
            }
        }

        int sampleIdx = 0;
        while (sampleIdx < totalSamples)
        {
            int chunk = std::min((int)(m_EncoderFrameSize - m_ringBufferFill), totalSamples - sampleIdx);
            
            std::vector<const float*> chunkPtrs(m_NumChannels);
            for (size_t ch = 0; ch < m_NumChannels; ++ch)
            {
                chunkPtrs[ch] = &planarPCM[ch][sampleIdx];
            }
            
            AddPCMToRingBuffer(chunkPtrs.data(), chunk);
            
            if (IsRingBufferFull())
            {
                // Convert float to int16 for FDK-AAC
                std::vector<int16_t> int16Buffer(m_EncoderFrameSize * m_NumChannels);
                for (size_t i = 0; i < m_EncoderFrameSize; ++i)
                {
                    for (size_t ch = 0; ch < m_NumChannels; ++ch)
                    {
                        float sample = m_pcmRingBuffer[ch][i];
                        if (sample > 1.0f) sample = 1.0f;
                        if (sample < -1.0f) sample = -1.0f;
                        int16Buffer[i * m_NumChannels + ch] = (int16_t)(sample * 32767.0f);
                    }
                }
                
                // Encode with FDK-AAC
                void* inBuf = int16Buffer.data();
                int inBufIds = IN_AUDIO_DATA;
                int inBufSize = int16Buffer.size() * sizeof(int16_t);
                int inBufElSize = sizeof(int16_t);
                
                AACENC_BufDesc inBufDesc;
                inBufDesc.numBufs = 1;
                inBufDesc.bufs = &inBuf;
                inBufDesc.bufferIdentifiers = &inBufIds;
                inBufDesc.bufSizes = &inBufSize;
                inBufDesc.bufElSizes = &inBufElSize;
                
                void* outBuf = m_OutputBuffer.data();
                int outBufIds = OUT_BITSTREAM_DATA;
                int outBufSize = m_OutputBuffer.size();
                int outBufElSize = 1;
                
                AACENC_BufDesc outBufDesc;
                outBufDesc.numBufs = 1;
                outBufDesc.bufs = &outBuf;
                outBufDesc.bufferIdentifiers = &outBufIds;
                outBufDesc.bufSizes = &outBufSize;
                outBufDesc.bufElSizes = &outBufElSize;
                
                AACENC_InArgs inArgs;
                inArgs.numInSamples = int16Buffer.size();
                
                AACENC_OutArgs outArgs;
                
                AACENC_ERROR err = aacEncEncode(m_hEncoder, &inBufDesc, &outBufDesc, &inArgs, &outArgs);
                
                if (err == AACENC_OK && outArgs.numOutBytes > 0)
                {
                    g_Log(logLevelInfo, "AAC Plugin :: Encoded %d bytes, creating output buffer", outArgs.numOutBytes);
                    
                    // CRITICAL: Create NEW output buffer and RESIZE it
                    HostBufferRef outBuf;
                    if (!outBuf.IsValid())
                    {
                        g_Log(logLevelError, "AAC Plugin :: Failed to create output buffer");
                        ResetRingBuffer();
                        continue;
                    }
                    
                    if (!outBuf.Resize(outArgs.numOutBytes))
                    {
                        g_Log(logLevelError, "AAC Plugin :: Failed to resize output buffer to %d bytes", outArgs.numOutBytes);
                        ResetRingBuffer();
                        continue;
                    }
                    
                    g_Log(logLevelInfo, "AAC Plugin :: Output buffer resized, locking...");
                    
                    char* outData = nullptr;
                    size_t outSize = 0;
                    if (outBuf.LockBuffer(&outData, &outSize))
                    {
                        g_Log(logLevelInfo, "AAC Plugin :: Locked output buffer, size: %zu", outSize);
                        
                        if (outSize >= (size_t)outArgs.numOutBytes)
                        {
                            memcpy(outData, m_OutputBuffer.data(), outArgs.numOutBytes);
                            outBuf.UnlockBuffer();
                            
                            // Set all properties
                            outBuf.SetProperty(pIOPropBitDepth, propTypeUInt32, &m_outputBitDepth, 1);
                            outBuf.SetProperty(pIOPropSamplingRate, propTypeUInt32, &m_SampleRate, 1);
                            outBuf.SetProperty(pIOPropNumChannels, propTypeUInt32, &m_NumChannels, 1);
                            
                            uint8_t isKey = 0;
                            outBuf.SetProperty(pIOPropIsKeyFrame, propTypeUInt8, &isKey, 1);
                            
                            // CRITICAL: Set PTS and Duration
                            outBuf.SetProperty(pIOPropPTS, propTypeInt64, &m_OutputPTS, 1);
                            
                            int64_t duration = (int64_t)m_EncoderFrameSize;
                            outBuf.SetProperty(pIOPropDuration, propTypeInt64, &duration, 1);
                            
                            g_Log(logLevelInfo, "AAC Plugin :: Sending output to muxer (PTS: %lld, Duration: %lld)", m_OutputPTS, duration);
                            
                            // Increment PTS for next frame
                            m_OutputPTS += m_EncoderFrameSize;
                            
                            // Pass to muxer
                            StatusCode result = IPluginCodecRef::DoProcess(&outBuf);
                            g_Log(logLevelInfo, "AAC Plugin :: DoProcess returned: %d", result);
                        }
                        else
                        {
                            g_Log(logLevelError, "AAC Plugin :: Output buffer too small: %zu < %d", outSize, outArgs.numOutBytes);
                            outBuf.UnlockBuffer();
                        }
                    }
                    else
                    {
                        g_Log(logLevelError, "AAC Plugin :: Failed to lock output buffer");
                    }
                }
                else
                {
                    if (err != AACENC_OK)
                    {
                        g_Log(logLevelError, "AAC Plugin :: Encoding error: %d", err);
                    }
                    else
                    {
                        g_Log(logLevelInfo, "AAC Plugin :: No output bytes");
                    }
                }
                
                ResetRingBuffer();
            }
            
            sampleIdx += chunk;
        }
        
        p_pBuff->UnlockBuffer();
    }
    else
    {
        g_Log(logLevelInfo, "AAC Plugin :: DoProcess called with NULL buffer (flush)");
    }
    
    return errNone;
}

void AACEncoder::AddPCMToRingBuffer(const float** planarPCM, size_t samples)
{
    for (size_t i = 0; i < samples; ++i)
    {
        for (size_t ch = 0; ch < m_NumChannels; ++ch)
        {
            if (m_ringBufferFill < m_EncoderFrameSize)
                m_pcmRingBuffer[ch][m_ringBufferFill] = planarPCM[ch][i];
        }
        m_ringBufferFill++;
    }
}

bool AACEncoder::IsRingBufferFull() const
{
    return m_ringBufferFill >= m_EncoderFrameSize;
}

void AACEncoder::GetFrameFromRingBuffer(int16_t* out, size_t samples)
{
    // Not used in this version
}

void AACEncoder::PadAndFlushRingBuffer()
{
    if (m_ringBufferFill == 0) return;
    for (size_t ch = 0; ch < m_NumChannels; ++ch)
    {
        for (size_t i = m_ringBufferFill; i < m_EncoderFrameSize; ++i)
        {
            m_pcmRingBuffer[ch][i] = 0.0f;
        }
    }
}

void AACEncoder::ResetRingBuffer()
{
    m_ringBufferFill = 0;
    for (size_t ch = 0; ch < m_NumChannels; ++ch)
    {
        std::fill(m_pcmRingBuffer[ch].begin(), m_pcmRingBuffer[ch].end(), 0.0f);
    }
}
EOF

cat > plugin.cpp << 'EOF'
#include "plugin.h"
#include <cstring>
#include "aac_encoder.h"

static const uint8_t pMyUUID[] = { 
    0x3e, 0x4f, 0x5a, 0x6b, 0x7c, 0x8d, 0x9e, 0xaf, 
    0xb0, 0xc1, 0xd2, 0xe3, 0xf4, 0xa5, 0xb6, 0xc7 
};

using namespace IOPlugin;

StatusCode g_HandleGetInfo(HostPropertyCollectionRef* p_pProps)
{
    StatusCode err = p_pProps->SetProperty(pIOPropUUID, propTypeUInt8, pMyUUID, 16);
    if (err == errNone)
    {
        err = p_pProps->SetProperty(pIOPropName, propTypeString, "AAC Encoder (FDK)", strlen("AAC Encoder (FDK)"));
    }
    return err;
}

StatusCode g_HandleCreateObj(unsigned char* p_pUUID, ObjectRef* p_ppObj)
{
    if (memcmp(p_pUUID, AACEncoder::s_UUID, 16) == 0)
    {
        *p_ppObj = new AACEncoder();
        return errNone;
    }
    return errUnsupported;
}

StatusCode g_HandlePluginStart()
{
    g_Log(logLevelInfo, "AAC Encoder Plugin (FDK) :: Started");
    return errNone;
}

StatusCode g_HandlePluginTerminate()
{
    g_Log(logLevelInfo, "AAC Encoder Plugin (FDK) :: Terminated");
    return errNone;
}

StatusCode g_ListCodecs(HostListRef* p_pList)
{
    return AACEncoder::s_RegisterCodecs(p_pList);
}

StatusCode g_ListContainers(HostListRef* p_pList)
{
    return errNone;
}

StatusCode g_GetEncoderSettings(unsigned char* p_pUUID, HostPropertyCollectionRef* p_pValues, HostListRef* p_pSettingsList)
{
    if (memcmp(p_pUUID, AACEncoder::s_UUID, 16) == 0)
    {
        return AACEncoder::s_GetEncoderSettings(p_pValues, p_pSettingsList);
    }
    return errNoCodec;
}
EOF

cat > Makefile << 'EOF'
CC = clang++
CFLAGS = -std=c++11 -fPIC -O2 -Wall -I. -Iinclude
CFLAGS += $(shell pkg-config --cflags fdk-aac)

LDFLAGS = -shared -Wl,-rpath,'$$ORIGIN' -lpthread -stdlib=libc++
LDFLAGS += $(shell pkg-config --libs fdk-aac)

TARGET = bin/aac_fdk_plugin.dvcp
OBJDIR = build
BINDIR = bin

SRCS = plugin.cpp aac_encoder.cpp
OBJS = $(SRCS:%.cpp=$(OBJDIR)/%.o)
WRAPPER_OBJS = $(OBJDIR)/host_api.o $(OBJDIR)/plugin_api.o

all: prereq wrapper-objs $(OBJS) $(TARGET)

prereq:
	mkdir -p $(OBJDIR)
	mkdir -p $(BINDIR)

wrapper-objs:
	$(CC) -c wrapper/host_api.cpp -o $(OBJDIR)/host_api.o $(CFLAGS)
	$(CC) -c wrapper/plugin_api.cpp -o $(OBJDIR)/plugin_api.o $(CFLAGS)

$(OBJDIR)/%.o: %.cpp
	$(CC) -c -o $@ $< $(CFLAGS)

$(TARGET): $(OBJS) $(WRAPPER_OBJS)
	$(CC) $(OBJDIR)/*.o $(LDFLAGS) -o $(TARGET)

clean:
	rm -rf $(OBJDIR) $(BINDIR)

.PHONY: all prereq wrapper-objs clean
EOF

echo -e "${BLUE}[*]${NC} Building..."
make

if [ ! -f "bin/aac_fdk_plugin.dvcp" ]; then
    echo -e "${RED}[✗]${NC} Build failed!"
    exit 1
fi

echo -e "${GREEN}[✓]${NC} Build successful"

BUNDLE_DIR="aac_fdk_plugin.dvcp.bundle"
mkdir -p "$BUNDLE_DIR/Contents/Linux-x86-64"
cp bin/aac_fdk_plugin.dvcp "$BUNDLE_DIR/Contents/Linux-x86-64/"
chmod 755 "$BUNDLE_DIR/Contents/Linux-x86-64/aac_fdk_plugin.dvcp"

sudo rm -rf /opt/resolve/IOPlugins/aac_fdk_plugin.dvcp.bundle
sudo cp -r "$BUNDLE_DIR" /opt/resolve/IOPlugins/
sudo chmod -R 755 /opt/resolve/IOPlugins/aac_fdk_plugin.dvcp.bundle

echo ""
echo -e "${GREEN}============================================${NC}"
echo -e "${GREEN}Installation Complete!${NC}"
echo -e "${GREEN}============================================${NC}"
echo ""
echo "KEY FIX: Using outBuf.Resize() to allocate buffers"
echo ""
echo "Restart: killall resolve && /opt/resolve/bin/resolve"
echo ""
