#ifndef UTIL_H
#define UTIL_H

#define boolean int                        // boolean 型のつもり
#define true     1
#define false    0
FILE* out;                                 // 出力ファイル
FILE* in;                                  // 入力ファイル
char *curFile;                              // 現在の入力ファイル
#define getB()    fgetc(in)
#define putB(c)   fputc(c,out)
void error(char *str);                    // エラーメッセージを表示して終了
void tblError(char *str);
void fError(char *str);                    // ファイル名付きでエラー表示
void xOpen(char *fname);                   // エラーチェック付きの fopen
void xSeek(int offset);                    // エラーチェック付きの SEEK ルーチン
void putW(int x);                          // 1ワード出力ルーチン
int getW();                                // 1ワード入力ルーチン


#endif