#include "library.h"

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
#include <libavutil/avutil.h>
#include <libavcodec/bsf.h>
}

enum class MediaType {
    Video,
    Audio,
    Unknown
};

MediaType getMediaType(AVFormatContext* formatContext) {
    for (unsigned int i = 0; i < formatContext->nb_streams; ++i) {
        if (formatContext->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            return MediaType::Video;
        } else if (formatContext->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
            return MediaType::Audio;
        }
    }
    return MediaType::Unknown;
}

double getMediaDuration(const char* filePath) {
    AVFormatContext* formatContext = nullptr;
    if (avformat_open_input(&formatContext, filePath, nullptr, nullptr) != 0) {
        avformat_close_input(&formatContext);
        return 0;
    }
    if (avformat_find_stream_info(formatContext, nullptr) < 0) {
        avformat_close_input(&formatContext);
        return 0;
    }
    double duration = 0;
    MediaType type = getMediaType(formatContext);
    if (type == MediaType::Video) {
        duration = static_cast<double>(formatContext->duration) / AV_TIME_BASE;
    }
    if (type == MediaType::Audio) {
        for (unsigned int i = 0; i < formatContext->nb_streams; ++i) {
            if (formatContext->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
                duration = static_cast<double>(formatContext->duration) / AV_TIME_BASE;
                break;
            }
        }
    }
    avformat_close_input(&formatContext);
    return duration;
}

int isValidMediaFile(const char* filePath) {
    avformat_network_init();
    AVFormatContext* formatContext = avformat_alloc_context();
    if (!formatContext)
        return 0;
    if (avformat_open_input(&formatContext, filePath, nullptr, nullptr) != 0) {
        avformat_close_input(&formatContext);
        return 0;
    }
    avformat_close_input(&formatContext);
    return 1;
}

int convertMediaFormat(const char* srcFilePath, const char* destDirPath, const char* outputFileName, const char* outputFormat) {
    AVFormatContext* inputFormatContext = nullptr;
    AVFormatContext* outputFormatContext = nullptr;
    AVPacket packet;

    // Open the input file for reading
    AVDictionary* options = nullptr;
    av_dict_set(&options, "bsf:v", "h264_mp4toannexb", 0);
    if (avformat_open_input(&inputFormatContext, srcFilePath, nullptr, &options) != 0) {
        return 0; // Couldn't open file
    }

    // Retrieve stream information
    if (avformat_find_stream_info(inputFormatContext, nullptr) < 0) {
        return 0; // Couldn't find stream information
    }

    // Construct output file path
    char destFilePath[1024];
    snprintf(destFilePath, sizeof(destFilePath), "%s/%s.%s", destDirPath, outputFileName, outputFormat);

    // Open output file
    avformat_alloc_output_context2(&outputFormatContext, nullptr, outputFormat, destFilePath);
    if (!outputFormatContext) {
        return 0; // Couldn't create output context
    }

    // Create new streams for output file and copy the parameters from the input streams
    for (unsigned int i = 0; i < inputFormatContext->nb_streams; ++i) {
        AVStream* inputStream = inputFormatContext->streams[i];
        AVStream* outputStream = avformat_new_stream(outputFormatContext, nullptr);
        if (!outputStream) {
            return 0; // Failed to create new stream
        }

        if (avcodec_parameters_copy(outputStream->codecpar, inputStream->codecpar) < 0) {
            return 0; // Failed to copy parameters
        }
        outputStream->codecpar->codec_tag = 0;

        // Copy the frame rate from the input stream to the output stream
        outputStream->avg_frame_rate = inputStream->avg_frame_rate;
    }

    // Open output file for writing
    if (!(outputFormatContext->oformat->flags & AVFMT_NOFILE)) {
        if (avio_open(&outputFormatContext->pb, destFilePath, AVIO_FLAG_WRITE) < 0) {
            return 0; // Failed to open output file
        }
    }

    // Write header of output file
    if (avformat_write_header(outputFormatContext, nullptr) < 0) {
        return 0; // Failed to write header
    }

    // Read each packet and write to the output file
    while (av_read_frame(inputFormatContext, &packet) >= 0) {
        // Rescale packet timestamp to match output stream time base
        av_packet_rescale_ts(&packet, inputFormatContext->streams[packet.stream_index]->time_base, outputFormatContext->streams[packet.stream_index]->time_base);

        // Write packet to output file
        av_interleaved_write_frame(outputFormatContext, &packet);
        av_packet_unref(&packet);
    }

    // Write the trailer of the output file
    av_write_trailer(outputFormatContext);

    // Close input and output files
    avformat_close_input(&inputFormatContext);
    if (outputFormatContext && !(outputFormatContext->oformat->flags & AVFMT_NOFILE)) {
        avio_closep(&outputFormatContext->pb);
    }
    avformat_free_context(outputFormatContext);

    return 1; // Successful conversion
}