#ifndef SYM_H
#define SYM_H

#define SYM_SIZ  3000                       // 名前表の大きさ (<=16kエントリ)

#ifndef UTIL_H                              //utilをインクルードせず、booleanがなかった時のための処置。
#define boolean int                         //本来utilをインクルードしているはずなので、日の目を見ることはないはず
#endif

int getSymSiz();                            //名前表の大きさを返す
void readSymTbl(int offs, int sSize);       //名前表の読み込み
void mergeStrTbl(int symIdxB,int strIdxB);  //文字列表の重複を見つけ、統合する
void mergeSymTbl();                         //名前を統合
void writeSymTbl();                         //名前表をファイルへ出力
void printSymType(int type);                //名前の種類を印刷
void printSymTbl();                         //名前表を印刷
#endif

