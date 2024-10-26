#ifndef MEDIA_LIBRARY_LIBRARY_H
#define MEDIA_LIBRARY_LIBRARY_H

#ifdef __cplusplus
extern "C" {
#endif

double getMediaDuration(const char* filePath);
int isValidMediaFile(const char* filePath);
int convertMediaFormat(const char* srcFilePath, const char* destDirPath, const char* outputFileName, const char* outputFormat);
int generateThumbnail(const char* srcFilePath, const char* outputDirPath, const char* outputFileName, const char* outputFormat, int width, int height);
char** generateThumbnails(const char* srcFilePath, const char* outputDirPath, int width, int height, int numThumbnails);

#ifdef __cplusplus
}
#endif

#endif //MEDIA_LIBRARY_LIBRARY_H
