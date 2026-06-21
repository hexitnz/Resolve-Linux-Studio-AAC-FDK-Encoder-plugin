#include "aac_encoder.h"
#include <cstring>
#include <cstdlib>
#include <algorithm>
#include <string>

const uint8_t AACEncoder::s_UUID[] = {
    0x9f, 0xb2, 0xe4, 0x73, 0xda, 0x86, 0x5c, 0x4d,
    0xb3, 0x5f, 0xe6, 0xa2, 0xff, 0xc3, 0xed, 0xe8
};

// ---------------------------------------------------------------------
// Stereo delivery only: 2.0 (L/R), 16-bit, 44.1/48 kHz. Bitrate is
// user-selectable via a combobox in Resolve's encoder settings UI,
// defaulting to 192 kbps.
// ---------------------------------------------------------------------
static const int32_t kDefaultBitRateKbps = 192;

// Allowed bitrates, in kbps. Kept to standard, well-supported CBR AAC-LC
// values for 44.1/48kHz stereo -- avoids offering rates FDK-AAC would
// reject or that would produce poor-quality output for this format.
static const std::vector<int32_t> kAllowedBitRatesKbps = { 96, 128, 160, 192, 224, 256, 320 };

class UIAACSettingsController
{
public:
    UIAACSettingsController()
        : m_BitRateKbps(kDefaultBitRateKbps)
    {
    }

    // Reads the user's current bitrate selection (if any) from the property
    // provider Resolve hands us. p_pValues holds whatever was previously
    // selected/saved for this encoder instance; if the property is absent
    // (first time the codec is configured), we keep the default.
    void Load(IPropertyProvider* p_pValues)
    {
        if (p_pValues == nullptr)
        {
            return;
        }

        int32_t bitRateBps = 0;
        if (p_pValues->GetINT32(pIOPropBitRate, bitRateBps) && bitRateBps > 0)
        {
            int32_t kbps = bitRateBps / 1000;
            // Snap to nearest allowed value rather than rejecting outright,
            // in case Resolve hands back a value just off due to rounding.
            int32_t closest = kAllowedBitRatesKbps.front();
            int32_t bestDiff = std::abs(closest - kbps);
            for (int32_t candidate : kAllowedBitRatesKbps)
            {
                int32_t diff = std::abs(candidate - kbps);
                if (diff < bestDiff)
                {
                    closest = candidate;
                    bestDiff = diff;
                }
            }
            m_BitRateKbps = closest;
        }
    }

    // Builds the combobox UI entry shown in Resolve's encoder settings
    // panel. Values are stored/transmitted in bits/second (matching
    // pIOPropBitRate's documented uint64_t bits-per-second convention),
    // even though the dropdown displays "kbps" to the user.
    StatusCode Render(HostListRef* p_pSettingsList)
    {
        HostUIConfigEntryRef bitrateEntry(pIOPropBitRate);

        std::vector<std::string> texts;
        std::vector<int32_t> values;
        texts.reserve(kAllowedBitRatesKbps.size());
        values.reserve(kAllowedBitRatesKbps.size());

        for (int32_t kbps : kAllowedBitRatesKbps)
        {
            texts.push_back(std::to_string(kbps) + " kbps");
            values.push_back(kbps * 1000); // stored in bits/second
        }

        bitrateEntry.MakeComboBox("Bitrate", texts, values, m_BitRateKbps * 1000, "");

        if (!bitrateEntry.IsSuccess())
        {
            g_Log(logLevelError, "AAC Plugin :: Failed to build bitrate combobox UI entry");
            return errFail;
        }

        if (!p_pSettingsList->Append(&bitrateEntry))
        {
            g_Log(logLevelError, "AAC Plugin :: Failed to append bitrate combobox to settings list");
            return errFail;
        }

        return errNone;
    }

    int32_t GetBitRateKbps() const { return m_BitRateKbps; }

private:
    int32_t m_BitRateKbps;
};

