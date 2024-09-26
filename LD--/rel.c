
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "util.h"
#include "sym.h"
#include "str.h"
#include "rel.h"



/* 再配置表 */
static struct Reloc relTbl[REL_SIZ];               // 再配置表の定義
static int relIdx;                          // 表のどこまで使用したか

static int trSize = 0;
static int drSize = 0;




int getRelIdx(){    //使用した表の領域のゲッター
  return relIdx;
}

struct Reloc getRelTbl(int index){ //再配置表のゲッター
  if(index >= REL_SIZ || index < 0){
    error("再配置表の参照先がおかしい");    //存在しない番地
  }
  return relTbl[index];
}


int getTrSize(){
  return trSize;
}

int getDrSize(){
  return drSize;
}





/* 表がパンクしたときに使用する */  //utilに移動
// static void relTblError() {
//   fprintf(stderr, "  再配置表がパンクした\t%5d/%5d\n", relIdx, REL_SIZ);
//   exit(1);
// }

void readTrRelTbl(int offs, int cTrSize, int symBase, int segBase){
  trSize = trSize + cTrSize;
  readRelTbl(offs,cTrSize,symBase,segBase);
}

void readDrRelTbl(int offs, int cDrSize, int symBase, int segBase){
  drSize = drSize + cDrSize;
  readRelTbl(offs,cDrSize,symBase,segBase);
}

static void readRelTbl(int offs, int relSize, int symBase, int segBase){
  xSeekIn(offs);
  for (int i=0; i<relSize; i=i+4) {         // 再配置表の1エントリは4バイト
    int addr = getW() + adrBase;            // 再配置アドレス
    int symx = getW() & 0x3fff;             // 名前表のエントリ番号
    symx = symx + symBase / 4;              // 名前表の1エントリは4バイト
    while (getSymTbl(symx).type==SYMPTR)    // PTRならポインターをたぐる
      symx = getSymTbl(symx).val;           //   PTRを使用する再配置情報はない
    if (relIdx>=REL_SIZ) tblError("再配置表がパンクした",relIdx,REL_SIZ);
    if ((addr&1)!=0) fError("再配置表に奇数アドレスがある");
    relTbl[relIdx].addr = addr;
    relTbl[relIdx].symx = symx;             // PTRではなく本体を指す
    relIdx = relIdx + 1;
  }
}



void writeRelTbl() {                       // 再配置表をファイルへ出力
  for (int i=0; i<relIdx; i=i+1) {
    int addr = relTbl[i].addr;
    int symx = relTbl[i].symx;
    int type = getSymTbl(symx).type; 
    putW(addr);
    putW((type<<14) | symx);
  }
}

void printRelTbl() {                       // 再配置表をリスト出力
  printf("*** 再配置表 ***\n");
  printf("Addr\tName\tType\tNo.\n");
  for (int i=0; i<relIdx; i=i+1) {
    int addr = relTbl[i].addr;
    int symx = relTbl[i].symx;
    int type = getSymTbl(symx).type;
    
    printf("%04x\t",addr);
    printSymName(symx);
    putStr(stdout,getSymTbl(symx).strx); //シンボルテーブルの内容を印刷する関数を用意する
    printf("\t");
    printSymType(type);
    printf("\t%d\n", symx);
  }
  printf("\n");
}

void updateRelSymx(int ptrIdx){
  for (int j=0; j<relIdx; j=j+1) {      //   再配置情報全てについて
	      if (relTbl[j].symx>=i)              //     名前表の削除位置より後ろを
	        relTbl[j].symx=relTbl[j].symx-1;  //     参照しているインデクスを調整
      }
}