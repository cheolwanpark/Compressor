#include <stdio.h>
#include <string.h>
#include <stdlib.h>

typedef unsigned char byte;
typedef unsigned long long size;
typedef unsigned int uint;

struct buf {
    byte *mem;
    size len;
    size max;
};
void pushbuf(buf *buf, byte* data, size len);

void compress_huffman(const char *src, const char *dst);
void decompress_huffman(const char *src, const char *dst);

const char *HUFFMAN_HEADER_SYMBOL = "HFMN";
void huffman(byte *rbuf, size len, buf *r);
void dhuffman(byte *rbuf, size len, buf *d);

int main(int argc, char *argv[])
{
    if(argc !=4)
    {
        return -1;
    }

    if(!strcmp(argv[1], "c"))
    {
        compress_huffman(argv[2], argv[3]);
    }
    else if(!strcmp(argv[1], "d"))
    {
        decompress_huffman(argv[2], argv[3]);
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