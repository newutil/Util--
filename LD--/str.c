#include <stdio.h>
#include <stdlib.h>
#include "util.h"
#include "str.h"
#include "sym.h"

#define STR_SIZ  16000               // 文字列表の大きさ(<=16kB)

int  strIdx = 0;              // 表のどこまで使用したか
char strTbl[STR_SIZ];         // 文字列表
int  preStrIdx = 0;

int getStrIdx(){
  return strIdx;
}

//文字列表の値がローカルなら正を返す
boolean isStrLocal(int i) {
  return strTbl[i]=='.';
}

// 文字列表中の文字列(n)の長さを返す
int strLen(int n) {
  int i = n;
  while(strTbl[i]!='\0')
    i = i + 1;
  return i - n + 1;    // '\0' も数える
}

// 文字列表[n]〜と[m]〜 を比較する
boolean cmpStr(int n, int m) {
  for (int i=0; ; i=i+1) {
    char t = strTbl[n+i];
    char s = strTbl[m+i];
    if (t!=s) return false;    //   異なる
    if (t=='\0') break;        //   同じ
  }
  return true;
}

// 文字列表の文字列[n]を表示する
void putStr(FILE* fp,int n) {
  if (n>0x3fff || n>=strIdx) error("putStr:バグ");
  while (strTbl[n]!='\0') {
    putc(strTbl[n],fp);
    n = n + 1;
  }
}

// 文字列表から統合した綴りを削除する
void packStrTbl(int idxI,int len) {
  for (int k=idxI; k<strIdx-len; k=k+1) {
    strTbl[k] = strTbl[k+len];       //   文字列を前につめる
  }
  strIdx = strIdx - len;             //   文字列表を縮小
}

// 文字列表の読み込み
void readStrTbl(int offs) {
  xSeekIn(offs);                     // 文字列表の位置に移動
  int c;
  while ((c=getB())!=EOF && isInLibRange(offs)) { // EOFになるまで読み込む
    if (strIdx>=STR_SIZ) tblError("文字列表がパンクした", strIdx, STR_SIZ);
    strTbl[strIdx] = c;
    strIdx = strIdx + 1;
    offs = offs + 1;
  }
}

// 文字列表に新規追加の綴に，以前からの綴と重複があれば統合
void mergeStrTbl(int strIdxB) {

  // 追加された全ての綴について
  for (int idxI=strIdxB; idxI<strIdx; idxI=idxI+strLen(idxI)) {
    boolean isMerged = false;

    do{
      isMerged = false;
      // 以前からある全ての綴と比較
      for (int idxJ=0; idxJ<strIdxB; idxJ=idxJ+strLen(idxJ)) {
        if(cmpStr(idxI, idxJ)) {            // 同じ綴が見つかったら

          int len = strLen(idxI);
          updateSymStrx(idxJ, idxI, len);   // 名前表のアドレスを調整して
          packStrTbl(idxI,len);             // 文字列表から統合した綴りを削除
          isMerged = true;
          break;                         // 同じ綴は複数存在しない
        }
      }
    }while(isMerged && idxI < strIdx); // 統合が起きたなら次の文字は今いるところ
  }
}

// 文字列表をファイルへ出力
void writeStrTbl() {
  for (int i=0; i<strIdx; i=i+1) {        // 全ての文字について
    putB(strTbl[i]);                      // 出力する
  }
}

// 文字列表をセーブ
void saveStrTbl() {
  preStrIdx = strIdx;
}

// セーブした文字列表をロード
void rollbackStrTbl() {
  strIdx = preStrIdx;
}