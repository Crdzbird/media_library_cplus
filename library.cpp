#include "library.h"

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
#include <libavutil/avutil.h>
#include <libavcodec/bsf.h>
#include <jpeglib.h>
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
        }
        if (formatContext->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
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

int saveAsJPEG(const char* filename, uint8_t* buffer, int width, int height, int stride) {
    struct jpeg_compress_struct cinfo{};
    struct jpeg_error_mgr jerr{};

    // Open the output file
    FILE* outfile = fopen(filename, "wb");
    if (!outfile) {
        fprintf(stderr, "Error: Could not open output file %s\n", filename);
        return -1;
    }

    // Initialize the JPEG compression object
    cinfo.err = jpeg_std_error(&jerr);
    jpeg_create_compress(&cinfo);
    jpeg_stdio_dest(&cinfo, outfile);

    // Set the image parameters
    cinfo.image_width = width;
    cinfo.image_height = height;
    cinfo.input_components = 3; // RGB
    cinfo.in_color_space = JCS_RGB;
    jpeg_set_defaults(&cinfo);
    jpeg_set_quality(&cinfo, 85, TRUE); // Quality (0-100)

    // Start compression
    jpeg_start_compress(&cinfo, TRUE);

    // Write each row of the image
    while (cinfo.next_scanline < cinfo.image_height) {
        JSAMPROW row_pointer[1] = {buffer + cinfo.next_scanline * stride};
        jpeg_write_scanlines(&cinfo, row_pointer, 1);
    }

    // Finish compression
    jpeg_finish_compress(&cinfo);
    fclose(outfile);
    jpeg_destroy_compress(&cinfo);

    return 0; // Success
}

int generateThumbnail(const char* srcFilePath, const char* outputDirPath, const char* outputFileName, const char* outputFormat, int width, int height) {
    AVFormatContext* formatContext = nullptr;
    AVCodecContext* codecContext = nullptr;
    AVFrame* frame = nullptr;
    AVFrame* frameRGB = nullptr;
    AVPacket packet;
    struct SwsContext* swsContext = nullptr;
    int videoStreamIndex = -1;
    int ret = 0;

    // Open input file
    if (avformat_open_input(&formatContext, srcFilePath, nullptr, nullptr) != 0) {
        return -1; // Couldn't open file
    }

    // Retrieve stream information
    if (avformat_find_stream_info(formatContext, nullptr) < 0) {
        avformat_close_input(&formatContext);
        return -1; // Couldn't find stream information
    }

    // Find the first video stream
    for (unsigned int i = 0; i < formatContext->nb_streams; ++i) {
        if (formatContext->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            videoStreamIndex = int(i);
            break;
        }
    }
    if (videoStreamIndex == -1) {
        avformat_close_input(&formatContext);
        return -1; // Didn't find a video stream
    }

    // Get codec parameters and find decoder
    AVCodecParameters* codecParameters = formatContext->streams[videoStreamIndex]->codecpar;
    auto* codec = const_cast<AVCodec *>(avcodec_find_decoder(codecParameters->codec_id));
    if (!codec) {
        avformat_close_input(&formatContext);
        return -1; // Codec not found
    }

    // Allocate codec context
    codecContext = avcodec_alloc_context3(codec);
    if (!codecContext) {
        avformat_close_input(&formatContext);
        return -1; // Could not allocate codec context
    }

    // Copy codec parameters to codec context
    if (avcodec_parameters_to_context(codecContext, codecParameters) < 0) {
        avcodec_free_context(&codecContext);
        avformat_close_input(&formatContext);
        return -1; // Could not copy codec parameters
    }

    // Open codec
    if (avcodec_open2(codecContext, codec, nullptr) < 0) {
        avcodec_free_context(&codecContext);
        avformat_close_input(&formatContext);
        return -1; // Could not open codec
    }

    // Allocate frames
    frame = av_frame_alloc();
    frameRGB = av_frame_alloc();
    if (!frame || !frameRGB) {
        avcodec_free_context(&codecContext);
        avformat_close_input(&formatContext);
        return -1; // Could not allocate frame
    }

    // Allocate buffer for RGB frame
    int numBytes = av_image_get_buffer_size(AV_PIX_FMT_RGB24, width, height, 1);
    auto* buffer = (uint8_t*)av_malloc(numBytes * sizeof(uint8_t));
    if (!buffer) {
        av_frame_free(&frame);
        av_frame_free(&frameRGB);
        avcodec_free_context(&codecContext);
        avformat_close_input(&formatContext);
        return -1; // Could not allocate buffer
    }

    // Set up the frameRGB with the buffer
    av_image_fill_arrays(frameRGB->data, frameRGB->linesize, buffer, AV_PIX_FMT_RGB24, width, height, 1);

    // Initialize SWS context for software scaling
    swsContext = sws_getContext(codecContext->width, codecContext->height, codecContext->pix_fmt, width, height, AV_PIX_FMT_RGB24, SWS_BILINEAR, nullptr, nullptr, nullptr);
    if (!swsContext) {
        av_free(buffer);
        av_frame_free(&frame);
        av_frame_free(&frameRGB);
        avcodec_free_context(&codecContext);
        avformat_close_input(&formatContext);
        return -1; // Could not initialize SWS context
    }

    // Read frames and save the first frame as a thumbnail
    while (av_read_frame(formatContext, &packet) >= 0) {
        if (packet.stream_index == videoStreamIndex) {
            if (avcodec_send_packet(codecContext, &packet) == 0) {
                if (avcodec_receive_frame(codecContext, frame) == 0) {
                    // Convert the image from its native format to RGB
                    sws_scale(swsContext, (uint8_t const* const*)frame->data, frame->linesize, 0, codecContext->height, frameRGB->data, frameRGB->linesize);

                    // Construct thumbnail file path
                    char thumbnailFilePath[1024];
                    snprintf(thumbnailFilePath, sizeof(thumbnailFilePath), "%s/%s.%s", outputDirPath, outputFileName, outputFormat);

                    // Save as JPEG
                    if (strcmp(outputFormat, "jpeg") == 0 || strcmp(outputFormat, "jpg") == 0) {
                        if (saveAsJPEG(thumbnailFilePath, frameRGB->data[0], width, height, frameRGB->linesize[0]) == 0) {
                            ret = 1; // Success
                        }
                    }
                    break;
                }
            }
        }
        av_packet_unref(&packet);
    }

    // Free resources
    sws_freeContext(swsContext);
    av_free(buffer);
    av_frame_free(&frame);
    av_frame_free(&frameRGB);
    avcodec_free_context(&codecContext);
    avformat_close_input(&formatContext);

    return ret;
}

char** generateThumbnails(const char* srcFilePath, const char* outputDirPath, int width, int height, int numThumbnails) {
    char** thumbnails = new char*[numThumbnails];
    AVFormatContext* formatContext = nullptr;
    AVCodecContext* codecContext = nullptr;
    AVFrame* frame = nullptr;
    AVFrame* frameRGB = nullptr;
    AVPacket packet;
    struct SwsContext* swsContext = nullptr;
    int videoStreamIndex = -1;

    // Open input file
    if (avformat_open_input(&formatContext, srcFilePath, nullptr, nullptr) != 0) {
        return thumbnails; // Couldn't open file
    }

    // Retrieve stream information
    if (avformat_find_stream_info(formatContext, nullptr) < 0) {
        avformat_close_input(&formatContext);
        return thumbnails; // Couldn't find stream information
    }

    // Find the first video stream
    for (unsigned int i = 0; i < formatContext->nb_streams; ++i) {
        if (formatContext->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            videoStreamIndex = int(i);
            break;
        }
    }
    if (videoStreamIndex == -1) {
        avformat_close_input(&formatContext);
        return thumbnails; // Didn't find a video stream
    }

    // Get codec parameters and find decoder
    AVCodecParameters* codecParameters = formatContext->streams[videoStreamIndex]->codecpar;
    auto* codec = const_cast<AVCodec *>(avcodec_find_decoder(codecParameters->codec_id));
    if (!codec) {
        avformat_close_input(&formatContext);
        return thumbnails; // Codec not found
    }

    // Allocate codec context
    codecContext = avcodec_alloc_context3(codec);
    if (!codecContext) {
        avformat_close_input(&formatContext);
        return thumbnails; // Could not allocate codec context
    }

    // Copy codec parameters to codec context
    if (avcodec_parameters_to_context(codecContext, codecParameters) < 0) {
        avcodec_free_context(&codecContext);
        avformat_close_input(&formatContext);
        return thumbnails; // Could not copy codec parameters
    }

    // Open codec
    if (avcodec_open2(codecContext, codec, nullptr) < 0) {
        avcodec_free_context(&codecContext);
        avformat_close_input(&formatContext);
        return thumbnails; // Could not open codec
    }

    // Allocate frames
    frame = av_frame_alloc();
    frameRGB = av_frame_alloc();
    if (!frame || !frameRGB) {
        avcodec_free_context(&codecContext);
        avformat_close_input(&formatContext);
        return thumbnails; // Could not allocate frame
    }

    // Allocate buffer for RGB frame
    int numBytes = av_image_get_buffer_size(AV_PIX_FMT_RGB24, width, height, 1);
    auto* buffer = (uint8_t*)av_malloc(numBytes * sizeof(uint8_t));
    if (!buffer) {
        av_frame_free(&frame);
        av_frame_free(&frameRGB);
        avcodec_free_context(&codecContext);
        avformat_close_input(&formatContext);
        return thumbnails; // Could not allocate buffer
    }

    // Set up the frameRGB with the buffer
    av_image_fill_arrays(frameRGB->data, frameRGB->linesize, buffer, AV_PIX_FMT_RGB24, width, height, 1);

    // Initialize SWS context for software scaling
    swsContext = sws_getContext(codecContext->width, codecContext->height, codecContext->pix_fmt, width, height, AV_PIX_FMT_RGB24, SWS_BILINEAR, nullptr, nullptr, nullptr);
    if (!swsContext) {
        av_free(buffer);
        av_frame_free(&frame);
        av_frame_free(&frameRGB);
        avcodec_free_context(&codecContext);
        avformat_close_input(&formatContext);
        return thumbnails; // Could not initialize SWS context
    }

    // Read frames and save thumbnails
    int frameCount = 0;
    while (av_read_frame(formatContext, &packet) >= 0 && frameCount < numThumbnails) {
        if (packet.stream_index == videoStreamIndex) {
            if (avcodec_send_packet(codecContext, &packet) == 0) {
                if (avcodec_receive_frame(codecContext, frame) == 0) {
                    // Convert the image from its native format to RGB
                    sws_scale(swsContext, (uint8_t const* const*)frame->data, frame->linesize, 0, codecContext->height, frameRGB->data, frameRGB->linesize);

                    // Construct thumbnail file path
                    char thumbnailFilePath[1024];
                    snprintf(thumbnailFilePath, sizeof(thumbnailFilePath), "%s/thumbnail_%d.ppm", outputDirPath, frameCount);

                    // Save the frame to a file
                    FILE* file = fopen(thumbnailFilePath, "wb");
                    if (file) {
                        fprintf(file, "P6\n%d %d\n255\n", width, height);
                        for (int y = 0; y < height; y++) {
                            fwrite(frameRGB->data[0] + y * frameRGB->linesize[0], 1, width * 3, file);
                        }
                        fclose(file);
                        thumbnails[frameCount] = thumbnailFilePath;
                        frameCount++;
                    }
                }
            }
        }
        av_packet_unref(&packet);
    }

    // Free resources
    sws_freeContext(swsContext);
    av_free(buffer);
    av_frame_free(&frame);
    av_frame_free(&frameRGB);
    avcodec_free_context(&codecContext);
    avformat_close_input(&formatContext);

    return thumbnails;
}


