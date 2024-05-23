#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "util.h"
#include "sym.h"

// #define boolean int                        // boolean 型のつもり
// #define true     1
// #define false    0
//#define true     1
//#define false    0
// #define WORD     2                         // 1ワード2バイト
// #define MAGIC    0x0107                    // .o 形式のマジック番号
// #define HDRSIZ   16                        // .o 形式のヘッダーサイズ

extern int strIdx;                                //表のどこまで使用したか
extern int textBase;                              // 現在の入力ファイルの
extern int dataBase;                              //   各セグメントの
extern int bssBase;                               //     ロードアドレス

extern int bssSize;
extern int symSize;


int strLen(int n) {                         // 文字列表中の文字列(n)の長さ
  int i = n;
  while(strTbl[i]!='\0')
    i = i + 1;
  return i - n + 1;                         // '\0' も数えた値を返す
}

/* 名前表 */
// #define SYM_SIZ  3000                       // 名前表の大きさ (<=16kエントリ)

struct SymTbl {                             // 名前表の型定義
  int strx;                                 // 文字列表の idx (14bitが有効)
  int type;                                 // type の意味は下に #define
  int val;                                  // 名前の値
};

#define SYMUNDF 0                           // 未定義ラベル
#define SYMTEXT 1                           // TEXTのラベル
#define SYMDATA 2                           // DATAのラベル
#define SYMBSS  3                           // BSSのラベル
#define SYMPTR  4                           // 表の他要素へのポインタ

struct SymTbl symTbl[SYM_SIZ];              // 名前表本体の定義
int symIdx = 0;                             // 表のどこまで使用したか

boolean cmpStr(int n, int m) {              // 文字列表[n]〜と[m]〜 を比較する
  for (int i=0; ; i=i+1) {
    char t = strTbl[n+i];
    char s = strTbl[m+i];
    if (t!=s) return false;                 //   異なる
    if (t=='\0') break;                     //   同じ
  }
  return true;
}

void readSymTbl(int offs, int sSize) {      // 名前表の読み込み
  xSeek(offs);                              // 名前表の位置に移動
  for (int i=0; i<sSize; i=i+4) {           // ファイルの名前表について
    int strx = getW();
    int type = (strx >> 14) & 0x3;          // 名前の型を分離
    strx = strIdx + (strx & 0x3fff);        // 名前のインデクスを分離
    int val  = getW();                      // 名前の値はセグメントの
    if (type==SYMTEXT)                      //   ロードアドレスにより変化する
      val = val + textBase;                 //     TEXTセグメントの場合
    else if (type==SYMDATA)                 //     DATAセグメントの場合
      val = val + dataBase;                 //     BSSセグメントの場合はサイズ
    if (symIdx>=SYM_SIZ) tblError("名前表がパンクした");
    symTbl[symIdx].strx = strx;             // 名前の綴
    symTbl[symIdx].type = type;             // 名前の型
    symTbl[symIdx].val  = val;              // 名前の値
    symIdx = symIdx + 1;
  }
}

void mergeStrTbl(int symIdxB,int strIdxB) { // 文字列表に新しく追加した綴りに
                                            //   重複があれば統合する
  for (int i=symIdxB; i<symIdx; i=i+1) {   // 追加された文字列について
    int idxI = symTbl[i].strx;
    if (idxI < strIdxB) continue;           //  既に統合済みなら処理しない
    for (int j=0; j<symIdxB; j=j+1) {      //  以前からある文字列と比較
      int idxJ = symTbl[j].strx;
      if (cmpStr(idxI, idxJ)) {             //  同じ綴が見つかったら
	int len=strLen(idxI);
	for (int k=i; k<symIdx; k=k+1) {    //  名前表の残り部分について
	  int idxK = symTbl[k].strx;
	  if (idxK == idxI)                 //   同じ文字列は
	    symTbl[k].strx = idxJ;          //     以前からある方を使用する
	  else if (idxK > idxI)             //   前につめる部分にある文字列は
	    symTbl[k].strx = idxK - len;    //     位置調整
	}
	for (int k=idxI; k<strIdx-len; k=k+1)//  文字列表から統合した綴り削除
	  strTbl[k] = strTbl[k+len];        //     文字列を前につめる
	strIdx = strIdx - len;              //   文字列表を縮小
	break;
      }
    }
  }
}

