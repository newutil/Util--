#include <stdio.h>
#include <stdlib.h>
#include "util.h"
#include "str.h"
#include "sym.h"

#define STR_SIZ  16000               // 文字列表の大きさ(<=16kB)

// #define boolean int               // boolean 型のつもり
// #define true     1
// #define false    0

static int  strIdx = 0;              // 表のどこまで使用したか
static char strTbl[STR_SIZ];         // 文字列表


int getStrIdx(){
  return strIdx;
}

boolean isStrLocal(int i) {          //文字列表の値がローカルなら正を返す
  return strTbl[i]=='.';
}

int strLen(int n) {                  // 文字列表中の文字列(n)の長さ
  int i = n;
  while(strTbl[i]!='\0')
    i = i + 1;
  return i - n + 1;                  // '\0' も数えた値を返す
}

boolean cmpStr(int n, int m) {       // 文字列表[n]〜と[m]〜 を比較する
  for (int i=0; ; i=i+1) {
    char t = strTbl[n+i];
    char s = strTbl[m+i];
    if (t!=s) return false;          //   異なる
    if (t=='\0') break;              //   同じ
  }
  return true;
}

void putStr(FILE* fp,int n) {        // 文字列表の文字列[n]を表示する
  if (n>0x3fff || n>=strIdx) error("putStr:バグ");
  while (strTbl[n]!='\0') {
    putc(strTbl[n],fp);
    n = n + 1;
  }
}

void packStrTbl(int idxI,int len) {  //  文字列表から統合した綴りを削除する
  for (int k=idxI; k<strIdx-len; k=k+1) {
    strTbl[k] = strTbl[k+len];       //   文字列を前につめる
  }
  strIdx = strIdx - len;             //   文字列表を縮小
}

void readStrTbl(int offs) {          // 文字列表の読み込み
  xSeekIn(offs);                     // 文字列表の位置に移動
  int c;
  while ((c=getB())!=EOF) {          // EOFになるまで読み込む
    if (strIdx>=STR_SIZ) tblError("文字列表がパンクした", strIdx, STR_SIZ);
    strTbl[strIdx] = c;
    strIdx = strIdx + 1;
  }
}
// 文字列表に新規追加の綴に，以前からの綴と重複があれば統合
void mergeStrTbl(int strIdxB) {
  // 追加された全ての綴について
  for (int idxI=strIdxB; idxI<strIdx; idxI=idxI+strLen(idxI)) {
    // 以前からある全ての綴と比較
    for (int idxJ=0; idxJ<strIdxB; idxJ=idxJ+strLen(idxJ)) {
      if(cmpStr(idxI, idxJ)) {              // 同じ綴が見つかったら
        int len = strLen(idxI);
        updateSymStrx(idxJ, idxI, len);     // 名前表のアドレスを調整して
        packStrTbl(idxI,len);               // 文字列表から統合した綴りを削除
        break;                              // 同じ綴は複数存在しない
      }
    }
  }
}

void writeStrTbl() {                        // 文字列表をファイルへ出力
  for (int i=0; i<strIdx; i=i+1) {          // 全ての文字について
    putB(strTbl[i]);                        // 出力する
  }
}