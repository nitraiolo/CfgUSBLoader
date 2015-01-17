#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>

// 0000:000000000000000000004A506A505A50499E0000000000000000000000000000
// 0020:00000000000000000000000000000000

#define dbg_printf(...) if (0) fprintf(stderr,__VA_ARGS__)
#define err_printf(...) fprintf(stderr,__VA_ARGS__)

struct
{
    int len;
    unsigned char glyph[16*2];
} table[0xFFFF];

int main()
{
    char line[100];
    char *s;
    int len;
    int i, j;
    unsigned id;
    unsigned x;

    memset(table, 0, sizeof(table));

    while (fgets(line, sizeof(line), stdin)) {
        dbg_printf("line: %s", line);
        s = strchr(line, '\r');
        if (s) *s = 0;
        s = strchr(line, '\n');
        if (s) *s = 0;
        if (line[4] != ':') goto err;
        if (sscanf(line, "%x", &id) != 1) goto err;
        dbg_printf("    : %04x:", id);
        len = strlen(line);
        if (len == 37) {
            table[id].len = 1;
            s = &line[5];
            for (i=0; i<16; i++) {
                sscanf(s, "%2x", &x);
                dbg_printf("%02x", x);
                table[id].glyph[i] = x;
                s += 2;
            }
            dbg_printf("\n");

        } else if (len == 69) {
            table[id].len = 2;
            s = &line[5];
            for (i=0; i<32; i++) {
                sscanf(s, "%2x", &x);
                dbg_printf("%02x", x);
                j = i / 2 + 16 * (i&1);
                table[id].glyph[j] = x;
                s += 2;
            }
            dbg_printf("\n           ");
            for (i=0; i<32; i++) {
                dbg_printf("%02x", table[id].glyph[i]);
            }
            dbg_printf("\n");

        } else {
            err:
            err_printf("ERR: %s\n", line);
        }
    }

    fwrite("UNIFONT", 8, 1, stdout);
    x = htonl(0xFFFF);
    fwrite(&x, sizeof(x), 1, stdout);
    x = 0;
    fwrite(&x, sizeof(x), 1, stdout);
    fwrite(&x, sizeof(x), 1, stdout);
    fwrite("INDX", 4, 1, stdout);
    int idx = 0;
    for (i=0; i<=0xFFFF; i++) {
        if (table[i].len) {
            x = (idx << 8) + table[i].len;
        } else {
            x = 0;
        }
        x = htonl(x);
        fwrite(&x, sizeof(x), 1, stdout);
        idx += table[i].len;
    }
    fwrite("GLYP", 4, 1, stdout);
    x = htonl(idx);
    fwrite(&x, sizeof(x), 1, stdout);
    for (i=0; i<=0xFFFF; i++) {
        if (table[i].len) {
            fwrite(table[i].glyph, 16 * table[i].len, 1, stdout);
        }
    }
    fwrite("END", 4, 1, stdout);


    return 0;
}

