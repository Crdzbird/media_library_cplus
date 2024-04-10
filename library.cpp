#include "library.h"

extern "C" {
#include <libavformat/avformat.h>
}


double getVideoDuration(const char* filePath) {
    AVFormatContext* formatContext = nullptr;
    if (avformat_open_input(&formatContext, filePath, nullptr, nullptr) != 0) {
        avformat_close_input(&formatContext);
        return 0;
    }
    if (avformat_find_stream_info(formatContext, nullptr) < 0) {
        avformat_close_input(&formatContext);
        return 0;
    }
    double duration = static_cast<double>(formatContext->duration) / AV_TIME_BASE;
    avformat_close_input(&formatContext);
    return duration;
}

double getAudioDuration(const char* audioPath) {
    AVFormatContext* formatContext = nullptr;
    if (avformat_open_input(&formatContext, audioPath, nullptr, nullptr) != 0) {
        avformat_close_input(&formatContext);
        return 0;
    }
    if (avformat_find_stream_info(formatContext, nullptr) < 0) {
        avformat_close_input(&formatContext);
        return 0;
    }
    double duration = 0;
    for (unsigned int i = 0; i < formatContext->nb_streams; ++i) {
        if (formatContext->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
            duration = static_cast<double>(formatContext->duration) / AV_TIME_BASE;
            break;
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