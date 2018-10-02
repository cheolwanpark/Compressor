#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#define td typedef
#define e(i) exit(i)
#define scm(a, b) strcmp(a, b)
#define pt printf
#define st struct

                  td char ch;td const char cch;td unsigned char bt;td unsigned int ui;td unsigned short uii;td unsigned long long sz;td long long li;
            td double db;st b{bt *m;sz l;sz mx;};void pb(b*,bt*,sz);void cml(cch*,cch*);void dcl(cch*,cch*);cch *lhsy="LZSS";void lz(bt*,sz,b*);void dlz(bt
        *,sz,b*);void cmh(cch*,cch*);void dch(cch*,cch*);cch*hfsy = "HFMN";void hf(bt*,sz,b*);void dhf(bt*,b*);cch*btsy="BTLH";void cmb(cch*,cch*);void dcb(cch*
        ,cch*);cch*hp();int main(int ac,ch*a[]){if(2==ac&&!scm(a[1],"help")){pt("%s\n",hp());e(0);}else if(5!=ac){pt("invalid option, use 'Compress help'\n");e(
        -1);}if(!scm(a[1],"c")){if(!scm(a[2],"lzss")){cml(a[3],a[4]);}else if(!scm(a[2],"huffman")){cmh(a[3], a[4]);}else if(!scm(a[2],"both")){cmb(a[3],a[4]);}
        else{pt("invalid compress method\n");}}else if(!scm(a[1],"d")){if(!scm(a[2],"lzss")){dcl(a[3], a[4]);}else if(!scm(a[2],"huffman")){dch(a[3],a[4]);}else
        if(!scm(a[2],"both")){dcb(a[3],a[4]);}else{pt("invalid compress method\n");}}else{pt("invalid option, use 'Compress help'\n");e(-1);}}void pb(b *b,bt* d
        ,sz l){while(b->l+l>b->mx){b->m=(bt*)realloc(b->m,b ->mx*2);memset(b->m+b ->mx,0,b->mx);b->mx*=2;} memcpy(b->m+b->l,d,l);b->l+=l;}void cml(cch*s,cch*d){
        FILE*f=fopen(s,"rb");if(!f){pt("failed open %s\n",s);e(-1);}sz fs=0;fseek(f,0,2);fs=ftell(f);rewind(f);pt("src file : %s\nfilesize : %llu\n",s,fs);b r={
      0,0,0};r.m=(bt*)malloc((r.mx=32*1024));memset(r.m, 0,r.mx); bt*rd=(bt*)malloc( fs); fread(rd,1,fs,f);fclose(f); f=0;lz(rd,fs,&r);free(rd);rd=0;f=fopen (d,
     "wb");if(!f){pt("failed create %s\n",d);e(-1);}fwrite(r.m,1,r.l,f);pt("compressed %llu -> %ld\n",fs,ftell(f));fclose(f);f=0;free(r.m);}void dcl(cch*s,cch*d
                    ){FILE *f=fopen(s,"rb");if(!f){pt("failed"                                              " open %s\n",s);e(-1);}sz fs=0;fseek(f,0,2);fs=ftell
                    (f);rewind(f);pt("src file : %s\nfilesize"                                              " : %llu\n",s,fs);bt *rb=(bt*)malloc(fs); fread(rb,1
            ,fs,f);fclose(f);f=0;b dc={0,0,0};dlz(rb,fs,&dc);free(rb);rb=0                                  ;f=fopen(d,"wb");if(!f){pt("failed create %s\n",d);e
            (-1);}fwrite(dc.m,1,dc.l,f);pt("decompressed %lld -> %ld\n",fs                                  ,ftell(f));fclose(f);f=0; free(dc.m);}void lz(bt *rb
            ,sz l,b*r){ui mcl=4;pb(r,(bt*)lhsy,4);pb(r,(bt*)&l,8);st{uii p                                  ;uii l;}d,t;li i=0,f=0,j=0,k=0;b w={0,0,0};w.m=(bt*)
            malloc((w.mx=64));bt kb=0;for(i=0;i<l;){kb=0;memset(w.m,0,w.mx                                  );w.l=0;for(j=0;j<8&&i<l;++j){f=i;d.p=d.l=0;for(;i>=
            0&&f-i<65535;--i){if(rb[i]==rb[f]){t.p=f-i;t.l=0;for(k=0;i+k<f                                  &&f+k<l;++k){if(rb[i+k]!=rb[f+k]){break;}else{++t.l;
            }}if(t.l>d.l){d=t;}}}i=f;if(d.l>mcl){pb(&w,(bt*)&d,sizeof(d));                                  kb+=(1<<j);i+=d.l;}else{pb(&w,rb+i,1);++i;}}pb(r,&kb
            ,1);pb(r,w.m,w.l);pt("\rcompressing %.1f%%",(db)i*100.0/l);}pt                                  ("\n");free(w.m);}void dlz(bt*r,sz s,b*d){ch*sy=(ch*
            )r;sz dl=*(sz*)(r+4);for(int i=0;i<4;++i){if(sy[i]!=lhsy[i]) {                                  pt("unvalid file: incorrect symbol(%4s)\n",sy);e(-1)
            ;}}bt*dc=(bt*)malloc(dl);if(!dc){pt("failed alloc memory for "                                  "decode\n");e(-1);}uii p=0,l=0;for(sz i=12,kb=0,di=0
            ,od=0;i<s;){kb=r[i++];for(sz j=0;j<8&&i<s;++j){if(kb&(1<<j)){p                                  =*(uii*)(r+i);i+=2;l=*(uii*) (r+i);i+=2;od=di;for(sz 
            k=0;k<l;++k){dc[di++]=dc[od-p+k];}}else{dc[di++]=r[i++];}} pt(                                  "\rdecompressing %.1f%%",(db)di*100.0/dl);}pt("\n");
            d->m=dc;d->l=d->mx=dl;}void cmh(cch*s,cch*d){FILE*f=fopen(s,""                                  "rb");if(!f){pt("failed open %s\n",s);e(-1);}bt*rb=0
            ;sz fs=0;fseek(f,0,2);fs=ftell(f);rewind(f);rb=(bt*)malloc(fs)                                  ;fread(rb,1,fs,f);fclose(f);f=0;b r={0,0,0};r.m=(bt*
            )malloc(r.mx=32*1024);hf(rb,fs,&r);free(rb);rb=0;f=fopen(d,"w"                                  "b");if(!f){pt("failed create %s\n",d);e(-1);}fwrite
            ( r.m,1, r.l,f);pt("compres""sed %llu -> %ld\n",fs, ftell(f));                                  fclose(f);f=0 ;free(r.m);}void dch(cch*s,cch*d){FILE
            *f=fopen(s,"rb");if(!f){pt("failed open %s\n",s);e(-1);}sz fs=                                  0;fseek(f,0,2);fs=ftell(f);rewind(f);pt("src file :"
            " %s\nfilesize : %llu\n",s,fs);bt*r=(bt*)malloc(fs);fread(r,1,                                  fs,f);fclose(f);f=0;b dc={0,0,0};dhf(r,&dc);free(r);
            r=0;f=fopen(d,"wb");if(!f){pt("failed create %s\n",d);e(-1) ;}                                  fwrite(dc.m,1,dc.l,f );pt("decompressed %lld -> %ld"
            "\n",fs,ftell(f));fclose(f);f=0;free(dc.m);}st bv{sz l;sz m;sz                                  ms;bt *b;};st bi{bt d[32];ui l;};st n{n*l;n *r;ui f;
            bi cd;bt c;bt p[3];};st qn{n a;qn *n;};st qp{qn*d;ui m;ui l;};                                  void pb(bi*bs,int b){bt*p=bs->d+(bs->l/8);if(b){(*p)
            =(*p)|(1<<(bs->l%8));}else{(*p)=(*p)&(~(1<<(bs->l%8)));}++bs->                                  l;}int gb(bi*b,int n){bt*p=b->d+(n/8);return(((*p)>>
            (n%8))&1);}void ibv(bv*v,sz l){v->b=(bt*)malloc(v->ms=l);v->m=                                  v->ms*8;v->l=0;}void vpb (bv*v,bi*b){if(v->l+b->l>=v
            ->m){v->b=(bt*)realloc(v->b,v->ms*2);memset((v->b+v->ms),0,v->                                  ms);v->ms*=2;v->m*=2;}bt*p=0;for(ui i=0;i<b->l;++i){
                    p=v->b+(v->l/8);if(gb(b,i)){(*p)=(*p)|(1<<                                              (v->l%8));}else{ (*p)=(*p)&(~(1<<( v->l%8)));}++(v->
                    l) ;}}int vgb(bv *v,sz n){bt *part=v->b+(n                                              /8);return(((*part)>>(n%8))&1);}void iqp(qp*p,ui m){
     p->d=(qn*)malloc((p->m=m)*sizeof(qn));memset(p->d,0,p->m*sizeof(qn));p->l=0;}qn*nqn(qp*p){if(p->l>=p->m){p->d=(qn*)realloc(p->d,p->m*2);memset((p->d+p->m),
      0,p->m);p->m*=2;}return&(p->d)[p->l++];}void iqn(qn*r,qn*nn){qn*l=r;qn*n=r->n;while(0!=n&&nn->a.f>=n->a.f){l=n;n=n->n;}l->n=nn;nn->n=n;}qn*pqn(qn*r){qn*o=
        r->n;if(r->n){r->n=r->n->n;}return o;}n *mht(ui*f,qp*p){qn*r=nqn(p);r->a.c=r->a.f=0;r->a.r=r->a.l;r->n=0;qn*n=0;for(ui i=0;i<256;++i){if(f[i]>0){n=nqn(p
        );n->a.c=i;n->a.f=f[i];n->a.r=n->a.l=0;n->n=0;iqn(r,n);}}qn*a=0,*b=0;while(r->n->n){a=pqn(r);b=pqn(r);n=nqn(p);n->a.f=a->a.f+b->a.f;n->a.l=&(a->a);n->a.
        r=&(b->a);iqn(r,n);}return&(r->n->a);}void mhc(n*r,bi*c){n*v[512]={0,};ui vl=0;v[0]=r;vl=1;n*n=0;while(vl>0){n=v[--vl];if(n->r&&n->l){n->l->cd=n->cd;n->
        r->cd=n->cd;pb(&(n->l->cd),1);pb(&(n->r->cd),0);v[vl++]=n->l;v[vl++]=n->r;}else{c[n->c]=n->cd;}}}void hf(bt*rb,sz l,b*r){pb(r,(bt*)hfsy,4);pb(r,(bt*)&l,
        8);ui f[256]={0,};for(ui i=0;i<l;++i){++f[rb[i]];}pb(r,(bt*)f,1024);qp qp= {0,0,0};iqp(&qp,256);n*rt=mht(f,&qp);bi*c=(bi*)malloc(sizeof(bi)*256);memset(
        c,0,sizeof(bi)*256);mhc(rt,c);free(qp.d);bv b={0,0,0,0};ibv(&b,32*1024);for(ui i=0;i<l;++i){vpb(&b,(c+rb[i]));pt("\rcompressing %.1f%%",(db)i*100.0/l);}
        pt("\n");free(c);c=0;pb(r,(bt*)&b.l,8);pb(r,(bt*)b.b,b.l/8+1);}void dhf(bt*r,b*d){ch*s=(ch*)r;sz dl=*(sz*)(r+4);for(int i=0;i<4;++i){if(s[i]!= hfsy[i]){
          pt("unvalid file: in""correct symbol(%4s)\n",s);e(-1);}}ui*f=(ui*)(r+4+8);qp qp={0,0,0};iqp(&qp,256);n*rt=mht(f,&qp);bv b={0,0,0,0};b.l=b.m =*(sz*)(
            r+(4+8+1024));b.b=(bt*)(r+(4+8+1024+8));bt*dc=(bt*)malloc(dl);n*n=0;int bt=0;sz bi=0;for(sz i=0;i<dl;++i){n=rt;while(n->l&&n->r){bt=vgb(&b,bi++)
                  ;if(bt){n =n->l;}else{n=n->r;}}dc [i]=n->c;pt("\rdecompressing %.1f%%",(db)i*100.0/dl);} pt("\n");free(qp.d);d->m=dc;d->l=d->mx=dl;}
                       void cmb(cch*s,cch*d){FILE*f=fopen(s
                       ,"rb");if(!f){pt("failed open %s\n",
                       s);e(-1);}sz fs=0; fseek(f,0,2); fs=
                       ftell(f); rewind(f);pt("src file : "
                       "%s\nfilesize : %llu\n",s,fs);bt*r=(
                       bt*) malloc(fs); fread(r, 1, fs, f);
                       fclose(f);f=0;b r0={0,0,0};r0.m=(bt*
                       ) malloc((r0.mx=32*1024));memset(r0.
                m,0,r0.mx);pt("lzss\n");lz(r,fs,&r0);free(r);r=0;pt
                ("lzss complete\n");b r1={0,0,0};r1.m=(bt*)malloc((
                r1.mx=32*1024));memset(r1.m,0,r1.mx);pt("huffman\n"
                );hf(r0.m,r0.l,&r1);free(r0.m);pt("huffman complet"
"e\n");f=fopen(d, "wb");if(!f){pt("failed create %s\n",d);e(-1);}fwrite(btsy,1,4,f)
;fwrite(r1.m,1,r1.l,f); pt("compressed %llu -> %ld\n", fs, ftell(f));fclose(f);f=0;
free(r1.m);}void dcb(cch*s,cch*d){FILE*f=fopen(s,"rb");if(!f){pt("failed open %s\n"
                ,s);e(-1);}sz fs=0;fseek(f,0,2);fs=ftell(f);rewind(
                f);pt("src file : %s\nfilesize : %llu\n",s,fs);bt*r
                =(bt*)malloc(fs);fread(r,1,fs,f);fclose(f);f=0; ch*
sy=(ch*)r;for(int i=0;i<4;++i){if(sy[i]!=btsy[i]){pt("unvalid file: incorrect symb"
"ol(%4s)\n",sy);e(-1);}}pt("huffman\n");b d0={0,0,0}; dhf(r+4,&d0);free(r); r=0;pt(
"huffman complete\n"); pt("lzss\n");b d1={0,0,0}; dlz(d0.m,d0.l,&d1);free(d0.m);pt(
                "lzss complete\n");f=fopen(d,"wb");if(!f){pt("fail"
                "ed create %s\n",d);e(-1);}fwrite(d1.m,1,d1.l,f);pt
                ("decompressed %lld -> %ld\n",fs,ftell(f));fclose(f
                );f=0;free(d1.m);}cch*hp(){return("usage : Compres"
                "s.exe <operation> <compress method> <in-file> <ou"
                       "t-file>\n\n- operations\nc : compr"
                       "essing\nd : decompressing\n\n- com"
                       "press method\nlzss : https://en.wi"
                       "kipedia.org/wiki/LZ77_and_LZ78\nhu"
                       "ffman : https://en.wikipedia.org/w"
                       "iki/Huffman_coding\nboth : lzss + "
                                "huffman\n\n");}
