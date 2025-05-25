// Agrona
// Copyright (c) 2025 CGLJ08. All rights reserved.
// This project includes code derived from Microsoft's MSDN samples. See the LICENSE file for details.

#pragma once

#include "pch.h"
#include <map>
#include <string>
#include <vector>

// Structure to hold loaded wave data
struct WaveData {
    WAVEFORMATEX WaveFormat;
    std::vector<BYTE> AudioData;
};

class AudioManager {
public:
    AudioManager();
    ~AudioManager();

    bool Initialize();
    void Shutdown();

    // Load a WAV file (basic RIFF/WAV parsing without external libs)
    // Returns true on success, false on failure
    bool LoadWaveFile(const std::wstring& filename, const std::string& soundName);

    // Play a loaded sound effect
    // Returns the source voice used, or nullptr on failure
    IXAudio2SourceVoice* PlaySoundEffect(const std::string& soundName, float volume = 1.0f, float pitch = 1.0f, bool loop = false);

    // Stop a specific sound effect instance (requires the pointer returned by PlaySoundEffect)
    void StopSoundEffect(IXAudio2SourceVoice*& pSourceVoice); // Pass by ref to null it out

    // Functions for background music (can reuse PlaySoundEffect with looping, or add dedicated streaming)
    void PlayMusic(const std::string& soundName, float volume = 0.7f);
    void StopMusic();
    void SetMusicVolume(float volume);

    // Set master volume (0.0f to 1.0f+)
    void SetMasterVolume(float volume);

    // Suspend/Resume audio engine (e.g., when app loses focus)
    void Suspend();
    void Resume();

private:
    Microsoft::WRL::ComPtr<IXAudio2> m_pXAudio2;
    IXAudio2MasteringVoice* m_pMasterVoice = nullptr; // Not a ComPtr, managed by XAudio2 engine lifetime

    // Store loaded wave data
    std::map<std::string, WaveData> m_loadedSounds;

    // Store active sound effect voices (could manage pooling later)
    std::vector<IXAudio2SourceVoice*> m_activeSoundEffects;

    // Background music voice
    IXAudio2SourceVoice* m_pMusicVoice = nullptr;
    std::string m_currentMusicName;

    // Helper to find existing voice playing a specific sound (if needed)
    // IXAudio2SourceVoice* FindExistingVoice(const std::string& soundName);

    // Basic WAV file parser helper
    bool ParseWaveFile(const BYTE* fileData, size_t fileSize, WaveData& outWaveData);
};
