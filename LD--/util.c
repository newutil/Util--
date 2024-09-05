#include <stdio.h>
#include <stdlib.h>
#include "util.h"

void error(char *str) {                   // エラーメッセージを表示して終了
  fprintf(stderr, "%s\n", str);
  exit(1);
}

// void tblError(char *str);

// ファイル関係
// FILE* out;                                 // 出力ファイル
// FILE* in;                                  // 入力ファイル
char *curFile = "";                        // 現在の入出力ファイル

#define getB(in)    fgetc(in)
#define putB(c,out)   fputc(c,out)
//inとoutをmainに持っていきたいので、putBをファイルを指定して行うようにした

void fError(char *str) {                   // ファイル名付きでエラー表示
  perror(curFile);
  error(str);
}

FILE* xOpen(char *fname, char *chmod) {                  // エラーチェック付きの fopen
  FILE* file;
  curFile = fname;
  if ((file = fopen(fname, chmod))==NULL) {   // 入力ファイルオープン
    fError("can't open");
  }
  return file;

}

void tblError(char *str, int index, int size) {     //表がパンクした時のエラー表示
  fprintf(stderr, "%s\t%5d/%5d\n",str,index,size);
  exit(1);

}

// void xOpenOut(char *fname){                 // エラーチェック付きの fopen
//   curFile = fname;
//   if ((out = fopen(fname,"wb"))==NULL) {    // 出力ファイルオープン
//     fError("can't open");
//   }
  
// }
  
void xSeek(int offset) {                   // エラーチェック付きの SEEK ルーチン
  if ((offset&1)!=0 || fseek(in, (long)offset, SEEK_SET)!=0)
    fError("file format");
}

void putW(int x,FILE* out) {                          // 1ワード出力ルーチン
  putB(x>>8,out);
  putB(x,out);
}

int getW(FILE* in) {                                // 1ワード入力ルーチン
  int x1 = getB(in);
  int x2 = getB(in);
  if (x1==EOF || x2==EOF) fError("undexpected EFO");
  return (x1 << 8) | x2;
}

