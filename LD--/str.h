#ifndef STR_H
#define STR_H

int getStrIdx();
boolean isStrLocal(int i);                      //文字列表からi番地の文字を読み込む
int strLen(int n);                          // 文字列表中の文字列(n)の長さ
void readStrTbl(int offs);                  // 文字列表の読み込み
void writeStrTbl();                         // 文字列表をファイルへ出力
boolean cmpStr(int n, int m);               //文字列表[n]〜と[m]を比較〜
void putStr(FILE* fp,int n);                //文字列表の文字列[n]を表示する
void packStrTbl(int idxI,int len);           //文字列表から統合した綴りを削除する
void mergeStrTbl(int symIdxB, int strIdxB);
#endif