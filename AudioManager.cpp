
#include "pch.h"
#include "AudioManager.h"

#pragma pack(push, 1) // Ensure compiler doesn't add padding
struct RiffChunkHeader {
    char ChunkId[4]; // e.g., "RIFF" or "fmt " or "data"
    uint32_t ChunkSize;
};

struct RiffFileHeader {
    RiffChunkHeader Header; // Contains "RIFF", size of rest of file
    char Format[4];      // Contains "WAVE"
};

struct FmtSubchunk {
    RiffChunkHeader Header;     // Contains "fmt ", size of this subchunk (usually 16 for PCM)
    uint16_t AudioFormat;    // PCM = 1
    uint16_t NumChannels;
    uint32_t SampleRate;
    uint32_t ByteRate;       // SampleRate * NumChannels * BitsPerSample/8
    uint16_t BlockAlign;     // NumChannels * BitsPerSample/8
    uint16_t BitsPerSample;
    // Optional extra format bytes might follow if not PCM
};

struct DataSubchunkHeader {
     RiffChunkHeader Header; // Contains "data", size of audio data
};
#pragma pack(pop)


AudioManager::AudioManager() : m_pMasterVoice(nullptr), m_pMusicVoice(nullptr) {}

AudioManager::~AudioManager() {
    Shutdown();
}

bool AudioManager::Initialize() {
    HRESULT hr = S_OK;

    // Required for XAudio2
    hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    if (FAILED(hr)) {
        OutputDebugString(L"CoInitializeEx failed for XAudio2.\n");
        // Don't necessarily return false, CoInitialize might have been called already
    }

    UINT32 flags = 0;
#ifdef _DEBUG
    flags |= XAUDIO2_DEBUG_ENGINE;
#endif

    hr = XAudio2Create(&m_pXAudio2, flags);
    if (FAILED(hr)) {
        MessageBox(nullptr, L"Failed to initialize XAudio2 engine.", L"Audio Error", MB_OK);
        return false;
    }

    hr = m_pXAudio2->CreateMasteringVoice(&m_pMasterVoice);
    if (FAILED(hr)) {
        MessageBox(nullptr, L"Failed to create XAudio2 mastering voice.", L"Audio Error", MB_OK);
        m_pXAudio2.Reset(); // Release engine if master voice fails
        return false;
    }

    return true;
}

void AudioManager::Shutdown() {
    // Stop all sounds first
    Suspend(); // Stops voices

    // Destroy music voice explicitly if it exists
    if (m_pMusicVoice) {
        m_pMusicVoice->DestroyVoice();
        m_pMusicVoice = nullptr;
    }

    // Destroy all active sound effect voices
    for (auto& voice : m_activeSoundEffects) {
        if (voice) {
            voice->DestroyVoice();
        }
    }
    m_activeSoundEffects.clear();

    // Clear loaded data (vectors will clean up themselves)
    m_loadedSounds.clear();

    // Destroy mastering voice (implicitly destroyed when engine releases, but good practice)
    if (m_pMasterVoice) {
        m_pMasterVoice->DestroyVoice(); // Technically not required
        m_pMasterVoice = nullptr;
    }

    // Release the engine
    m_pXAudio2.Reset();

    CoUninitialize(); // Match CoInitializeEx
}

