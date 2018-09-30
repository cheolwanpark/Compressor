#include <stdio.h>
#include <string.h>
#include <stdlib.h>

typedef unsigned char byte;

void huffman(byte *rbuf, size_t len, byte *result);

int main()
{
    char in[512] = {0,};
    byte result[1024*32];
    scanf("%s", in);
    huffman((byte*)in, strlen(in), result); 
}

struct bits {
    unsigned int data[4];
    unsigned int len;
};

struct node {
    node *l;
    node *r;
    size_t freq;
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
    size_t max;
    size_t len;
};

void pushbit(bits *bits, int bit);
int getbit(bits *bits, int n);

void initqnpool(qnpool *pool, size_t max);
qnode *newqnode(qnpool *pool);
void insertqnode(qnode *root, qnode *newnode);
qnode *popqnode(qnode *root);


#include <queue>
void huffman(byte *rbuf, size_t len, byte *result)
{
    // counting frequency
    size_t freq[256] = {0,};
    for(size_t i=0; i<len; ++i)
    {
        ++freq[rbuf[i]];
    }

    // ready priority queue
    qnpool qnpool = {0, 0, 0};
    initqnpool(&qnpool, 256);
    // create root node 
    qnode *root = newqnode(&qnpool);
    root->val.c = root->val.freq = 0;
    root->val.r = root->val.l;
    root->next = 0;
    // insert nodes
    qnode *newnode = 0;
    for(int i=1; i<256; ++i)
    {
        if(freq[i] > 0)
        {
            newnode = newqnode(&qnpool);
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
        newnode = newqnode(&qnpool);
        newnode->val.freq = a->val.freq + b->val.freq;
        newnode->val.l = &(a->val);
        newnode->val.r = &(b->val);
        insertqnode(root, newnode);
    }

    // make code
    bits *codes = (bits*)malloc(sizeof(bits)*256);
    memset(codes, 0, sizeof(bits)*256);
    // stack for iterating tree
    node *visit[512] = {0,};
    unsigned int visitlen = 0;
    // init stack
    visit[0] = &(root->next->val);
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
    free(qnpool.qnodes);

    for(int i=0; i<256; ++i)
    {
        if(freq[i]>0)
        {
            printf("%c: ", i);
            for(int j=0; j<codes[i].len; ++j)
            {
                printf("%d", getbit(codes+i, j));
            }
            printf("\n");
        }
    }
    
    free(codes);
}

void pushbit(bits *bits, int bit)
{
    unsigned int *part = bits->data + (bits->len/32);
    if(bit)
    {
        (*part) = (*part)|(1<<(bits->len%32));
    }
    else
    {
        (*part) = (*part)&(~(1<<(bits->len%32)));
    }
    ++bits->len;
}

int getbit(bits *bits, int n)
{
    unsigned int *part = bits->data + (n/32);
    return (((*part)>>(n%32))&1);
}

void initqnpool(qnpool *pool, size_t max)
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