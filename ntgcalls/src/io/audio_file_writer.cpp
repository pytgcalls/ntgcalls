//
// Created by Lauren on 07/10/24.
//

#include <ntgcalls/exceptions.hpp>
#include <ntgcalls/io/audio_file_writer.hpp>
#include <rtc_base/logging.h>

namespace ntgcalls::io {
    AudioFileWriter::AudioFileWriter(const std::string& path, media::BaseSink* sink): BaseIO(sink), ThreadedAudioMixer(sink) {
        source_ = std::ofstream(path, std::ios::binary);
        if (!source_) {
            RTC_LOG(LS_ERROR) << "Unable to open the file located at \"" << path << "\"";
            throw FileError("Unable to open the file located at \"" + path + "\"");
        }
    }

    AudioFileWriter::~AudioFileWriter() {
        if (source_.is_open()) {
            source_.close();
        }
        source_.clear();
        RTC_LOG(LS_VERBOSE) << "AudioFileWriter closed";
    }

    void AudioFileWriter::write(const bytes::unique_binary& data) {
        if (!source_ || source_.fail() || !source_.is_open()) {
            RTC_LOG(LS_WARNING) << "Error while writing to the file";
            throw FileError("Error while writing to the file");
        }
        source_.write(reinterpret_cast<const char*>(data.get()), sink_->frame_size());
        if (source_.fail()) {
            RTC_LOG(LS_ERROR) << "Error while writing to the file";
            throw FileError("Error while writing to the file");
        }
    }
} // ntgcalls::io