
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define boolean int                        // boolean 型のつもり
#define true     1
#define false    0
#define WORD     2                         // 1ワード2バイト
#define MAGIC    0x0107                    // .o 形式のマジック番号
#define HDRSIZ   16                        // .o 形式のヘッダーサイズ


/* 再配置表 */
#define REL_SIZ  6000                       // 再配置表の大きさ

struct Reloc {                              // 再配置表
  int addr;                                 // ポインタのセグメント内 Offs
  int symx;                                 // シンボルテーブル上の番号
};

struct Reloc relTbl[REL_SIZ];               // 再配置表の定義
int relIdx;                                 // 表のどこまで使用したか

void readRelTbl(int offs, int relSize, int symBase, int textBase){
  xSeek(offs);
  for (int i=0; i<relSize; i=i+4) {         // 再配置表の1エントリは4バイト
    int addr = getW() + textBase;           // 再配置アドレス
    int symx = getW() & 0x3fff;             // 名前表のエントリ番号
    symx = symx + symBase / 4;              //   名前表の1エントリは4バイト
    while (symTbl[symx].type==SYMPTR)       // PTRならポインターをたぐる
      symx = symTbl[symx].val;              //   PTRを使用する再配置情報はない
    if (relIdx>=REL_SIZ) tblError("再配置表がパンクした");
    if ((addr&1)!=0) fError("再配置表に奇数アドレスがある");
    relTbl[relIdx].addr = addr;
    relTbl[relIdx].symx = symx;             // PTRではなく本体を指す
    relIdx = relIdx + 1;
  }
}

void packSymTbl()  {                        // 名前表の不要エントリーを削除
  int i = 0;
  while (i<symIdx) {                        // 全てのエントリーについて
    if (symTbl[i].type==SYMPTR) {           // PTRなら以下のように削除する
      for (int j=0; j<relIdx; j=j+1) {      //   再配置情報全てについて
	if (relTbl[j].symx>=i)              //     名前表の削除位置より後ろを
	  relTbl[j].symx=relTbl[j].symx-1;  //     参照しているインデクスを調整
      }
      for (int j=i; j<symIdx-1; j=j+1) {    //   名前表を前につめる
	symTbl[j].strx = symTbl[j+1].strx;
	symTbl[j].type = symTbl[j+1].type;
	symTbl[j].val  = symTbl[j+1].val;
      }
      symIdx = symIdx - 1;                  //   名前表を縮小する
    } else
      i = i + 1;                            // PTR以外なら進める
  }
}

void writeRelTbl() {                       // 再配置表をファイルへ出力
  for (int i=0; i<relIdx; i=i+1) {
    int addr = relTbl[i].addr;
    int symx = relTbl[i].symx;
    int type = symTbl[symx].type;
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
    int type = symTbl[symx].type;
    
    printf("%04x\t",addr);
    putStr(stdout,symTbl[symx].strx);
    printf("\t");
    printSymType(type);
    printf("\t%d\n", symx);
  }
  printf("\n");
}
