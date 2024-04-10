#ifndef MEDIA_LIBRARY_LIBRARY_H
#define MEDIA_LIBRARY_LIBRARY_H


#ifdef __cplusplus
extern "C" {
#endif

double getVideoDuration(const char* filePath);
double getAudioDuration(const char* audioPath);
int isValidMediaFile(const char* filePath);

#ifdef __cplusplus
}
#endif

#endif //MEDIA_LIBRARY_LIBRARY_H