void mergeSymTbl() {                        // 名前の結合を行う
  for (int i=0; i<symIdx; i=i+1) {          // 全ての名前について
    int typeI = symTbl[i].type;
    if (strTbl[symTbl[i].strx]=='.')        // ローカルは無視する
      continue;
    for (int j=0; j<i; j=j+1) {
      int typeJ = symTbl[j].type;           // PTR以外で同じ綴りを探す
      if (typeJ!=SYMPTR && cmpStr(symTbl[i].strx,symTbl[j].strx)) {
	if (typeJ==SYMUNDF && typeI!=SYMUNDF) {        // 後ろ(i)に統合
	  symTbl[j].type = SYMPTR;
	  symTbl[j].val  = i;
	} else if (typeJ!=SYMUNDF && typeI==SYMUNDF) { // 前(j)に統合
	  symTbl[i].type = SYMPTR;
	  symTbl[i].val  = j;
	} else if (typeJ==SYMUNDF && typeI==SYMUNDF) { // 前(j)に統合
	  symTbl[i].type = SYMPTR;
	  symTbl[i].val  = j;
	} else if(typeJ==SYMBSS  && typeI==SYMDATA) {  // BSSとDATAはDATAに統合
	  bssSize = bssSize - symTbl[j].val;
	  symTbl[j].type = SYMPTR;
	  symTbl[j].val  = i;
	} else if(typeJ==SYMDATA  && typeI==SYMBSS) { // DATAとBSSもDATAに統合
	  bssSize = bssSize - symTbl[i].val;
	  symTbl[i].type = SYMPTR;
	  symTbl[i].val  = j;
	} else if (typeJ==SYMBSS && typeI==SYMBSS) {  // BSS同士は
	  int valJ = symTbl[j].val;
	  int valI = symTbl[i].val;
	  if (valJ<valI) {                            //   サイズの大きい方に
	    bssSize = bssSize - valJ;                 //      統合する
	    symTbl[j].type = SYMPTR;
	    symTbl[j].val  = i;
	  } else {
	    bssSize = bssSize - valI;
	    symTbl[i].type = SYMPTR;
	    symTbl[i].val  = j;
	  }
	} else {
	  putStr(stderr,symTbl[i].strx);
	  error(":ラベルの二重定義");
	}
	symSize = symSize - 4;                        // 1項目4バイト減少
	break;
      }
    }
  }
}

void writeSymTbl() {                        // 名前表をファイルへ出力
  for (int i=0; i<symIdx; i=i+1) {
    putW((symTbl[i].type<<14) | symTbl[i].strx);
    putW(symTbl[i].val);
  }
}

void printSymType(int type) {               // 名前の種類を印刷
  if (type==SYMTEXT) printf("TEXT");        //   = 1
  else if (type==SYMDATA) printf("DATA");   //   = 2
  else if (type==SYMBSS)  printf("BSS");    //   = 3
  else if (type==SYMUNDF) printf("UNDF");   //   = 0
  else error("printSymType:バグ");
}

void printSymTbl() {                        // 名前表をリストへ出力
  printf("*** 名前表 ***\n");
  printf("No.\tName\tType\tValue\n");
  for (int i=0; i<symIdx; i=i+1) {
    int strx = symTbl[i].strx;
    int type = symTbl[i].type;
    int val  = symTbl[i].val;

    printf("%d\t",i);
    putStr(stdout,strx);
    printf("\t");
    printSymType(type);
    printf("\t%04x\n", val&0xffff);
  }
}
