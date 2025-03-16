//
// Created by Laky64 on 07/10/24.
//

#include <ntgcalls/exceptions.hpp>
#include <ntgcalls/io/audio_file_writer.hpp>
#include <rtc_base/logging.h>

namespace ntgcalls {
    AudioFileWriter::AudioFileWriter(const std::string& path, BaseSink* sink): BaseIO(sink), ThreadedAudioMixer(sink) {
        source = std::ofstream(path, std::ios::binary);
        if (!source) {
            RTC_LOG(LS_ERROR) << "Unable to open the file located at \"" << path << "\"";
            throw FileError("Unable to open the file located at \"" + path + "\"");
        }
    }

    AudioFileWriter::~AudioFileWriter() {
        if (source.is_open()) {
            source.close();
        }
        source.clear();
        RTC_LOG(LS_VERBOSE) << "AudioFileWriter closed";
    }

    void AudioFileWriter::write(const bytes::unique_binary& data) {
        if (!source || source.fail() || !source.is_open()) {
            RTC_LOG(LS_WARNING) << "Error while writing to the file";
            throw FileError("Error while writing to the file");
        }
        source.write(reinterpret_cast<const char*>(data.get()), sink->frameSize());
        if (source.fail()) {
            RTC_LOG(LS_ERROR) << "Error while writing to the file";
            throw FileError("Error while writing to the file");
        }
    }
} // ntgcalls