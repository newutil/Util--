#include <stdio.h>
#include <stdlib.h>
#include "util.h"
#include "str.h"

#define STR_SIZ  16000                      // 文字列表の大きさ(<=16kB)

// #define boolean int                        // boolean 型のつもり
// #define true     1
// #define false    0

char strTbl[STR_SIZ];                       // 文字列表
int  strIdx = 0;                            // 表のどこまで使用したか



int getStrIdx(){
  return strIdx;
}


char getStrTbl(int i){                      //文字列表の値を返す
  return strTbl[i];
}

int strLen(int n) {                         // 文字列表中の文字列(n)の長さ
  int i = n;
  while(strTbl[i]!='\0')
    i = i + 1;
  return i - n + 1;                         // '\0' も数えた値を返す
}


/* 表がパンクしたときに使用する */    //utilに移動
// static void strTblError() {
//   fprintf(stderr, "  文字列表がパンクした\t%5d/%5d\n", maxStrIdx, STR_SIZ);
//   exit(1);
// }

void readStrTbl(int offs) {                 // 文字列表の読み込み
  xSeek(offs);                              // 文字列表の位置に移動
  int c;
  while ((c=getB())!=EOF) {                 // EOFになるまで読み込む
    if (strIdx>=STR_SIZ) tblError("文字列表がパンクした",maxStrIdx, STR_SIZ);
    strTbl[strIdx] = c;
    strIdx = strIdx + 1;
  }
}

void writeStrTbl() {                        // 文字列表をファイルへ出力
  for (int i=0; i<strIdx; i=i+1) {          // 全ての文字について
    putB(strTbl[i]);                        //   出力する
  }
}


boolean cmpStr(int n, int m) {              // 文字列表[n]〜と[m]〜 を比較する
  for (int i=0; ; i=i+1) {
    char t = strTbl[n+i];
    char s = strTbl[m+i];
    if (t!=s) return false;                 //   異なる
    if (t=='\0') break;                     //   同じ
  }
  return true;
}

void putStr(FILE* fp,int n) {               // 文字列表の文字列[n]を表示する
  if (n>0x3fff || n>=strIdx) error("putStr:バグ");
  while (strTbl[n]!='\0') {
    putc(strTbl[n],fp);
    n = n + 1;
  }
}

void packStrTbl(int idxI,int len){   //  文字列表から統合した綴りを削除する
  for (int k=idxI; k<strIdx-len; k=k+1)
	  strTbl[k] = strTbl[k+len];        //     文字列を前につめる
	strIdx = strIdx - len;              //   文字列表を縮小
}

void mergeStrTbl(int strIdxB) { // 文字列表に新しく追加した綴りに
                                            //   重複があれば統合する
  
  
}