/*
 * TaC linker Source Code
 *    Tokuyama kousen Educational Computer 16bit Ver.
 *
 * Copyright (C) 2008-2012 by
 *                      Dept. of Computer Science and Electronic Engineering,
 *                      Tokuyama College of Technology, JAPAN
 *
 *   上記著作権者は，Free Software Foundation によって公開されている GNU 一般公
 * 衆利用許諾契約書バージョン２に記述されている条件を満たす場合に限り，本ソース
 * コード(本ソースコードを改変したものを含む．以下同様)を使用・複製・改変・再配
 * 布することを無償で許諾する．
 *
 *   本ソースコードは＊全くの無保証＊で提供されるものである。上記著作権者および
 * 関連機関・個人は本ソースコードに関して，その適用可能性も含めて，いかなる保証
 * も行わない．また，本ソースコードの利用により直接的または間接的に生じたいかな
 * る損害に関しても，その責任を負わない．
 *
 *
 */

/*
 * ld.c : LD-- 本体
 *
 * 2016.03.12           : 使用方法メッセージ表示用のオプション -h, -v 追加
 * 2012.08.02  v.2.0    : TaC-CPU V2 に対応
 * 2010.12.07           : 1. Usage メッセージがバージョン情報を含むように改良
 *                      : 2. 次のバグを訂正
 *                      :   (1) 綴を共有したローカルシンボルをもつ .o ファイル
 *                      :       を更に新しい .o のローカルシンボルと綴を共有
 *                      :       するときに綴を間違える mergeStrTbl のバグ
 *             v.0.0.4  :   (2) PTR 連鎖を一段しかたどらない readRel のバグ
 * 2010.07.20  v.0.0.3  : Subversion による管理を開始
 * 2009.08.01  v.0.0.2  : 表がパンクしたとき、各表の込み具合を表示する
 * 2009.04.21  v.0.0.1  : バイナリファイルの fopen のモードに "b" を追加
 * 2008.07.08  v.0.0    : 開発開始
 *
 * $Id$
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "util.h"
#include "rel.h"
#include "sym.h"
#include "str.h"


// #define boolean int                        // boolean 型のつもり
// #define true     1
// #define false    0
#define WORD     2                         // 1ワード2バイト
#define MAGIC    0x0107                    // .o 形式のマジック番号
#define HDRSIZ   16                        // .o 形式のヘッダーサイズ

// readHdr と main の共有変数
int cTextSize;                             // 現在の入力ファイルのTEXTサイズ
int cDataSize;                             // 現在の入力ファイルのDATAサイズ
int cBssSize;                              // 現在の入力ファイルのBSS サイズ
int cSymSize;                              // 現在の入力ファイルのSYMSサイズ
int cTrSize;                               // 現在の入力ファイルのTr  サイズ
int cDrSize;                               // 現在の入力ファイルのDr  サイズ

// writeHdr と copyCode と main の共有変数
int textSize;                              // 出力ファイルのTEXTサイズ

// writeHdr と main の共有変数
int dataSize;                              // 出力ファイルのDATAサイズ
int bssSize;                               // 出力ファイルのBSS サイズ
//int symSize;                             // 出力ファイルのSYMSサイズ sym.cに移動
// int trSize;                             // 出力ファイルのTr  サイズ rel.cに移動
// int drSize;                             // 出力ファイルのDr  サイズ rel.cに移動

void writeHdr() {                           // ヘッダ書き出しルーチン
  putW(MAGIC,out);                             //   マジックナンバー
  putW(textSize,out);                           //   TEXTサイズ
  putW(dataSize,out);                           //   DATAサイズ
  putW(bssSize,out);                            //   BSS サイズ
  putW(getSymSize(),out);                       //   SYMSサイズ
  putW(0,out);                                  //   ENTRY
  putW(getTrSize(),out);                        //   Trサイズ
  putW(getDrSize(),out);                        //   Drサイズ
}

void readHdr() {                            // ヘッダ読込みルーチン
  if (getW()!=MAGIC)                        //   マジックナンバー
    fError("扱えないファイル");
  cTextSize=getW();                         //   TEXTサイズ
  cDataSize=getW();                         //   DATAサイズ
  cBssSize=getW();                          //   BSS サイズ
  cSymSize=getW();                          //   SYMSサイズ
  getW();                                   //   ENTRY
  cTrSize=getW();                           //   Trサイズ
  cDrSize=getW();                           //   Drサイズ
}

/* プログラムやデータをリロケートしながらコピーする */
void copyCode(int offs, int segSize, int segBase, int relBase) {
  xSeekIn(offs);
  int rel = relBase;
  for (int i=segBase; i<segBase+segSize; i=i+WORD) {
    int w = getW();
    if (rel<getRelIdx() && getRelTbl(rel).addr==i) {//ポインタのアドレスに達した
      int symx = getRelTbl(rel).symx;               // 名前表のインデクスに変換
      int type = getSymTbl(symx,"type");
      if (type!=SYMUNDF && type!=SYMBSS) {    // UNDF と BSS は 0 のまま
	w = getSymTbl(symx,"val");
	if (type==SYMDATA) w=w+textSize;      // データセグメントなら(一応)
      }                                       // 絶対番地を書き込んでおく
      rel = rel + 1;                          // 次のポインタに進む
    }
    putW(w);
  }
}