// Basic WAV Parser - Limited error checking, assumes valid PCM format
bool AudioManager::ParseWaveFile(const BYTE* fileData, size_t fileSize, WaveData& outWaveData) {
    if (!fileData || fileSize < sizeof(RiffFileHeader) + sizeof(FmtSubchunk) + sizeof(DataSubchunkHeader)) {
        OutputDebugString(L"WAV file too small or invalid.\n");
        return false;
    }

    const BYTE* currentPtr = fileData;
    size_t remainingSize = fileSize;

    // Read RIFF Header
    const RiffFileHeader* riffHeader = reinterpret_cast<const RiffFileHeader*>(currentPtr);
    if (strncmp(riffHeader->Header.ChunkId, "RIFF", 4) != 0 || strncmp(riffHeader->Format, "WAVE", 4) != 0) {
        OutputDebugString(L"Invalid RIFF or WAVE header.\n");
        return false;
    }
    currentPtr += sizeof(RiffFileHeader);
    remainingSize -= sizeof(RiffFileHeader);

    const FmtSubchunk* fmtChunk = nullptr;
    const DataSubchunkHeader* dataChunkHeader = nullptr;
    const BYTE* audioDataPtr = nullptr;
    uint32_t audioDataSize = 0;

    // Find 'fmt ' and 'data' chunks
    while (remainingSize >= sizeof(RiffChunkHeader)) {
        const RiffChunkHeader* chunkHeader = reinterpret_cast<const RiffChunkHeader*>(currentPtr);
        size_t currentChunkTotalSize = sizeof(RiffChunkHeader) + chunkHeader->ChunkSize;

        if (remainingSize < currentChunkTotalSize) {
            OutputDebugString(L"WAV chunk size mismatch or corrupted file.\n");
            return false; // Chunk claims to be larger than remaining file
        }

        if (strncmp(chunkHeader->ChunkId, "fmt ", 4) == 0) {
            if (chunkHeader->ChunkSize >= (sizeof(FmtSubchunk) - sizeof(RiffChunkHeader))) {
                 fmtChunk = reinterpret_cast<const FmtSubchunk*>(currentPtr);
            } else {
                OutputDebugString(L"fmt chunk too small.\n");
                return false;
            }
        } else if (strncmp(chunkHeader->ChunkId, "data", 4) == 0) {
            dataChunkHeader = reinterpret_cast<const DataSubchunkHeader*>(currentPtr);
            audioDataPtr = currentPtr + sizeof(DataSubchunkHeader);
            audioDataSize = dataChunkHeader->Header.ChunkSize;
            // Ensure data size doesn't exceed remaining file size
            if (audioDataSize > remainingSize - sizeof(DataSubchunkHeader)) {
                 OutputDebugString(L"WAV data chunk size mismatch or corrupted file.\n");
                 return false;
            }
        }
        // Add checks for other chunk types like 'LIST' if needed and skip them

        currentPtr += currentChunkTotalSize;
        // Align to WORD boundary if chunk size is odd (RIFF standard)
        if (chunkHeader->ChunkSize % 2 != 0) {
            currentPtr++;
            remainingSize--; // Account for padding byte
        }
        remainingSize -= currentChunkTotalSize;

         // Break early if we found both essential chunks
         if (fmtChunk && dataChunkHeader) break;
    }

    if (!fmtChunk || !dataChunkHeader || !audioDataPtr || audioDataSize == 0) {
        OutputDebugString(L"Essential WAV chunks ('fmt ', 'data') not found or data size is zero.\n");
        return false;
    }

    // We only support basic PCM for now
    if (fmtChunk->AudioFormat != 1 /*WAVE_FORMAT_PCM*/) {
         OutputDebugString(L"Unsupported WAV audio format (only PCM supported).\n");
         return false;
    }

    // Copy format information
    memset(&outWaveData.WaveFormat, 0, sizeof(WAVEFORMATEX));
    outWaveData.WaveFormat.wFormatTag = fmtChunk->AudioFormat;
    outWaveData.WaveFormat.nChannels = fmtChunk->NumChannels;
    outWaveData.WaveFormat.nSamplesPerSec = fmtChunk->SampleRate;
    outWaveData.WaveFormat.nAvgBytesPerSec = fmtChunk->ByteRate;
    outWaveData.WaveFormat.nBlockAlign = fmtChunk->BlockAlign;
    outWaveData.WaveFormat.wBitsPerSample = fmtChunk->BitsPerSample;
    outWaveData.WaveFormat.cbSize = 0; // No extra format info for PCM

    // Copy audio data
    outWaveData.AudioData.resize(audioDataSize);
    memcpy(outWaveData.AudioData.data(), audioDataPtr, audioDataSize);

    return true;
}


bool AudioManager::LoadWaveFile(const std::wstring& filename, const std::string& soundName) {
    if (m_loadedSounds.count(soundName)) {
        // Already loaded
        return true;
    }

    std::ifstream file(filename, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        OutputDebugString((L"Failed to open WAV file: " + filename + L"\n").c_str());
        return false;
    }

    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);

    std::vector<char> buffer(static_cast<size_t>(size));
    if (!file.read(buffer.data(), size)) {
        OutputDebugString((L"Failed to read WAV file: " + filename + L"\n").c_str());
        return false;
    }
    file.close();

    WaveData waveData;
    if (!ParseWaveFile(reinterpret_cast<const BYTE*>(buffer.data()), buffer.size(), waveData)) {
        OutputDebugString((L"Failed to parse WAV file: " + filename + L"\n").c_str());
        return false;
    }

    m_loadedSounds[soundName] = std::move(waveData);
    return true;
}