StatusCode AACEncoder::s_RegisterCodecs(HostListRef* p_pList)
{
    HostPropertyCollectionRef codecInfo;
    if (!codecInfo.IsValid())
        return errAlloc;

    codecInfo.SetProperty(pIOPropUUID, propTypeUInt8, AACEncoder::s_UUID, 16);

    const char* pCodecName = "AAC 2.0 (FDK-AAC)";
    codecInfo.SetProperty(pIOPropName, propTypeString, pCodecName, strlen(pCodecName));

    uint32_t val = 'aac ';
    codecInfo.SetProperty(pIOPropFourCC, propTypeUInt32, &val, 1);

    val = mediaAudio;
    codecInfo.SetProperty(pIOPropMediaType, propTypeUInt32, &val, 1);

    val = dirEncode;
    codecInfo.SetProperty(pIOPropCodecDirection, propTypeUInt32, &val, 1);

    std::vector<uint32_t> bitDepths({16});
    codecInfo.SetProperty(pIOPropBitDepth, propTypeUInt32, bitDepths.data(), bitDepths.size());

    std::vector<uint32_t> samplingRates({44100, 48000});
    codecInfo.SetProperty(pIOPropSamplingRate, propTypeUInt32, samplingRates.data(), samplingRates.size());

    // --- THE FIX ---
    // Explicitly advertise supported channel counts. Without this, Resolve
    // had no declared constraint to negotiate against and was defaulting
    // to a 4-channel bus, which FDK-AAC then encoded as MODE_1_2_2 (LCRS).
    // Restricting this list to {2} forces stereo-only delivery for this
    // codec entry -- no mono, no surround.
    std::vector<uint32_t> numChannels({2});
    codecInfo.SetProperty(pIOPropNumChannels, propTypeUInt32, numChannels.data(), numChannels.size());

    // --- THE REAL FIX ---
    // pIOPropNumChannels alone only declares a raw count. Resolve has a
    // SEPARATE property, pIOPropAudioChannelLayout ("audLayout"), which
    // carries the actual AudioChannelLayout enum (mono/stereo/5.1/7.1).
    // If this is never set, Resolve has no declared layout to negotiate
    // against and falls back to its own default -- which on this system
    // was resolving to a 4-channel layout, and FDK-AAC dutifully encoded
    // whatever it was handed as MODE_1_2_2 (LCRS). Declaring numChannels
    // without audLayout was the gap; both must be set together.
    uint32_t channelLayout = audLayoutStereo;
    codecInfo.SetProperty(pIOPropAudioChannelLayout, propTypeUInt32, &channelLayout, 1);

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

    g_Log(logLevelInfo, "AAC (FDK-AAC) Plugin :: Registered (2.0 / 16-bit, selectable bitrate, default 192kbps)");
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
    , m_BitRateKbps(kDefaultBitRateKbps)
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

    // --- VALIDATION GUARD ---
    // Fail loudly instead of silently encoding whatever channel count
    // Resolve happens to send. This is what would have caught the
    // 4-channel LCRS problem immediately instead of producing a file
    // that "worked" but was wrong.
    if (m_NumChannels != 2)
    {
        g_Log(logLevelError,
              "AAC Plugin :: Rejecting unsupported channel count: %d (this build only supports 2.0 stereo)",
              m_NumChannels);
        return errFail;
    }

    if (m_outputBitDepth != 16)
    {
        g_Log(logLevelError,
              "AAC Plugin :: Rejecting unsupported bit depth: %d (this build only supports 16-bit)",
              m_outputBitDepth);
        return errFail;
    }

    if (m_SampleRate != 44100 && m_SampleRate != 48000)
    {
        g_Log(logLevelError,
              "AAC Plugin :: Rejecting unsupported sample rate: %d (only 44100/48000 supported)",
              m_SampleRate);
        return errFail;
    }

    // Read the user's selected bitrate, if Resolve provides it at this
    // stage. pIOPropBitRate is documented as bits/second; the combobox
    // stores values the same way (see UIAACSettingsController::Render).
    // DoOpen will also (re-)apply this via m_pSettings->Load() against the
    // DoOpen-time buffer, so this is a first pass / early validation, not
    // the only place this is read -- some hosts populate the selection at
    // DoInit, others only at DoOpen.
    int32_t bitRateBps = 0;
    if (p_pProps->GetINT32(pIOPropBitRate, bitRateBps) && bitRateBps > 0)
    {
        int32_t kbps = bitRateBps / 1000;
        bool isAllowed = false;
        for (int32_t candidate : kAllowedBitRatesKbps)
        {
            if (candidate == kbps)
            {
                isAllowed = true;
                break;
            }
        }

        if (isAllowed)
        {
            m_BitRateKbps = kbps;
        }
        else
        {
            g_Log(logLevelError,
                  "AAC Plugin :: Ignoring out-of-range bitrate selection %d kbps, keeping %d kbps",
                  kbps, m_BitRateKbps);
        }
    }

    g_Log(logLevelInfo, "AAC Plugin :: Init - %d Hz, %d ch, %d-bit, %d kbps",
          m_SampleRate, m_NumChannels, m_outputBitDepth, m_BitRateKbps);

    return errNone;
}

