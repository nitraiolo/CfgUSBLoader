#ifndef _GETTEXT_H_
#define _GETTEXT_H_

#ifdef __cplusplus
extern "C" {
#endif


    bool gettextLoadLanguage(const char* langFile);
    void gettextCleanUp(void);
    /*
     * input msg = a text in ASCII
     * output = the translated msg in utf-8
    */
    const char *gettext(const char *msg);
#define gt(s) (char*)gettext(s)
	// gts can be used in static initializations
#define gts(s) s

char** translate_array(int n, char *src[], char *dest[]);

#ifdef __cplusplus
}
#endif

#endif /* _GETTEXT_H_ */