IXAudio2SourceVoice* AudioManager::PlaySoundEffect(const std::string& soundName, float volume, float pitch, bool loop) {
    auto it = m_loadedSounds.find(soundName);
    if (it == m_loadedSounds.end()) {
        OutputDebugStringA(("Sound not loaded: " + soundName + "\n").c_str());
        return nullptr;
    }

    const WaveData& waveData = it->second;

    IXAudio2SourceVoice* pSourceVoice = nullptr;
    HRESULT hr = m_pXAudio2->CreateSourceVoice(&pSourceVoice, &(waveData.WaveFormat));
    if (FAILED(hr) || !pSourceVoice) {
        OutputDebugStringA(("Failed to create source voice for: " + soundName + "\n").c_str());
        return nullptr;
    }

    XAUDIO2_BUFFER buffer = {0};
    buffer.AudioBytes = static_cast<UINT32>(waveData.AudioData.size());
    buffer.pAudioData = waveData.AudioData.data();
    buffer.Flags = XAUDIO2_END_OF_STREAM; // Tell the source voice not to expect any data after this buffer
    if (loop) {
        buffer.LoopCount = XAUDIO2_LOOP_INFINITE;
    }

    hr = pSourceVoice->SubmitSourceBuffer(&buffer);
    if (FAILED(hr)) {
        OutputDebugStringA(("Failed to submit source buffer for: " + soundName + "\n").c_str());
        pSourceVoice->DestroyVoice(); // Clean up created voice
        return nullptr;
    }

    hr = pSourceVoice->SetVolume(volume);
    // Ignore volume failure?

    hr = pSourceVoice->SetFrequencyRatio(pitch);
    // Ignore pitch failure?

    hr = pSourceVoice->Start(0);
    if (FAILED(hr)) {
         OutputDebugStringA(("Failed to start source voice for: " + soundName + "\n").c_str());
         pSourceVoice->DestroyVoice();
         return nullptr;
    }

    // Add to active list (consider removing finished voices periodically)
    m_activeSoundEffects.push_back(pSourceVoice);

    return pSourceVoice;
}

void AudioManager::StopSoundEffect(IXAudio2SourceVoice*& pSourceVoice) {
     if (!pSourceVoice) return;

     pSourceVoice->Stop();
     pSourceVoice->FlushSourceBuffers(); // Clear submitted buffers
     pSourceVoice->DestroyVoice();       // Release the voice object
     pSourceVoice = nullptr;             // Null out the caller's pointer

     // Remove from active list (can be inefficient with vector, consider list or map if many voices)
     m_activeSoundEffects.erase(std::remove(m_activeSoundEffects.begin(), m_activeSoundEffects.end(), nullptr), m_activeSoundEffects.end());
     // This cleanup assumes the pointer passed in was previously in the vector, which might not be true
     // A safer approach is to find the pointer in the vector and remove it specifically
     auto it = std::find(m_activeSoundEffects.begin(), m_activeSoundEffects.end(), pSourceVoice); // Find the pointer before destroying
     if (it != m_activeSoundEffects.end()) {
         (*it)->Stop();
         (*it)->FlushSourceBuffers();
         (*it)->DestroyVoice();
         m_activeSoundEffects.erase(it);
     }
     pSourceVoice = nullptr; // Null out caller's pointer regardless
}

void AudioManager::PlayMusic(const std::string& soundName, float volume) {
    StopMusic(); // Stop previous music if any

    m_pMusicVoice = PlaySoundEffect(soundName, volume, 1.0f, true); // Loop music
    if (m_pMusicVoice) {
        m_currentMusicName = soundName;
    } else {
        m_currentMusicName.clear();
    }
}

void AudioManager::StopMusic() {
    if (m_pMusicVoice) {
        // We don't use StopSoundEffect here because music voice is managed separately
        m_pMusicVoice->Stop();
        m_pMusicVoice->FlushSourceBuffers();
        m_pMusicVoice->DestroyVoice();
        m_pMusicVoice = nullptr;
        m_currentMusicName.clear();
    }
}

void AudioManager::SetMusicVolume(float volume) {
    if (m_pMusicVoice) {
        m_pMusicVoice->SetVolume(volume);
    }
}

void AudioManager::SetMasterVolume(float volume) {
    if (m_pMasterVoice) {
        m_pMasterVoice->SetVolume(volume);
    }
}

void AudioManager::Suspend() {
    if (m_pXAudio2) {
        m_pXAudio2->StopEngine();
    }
}

void AudioManager::Resume() {
    if (m_pXAudio2) {
        m_pXAudio2->StartEngine();
    }
}