// 使い方表示関数
static void usage(char *name) {
  fprintf(stderr,"使用方法 : %s [-h] [-v] <outfile> <objfile>...\n", name);
  fprintf(stderr, "    <objfile> (複数)から入力し\n");
  fprintf(stderr, "    <outfile> へ出力\n");
  fprintf(stderr, "\n");
  fprintf(stderr, "    -h, -v  : このメッセージを表示\n");
  fprintf(stderr, "\n");
  fprintf(stderr, "%s version %s\n", name, VER);
  fprintf(stderr, "(build date : %s)\n", DATE);
  fprintf(stderr, "\n");
}

// main 関数
int main(int argc, char **argv) {
  int textBase;                              // 現在の入力ファイルの
  int dataBase;                              //   各セグメントの
  int bssBase;                               //     ロードアドレス

  if (argc>1 &&
      (strcmp(argv[1],"-v")==0 ||              //  "-v", "-h" で、使い方と
       strcmp(argv[1],"-h")==0   ) ) {         //   バージョンを表示
    usage(argv[0]);
    exit(0);
  }

  if (argc<3) {
    usage(argv[0]);
    exit(0);
  }

  xOpenOut(argv[1]);    //出力ファイルオープン

  /* 入力ファイルのシンボルテーブルを読み込んで統合する */
  textBase = dataBase = bssBase = 0;
  trSize = drSize  = 0;
  for (int i=2; i<argc; i=i+1) {
    xOpenIn(argv[i]); //入力ファイルオープン 
    int newSymBase = getSymIdx();
    int newStrBase = getStrIdx();

    readHdr();
    readSymTbl(HDRSIZ+cTextSize+cDataSize+cTrSize+cDrSize,
               cSymSize,textBase,dataBase);
    readStrTbl(HDRSIZ+cTextSize+cDataSize+cTrSize+cDrSize+cSymSize);

    mergeStrTbl(newSymBase, newStrBase);  //文字列テーブルの統合

    textBase = textBase + cTextSize;
    dataBase = dataBase + cDataSize;
    bssBase  = bssBase  + cBssSize;

    //trSize   = trSize   + cTrSize;
    //drSize   = drSize   + cDrSize;
    //symSize  = symSize  + cSymSize;

    fcloseIn();
  }

  textSize = textBase;
  dataSize = dataBase;
  bssSize  = bssBase;

  bssSize = mergeSymTbl(bssSize); 
                             // シンボルテーブルの統合をする
                             // bssSize, symSize も再計算する


  xSeekOut(HDRSIZ);         //ヘッダ分の位置を開けておく
  
  /* テキストセグメントを入力して結合後出力する */
  int symBase = 0;
  textBase = 0;

  for (int i=2; i<argc; i=i+1) {
    xOpenIn(argv[i]);   //入力ファイルオープン
    readHdr();
    int relBase = getRelIdx();  // relIdx
    readTrRelTbl(HDRSIZ+cTextSize+cDataSize,cTrSize,symBase,textBase);
    copyCode(HDRSIZ,cTextSize,textBase,relBase);  // テキストをコピー

    textBase = textBase + cTextSize;
    symBase  = symBase  + cSymSize;
    fcloseIn();
  }

  /* データセグメントを入力して結合後出力する */
  dataBase = symBase = 0;
  for (int i=2; i<argc; i=i+1) {
    xOpenIn(argv[i]);
    readHdr();
    int relBase = getRelIdx();/*relIdx;*/
    readDrRelTbl(HDRSIZ+cTextSize+cDataSize+cTrSize,cDrSize,symBase,dataBase);
    copyCode(HDRSIZ+cTextSize,cDataSize,dataBase,relBase);   // データをコピー

    dataBase = dataBase + cDataSize;
    symBase  = symBase  + cSymSize;
    fcloseIn();
  }


  packSymTbl();                            // 名前表から結合した残骸を削除

  writeRelTbl();                           // 再配置表を出力する
  printRelTbl();                           // 再配置表をリスト出力する
  writeSymTbl();                           // 名前表を出力する
  printSymTbl();                           // 名前表をリスト出力する
  writeStrTbl();                           // 文字列表を出力する

  xSeekOut(0);               // 先頭に戻る
  writeHdr();                // ヘッダを出力する
  fcloseOut();
  exit(0);
}