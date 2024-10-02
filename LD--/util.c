#include <stdio.h>
#include <stdlib.h>
#include "util.h"

// ファイル関係
static FILE* out;                                 // 出力ファイル
static FILE* in;                                  // 入力ファイル
static char *curFile = "";                        // 現在の入出力ファイル

int getB() {
  return fgetc(in);
}

void putB(char c) {
  fputc(c,out);
}


void tblError(char *str, int idx, int size) {  //表がパンクした時のエラー表示
  fprintf(stderr, "%s\t%5d/%5d\n",str,idx,size);
  exit(1);
}

void fError(char *str) {                   // ファイル名付きでエラー表示
  perror(curFile);
  error(str);
}

void error(char *str) {                   // エラーメッセージを表示して終了
  fprintf(stderr, "%s\n", str);
  exit(1);
}


void xOpenIn(char *fname) {                // エラーチェック付きの fopen
  curFile = fname;
  if ((in = fopen(fname, "rb"))==NULL) {   // 入力ファイルオープン
    fError("can't open");
  }
}

void xOpenOut(char *fname){                 // エラーチェック付きの fopen
  if ((out = fopen(fname,"wb"))==NULL) {    // 出力ファイルオープン
    perror(fname);
    error("can't open");
  }
}
 
void fcloseIn(){
  fclose(in);
}

void fcloseOut(){
  fclose(out);
}

void xSeekIn(int offset) {                   // エラーチェック付きの SEEK ルーチン
  if ((offset&1)!=0 || fseek(in, (long)offset, SEEK_SET)!=0)
    fError("file format");
}

void xSeekOut(int offset) {
  if ((offset&1)!=0 || fseek(out, (long)offset, SEEK_SET)!=0)
    fError("file format");
}

void putW(int x) {                          // 1ワード出力ルーチン
  putB(x>>8);
  putB(x);
}

int getW() {                                // 1ワード入力ルーチン
  int x1 = getB();
  int x2 = getB();
  if (x1==EOF || x2==EOF) fError("undexpected EOF");
  return (x1 << 8) | x2;
}
