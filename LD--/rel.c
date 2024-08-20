
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "util.h"
#include "sym.h"
#include "str.h"



/* 再配置表 */
#define REL_SIZ  6000                       // 再配置表の大きさ


struct Reloc {                              // 再配置表
  int addr;                                 // ポインタのセグメント内 Offs
  int symx;                                 // シンボルテーブル上の番号
};

struct Reloc relTbl[REL_SIZ];               // 再配置表の定義
static int relIdx;                          // 表のどこまで使用したか


int getRelIdx(){    //使用した表の領域のゲッター
  return relIdx;
}

void setRelIdx(int num){    //使用した表の領域のセッター
  if(num >= REL_SIZ || num < 0){
    error("再配置表の値がおかしい");    //存在しない番地
    return; //念の為
  }
  relIdx = num;
}

int getRelTbl(int index,char *str){ //再配置表のゲッター
  if(index >= REL_SIZ || index < 0){
    error("再配置表の参照先がおかしい");    //存在しない番地
  }
  if(strcmp(str,"addr")==0) return relTbl[index].addr;
  else if(strcmp(str,"symx")==0) return relTbl[index].symx;
  else{
    error("再配置表の参照名がおかしい");
  }
  return -1;
}

void setRelTbl(int index, int newAddr, int newSymx){  //再配置表のセッター
  relTbl[index].addr=newAddr;
  relTbl[index].symx=newSymx;
}







/* 表がパンクしたときに使用する */
static void relTblError() {
  fprintf(stderr, "  再配置表がパンクした\t%5d/%5d\n", relIdx, REL_SIZ);
  exit(1);
}

void readRelTbl(int offs, int relSize, int symBase, int textBase,FILE* in){
  xSeek(offs);
  for (int i=0; i<relSize; i=i+4) {         // 再配置表の1エントリは4バイト
    int addr = getW(in) + textBase;           // 再配置アドレス
    int symx = getW(in) & 0x3fff;             // 名前表のエントリ番号
    symx = symx + symBase / 4;              //   名前表の1エントリは4バイト
    while (getSymTbl(symx,"type")/*symTbl[symx].type*/==SYMPTR)       // PTRならポインターをたぐる
      symx = getSymTbl(symx,"val"); /*symTbl[symx].val;*/              //   PTRを使用する再配置情報はない
    if (relIdx>=REL_SIZ) relTblError();
    if ((addr&1)!=0) fError("再配置表に奇数アドレスがある");
    relTbl[relIdx].addr = addr;
    relTbl[relIdx].symx = symx;             // PTRではなく本体を指す
    relIdx = relIdx + 1;
  }
}

void packSymTbl()  {                        // 名前表の不要エントリーを削除
  int i = 0;
  int symIdx = getSymIdx();
  while (i<symIdx) {                        // 全てのエントリーについて
    if (getSymTbl(i,"type")/*symTbl[i].type*/==SYMPTR) {           // PTRなら以下のように削除する
      for (int j=0; j<relIdx; j=j+1) {      //   再配置情報全てについて
	if (relTbl[j].symx>=i)              //     名前表の削除位置より後ろを
	  relTbl[j].symx=relTbl[j].symx-1;  //     参照しているインデクスを調整
      }
      for (int j=i; j<symIdx-1; j=j+1) {    //   名前表を前につめる
    setSymTbl(j,getSymTbl(j+1,"strx"),getSymTbl(j+1,"type"),getSymTbl(j+1,"val"));

	// symTbl[j].strx = symTbl[j+1].strx;
	// symTbl[j].type = symTbl[j+1].type;
	// symTbl[j].val  = symTbl[j+1].val;
      }
      setSymIdx(symIdx-1);
      //symIdx = symIdx - 1;                  //   名前表を縮小する
    } else
      i = i + 1;                            // PTR以外なら進める
  }
}

void writeRelTbl(FILE* out) {                       // 再配置表をファイルへ出力
  for (int i=0; i<relIdx; i=i+1) {
    int addr = relTbl[i].addr;
    int symx = relTbl[i].symx;
    int type = getSymTbl(symx,"type"); /*symTbl[symx].type;*/
    putW(addr,out);
    putW((type<<14) | symx, out);
  }
}

void printRelTbl() {                       // 再配置表をリスト出力
  printf("*** 再配置表 ***\n");
  printf("Addr\tName\tType\tNo.\n");
  for (int i=0; i<relIdx; i=i+1) {
    int addr = relTbl[i].addr;
    int symx = relTbl[i].symx;
    int type = getSymTbl(symx,"type");/*symTbl[symx].type;*/
    
    printf("%04x\t",addr);
    putStr(stdout,getSymTbl(symx,"strx")/*symTbl[symx].strx*/);
    printf("\t");
    printSymType(type);
    printf("\t%d\n", symx);
  }
  printf("\n");
}
