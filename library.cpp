#include "library.h"

extern "C" {
#include <libavformat/avformat.h>
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