#include <stdio.h>
#include <string.h>
#include <stdlib.h>

typedef unsigned char byte;
typedef struct {
    byte *mem;
    size_t len;
    size_t max;
} buf;

void compress_lzss(const char *src, const char *dst);
void decompress_lzss(const char *src, const char *dst);

void push(buf &buf, byte* data, size_t len);
const char *LZSS_HEADER_SYMBOL = "CLSS";
void lzss(FILE *f, size_t len, buf &w);
void dlzss(FILE *f, size_t len, buf &d);
const char *helpstr();

int main(int argc, char *argv[])
{
    // check options
    if(5 != argc)
    {
        if(!strcmp(argv[1], "help"))
        {
            printf("%s\n", helpstr());
            return 0;
        }
        else
        {
            printf("invalid options\n");
            exit(-1);
        }
    }
    

    if(!strcmp(argv[1], "compress"))
    {
        // check compress method
        if(!strcmp(argv[2], "lzss"))
        {
            compress_lzss(argv[3], argv[4]);
        }
        else
        {
            printf("invalid compress method\n");
        }
    }
    else if(!strcmp(argv[1], "decompress"))
    {
        // check compress method
        if(!strcmp(argv[2], "lzss"))
        {
            decompress_lzss(argv[3], argv[4]);
        }
        else
        {
            printf("invalid compress method\n");
        }
    }
    else
    {
        printf("invalid option\n");
    }
    return 0;
}

void compress_lzss(const char *src, const char *dst)
{
    // window buffer for lzss
    buf w = {0, 0, 0};
    w.mem = (byte*)malloc((w.max=32*1024));
    memset(w.mem, 0, w.max);

    // open src file
    FILE *f = fopen(src, "rb");
    size_t fs = 0;
    fseek(f, 0L, SEEK_END);
    fs = ftell(f);
    rewind(f);
    printf("src file : %s\nfilesize : %lu\n", src, fs);

    // compress
    lzss(f, fs, w);
    fclose(f); f=0;

    // write compressed data to dst file
    f = fopen(dst, "wb");
    fwrite(LZSS_HEADER_SYMBOL, 1, 4, f);
    fwrite(&fs, 8, 1, f);
    fwrite(w.mem, 1, w.len, f);
    printf("compressed %ul -> %ul\n", fs, ftell(f));
    fclose(f);

    free(w.mem);
}

void decompress_lzss(const char *src, const char *dst)
{
    // open src file
    FILE *f = fopen(src, "rb");
    if(!f)
    {
        printf("failed open %s\n", src);
        exit(-1);
    }
    size_t fs = 0;
    fseek(f, 0L, SEEK_END);
    fs = ftell(f);
    rewind(f);
    printf("src file : %s\nfilesize : %lu\n", src, fs);
    
    // decode
    buf decode = {0, 0, 0};
    dlzss(f, fs, decode);
    fclose(f); f=0;

    // write to dst file
    f = fopen(dst, "wb");
    if(!f)
    {
        printf("failed create %s\n", dst);
        exit(-1);
    }
    fwrite(decode.mem, 1, decode.len, f);
    printf("decompressed %ul -> %ul\n", fs, ftell(f));
    fclose(f);

    free(decode.mem);
}

void push(buf &buf, byte* data, size_t len)
{
    while(buf.len + len > buf.max)
    {
        buf.mem = (byte*)realloc(buf.mem, buf.max*2);
        memset(buf.mem+buf.max, 0, buf.max);
        buf.max *= 2;
    }
    memcpy(buf.mem + buf.len, data, len);
    buf.len += len;
}

void lzss(FILE *f, size_t len, buf &w)
{
    const int MIN_COMPRESS_LEN = 4;

    // data struct
    struct {
        unsigned short p; 
        unsigned short l;
    } d, t;

    // read buffer
    byte *rbuf = (byte*)malloc(len);
    fread(rbuf, 1, len, f);

    // variable for process
    long long i=0, find=0, j=0, k=0;
    
    // write buffer
    buf wrt = {0, 0, 0};
    wrt.mem = (byte*)malloc((wrt.max=sizeof(d)*16));

    byte keybyte = 0;
    for(i=0; i<len;)
    {
        // init for make write buffer
        keybyte = 0;
        memset(wrt.mem, 0, wrt.max);
        wrt.len = 0;

        // write compressed data to write buffer
        for(j=0; j<8 && i<len; ++j)
        {
            find = i;
            d.p = d.l = 0;
            for(; i>=0 && find-i < 65535; --i) 
            {
                // find first byte
                if(rbuf[i] == rbuf[find])
                {
                    t.p = find - i;
                    t.l = 0;
                    // check pattern
                    for(k=0; i+k<find && find+k < len; ++k)
                    {
                        if(rbuf[i+k] != rbuf[find+k])
                        {
                            break;
                        }
                        else
                        {
                            ++t.l;
                        }
                    }

                    // check found pattern is the longest pattern
                    if(t.l > d.l)
                    {
                        d.p = t.p;
                        d.l = t.l;
                    }
                }
            }
            i = find;
            if(d.l > MIN_COMPRESS_LEN)
            {
                push(wrt, (byte*)&d, sizeof(d));
                keybyte += (1<<j);
                i += d.l;
            }
            else
            {
                push(wrt, rbuf+i, 1);
                   ++i;
               }
        }

        // write keybyte and writebuffer to window buffer
        push(w, &keybyte, 1);
        push(w, wrt.mem, wrt.len);

         // print percentage
        printf("\rcompressing %.1f%%", (double)i*100.0/len);
    }
    printf("\n");
    free(rbuf);
    free(wrt.mem);
}

void dlzss(FILE *f, size_t len, buf &d)
{
    // read header 
    size_t *cnkpos = 0;
    char symbol[4] = {0,};

    fread(symbol, 1, 4, f);
    for(int i=0; i<4; ++i)
    {
        if(symbol[i] != LZSS_HEADER_SYMBOL[i])
        {
            printf("unvalid file: different symbol(%4s)\n", symbol);
            printf("different at %d, lzss-symbol(%c) : readsymbol(%c)\n", i, symbol[i], LZSS_HEADER_SYMBOL[i]);
            exit(-1);
        }
    }

    // read and decompress data
    size_t dlen=0;
    fread(&dlen, 8, 1, f);
    byte *decode = (byte*)malloc(dlen);
    if(!decode)
    {
        printf("failed alloc memory for decode\n");
        exit(-1);
    }
    unsigned short p=0, l=0;
    for(size_t i=0, keybyte = 0, di=0, orgdi=0; !feof(f);)
    {
        fread(&keybyte, 1, 1, f);
        for(size_t j=0; j<8 && !feof(f); ++j)
        {
            if(keybyte & (1<<j))
            {
                fread(&p, 2, 1, f);
                fread(&l, 2, 1, f);
                orgdi = di;
                for(size_t k=0; k<l; ++k)
                    decode[di++] = decode[orgdi-p+k];
            }
            else
            {
                fread(&decode[di++], 1, 1, f);
            }
        }
        printf("\rdecompressing %.1f%%", (double)di*100.0/dlen);
    }
    printf("\n");

    d.mem = decode;
    d.len = d.max = dlen;
}

const char *helpstr()
{
    return (
        "usage : Compress.exe <operation> <compress method> <in-file> <out-file>\n"
        "\n"
        "- operations\n"
        "compress : compressing\ndecompress : decompressing\n"
        "\n"
        "- compress method\n"
        "lzss : https://en.wikipedia.org/wiki/LZ77_and_LZ78\n"
        "\n"
    );
}