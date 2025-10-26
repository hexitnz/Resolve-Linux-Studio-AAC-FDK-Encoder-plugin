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
