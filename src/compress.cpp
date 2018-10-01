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

// ==================== lzss ===========================
void compress_lzss(const char *src, const char *dst);
void decompress_lzss(const char *src, const char *dst);

const char *LZSS_HEADER_SYMBOL = "CLSS";
void lzss(byte *rbuf, size len, buf *r);
void dlzss(byte *rbuf, size len, buf *d);
// =====================================================

// ==================== huffman ========================
void compress_huffman(const char *src, const char *dst);
void decompress_huffman(const char *src, const char *dst);

const char *HUFFMAN_HEADER_SYMBOL = "HFMN";
void huffman(byte *rbuf, size len, buf *r);
void dhuffman(byte *rbuf, size len, buf *d);
// =====================================================

// ==================== both ===========================
void compress_both(const char *src, const char *dst);
void decompress_both(const char *src, const char *dst);
// =====================================================

const char *helpstr();

int main(int argc, char *argv[])
{
    // check options
    if(2 == argc && !strcmp(argv[1], "help"))
    {
        printf("%s\n", helpstr());
        return 0;
    }
    else if(5 != argc)
    {
        printf("invalid option, use 'Compress help'\n");
        exit(-1);
    }

    if(!strcmp(argv[1], "c"))
    {
        // check compress method
        if(!strcmp(argv[2], "lzss"))
        {
            compress_lzss(argv[3], argv[4]);
        }
        else if(!strcmp(argv[2], "huffman"))
        {
            compress_huffman(argv[3], argv[4]);
        }
        else if(!strcmp(argv[2], "both"))
        {
            compress_both(argv[3], argv[4]);
        }
        else
        {
            printf("invalid compress method\n");
        }
    }
    else if(!strcmp(argv[1], "d"))
    {
        // check compress method
        if(!strcmp(argv[2], "lzss"))
        {
            decompress_lzss(argv[3], argv[4]);
        }
        else if(!strcmp(argv[2], "huffman"))
        {
            decompress_huffman(argv[3], argv[4]);
        }
        else if(!strcmp(argv[2], "both"))
        {
            decompress_both(argv[3], argv[4]);
        }
        else
        {
            printf("invalid compress method\n");
        }
    }
    else
    {
        printf("invalid option, use 'Compress help'\n");
        exit(-1);
    }
    return 0;
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

// ==================== lzss ===========================
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
// =====================================================

// ==================== huffman ========================
void compress_huffman(const char *src, const char *dst)
{
    // open src file
    FILE *f = fopen(src, "rb");
    if(!f)
    {
        printf("failed open %s\n", src);
        exit(-1);
    }
    byte *rbuf = 0;
    size fs = 0;
    fseek(f, 0L, SEEK_END);
    fs = ftell(f);
    rewind(f);
    
    // read src file
    rbuf = (byte*)malloc(fs);
    fread(rbuf, 1, fs, f);
    fclose(f); f=0;

    // compress
    buf r = {0, 0, 0};
    r.mem = (byte*)malloc(r.max = 32*1024);
    huffman(rbuf, fs, &r);
    free(rbuf); rbuf=0;
    printf("huffman complete\n");

    // write to dst file
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

void decompress_huffman(const char *src, const char *dst)
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
    dhuffman(rbuf, fs, &decode);
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

struct bitvector {
    size len;
    size max;
    size memsize;
    byte *bits;
};

struct bits {
    byte data[32];
    uint len;
};

struct node {
    node *l;
    node *r;
    uint freq;
    bits code;
    byte c;
    byte pad[3];
};

struct qnode {
    node val;
    qnode *next;
};

struct qnpool {
    qnode *qnodes;
    uint max;
    uint len;
};

void initbitv(bitvector *vec, size len);
void vpushbit(bitvector *vec, bits *bit);
int vgetbit(bitvector *vec, size n);

void pushbit(bits *bits, int bit);
int getbit(bits *bits, int n);

void initqnpool(qnpool *pool, uint max);
qnode *newqnode(qnpool *pool);
void insertqnode(qnode *root, qnode *newnode);
qnode *popqnode(qnode *root);

node *makehuffmantree(uint *freq, qnpool *pool)
{
    // create root node 
    qnode *root = newqnode(pool);
    root->val.c = root->val.freq = 0;
    root->val.r = root->val.l;
    root->next = 0;
    // insert nodes
    qnode *newnode = 0;
    for(uint i=0; i<256; ++i)
    {
        if(freq[i] > 0)
        {
            newnode = newqnode(pool);
            newnode->val.c = i;
            newnode->val.freq = freq[i];
            newnode->val.r = newnode->val.l = 0;
            newnode->next = 0;
            insertqnode(root, newnode);
        }
    }

    // make tree
    qnode *a=0, *b=0;
    while(root->next->next)
    {
        a = popqnode(root);
        b = popqnode(root);
        newnode = newqnode(pool);
        newnode->val.freq = a->val.freq + b->val.freq;
        newnode->val.l = &(a->val);
        newnode->val.r = &(b->val);
        insertqnode(root, newnode);
    }

    return &(root->next->val);
}

void makehuffmancodes(node *root, bits *codes)
{
    // stack for iterating tree
    node *visit[512] = {0,};
    uint visitlen = 0;
    // init stack
    visit[0] = root;
    visitlen = 1;
    // iterate tree
    node *now = 0;
    while(visitlen > 0)
    {
        now = visit[--visitlen];
        if(now->r && now->l)
        {
            now->l->code = now->code;
            now->r->code = now->code;
            pushbit(&(now->l->code), 1);
            pushbit(&(now->r->code), 0);
            visit[visitlen++] = now->l;
            visit[visitlen++] = now->r;
        }
        else
        {
            codes[now->c] = now->code;
        }
    }
}

void huffman(byte *rbuf, size len, buf *r)
{
    // push head data
    pushbuf(r, (byte*)HUFFMAN_HEADER_SYMBOL, 4);
    pushbuf(r, (byte*)&len, 8);

    // counting frequency
    uint freq[256] = {0,};
    for(uint i=0; i<len; ++i)
    {
        ++freq[rbuf[i]];
    }
    pushbuf(r, (byte*)freq, 1024);

    // ready queue node pool
    qnpool qnpool = {0, 0, 0};
    initqnpool(&qnpool, 256);
    // make huffman tree
    printf("make huffman tree...       ");
    node *root = makehuffmantree(freq, &qnpool);
    printf("complete\n");

    // make code
    bits *codes = (bits*)malloc(sizeof(bits)*256);
    memset(codes, 0, sizeof(bits)*256);
    printf("make huffman codes...      ");
    makehuffmancodes(root, codes);
    printf("complete\n");
    free(qnpool.qnodes);

    // result bit vector
    bitvector bitvec = {0, 0, 0, 0};
    initbitv(&bitvec, 32*1024);
    // push bits
    for(uint i=0; i<len; ++i)
    {
        vpushbit(&bitvec, (codes+rbuf[i]));
        printf("\rcompressing %.1f%%", (double)i*100.0/len);
    }
    printf("\n");
    free(codes); codes=0;

    pushbuf(r, (byte*)&bitvec.len, 8); 
    pushbuf(r, (byte*)bitvec.bits, bitvec.len/8+1);
}

void dhuffman(byte *rbuf, size len, buf *d)
{
    // read header 
    char *symbol = (char*)rbuf;
    size dlen = *(size*)(rbuf+4);

    // compare symbol
    for(int i=0; i<4; ++i)
    {
        if(symbol[i] != HUFFMAN_HEADER_SYMBOL[i])
        {
            printf("unvalid file: different symbol(%4s)\n", symbol);
            printf("different at %d, lzss-symbol(%c) : readsymbol(%c)\n", i, symbol[i], HUFFMAN_HEADER_SYMBOL[i]);
            exit(-1);
        }
    }

    // read freq buffer
    uint *freq = (uint*)(rbuf+4+8);
    // ready queue node pool
    qnpool qnpool = {0, 0, 0};
    initqnpool(&qnpool, 256);
    // make huffman tree
    printf("make huffman tree...       ");
    node *root = makehuffmantree(freq, &qnpool);
    printf("complete\n");

    // read bitvector
    bitvector bitvec = {0, 0, 0, 0};
    bitvec.len = bitvec.max = *(size*)(rbuf+(4+8+1024));
    bitvec.bits = (byte*)(rbuf+(4+8+1024+8));

    // decode huffman codes
    byte *decode = (byte*)malloc(dlen);
    node *now = 0;
    int bit = 0;
    size bitidx=0;
    for(size i=0; i<dlen; ++i)
    {
        now = root;
        while(now->l && now->r)
        {
            bit = vgetbit(&bitvec, bitidx++);
            if(bit)
            {
                now = now->l;
            }
            else
            {
                now = now->r;
            }
        }
        decode[i] = now->c;
        printf("\rdecompressing %.1f%%", (double)i*100.0/dlen);
    }
    printf("\n");
    free(qnpool.qnodes);

    d->mem = decode;
    d->len = d->max = dlen;
}

void initbitv(bitvector *vec, size len)
{
    vec->bits = (byte*)malloc(vec->memsize=len);
    vec->max = vec->memsize*8;
    vec->len = 0;
}

void vpushbit(bitvector *vec, bits *bit)
{
    if(vec->len + bit->len >= vec->max)
    {
        vec->bits = (byte*)realloc(vec->bits, vec->memsize*2);
        memset((vec->bits + vec->memsize), 0, vec->memsize);
        vec->memsize *= 2;
        vec->max *= 2;
    }
    byte *part = 0;
    for(uint i=0; i<bit->len; ++i)
    {
        part = vec->bits + (vec->len/8);
        if(getbit(bit, i))
        {
            (*part) = (*part)|(1<<(vec->len%8));
        }
        else
        {
            (*part) = (*part)&(~(1<<(vec->len%8)));
        }
        ++(vec->len);
    }
}

int vgetbit(bitvector *vec, size n)
{
    byte *part = vec->bits + (n/8);
    return (((*part)>>(n%8))&1);
}

void pushbit(bits *bits, int bit)
{
    byte *part = bits->data + (bits->len/8);
    if(bit)
    {
        (*part) = (*part)|(1<<(bits->len%8));
    }
    else
    {
        (*part) = (*part)&(~(1<<(bits->len%8)));
    }
    ++bits->len;
}

int getbit(bits *bits, int n)
{
    byte *part = bits->data + (n/8);
    return (((*part)>>(n%8))&1);
}

void initqnpool(qnpool *pool, uint max)
{
    pool->qnodes = (qnode*)malloc((pool->max = max)*sizeof(qnode));
    memset(pool->qnodes, 0, pool->max*sizeof(qnode));
    pool->len = 0;
}

qnode *newqnode(qnpool *pool)
{
    if(pool->len >= pool->max)
    {
        pool->qnodes = (qnode*)realloc(pool->qnodes, pool->max*2);
        memset((pool->qnodes + pool->max), 0, pool->max);
        pool->max *= 2;
    }
    return &(pool->qnodes)[pool->len++];
}

void insertqnode(qnode *root, qnode *newnode)
{
    qnode *last = root;
    qnode *now = root->next;
    while(0 != now && newnode->val.freq >= now->val.freq)
    {
        last = now;
        now = now->next;
    }
    last->next = newnode;
    newnode->next = now;
}

qnode *popqnode(qnode *root)
{
    qnode *result = root->next;
    if(root->next)
    {
        root->next = root->next->next;
    }
    return result;
}
// =====================================================

// ==================== both ===========================
void compress_both(const char *src, const char *dst)
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

    // read file
    byte *rbuf = (byte*)malloc(fs);
    fread(rbuf, 1, fs, f);
    fclose(f); f=0;
    
    // ready result buffer for lzss
    buf r0 = {0, 0, 0};
    r0.mem = (byte*)malloc((r0.max=32*1024));
    memset(r0.mem, 0, r0.max);
    // compress lzss
    lzss(rbuf, fs, &r0);
    free(rbuf); rbuf=0;

    // ready result buffer for huffman
    buf r1 = {0, 0, 0};
    r1.mem = (byte*)malloc((r1.max=32*1024));
    memset(r1.mem, 0, r1.max);
    // compress huffman
    huffman(r0.mem, r0.len, &r1);
    free(r0.mem);

    // write compressed data to dst file
    f = fopen(dst, "wb");
    if(!f)
    {
        printf("failed create %s\n", dst);
        exit(-1);
    }
    fwrite(r1.mem, 1, r1.len, f);
    printf("compressed %llu -> %ld\n", fs, ftell(f));
    fclose(f); f=0;

    free(r1.mem);
}

void decompress_both(const char *src, const char *dst)
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
    
    // decode huffman
    buf d0 = {0, 0, 0};
    dhuffman(rbuf, fs, &d0);
    free(rbuf); rbuf=0;

    // decode lzss
    buf d1 = {0, 0, 0};
    dlzss(d0.mem, d0.len, &d1);
    free(d0.mem);

    // write to dst file
    f = fopen(dst, "wb");
    if(!f)
    {
        printf("failed create %s\n", dst);
        exit(-1);
    }
    fwrite(d1.mem, 1, d1.len, f);
    printf("decompressed %lld -> %ld\n", fs, ftell(f));
    fclose(f); f=0;

    free(d1.mem);
}
// =====================================================


const char *helpstr()
{
    return (
        "usage : Compress.exe <operation> <compress method> <in-file> <out-file>\n"
        "\n"
        "- operations\n"
        "c : compressing\n"
        "d : decompressing\n"
        "\n"
        "- compress method\n"
        "lzss : https://en.wikipedia.org/wiki/LZ77_and_LZ78\n"
        "huffman : https://en.wikipedia.org/wiki/Huffman_coding\n"
        "both : lzss + huffman\n"
        "\n"
    );
}