
/***********************************************************************************
 * Descri   : url parse functions
 *
 * Author   : Paul Yang, zhenahoji@gmail.com
 *
 * Create   : 2009-10-19
 *
 * Update   : 2009-10-19
 **********************************************************************************/
#ifndef PY_URL_H
#define PY_URL_H

#ifdef __cplusplus
extern "C" {
#endif

/*
 * func : parse url
 *
 * args : url, the input url
 *      : site, ssize, url site buffer and its size
 *      : port, url port
 *      : file, fsize, url file buffer and its size
 *
 * ret  : 0, succeed
 *      : else, failed.
 */
    int py_parse_url(const char *url, char *site, int ssize, int *port,
                     char *file, int fsize);


/*
 * func : get url site
 *
 * args : url, the input url
 *      : site, ssize, site buffer and its size
 *
 * ret  : 0,  succeed
 *      : -1, failed.
 */
    int py_get_site(const char *url, char *site, int ssize);




/*
 * func : decode url, change %XX%YY into multi-byte word
 *
 * args : src, srclen, source string and its length
 *
 * ret  : -1, error
 *      : else, length of result string
 */

    int decode_url(const char *src, int srclen, char *dest, int dsize);
    int encode_url(const char *src, int srclen, char *dest, int dsize);


#ifdef __cplusplus
}
#endif
#endif
