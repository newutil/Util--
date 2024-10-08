#ifndef SYM_H
#define SYM_H

#define SYM_SIZ  3000                       // 名前表の大きさ (<=16kエントリ)

struct SymTbl {                             // 名前表の型定義
  int strx;                                 // 文字列表の idx (14bitが有効)
  int type;                                 // type の意味は下に #define
  int val;                                  // 名前の値
};


struct SymTbl getSymTbl(int index);         // 名前表のゲッター
int getSymSize();
void readSymTbl(int offs, int sSize, int textBase, int dataBase);   // 名前表の読み込み
int mergeSymTbl(int bssSize);               // 名前を統合
void updateSymStrx(int curIdx, int changeIdx, int len);
void writeSymTbl();                         // 名前表をファイルへ出力
void printSymType(int type);                // 名前表の種類を印刷
void printSymName(int symx);
void printSymTbl();                         // 名前表をリストへ出力
void packSymTbl();
#endif
