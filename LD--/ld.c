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

// 元々main関数の中にあった変数で、main関数のモジュール化により共有変数にしたもの
int textBase;                            // 現在の入力ファイルの
int dataBase;                            //   各セグメントの
int bssBase;                             //     ロードアドレス

void writeHdr() {                          // ヘッダ書き出しルーチン
  putW(MAGIC);                             //   マジックナンバー
  putW(textSize);                          //   TEXTサイズ
  putW(dataSize);                          //   DATAサイズ
  putW(bssSize);                           //   BSS サイズ
  putW(getSymSize());                      //   SYMSサイズ
  putW(0);                                 //   ENTRY
  putW(getTrSize());                       //   Trサイズ
  putW(getDrSize());                       //   Drサイズ
}

void readHdr() {                           // ヘッダ読込みルーチン
  if (getW()!=MAGIC) {                     //   マジックナンバー
    fError("扱えないファイル");
  }
  cTextSize=getW();                        //   TEXTサイズ
  cDataSize=getW();                        //   DATAサイズ
  cBssSize=getW();                         //   BSS サイズ
  cSymSize=getW();                         //   SYMSサイズ
  getW();                                  //   ENTRY
  cTrSize=getW();                          //   Trサイズ
  cDrSize=getW();                          //   Drサイズ
}

/* プログラムやデータをリロケートしながらコピーする */
void copyCode(int offs, int segSize, int segBase, int relBase) {
  xSeekIn(offs);
  int rel = relBase;
  for (int i=segBase; i<segBase+segSize; i=i+WORD) {
    int w = getW();
    if (rel<getRelIdx() && getRelTbl(rel).addr==i) {// ポインタのアドレスに達した
      int symx = getRelTbl(rel).symx;               // 名前表のインデクスに変換
      int type = getSymTbl(symx).type;
      if (type!=SYMUNDF && type!=SYMBSS) {    // UNDF と BSS は 0 のまま
          w = getSymTbl(symx).val;
          if (type==SYMDATA) w=w+textSize;    // データセグメントなら(一応)
        }                                     // 絶対番地を書き込んでおく
      rel = rel + 1;                          // 次のポインタに進む
    }
    putW(w);
  }
}

/*必要なライブラリ関数を調べ、名前表に追加する*/
void importLibSymTbls(int argc, char **argv) {

  boolean merged = true;  // 統合が行われたかどうかを判定 
  int startIdx = 0;       // 統合チェックの開始位置
  int nowSymIdx;          // 統合前のシンボルテーブルの位置を保存

  while(merged){          // 結合が行われなくなるまで繰り返す
    printSymTbl();   // デバッグ用
    printf("\nimportLibSymTbls\nstartIdx = %d\n",startIdx); //デバッグ用
    merged = false;
    nowSymIdx = getSymIdx(); // 今のsymIdxを保存
    for(int i=2;i<argc; i=i+1) {
      int length = strlen(argv[i]); 
      if(argv[i][length-2]=='.' && argv[i][length-1]=='a') { // アーカイブファイルに対して

        xOpenIn(argv[i]);
        do {
          saveSymTbl();      // 現在のシンボルテーブルを保存
          saveStrTbl();      // 現在の文字列テーブルを保存

          int newStrBase = getStrIdx();

          readHdr();         // 通常と同じように読み込む
          readSymTbl(HDRSIZ+cTextSize+cDataSize+cTrSize+cDrSize,
                   cSymSize,textBase,dataBase);
          readStrTbl(HDRSIZ+cTextSize+cDataSize+cTrSize+cDrSize+cSymSize);
          mergeStrTbl(newStrBase);

          if(checkSymMerge(startIdx)) { // チェックの開始地点を指定して判定を行う
                                        // 名前解決が可能ならば文字列表も読む
            merged = true;
            addSymArcv(i,getLibHead()); // ライブラリ関数の目印シンボルを追加する
          }
          else {             // 名前解決ができないならば
            loadSymTbl();    // 保存しておいたシンボルテーブルと
            loadStrTbl();    // 文字列表をロードして
          }                  // 読み込んだライブラリ関数を捨てる

        } while(nextFile()); // アーカイブ内全てのライブラリ関数について同様の処理を行う
      }
    }
    
      textSize = textBase;
      dataSize = dataBase;
      bssSize  = bssBase;
                                      // シンボルテーブルの統合
      bssSize = mergeSymTbl(bssSize); // bssSizeの値を再計算させる
    
                             // 名前解決が発生する場合、
    startIdx = nowSymIdx;    // 次は読み込んだライブラリ関数について名前解決を試みる
  }
}

 /* 入力ファイルのシンボルテーブルを読み込んで統合する */
void importSymTbls(int argc, char **argv) {
  printf("\nimportSymTbls\n"); //デバッグ用
  textBase = dataBase = bssBase = 0;
  //trSize = drSize  = 0;
  for (int i=2; i<argc; i=i+1) {
    int length = strlen(argv[i]);
    if(argv[i][length-2]=='.' && argv[i][length-1]=='o') { // オブジェクトファイルに対して
      xOpenIn(argv[i]); //入力ファイルオープン 
      int newStrBase = getStrIdx();
      readHdr();
      readSymTbl(HDRSIZ+cTextSize+cDataSize+cTrSize+cDrSize,
                 cSymSize,textBase,dataBase);

      readStrTbl(HDRSIZ+cTextSize+cDataSize+cTrSize+cDrSize+cSymSize);

      mergeStrTbl(newStrBase);  //文字列テーブルの統合

      textBase = textBase + cTextSize;
      dataBase = dataBase + cDataSize;
      bssBase  = bssBase  + cBssSize;

      fcloseIn();
    }
  }

  textSize = textBase;
  dataSize = dataBase;
  bssSize  = bssBase;

  bssSize = mergeSymTbl(bssSize);  // シンボルテーブルの統合
                                   // bssSizeの値を再計算させる

  importLibSymTbls(argc, argv);  // ライブラリ関数から必要なものを入力する

}


