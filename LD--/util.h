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

int getB();
void putB(char c);
void error(char *str);                    // エラーメッセージを表示して終了
void tblError(char *str);
void fError(char *str);                    // ファイル名付きでエラー表示
void tblError(char *str,int index, int size);   //表がパンクした時のエラー表示
void xOpenIn(char *fname);                 // エラーチェック付きの fopen:入力ファイル用
void xOpenOut(char *fname);                // エラーチェック付きの fopen:出力ファイル用
void xSeekIn(int offset);                    // エラーチェック付きの SEEK ルーチン
void xSeekOut(int offset);
void putW(int x,FILE* out);                          // 1ワード出力ルーチン
int getW(FILE* in);                                // 1ワード入力ルーチン


#endif
