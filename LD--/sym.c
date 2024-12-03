#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "util.h"
#include "str.h"
#include "sym.h"
#include "rel.h"

#define SYM_SIZ  3000              // 名前表の大きさ (<=16kエントリ)

struct SymTbl symTbl[SYM_SIZ];     // 名前表本体の定義
int symIdx = 0;                    // 表のどこまで使用したか
int symSize = 0;                   // 出力ファイルのSYMSのサイズ

int preSymIdx = 0;                 // 一時保存用symIdx
int preSymSize = 0;                // 一時保存用symSize

int getSymIdx() {                         // symIdxを返す
  return symIdx;
}

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

// 名前の結合を行う
int mergeSymTbl(int bssSize) {
  for (int i=0; i<symIdx; i=i+1) {          // 全ての名前について
    int typeI = symTbl[i].type;
                                            // ローカルなものと
                                            // ARCV,PTRは無視する
    if (typeI==SYMPTR || typeI==SYMARCV
        || isStrLocal(symTbl[i].strx)) {
      continue;
    }
    for (int j=0; j<i; j=j+1) {
      int typeJ = symTbl[j].type;           // ARCVとPTR以外で同じ綴りを探す
      if (typeJ!=SYMARCV && typeJ!=SYMPTR 
        && cmpStr(symTbl[i].strx,symTbl[j].strx)) {
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
          printf("No.%d\t",i);
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
    if(symTbl[i].type == SYMARCV) {         // ライブラリのラベルは無視
      continue;
    }        
    else if(idxI == changeIdx) {            // 統合した文字列を指しているならば
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
    int val  = symTbl[i].val;  //デバッグのためptrとARCVも表示するようにしている
    if(type==SYMARCV){
      printf("%d\t",i);
      printf("%d",strx);
      printf("\tARCV");
      printf("\t%04x\n", val&0xffff);
    }
    else {
    printf("%d\t",i);
    putStr(stdout,strx);
    printf("\t");
    if(type==SYMPTR){
      printf("PTR");
    } else printSymType(type);
    printf("\t%04x\n", val&0xffff);
    }
  }
}

// 名前の種類を印刷
void printSymType(int type) {
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
    if (symTbl[i].type==SYMARCV) {          // ARCVなら以下のように削除する
      updateRelSymx(i);                     // 再配置表のインデクスを調整する
      for (int j=i; j<symIdx-1; j=j+1) {    // 名前表を前につめる
        symTbl[j] = symTbl[j+1];
      }
      symIdx = symIdx - 1;                  // 名前表を縮小する
    }

    else if (symTbl[i].type==SYMPTR) {      // PTRなら以下のように削除する
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

//名前解決が可能か調べる
boolean checkSymMerge(int startIdx) {
  for (int i=startIdx; i<preSymIdx; i=i+1) {// 調べる名前について
    int typeI = symTbl[i].type;
    if (isStrLocal(symTbl[i].strx)          // ローカルと
        || typeI == SYMPTR                  // ポインタと
        || typeI == SYMARCV) {              // ARCVは無視する    
      continue;
    }                                       // ライブラリ関数から
    for (int j=preSymIdx; j<symIdx; j=j+1) {// 読み込んだ名前表に対して
      int typeJ = symTbl[j].type;           // 同じ綴りのものを探す
      if (cmpStr(symTbl[i].strx,symTbl[j].strx)) {
                                            // 名前解決できるなら
        if((symTbl[i].type == SYMUNDF && symTbl[j].type != SYMUNDF)
        || (symTbl[i].type == SYMBSS && symTbl[j].type == SYMDATA)) {
          return true;                      // trueを返す
        }
      }
    }
  }
  return false;
}

// ライブラリ関数の位置を記録した特別なシンボルを末尾に追加する
// strxにargvの番号、valに内容の開始アドレスを記入
void addSymArcv(int num, int addr) {
  symTbl[symIdx].strx = num;
  symTbl[symIdx].type = SYMARCV;
  symTbl[symIdx].val = addr;
  symIdx = symIdx + 1;
}

//シンボルテーブルを保存
void saveSymTbl() {
  preSymIdx = symIdx;
  preSymSize = symSize;
}

//保存したシンボルテーブルをロード
void loadSymTbl() {
  symIdx = preSymIdx;
  symSize = preSymSize;
}