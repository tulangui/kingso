#ifndef UTILITY_COMPRESS_H
#define UTILITY_COMPRESS_H

#ifdef __cplusplus
extern "C" {
#endif

#define DEFLATE_CHUNK 16384

int compress_data(char *orig_data, int orig_len, char *compr_data, int *compr_len);
int uncompress_data(char *orig_data, int orig_len, char *uncompr_data, int *uncompr_len);

char* write_int(char *ptr, int n);
char* read_int(char *ptr, int *n);

#ifdef __cplusplus
};
#endif

#endif
