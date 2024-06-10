#ifndef UTIL_H
#define UTIL_H

#define boolean int                        // boolean 型のつもり
#define true     1
#define false    0
#define SYMUNDF 0                           // 未定義ラベル
#define SYMTEXT 1                           // TEXTのラベル
#define SYMDATA 2                           // DATAのラベル
#define SYMBSS  3                           // BSSのラベル
#define SYMPTR  4                           // 表の他要素へのポインタ
FILE* out;                                 // 出力ファイル
FILE* in;                                  // 入力ファイル
char *curFile;                              // 現在の入力ファイル
#define getB()    fgetc(in)
#define putB(c)   fputc(c,out)
void error(char *str);                    // エラーメッセージを表示して終了
void tblError(char *str);
void fError(char *str);                    // ファイル名付きでエラー表示
void xOpenIn(char *fname);                 // エラーチェック付きの fopen:入力ファイル用
void xOpenOut(char *fname);                // エラーチェック付きの fopen:出力ファイル用
void xSeek(int offset);                    // エラーチェック付きの SEEK ルーチン
void putW(int x);                          // 1ワード出力ルーチン
int getW();                                // 1ワード入力ルーチン


#endif