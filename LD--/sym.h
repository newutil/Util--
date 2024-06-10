#ifndef SYM_H
#define SYM_H

#define SYM_SIZ  3000                       // 名前表の大きさ (<=16kエントリ)



int getSymIdx();                            //使用した表の領域のゲッター
void setSymIdx(int num);                    //使用した表の領域のセッター
int getSymTbl(int index,char *str);                   //名前表のゲッター
void setSymTbl(int index,int newStrx,int newType,int newVal);       //名前表のセッター

void readSymTbl(int offs, int sSize, int textBase, int dataBase);       //名前表の読み込み
void mergeStrTbl();  //文字列表の重複を見つけ、統合する
void mergeSymTbl(int bssSize, int symSize);                         //名前を統合
void writeSymTbl();                         //名前表をファイルへ出力
void printSymType(int type);                        //名前表の種類を印刷
void printSymTbl();                         //名前表をリストへ出力
#endif
