#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "util.h"
#include "str.h"
#include "sym.h"
#include "rel.h"

#define SYM_SIZ  3000                     // 名前表の大きさ (<=16kエントリ)

struct SymTbl symTbl[SYM_SIZ];            // 名前表本体の定義
static int symIdx = 0;                    // 表のどこまで使用したか
static int symSize = 0;                   // 出力ファイルのSYMSのサイズ

int getSymSize() {                        // symSizeを返す
  return symSize;
}

struct SymTbl getSymTbl(int index) {      // 名前表のゲッター
  if(index >= symIdx || index < 0) {
    error("名前表の参照先がおかしい");        // 存在しない番地
  }
  return symTbl[index];
}

// 名前表の読み込み
void readSymTbl(int offs, int sSize,int textBase,int dataBase) {

  symSize = symSize + sSize;              // サイズを加算

  xSeekIn(offs);                          // 名前表の位置に移動
  for (int i=0; i<sSize; i=i+4) {         // ファイルの名前表について
    int strx = getW();
    int type = (strx >> 14) & 0x3;        // 名前の型を分離
    strx = getStrIdx() + (strx & 0x3fff); // 名前のインデクスを分離
    int val  = getW();                    // 名前の値はセグメントの
    if (type==SYMTEXT)                    //  ロードアドレスにより変化する
      val = val + textBase;               //   TEXTセグメントの場合
    else if (type==SYMDATA)               //   DATAセグメントの場合
      val = val + dataBase;               //   BSSセグメントの場合はサイズ
    if (symIdx>=SYM_SIZ) tblError("名前表がパンクした", symIdx, SYM_SIZ);
    symTbl[symIdx].strx = strx;           // 名前の綴
    symTbl[symIdx].type = type;           // 名前の型
    symTbl[symIdx].val  = val;            // 名前の値
    symIdx = symIdx + 1;
  }
 }

int mergeSymTbl(int bssSize) {              // 名前の結合を行う
  for (int i=0; i<symIdx; i=i+1) {          // 全ての名前について
    int typeI = symTbl[i].type;
    if (isStrLocal(symTbl[i].strx)) {       // ローカルは無視する
      continue;
    }
    for (int j=0; j<i; j=j+1) {
      int typeJ = symTbl[j].type;           // PTR以外で同じ綴りを探す
      if (typeJ!=SYMPTR && cmpStr(symTbl[i].strx,symTbl[j].strx)) {
        if (typeJ==SYMUNDF && typeI!=SYMUNDF) {
          symTbl[j].type = SYMPTR;          // 後ろ(i)に統合
          symTbl[j].val  = i;
        } else if (typeJ!=SYMUNDF && typeI==SYMUNDF) {
          symTbl[i].type = SYMPTR;          // 前(j)に統合
          symTbl[i].val  = j;
        } else if (typeJ==SYMUNDF && typeI==SYMUNDF) {
          symTbl[i].type = SYMPTR;          // 前(j)に統合
          symTbl[i].val  = j;
        } else if(typeJ==SYMBSS  && typeI==SYMDATA) {
          bssSize = bssSize - symTbl[j].val;// BSSとDATAはDATAに統合
          symTbl[j].type = SYMPTR;
          symTbl[j].val  = i;
        } else if(typeJ==SYMDATA  && typeI==SYMBSS) {
          bssSize = bssSize - symTbl[i].val;// DATAとBSSもDATAに統合
          symTbl[i].type = SYMPTR;
          symTbl[i].val  = j;
        } else if (typeJ==SYMBSS && typeI==SYMBSS) {
          int valJ = symTbl[j].val;         // BSS同士は
          int valI = symTbl[i].val;
          if (valJ<valI) {                  //  サイズの大きい方に
            bssSize = bssSize - valJ;       //   統合する
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
        symSize = symSize - 4;               // 1項目4バイト減少
        break;
      }
    }
  }
  return bssSize;
}

//文字列表の統合に合わせて名前表のアドレスを調整する
void updateSymStrx(int curIdx, int changeIdx, int len) {
  for(int i=0; i<symIdx; i=i+1) {
    int idxI = symTbl[i].strx;
    if(idxI == changeIdx) {                 // 統合した文字列を指しているならば
      symTbl[i].strx = curIdx;              // 以前からある方に合わせる
    } else if(idxI > changeIdx) {           // 前に詰めた部分にあるものは
      symTbl[i].strx = idxI - len;          // 位置調整 
    }
  }
}

// 名前表をファイルへ出力
void writeSymTbl() {
  for (int i=0; i<symIdx; i=i+1) {
    putW((symTbl[i].type<<14) | symTbl[i].strx);
    putW(symTbl[i].val);
  }
}

// 名前表をリストへ出力
void printSymTbl() {
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

void printSymType(int type) {               // 名前の種類を印刷
  if (type==SYMTEXT) printf("TEXT");        //   = 1
  else if (type==SYMDATA) printf("DATA");   //   = 2
  else if (type==SYMBSS)  printf("BSS");    //   = 3
  else if (type==SYMUNDF) printf("UNDF");   //   = 0
  else error("printSymType:バグ");
}

// 名前表の中から一つの名前(文字列)を印刷
void printSymName(int symx) {
  putStr(stdout,symTbl[symx].strx);
}

// 名前表の不要エントリーを削除
void packSymTbl() {
  int i = 0;
  while (i<symIdx) {                        // 全てのエントリーについて
    if (symTbl[i].type==SYMPTR) {           // PTRなら以下のように削除する
      updateRelSymx(i);                     // 再配置表のインデクスを調整する
      for (int j=i; j<symIdx-1; j=j+1) {    // 名前表を前につめる
        symTbl[j] = symTbl[j+1];
      }
      symIdx = symIdx - 1;                  // 名前表を縮小する
    } else {
      i = i + 1;                            // PTR以外なら進める
    }
  }
}
