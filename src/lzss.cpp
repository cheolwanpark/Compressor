#include <stdio.h>
#include <string.h>
#include <stdlib.h>

typedef unsigned char byte;
typedef unsigned int uint;
typedef unsigned long long size;

struct buf {
    byte *mem;
    size len;
    size max;
};
void pushbuf(buf *buf, byte* data, size len);

void compress_lzss(const char *src, const char *dst);
void decompress_lzss(const char *src, const char *dst);

const char *LZSS_HEADER_SYMBOL = "CLSS";
void lzss(byte *rbuf, size len, buf *r);
void dlzss(byte *rbuf, size len, buf *d);

int main(int argc, char *argv[])
{
    if(argc !=4)
    {
        return -1;
    }

    if(!strcmp(argv[1], "c"))
    {
        compress_lzss(argv[2], argv[3]);
    }
    else if(!strcmp(argv[1], "d"))
    {
        decompress_lzss(argv[2], argv[3]);
    }
}

void pushbuf(buf *buf, byte* data, size len)
{
    while(buf->len + len > buf->max)
    {
        buf->mem = (byte*)realloc(buf->mem, buf->max*2);
        memset(buf->mem+buf->max, 0, buf->max);
        buf->max *= 2;
    }
    memcpy(buf->mem + buf->len, data, len);
    buf->len += len;
}

void compress_lzss(const char *src, const char *dst)
{
    // open src file
    FILE *f = fopen(src, "rb");
    if(!f)
    {
        printf("failed open %s\n", src);
        exit(-1);
    }
    size fs = 0;
    fseek(f, 0L, SEEK_END);
    fs = ftell(f);
    rewind(f);
    printf("src file : %s\nfilesize : %llu\n", src, fs);
    
    // ready result buffer for lzss
    buf r = {0, 0, 0};
    r.mem = (byte*)malloc((r.max=32*1024));
    memset(r.mem, 0, r.max);
    // read file
    byte *rbuf = (byte*)malloc(fs);
    fread(rbuf, 1, fs, f);
    fclose(f); f=0;
    // compress
    lzss(rbuf, fs, &r);
    free(rbuf); rbuf=0;

    // write compressed data to dst file
    f = fopen(dst, "wb");
    if(!f)
    {
        printf("failed create %s\n", dst);
        exit(-1);
    }
    fwrite(r.mem, 1, r.len, f);
    printf("compressed %llu -> %ld\n", fs, ftell(f));
    fclose(f); f=0;

    free(r.mem);
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
    size fs = 0;
    fseek(f, 0L, SEEK_END);
    fs = ftell(f);
    rewind(f);
    printf("src file : %s\nfilesize : %llu\n", src, fs);

    // read buffer
    byte *rbuf = (byte*)malloc(fs);
    fread(rbuf, 1, fs, f);
    fclose(f); f=0;
    
    // decode
    buf decode = {0, 0, 0};
    dlzss(rbuf, fs, &decode);
    free(rbuf); rbuf=0;

    // write to dst file
    f = fopen(dst, "wb");
    if(!f)
    {
        printf("failed create %s\n", dst);
        exit(-1);
    }
    fwrite(decode.mem, 1, decode.len, f);
    printf("decompressed %lld -> %ld\n", fs, ftell(f));
    fclose(f); f=0;

    free(decode.mem);
}

void lzss(byte *rbuf, size len, buf *r)
{
    const int MIN_COMPRESS_LEN = 4;

    // push head data
    pushbuf(r, (byte*)LZSS_HEADER_SYMBOL, 4);
    pushbuf(r, (byte*)&len, 8);

    // data struct
    struct {
        unsigned short p; 
        unsigned short l;
    } d, t;

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
                        d = t;
                    }
                }
            }
            i = find;
            if(d.l > MIN_COMPRESS_LEN)
            {
                pushbuf(&wrt, (byte*)&d, sizeof(d));
                keybyte += (1<<j);
                i += d.l;
            }
            else
            {
                pushbuf(&wrt, rbuf+i, 1);
                ++i;
            }
        }

        // write keybyte and writebuffer to window buffer
        pushbuf(r, &keybyte, 1);
        pushbuf(r, wrt.mem, wrt.len);

         // print percentage
        printf("\rcompressing %.1f%%", (double)i*100.0/len);
    }
    printf("\n");
    free(wrt.mem);
}

void dlzss(byte *rbuf, size len, buf *d)
{
    // read header
    char *symbol = (char*)rbuf;
    size dlen=*(size*)(rbuf+4);

    // compare symbol
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
    byte *decode = (byte*)malloc(dlen);
    if(!decode)
    {
        printf("failed alloc memory for decode\n");
        exit(-1);
    }
    unsigned short p=0, l=0;
    for(size i=12, keybyte = 0, di=0, orgdi=0; i<len;)
    {
        keybyte = rbuf[i++];
        for(size j=0; j<8 && i<len; ++j)
        {
            if(keybyte & (1<<j))
            {
                p = *(unsigned short*)(rbuf+i); i+=2;
                l = *(unsigned short*)(rbuf+i); i+=2;
                orgdi = di;
                for(size k=0; k<l; ++k)
                    decode[di++] = decode[orgdi-p+k];
            }
            else
            {
                decode[di++] = rbuf[i++];
            }
        }
        printf("\rdecompressing %.1f%%", (double)di*100.0/dlen);
    }
    printf("\n");

    d->mem = decode;
    d->len = d->max = dlen;
}