StatusCode AACEncoder::DoOpen(HostBufferRef* p_pBuff)
{
    m_pSettings.reset(new UIAACSettingsController());
    m_pSettings->Load(p_pBuff);
    m_BitRateKbps = m_pSettings->GetBitRateKbps();

    // Re-assert the contract at open time too -- belt and suspenders.
    // m_NumChannels is already validated in DoInit, but DoOpen is where
    // the encoder handle is actually created, so we guard here as well
    // in case DoOpen is ever reachable without DoInit in some host path.
    if (m_NumChannels != 2)
    {
        g_Log(logLevelError, "AAC Plugin :: DoOpen - refusing to open encoder for %d channels", m_NumChannels);
        return errFail;
    }

    if (aacEncOpen(&m_hEncoder, 0, m_NumChannels) != AACENC_OK)
    {
        g_Log(logLevelError, "AAC Plugin :: Failed to open encoder");
        return errFail;
    }

    // Explicit, hard-coded stereo mode. Not derived from a ternary on
    // m_NumChannels anymore -- m_NumChannels is guaranteed == 2 here
    // because of the guards above, but we set MODE_2 directly so there
    // is no ambiguity about what channel mode FDK-AAC is being told to use.
    AACENC_ERROR paramErr;

    paramErr = aacEncoder_SetParam(m_hEncoder, AACENC_AOT, AOT_AAC_LC);
    if (paramErr != AACENC_OK)
    {
        g_Log(logLevelError, "AAC Plugin :: Failed to set AOT (err %d)", paramErr);
        return errFail;
    }

    paramErr = aacEncoder_SetParam(m_hEncoder, AACENC_SAMPLERATE, m_SampleRate);
    if (paramErr != AACENC_OK)
    {
        g_Log(logLevelError, "AAC Plugin :: Failed to set sample rate (err %d)", paramErr);
        return errFail;
    }

    paramErr = aacEncoder_SetParam(m_hEncoder, AACENC_CHANNELMODE, MODE_2);
    if (paramErr != AACENC_OK)
    {
        g_Log(logLevelError, "AAC Plugin :: Failed to set channel mode MODE_2 (err %d)", paramErr);
        return errFail;
    }

    // Explicitly pin channel order to MPEG/WMF ordering (L, R) -- FDK-AAC
    // defaults to this already (0), but setting it explicitly removes any
    // ambiguity about channel ordering for a 2-channel stream.
    paramErr = aacEncoder_SetParam(m_hEncoder, AACENC_CHANNELORDER, 0);
    if (paramErr != AACENC_OK)
    {
        g_Log(logLevelError, "AAC Plugin :: Failed to set channel order (err %d)", paramErr);
        return errFail;
    }

    paramErr = aacEncoder_SetParam(m_hEncoder, AACENC_BITRATE, m_BitRateKbps * 1000);
    if (paramErr != AACENC_OK)
    {
        g_Log(logLevelError, "AAC Plugin :: Failed to set bitrate (err %d)", paramErr);
        return errFail;
    }

    paramErr = aacEncoder_SetParam(m_hEncoder, AACENC_BITRATEMODE, 0); // 0 = CBR
    if (paramErr != AACENC_OK)
    {
        g_Log(logLevelError, "AAC Plugin :: Failed to set bitrate mode (err %d)", paramErr);
        return errFail;
    }

    paramErr = aacEncoder_SetParam(m_hEncoder, AACENC_TRANSMUX, TT_MP4_RAW);
    if (paramErr != AACENC_OK)
    {
        g_Log(logLevelError, "AAC Plugin :: Failed to set transmux (err %d)", paramErr);
        return errFail;
    }

    if (aacEncEncode(m_hEncoder, nullptr, nullptr, nullptr, nullptr) != AACENC_OK)
    {
        g_Log(logLevelError, "AAC Plugin :: Failed to initialize encoder");
        return errFail;
    }

    AACENC_InfoStruct info;
    aacEncInfo(m_hEncoder, &info);

    // Sanity-check what FDK-AAC actually configured itself for, post-init.
    // If this ever reports something other than 2 channels despite us
    // requesting MODE_2, that means the FDK-AAC library build itself is
    // behaving unexpectedly (e.g. an unusual libfdk-aac build/config) --
    // log it loudly so it's immediately visible rather than silently
    // producing a mismatched file again.
    if (info.inputChannels != 2)
    {
        g_Log(logLevelError,
              "AAC Plugin :: FDK-AAC reports inputChannels=%d after requesting MODE_2 -- aborting",
              info.inputChannels);
        aacEncClose(&m_hEncoder);
        m_hEncoder = nullptr;
        return errFail;
    }

    m_EncoderFrameSize = info.frameLength;
    m_OutputBuffer.resize(info.maxOutBufBytes);
    m_OutputPTS = 0;

    m_pcmRingBuffer.clear();
    m_pcmRingBuffer.resize(m_NumChannels);
    for (size_t ch = 0; ch < m_NumChannels; ++ch)
    {
        m_pcmRingBuffer[ch].resize(m_EncoderFrameSize, 0.0f);
    }
    m_ringBufferFill = 0;

    const uint64_t bitRate = static_cast<uint64_t>(m_BitRateKbps) * 1000;
    p_pBuff->SetProperty(pIOPropBitRate, propTypeUInt64, &bitRate, 1);

    // --- THE ACTUAL ROOT CAUSE ---
    // pIOPropDuration and pIOPropPTS are documented as being expressed "in
    // time base" (see pIOPropTimeBase in IOPluginProps.h) -- they are NOT
    // raw sample counts or seconds on their own. DoProcess writes
    // m_EncoderFrameSize (1024) directly into PTS/Duration every frame, but
    // pIOPropTimeBase was never declared anywhere, so the muxer had no idea
    // what unit "1024" was measured in. It had to assume some default time
    // base of its own, and was effectively back-deriving an output sample
    // rate from frame-count-vs-assumed-cadence rather than ever reading our
    // declared 48000 Hz -- explaining both the 64000 Hz figure (1024
    // samples/frame at the muxer's assumed 62.5 fps cadence = 64000) and why
    // forwarding the AudioSpecificConfig magic cookie alone didn't fix it:
    // the esds box can be correct while the actual track timing metadata is
    // still wrong, and mediainfo's "Sampling rate" reflects the latter.
    //
    // The correct time base for sample-counted PTS/Duration is 1/sampleRate
    // (numerator=1, denominator=sampleRate), i.e. "PTS/duration values are
    // counted in units of 1 sample." Declaring it once here, matching the
    // units already used for PTS/Duration in DoProcess.
    uint32_t timeBase[2] = { 1, m_SampleRate };
    p_pBuff->SetProperty(pIOPropTimeBase, propTypeUInt32, timeBase, 2);

    g_Log(logLevelInfo, "AAC Plugin :: Time base declared as 1/%u (sample-accurate PTS/Duration)", m_SampleRate);

    // We requested TT_MP4_RAW transport, which means FDK-AAC emits bare AAC
    // frames with NO in-band ADTS/LATM header describing sample rate or
    // object type. Without that, Resolve's MP4 muxer has no authoritative
    // source for the AudioSpecificConfig it must write into the esds box,
    // and falls back to its own internal default/guess -- which is why the
    // exported file reported 64000 Hz / AAC LTP (mp4a-40-4) even though our
    // encoder log confirms FDK-AAC itself was correctly configured for
    // 48000 Hz / AAC-LC (mp4a-40-2) the entire time.
    //
    // aacEncInfo() computes the real AudioSpecificConfig for us in
    // info.confBuf/info.confSize -- we just weren't forwarding it. Setting
    // it here via pIOPropMagicCookie gives the muxer the authoritative
    // config blob to write verbatim instead of inferring its own.
    if (info.confSize > 0 && info.confSize <= sizeof(info.confBuf))
    {
        m_MagicCookie.assign(info.confBuf, info.confBuf + info.confSize);

        p_pBuff->SetProperty(pIOPropMagicCookie, propTypeUInt8, m_MagicCookie.data(), m_MagicCookie.size());

        uint32_t magicCookieType = 'esds';
        p_pBuff->SetProperty(pIOPropMagicCookieType, propTypeUInt32, &magicCookieType, 1);

        g_Log(logLevelInfo, "AAC Plugin :: AudioSpecificConfig forwarded to muxer (%u bytes)", info.confSize);
    }
    else
    {
        m_MagicCookie.clear();
        g_Log(logLevelError,
              "AAC Plugin :: aacEncInfo returned invalid confSize=%u -- muxer will not receive AudioSpecificConfig",
              info.confSize);
    }

    g_Log(logLevelInfo, "AAC Plugin :: Opened - %d kbps CBR, 2.0 stereo, frame size: %d, inputChannels confirmed: %d",
          m_BitRateKbps, m_EncoderFrameSize, info.inputChannels);

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
        char* pBuf = nullptr;
        size_t bufSize = 0;
        if (!p_pBuff->LockBuffer(&pBuf, &bufSize))
        {
            return errNone;
        }

        if (bufSize == 0)
        {
            p_pBuff->UnlockBuffer();
            return errNone;
        }

        uint32_t inputBitDepth = m_outputBitDepth;
        p_pBuff->GetUINT32(pIOPropBitDepth, inputBitDepth);

        // Defensive: if the buffer's own channel count property disagrees
        // with what we negotiated at DoInit/DoOpen, bail rather than
        // mis-interleave the data into a corrupted stereo mix.
        uint32_t bufChannels = m_NumChannels;
        p_pBuff->GetUINT32(pIOPropNumChannels, bufChannels);
        if (bufChannels != 2)
        {
            g_Log(logLevelError,
                  "AAC Plugin :: DoProcess - input buffer reports %d channels, expected 2 -- dropping buffer",
                  bufChannels);
            p_pBuff->UnlockBuffer();
            return errFail;
        }

        int bytesPerSample = (inputBitDepth == 16) ? 2 : 3;
        int totalSamples = bufSize / (m_NumChannels * bytesPerSample);

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
        else
        {
            g_Log(logLevelError, "AAC Plugin :: DoProcess - unsupported input bit depth %d", inputBitDepth);
            p_pBuff->UnlockBuffer();
            return errFail;
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
                std::vector<int16_t> int16Buffer(m_EncoderFrameSize * m_NumChannels);
                for (size_t i = 0; i < (size_t)m_EncoderFrameSize; ++i)
                {
                    for (size_t ch = 0; ch < m_NumChannels; ++ch)
                    {
                        float sample = m_pcmRingBuffer[ch][i];
                        if (sample > 1.0f) sample = 1.0f;
                        if (sample < -1.0f) sample = -1.0f;
                        int16Buffer[i * m_NumChannels + ch] = (int16_t)(sample * 32767.0f);
                    }
                }

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
                    HostBufferRef outBuf2;
                    if (outBuf2.IsValid() && outBuf2.Resize(outArgs.numOutBytes))
                    {
                        char* outData = nullptr;
                        size_t outSize = 0;
                        if (outBuf2.LockBuffer(&outData, &outSize) && outSize >= (size_t)outArgs.numOutBytes)
                        {
                            memcpy(outData, m_OutputBuffer.data(), outArgs.numOutBytes);
                            outBuf2.UnlockBuffer();

                            // NOTE: pIOPropBitDepth is intentionally NOT set here.
                            // It is a PCM concept ("number of meaningful bits per
                            // sample") and is meaningless on a compressed AAC packet.
                            // A known issue in this plugin family (see
                            // github.com/Toxblh/davinci-linux-aac-codec/issues/13)
                            // found that publishing PCM-only properties (bit depth,
                            // bits-per-sample, float flags) on already-compressed AAC
                            // output buffers can confuse the host's muxing/stream
                            // description logic. We only publish properties that
                            // genuinely describe the compressed stream below.
                            outBuf2.SetProperty(pIOPropSamplingRate, propTypeUInt32, &m_SampleRate, 1);
                            outBuf2.SetProperty(pIOPropNumChannels, propTypeUInt32, &m_NumChannels, 1);

                            // --- THE FIX ---
                            // pIOPropBitRate was previously only set once on the
                            // open-time buffer in DoOpen. Same gap pattern as the
                            // magic cookie and time base bugs: the muxer apparently
                            // doesn't trust/use the open-time value for the actual
                            // written bitrate, and was instead falling back to some
                            // internal default (192 kbps) regardless of what the user
                            // selected. Setting it explicitly on every encoded sample
                            // buffer as well ensures the muxer has the real,
                            // user-selected bitrate available at the point it
                            // actually computes/writes the track's bitrate metadata.
                            const uint64_t outBitRate = static_cast<uint64_t>(m_BitRateKbps) * 1000;
                            outBuf2.SetProperty(pIOPropBitRate, propTypeUInt64, &outBitRate, 1);

                            // Re-attach the AudioSpecificConfig to every sample as well,
                            // defensively -- some muxers expect the magic cookie on the
                            // open-time buffer only, others expect it per-sample (at least
                            // on the first sample). Setting it on all of them costs nothing
                            // and removes ambiguity about which behavior Resolve's muxer
                            // actually requires.
                            if (!m_MagicCookie.empty())
                            {
                                outBuf2.SetProperty(pIOPropMagicCookie, propTypeUInt8, m_MagicCookie.data(), m_MagicCookie.size());
                                uint32_t magicCookieType = 'esds';
                                outBuf2.SetProperty(pIOPropMagicCookieType, propTypeUInt32, &magicCookieType, 1);
                            }

                            // Tag the output with an explicit stereo layout, not just a
                            // raw channel count -- this is what the muxer/container writer
                            // reads to decide what channel configuration to write into the
                            // AAC channel configuration field (and ultimately what gets
                            // baked into the mp4/mov audio track).
                            uint32_t outChannelLayout = audLayoutStereo;
                            outBuf2.SetProperty(pIOPropAudioChannelLayout, propTypeUInt32, &outChannelLayout, 1);

                            uint8_t isKey = 0;
                            outBuf2.SetProperty(pIOPropIsKeyFrame, propTypeUInt8, &isKey, 1);

                            // Re-declare time base on every buffer too, defensively -- same
                            // reasoning as the magic cookie: some muxers read per-stream
                            // metadata once at open, others expect it attached to (at least
                            // the first) sample. Costs nothing to set on all of them.
                            uint32_t timeBase[2] = { 1, m_SampleRate };
                            outBuf2.SetProperty(pIOPropTimeBase, propTypeUInt32, timeBase, 2);

                            // CRITICAL: PTS + Duration required or muxer silently drops audio.
                            // Both are counted in units declared by pIOPropTimeBase above
                            // (1/m_SampleRate), so these raw sample counts are now correctly
                            // interpreted as sample-accurate timing instead of being read
                            // against the muxer's own assumed/default time base.
                            outBuf2.SetProperty(pIOPropPTS, propTypeInt64, &m_OutputPTS, 1);
                            int64_t duration = (int64_t)m_EncoderFrameSize;
                            outBuf2.SetProperty(pIOPropDuration, propTypeInt64, &duration, 1);

                            m_OutputPTS += m_EncoderFrameSize;

                            IPluginCodecRef::DoProcess(&outBuf2);
                        }
                    }
                }

                ResetRingBuffer();
            }

            sampleIdx += chunk;
        }

        p_pBuff->UnlockBuffer();
    }

    return errNone;
}

void AACEncoder::AddPCMToRingBuffer(const float** planarPCM, size_t samples)
{
    for (size_t i = 0; i < samples; ++i)
    {
        for (size_t ch = 0; ch < m_NumChannels; ++ch)
        {
            if (m_ringBufferFill < (size_t)m_EncoderFrameSize)
                m_pcmRingBuffer[ch][m_ringBufferFill] = planarPCM[ch][i];
        }
        m_ringBufferFill++;
    }
}

bool AACEncoder::IsRingBufferFull() const
{
    return m_ringBufferFill >= (size_t)m_EncoderFrameSize;
}

void AACEncoder::GetFrameFromRingBuffer(int16_t* /*out*/, size_t /*samples*/)
{
    // Unused in current implementation (conversion done inline in DoProcess)
}

void AACEncoder::PadAndFlushRingBuffer()
{
    if (m_ringBufferFill == 0) return;
    for (size_t ch = 0; ch < m_NumChannels; ++ch)
    {
        for (size_t i = m_ringBufferFill; i < (size_t)m_EncoderFrameSize; ++i)
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