/* テキストセグメントを入力して結合後出力する */
void importTextSegments(int argc, char **argv) {
  printf("\nimportTextSegments\n"); // デバッグ用
  int symBase = 0;
  textBase = 0;
  for (int i=2; i<argc; i=i+1) {
    int length = strlen(argv[i]);
    if(argv[i][length-2]=='.' && argv[i][length-1]=='o') { // オブジェクトファイルに対して
      xOpenIn(argv[i]);   //入力ファイルオープン
      readHdr();
      int relBase = getRelIdx();  // relIdx
      readTrRelTbl(HDRSIZ+cTextSize+cDataSize,cTrSize,symBase,textBase);
      copyCode(HDRSIZ,cTextSize,textBase,relBase);  // テキストをコピー

      textBase = textBase + cTextSize;
      symBase  = symBase  + cSymSize;
  
      fcloseIn();
    }
  }
  int symIdx = getSymIdx();
  for(int i=0; i<symIdx; i=i+1) {
    if(getSymTbl(i).type == SYMARCV) {  // シンボルの型がSYMARCVのとき
      int num  = getSymTbl(i).strx;     // アーカイブファイルの番号と
      int addr = getSymTbl(i).val;      // ファイル内アドレスを取り出す
      xOpenIn(argv[num]);  // アーカイブファイルを開き
      xSeekArc(addr);    // ライブラリ関数の位置までSEEK
      printf("debug:アーカイブファイル名:%s\n",argv[num]); //デバッグ用
      printf("debug:ファイルの先頭アドレス:%d\n",addr); //デバッグ用

      readHdr();
      int relBase = getRelIdx();  // relIdx
      readTrRelTbl(HDRSIZ+cTextSize+cDataSize,cTrSize,symBase,textBase);
      copyCode(HDRSIZ,cTextSize,textBase,relBase);  // テキストをコピー

      textBase = textBase + cTextSize;
      symBase  = symBase  + cSymSize;
  
      fcloseIn();
    }
  }


}

/* データセグメントを入力して結合後出力する */
void importDataSegments(int argc, char **argv) {
  printf("\nimportDataSegments\n");//デバッグ用
  int symBase = 0;
  dataBase = 0;
  for (int i=2; i<argc; i=i+1) {
    
    int length = strlen(argv[i]);
    if(argv[i][length-2]=='.' && argv[i][length-1]=='o') { // オブジェクトファイルに対して
      
      xOpenIn(argv[i]);
      readHdr();
      int relBase = getRelIdx();
      readDrRelTbl(HDRSIZ+cTextSize+cDataSize+cTrSize,cDrSize,symBase,dataBase);
      copyCode(HDRSIZ+cTextSize,cDataSize,dataBase,relBase);   // データをコピー

      dataBase = dataBase + cDataSize;
      symBase  = symBase  + cSymSize;

      fcloseIn();

    }
  }

  int symIdx = getSymIdx();
  for(int i=0; i<symIdx; i=i+1) {
    if(getSymTbl(i).type == SYMARCV) {  // シンボルの型がSYMARCVのとき
      int num  = getSymTbl(i).strx;     // アーカイブファイルの番号と
      int addr = getSymTbl(i).val;      // ファイル内アドレスを取り出す
      xOpenIn(argv[num]);  // アーカイブファイルを開き
      xSeekArc(addr);    // ライブラリ関数の位置までSEEK

      printf("debug:アーカイブファイル名:%s\n",argv[num]); // デバッグ用
      printf("debug:ファイルの先頭アドレス:%d\n",addr);    // デバッグ用

      readHdr();
      int relBase = getRelIdx();
      readDrRelTbl(HDRSIZ+cTextSize+cDataSize+cTrSize,cDrSize,symBase,dataBase);
      copyCode(HDRSIZ+cTextSize,cDataSize,dataBase,relBase);   // データをコピー

      dataBase = dataBase + cDataSize;
      symBase  = symBase  + cSymSize;

      fcloseIn();
    
    
    }
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

  if (argc>1 &&
      (strcmp(argv[1],"-v")==0 ||          //  "-v", "-h" で、使い方と
       strcmp(argv[1],"-h")==0   ) ) {     //   バージョンを表示
    usage(argv[0]);
    exit(0);
  }

  if (argc<3) {
    usage(argv[0]);
    exit(0);
  }

  xOpenOut(argv[1]);    //出力ファイルオープン

  importSymTbls(argc, argv); // 入力ファイルのシンボルテーブルを読み込んで統合する
                             // 必要なライブラリ関数も調べる
  xSeekOut(HDRSIZ);          // ヘッダ分の空間を開けておく
  
  importTextSegments(argc, argv);    // テキストセグメントを入力して結合後出力する
  importDataSegments(argc, argv);    // データセグメントを入力して結合後出力する 


  packSymTbl();              // 名前表から結合した残骸を削除
  
  writeRelTbl();             // 再配置表を出力する
  printRelTbl();             // 再配置表をリスト出力する
  writeSymTbl();             // 名前表を出力する
  printSymTbl();             // 名前表をリスト出力する
  writeStrTbl();             // 文字列表を出力する

  xSeekOut(0);               // 先頭に戻る
  writeHdr();                // ヘッダを出力する
  fcloseOut();               // 出力ファイルクローズ
  exit(0);
}