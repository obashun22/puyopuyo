//課題
//2020/05/29

/*
【メモ】
InitPuyoArrayメンバ関数をChangeSize関数内で呼び出してメモリ確保の際に初期化するよう変更
回転した時のみpuyorotateを変更するようにした
セーブ機能を実装してGameOverになった時点のスコアを過去上位3位まで記録
SaveDataクラスの構成はコンストラクタでsave.dat生成, Saveはスコアを比較して入れ替える
PrintCommentは順位に応じてコメントするPrintSaveDataはランキングを表示
データは外部のsave.datにバイナリデータで保存
GameStartをGameMainに名前変更
main内のwhileがごちゃごちゃしてきたのでGameControlクラスを作成
ゲームの進行に必要な関数をまとめていて必要があればSaveDataクラスを参照
よって盤面情報・セーブ機能・ゲームコントロールの3つのクラスで動いている
コメントアウトを更新
Press 'q' to Quitを常時表示するように変更
盤面の大きさの下限を7x7に変更
盤面が小さい時に着地判定がバグる問題は7x7を下限にすることで一応対応
Press 'p'でポーズ・ポーズ解除を追加
standbypuyo構造体で次と次の次に出現するぷよを管理するよう変更

【To Do】
- [x] セーブする機能
- [x] 独立性の確認
- [x] コメントアウト
- [x] Press 'q' to Quit表示
- [x] 盤面が小さい時に着地判定がバグる問題
- [x] ポーズ機能
- [x] usleepの調整
- [] 次のぷよを表示
- [] 名前を記録する
- [] 音楽・BGMをつける//環境的に困難か
*/

#include <curses.h>
#include <stdlib.h> //rand, srand関数で使用
#include <time.h> //srand関数で使用
#include <unistd.h> //動作を止めるときに使用
#include <string.h> //.datを扱うために使用

//ぷよの色を表すの列挙型
//NONEが無し，RED,BLUE,..が色を表す
enum puyocolor { NONE, RED, BLUE, GREEN, YELLOW };

//盤面状態の保持に関するクラス
class PuyoArray
{
public:
	PuyoArray()
	{
		data = NULL;
		data_line = 0;
		data_column = 0;
	}

	~PuyoArray()
	{
		Release();
	}

	//盤面サイズ変更
	void ChangeSize(unsigned int line, unsigned int column)
	{
		Release();

		//新しいサイズでメモリ確保
		data = new puyocolor[line*column];

		data_line = line;
		data_column = column;

		InitPuyoArray();
	}

	//盤面の行数を返す
	unsigned int GetLine()
	{
		return data_line;
	}

	//盤面の列数を返す
	unsigned int GetColumn()
	{
		return data_column;
	}

	//盤面の指定された位置の値を返す
	puyocolor GetValue(unsigned int y, unsigned int x)
	{
		if (y >= GetLine() || x >= GetColumn())
		{
			//引数の値が正しくない
			return NONE;
		}

		return data[y*GetColumn() + x];
	}

	//盤面の指定された位置に値を書き込む
	void SetValue(unsigned int y, unsigned int x, puyocolor value)
	{
		if (y >= GetLine() || x >= GetColumn())
		{
			//引数の値が正しくない
			return;
		}

		data[y*GetColumn() + x] = value;
	}

	// 盤面をすべてNULLに上書き
	void InitPuyoArray()
	{
		for (int y = 0; y < GetLine(); y++)
		{
			for (int x = 0; x < GetColumn(); x++)
			{
				SetValue(y, x, NONE);
			}
		}
	}

protected:
	//盤面状態
	puyocolor *data;
	unsigned int data_line;
	unsigned int data_column;

	//メモリ開放
	void Release()
	{
		if (data == NULL) {
			return;
		}
		delete[] data;
		data = NULL;
	}
};

//「落下中」ぷよを管理するクラス
class PuyoArrayActive: public PuyoArray
{
public:
	PuyoArrayActive(){
		puyorotate = 0;
	}

	// ぷよの回転状態を返す
	int GetPuyoRotate(){
		return puyorotate;
	}

	// ぷよの回転状態を変更する
	void SetPuyoRotate(int state){
		puyorotate = state;
	}

private:
	// ぷよの回転状態を表す
	// 初期状態0で右回転毎に1増える
	int puyorotate;
};

//「着地済み」ぷよを管理するクラス
class PuyoArrayStack: public PuyoArray
{

};

//ぷよの発生・移動等のぷよ管理を行うクラス
class PuyoControl
{
public:
	PuyoControl()
	{
		srand((unsigned int)time(NULL));
		//ランダムなぷよ生成のためにrootを初期化
		randColor(standbypuyo.standbypuyo1_1);
		randColor(standbypuyo.standbypuyo1_2);
		randColor(standbypuyo.standbypuyo2_1);
		randColor(standbypuyo.standbypuyo2_2);
	}

	//puyocolorのメンバをランダムに与える関数
	void randColor(puyocolor &newpuyo)
	{
		switch (rand()%4+1){
			case 1:
				newpuyo = RED;
				break;
			case 2:
				newpuyo = BLUE;
				break;
			case 3:
				newpuyo = GREEN;
				break;
			case 4:
				newpuyo = YELLOW;
				break;
		}
	}

	//盤面に新しいぷよ生成
	void GeneratePuyo(PuyoArrayActive &puyoactive)
	{
		//puyocolorのメンバをランダムに与える
		puyoactive.SetValue(0, 5, standbypuyo.standbypuyo1_1);
		puyoactive.SetValue(0, 6, standbypuyo.standbypuyo1_2);
		standbypuyo.standbypuyo1_1 = standbypuyo.standbypuyo2_1;
		standbypuyo.standbypuyo1_2 = standbypuyo.standbypuyo2_2;
		srand((unsigned int)time(NULL));
		//ランダムなぷよ生成のためにrootを初期化
		randColor(standbypuyo.standbypuyo2_1);
		randColor(standbypuyo.standbypuyo2_2);
	}

	//ぷよの着地判定．着地判定があるとtrueを返す
	bool LandingPuyo(PuyoArrayActive &puyoactive, PuyoArrayStack &puyostack)
	{
		bool landed = false;

		for (int y = 0; y < puyoactive.GetLine(); y++)
		{
			for (int x = 0; x < puyoactive.GetColumn(); x++)
			{
				//ぷよが最低座標に位置しているまたは一つ下にぷよが存在する場合の処理
				if (puyoactive.GetValue(y, x) != NONE && (y == puyoactive.GetLine() - 1 || puyostack.GetValue(y + 1, x) != NONE))
				{
					landed = true;
					//どちらかが着地した時にすべてのぷよをstackに引き渡す
					for (int i = 0; i < puyoactive.GetLine(); i++){
						for (int j = 0; j < puyoactive.GetColumn(); j++){
							if (puyoactive.GetValue(j, i) == NONE) {
								continue;
							} else {
								puyostack.SetValue(j, i, puyoactive.GetValue(j, i));
								puyoactive.SetValue(j, i, NONE);
							}
						}
					}
					//puyoactiveがカラになるからbreakしてもいいかも
					//次のぷよのために回転状態を初期化する
					puyoactive.SetPuyoRotate(0);
				}
			}
		}
		return landed;
	}

	//左移動
	void MoveLeft(PuyoArrayActive &puyoactive, PuyoArrayStack &puyostack)
	{
		//一時的格納場所メモリ確保
		puyocolor *puyo_temp = new puyocolor[puyoactive.GetLine()*puyoactive.GetColumn()];

		for (int i = 0; i < puyoactive.GetLine()*puyoactive.GetColumn(); i++)
		{
			puyo_temp[i] = NONE;
		}

		//1つ左の位置にpuyoactiveからpuyo_tempへとコピー
		for (int y = 0; y < puyoactive.GetLine(); y++)
		{
			for (int x = 0; x < puyoactive.GetColumn(); x++)
			{
				if (puyoactive.GetValue(y, x) == NONE) {
					continue;
				}
				//最左縁ではなく左に何も存在しない場合の処理
				if (0 < x && puyoactive.GetValue(y, x - 1) == NONE && puyostack.GetValue(y, x - 1) == NONE)
				{
					puyo_temp[y*puyoactive.GetColumn() + (x - 1)] = puyoactive.GetValue(y, x);
					//コピー後に元位置のpuyoactiveのデータは消す
					puyoactive.SetValue(y, x, NONE);
				}
				else
				{
					//上記の場合に当てはまらない場合に値を引き継ぐ処理
					puyo_temp[y*puyoactive.GetColumn() + x] = puyoactive.GetValue(y, x);
				}
			}
		}

		//puyo_tempからpuyoactiveへコピー
		for (int y = 0; y < puyoactive.GetLine(); y++)
		{
			for (int x = 0; x < puyoactive.GetColumn(); x++)
			{
				puyoactive.SetValue(y, x, puyo_temp[y*puyoactive.GetColumn() + x]);
			}
		}

		//一時的格納場所メモリ解放
		delete[] puyo_temp;
	}

	//右移動
	void MoveRight(PuyoArrayActive &puyoactive, PuyoArrayStack &puyostack)
	{
		//一時的格納場所メモリ確保
		puyocolor *puyo_temp = new puyocolor[puyoactive.GetLine()*puyoactive.GetColumn()];

		for (int i = 0; i < puyoactive.GetLine()*puyoactive.GetColumn(); i++)
		{
			puyo_temp[i] = NONE;
		}

		//1つ右の位置にpuyoactiveからpuyo_tempへとコピー
		for (int y = 0; y < puyoactive.GetLine(); y++)
		{
			for (int x = puyoactive.GetColumn() - 1; x >= 0; x--)
			{
				if (puyoactive.GetValue(y, x) == NONE) {
					continue;
				}
				//最右縁ではなく右に何も存在しない場合の処理
				if (x < puyoactive.GetColumn() - 1 && puyoactive.GetValue(y, x + 1) == NONE && puyostack.GetValue(y, x + 1) == NONE)
				{
					puyo_temp[y*puyoactive.GetColumn() + (x + 1)] = puyoactive.GetValue(y, x);
					//コピー後に元位置のpuyoactiveのデータは消す
					puyoactive.SetValue(y, x, NONE);
				}
				else
				{
					//上記の場合に当てはまらない場合に値を引き継ぐ処理
					puyo_temp[y*puyoactive.GetColumn() + x] = puyoactive.GetValue(y, x);
				}
			}
		}

		//puyo_tempからpuyoactiveへコピー
		for (int y = 0; y <puyoactive.GetLine(); y++)
		{
			for (int x = 0; x <puyoactive.GetColumn(); x++)
			{
				puyoactive.SetValue(y, x, puyo_temp[y*puyoactive.GetColumn() + x]);
			}
		}

		//一時的格納場所メモリ解放
		delete[] puyo_temp;
	}

	//下移動
	void MoveDown(PuyoArrayActive &puyoactive, PuyoArrayStack &puyostack)
	{
		//一時的格納場所メモリ確保
		puyocolor *puyo_temp = new puyocolor[puyoactive.GetLine()*puyoactive.GetColumn()];

		for (int i = 0; i < puyoactive.GetLine()*puyoactive.GetColumn(); i++)
		{
			puyo_temp[i] = NONE;
		}

		//1つ下の位置にpuyoactiveからpuyo_tempへとコピー
		for (int y = puyoactive.GetLine() - 1; y >= 0; y--)
		{
			for (int x = 0; x < puyoactive.GetColumn(); x++)
			{
				if (puyoactive.GetValue(y, x) == NONE) {
					continue;
				}
				//最下縁ではなく下に何も存在しない場合の処理
				if (y < puyoactive.GetLine() - 1 && puyoactive.GetValue(y + 1, x) == NONE && puyostack.GetValue(y + 1, x) == NONE)
				{
					puyo_temp[(y + 1)*puyoactive.GetColumn() + x] = puyoactive.GetValue(y, x);
					//コピー後に元位置のpuyoactiveのデータは消す
					puyoactive.SetValue(y, x, NONE);
				}
				else
				{
					//上記の場合に当てはまらない場合に値を引き継ぐ処理
					puyo_temp[y*puyoactive.GetColumn() + x] = puyoactive.GetValue(y, x);
				}
			}
		}

		//puyo_tempからpuyoactiveへコピー
		for (int y = 0; y < puyoactive.GetLine(); y++)
		{
			for (int x = 0; x < puyoactive.GetColumn(); x++)
			{
				puyoactive.SetValue(y, x, puyo_temp[y*puyoactive.GetColumn() + x]);
			}
		}

		//一時的格納場所メモリ解放
		delete[] puyo_temp;
	}

	//各マスにおいてぷよの連結があるかを確認する関数
	int VanishPuyo(PuyoArrayStack &puyostack)
	{
		int vanishednumber = 0;
		//すべてのマスにおいて4以上の連結ぷよの削除を実行
		for (int y = 0; y < puyostack.GetLine(); y++)
		{
			for (int x = 0; x < puyostack.GetColumn(); x++)
			{
				vanishednumber += VanishPuyo(puyostack, y, x);
			}
		}
		//トータルの削除したぷよ数を返す
		return vanishednumber;
	}

	//あるマスにおいて4つ以上のぷよの連結があるかを確認してあれば削除する関数
	int VanishPuyo(PuyoArrayStack &puyostack, unsigned int y, unsigned int x)
	{
		//空のマスは飛ばす
		if (puyostack.GetValue(y, x) == NONE)
		{
			return 0;
		}
		//以降ぷよが存在する場合のみの実行となる

		enum checkstate{ NOCHECK, CHECKING, CHECKED };

		//指定されたマスごとにチェック専用のメモリを確保する
		enum checkstate *field_array_check;
		field_array_check = new enum checkstate[puyostack.GetLine()*puyostack.GetColumn()];

		//全盤面をNOCHECKに初期化
		for (int i = 0; i < puyostack.GetLine()*puyostack.GetColumn(); i++)
		{
			field_array_check[i] = NOCHECK;
		}

		//指定されたマスをCHECKINGに変更
		field_array_check[y*puyostack.GetColumn() + x] = CHECKING;


		bool checkagain = true;
		while (checkagain)
		{
			checkagain = false;

			//CHECKINGを探す
			for (int y = 0; y < puyostack.GetLine(); y++)
			{
				for (int x = 0; x < puyostack.GetColumn(); x++)
				{
					//CHECKINGを指しているときのみ実行
					if (field_array_check[y*puyostack.GetColumn() + x] == CHECKING)
					{
						//最右縁でない場合
						if (x < puyostack.GetColumn() - 1)
						{
							//ぷよが右隣と同じかつ右隣がまだ未確認だった場合
							if (puyostack.GetValue(y, x + 1) == puyostack.GetValue(y, x) && field_array_check[y*puyostack.GetColumn() + (x + 1)] == NOCHECK)
							{
								field_array_check[y*puyostack.GetColumn() + (x + 1)] = CHECKING;
								checkagain = true;
							}
						}

						//最左縁でない場合
						if (x > 0)
						{
							//ぷよが左隣と同じかつ左隣がまだ未確認だった場合
							if (puyostack.GetValue(y, x - 1) == puyostack.GetValue(y, x) && field_array_check[y*puyostack.GetColumn() + (x - 1)] == NOCHECK)
							{
								field_array_check[y*puyostack.GetColumn() + (x - 1)] = CHECKING;
								checkagain = true;
							}
						}

						//最下縁ではない場合
						if (y < puyostack.GetLine() - 1)
						{
							//ぷよが直下と同じかつ直下がまだ未確認だった場合
							if (puyostack.GetValue(y + 1, x) == puyostack.GetValue(y, x) && field_array_check[(y + 1)*puyostack.GetColumn() + x] == NOCHECK)
							{
								field_array_check[(y + 1)*puyostack.GetColumn() + x] = CHECKING;
								checkagain = true;
							}
						}

						//最上縁ではない場合
						if (y > 0)
						{
							//ぷよが直上と同じかつ直上がまだ未確認だった場合
							if (puyostack.GetValue(y - 1, x) == puyostack.GetValue(y, x) && field_array_check[(y - 1)*puyostack.GetColumn() + x] == NOCHECK)
							{
								field_array_check[(y - 1)*puyostack.GetColumn() + x] = CHECKING;
								checkagain = true;
							}
						}

						field_array_check[y*puyostack.GetColumn() + x] = CHECKED;
					}
				}
			}
		}

		//CHECKEDになったマス数をカウント
		int puyocount = 0;
		for (int i = 0; i < puyostack.GetLine()*puyostack.GetColumn(); i++)
		{
			if (field_array_check[i] == CHECKED)
			{
				puyocount++;
			}
		}

		int vanishednumber = 0;
		//CHECKEDになったマスが4以上の場合は削除を実行
		if (4 <= puyocount)
		{
			for (int y = 0; y < puyostack.GetLine(); y++)
			{
				for (int x = 0; x < puyostack.GetColumn(); x++)
				{
					if (field_array_check[y*puyostack.GetColumn() + x] == CHECKED)
					{
						puyostack.SetValue(y, x, NONE);

						vanishednumber++;
					}
				}
			}
		}

		delete[] field_array_check;

		//削除したぷよの数を返り値として返す
		return vanishednumber;
	}

	//stack上の落下できるぷよを全て落下させる
	void StackMoveDown(PuyoArrayStack &puyostack)
	{
		//一時的格納場所メモリ確保
		puyocolor *stack_temp = new puyocolor[puyostack.GetLine()*puyostack.GetColumn()];

		for (int i = 0; i < puyostack.GetLine()*puyostack.GetColumn(); i++)
		{
			stack_temp[i] = NONE;
		}

		//1つ下の位置にぷよをずらす
		for (int y = puyostack.GetLine() - 1; y >= 0; y--)
		{
			for (int x = 0; x < puyostack.GetColumn(); x++)
			{
				if (puyostack.GetValue(y, x) == NONE) {
					continue;
				}
				//最下縁ではなく下に何も存在しない場合の処理
				if (y < puyostack.GetLine() - 1 && puyostack.GetValue(y + 1, x) == NONE && puyostack.GetValue(y + 1, x) == NONE)
				{
					stack_temp[(y + 1)*puyostack.GetColumn() + x] = puyostack.GetValue(y, x);
					//コピー後に元位置のpuyostackデータは消す
					puyostack.SetValue(y, x, NONE);
				}
				else
				{
					//上記の場合に当てはまらない場合に値を引き継ぐ処理
					stack_temp[y*puyostack.GetColumn() + x] = puyostack.GetValue(y, x);
				}
			}
		}

		//stack_tempからstackへコピー
		for (int y = 0; y < puyostack.GetLine(); y++)
		{
			for (int x = 0; x < puyostack.GetColumn(); x++)
			{
				puyostack.SetValue(y, x, stack_temp[y*puyostack.GetColumn() + x]);
			}
		}

		//一時的格納場所メモリ解放
		delete[] stack_temp;
	}

	//ぷよを時計回りに回転させる
	void RotateClockwise(PuyoArrayActive &puyoactive, PuyoArrayStack &puyostack)
	{
		puyocolor puyo1, puyo2;
		int puyo1_x = 0;
		int puyo1_y = 0;
		int puyo2_x = 0;
		int puyo2_y = 0;

		// ぷよを0番目から探して順にpuyo1, puyo2とする
		bool findingpuyo1 = true;
		for (int y = 0; y < puyoactive.GetLine(); y++)
		{
			for (int x = 0; x < puyoactive.GetColumn(); x++)
			{
				if (puyoactive.GetValue(y, x) != NONE)
				{
					if (findingpuyo1)
					{
						puyo1 = puyoactive.GetValue(y, x);
						puyo1_x = x;
						puyo1_y = y;
						findingpuyo1 = false;
					}
					else
					{
						puyo2 = puyoactive.GetValue(y, x);
						puyo2_x = x;
						puyo2_y = y;
					}
				}
			}
		}

		// ぷよがあったマスの属性をNONEに変更
		puyoactive.SetValue(puyo1_y, puyo1_x, NONE);
		puyoactive.SetValue(puyo2_y, puyo2_x, NONE);

		// 回転状態にしたがって回転処理を行う
		switch (puyoactive.GetPuyoRotate())
		{
		// 回転状態0の場合
		case 0:
			if ((puyo1_y < puyoactive.GetLine() - 1) && (puyostack.GetValue(puyo1_y + 1, puyo1_x) == NONE))
			{
				puyoactive.SetValue(puyo1_y, puyo1_x, puyo1);
				puyoactive.SetValue(puyo2_y + 1, puyo2_x - 1, puyo2);
				puyoactive.SetPuyoRotate(1);
			} else {
					puyoactive.SetValue(puyo1_y, puyo1_x, puyo1);
					puyoactive.SetValue(puyo2_y, puyo2_x, puyo2);
			}
			break;

		// 回転状態1の場合
		case 1:
			if ((0 < puyo1_x) && (puyostack.GetValue(puyo1_y, puyo1_x - 1) == NONE))
			{
				puyoactive.SetValue(puyo1_y, puyo1_x, puyo1);
				puyoactive.SetValue(puyo2_y - 1, puyo2_x - 1, puyo2);
				puyoactive.SetPuyoRotate(2);
			} else {
					puyoactive.SetValue(puyo1_y, puyo1_x, puyo1);
					puyoactive.SetValue(puyo2_y, puyo2_x, puyo2);
			}
			break;

		// 回転状態2の場合
		case 2:
			if ((0 < puyo2_y) && (puyostack.GetValue(puyo2_y - 1, puyo2_x) == NONE))
			{
				puyoactive.SetValue(puyo2_y, puyo2_x, puyo2);
				puyoactive.SetValue(puyo1_y - 1, puyo1_x + 1, puyo1);
				puyoactive.SetPuyoRotate(3);
			} else {
					puyoactive.SetValue(puyo1_y, puyo1_x, puyo1);
					puyoactive.SetValue(puyo2_y, puyo2_x, puyo2);
			}
			break;

		// 回転状態3の場合
		case 3:
			if ((puyo2_x < puyoactive.GetColumn() - 1) && (puyostack.GetValue(puyo2_y, puyo2_x + 1) == NONE))
			{
				puyoactive.SetValue(puyo2_y, puyo2_x, puyo2);
				puyoactive.SetValue(puyo1_y + 1, puyo1_x + 1, puyo1);
				puyoactive.SetPuyoRotate(0);
			} else {
					puyoactive.SetValue(puyo1_y, puyo1_x, puyo1);
					puyoactive.SetValue(puyo2_y, puyo2_x, puyo2);
			}
			break;

		default:
			break;
		}
	}

	//ぷよを反時計回りに回転させる
	void RotateCounterClockwise(PuyoArrayActive &puyoactive, PuyoArrayStack &puyostack)
	{
		puyocolor puyo1, puyo2;
		int puyo1_x = 0;
		int puyo1_y = 0;
		int puyo2_x = 0;
		int puyo2_y = 0;

		// ぷよを0番目から探して順にpuyo1, puyo2とする
		bool findingpuyo1 = true;
		for (int y = 0; y < puyoactive.GetLine(); y++)
		{
			for (int x = 0; x < puyoactive.GetColumn(); x++)
			{
				if (puyoactive.GetValue(y, x) != NONE)
				{
					if (findingpuyo1)
					{
						puyo1 = puyoactive.GetValue(y, x);
						puyo1_x = x;
						puyo1_y = y;
						findingpuyo1 = false;
					}
					else
					{
						puyo2 = puyoactive.GetValue(y, x);
						puyo2_x = x;
						puyo2_y = y;
					}
				}
			}
		}

		// ぷよがあったマスの属性をNONEに変更
		puyoactive.SetValue(puyo1_y, puyo1_x, NONE);
		puyoactive.SetValue(puyo2_y, puyo2_x, NONE);

		switch (puyoactive.GetPuyoRotate())
		{
		// 回転状態0の場合
		case 0:
			if ((0 < puyo1_y) && (puyostack.GetValue(puyo1_y - 1, puyo1_x) == NONE))
			{
				puyoactive.SetValue(puyo1_y, puyo1_x, puyo1);
				puyoactive.SetValue(puyo2_y - 1, puyo2_x - 1, puyo2);
				puyoactive.SetPuyoRotate(3);
			} else {
					puyoactive.SetValue(puyo1_y, puyo1_x, puyo1);
					puyoactive.SetValue(puyo2_y, puyo2_x, puyo2);
			}
			break;

		// 回転状態1の場合
		case 1:
			if ((puyo1_x < puyoactive.GetColumn() - 1) && (puyostack.GetValue(puyo1_y, puyo1_x + 1) == NONE))
			{
				puyoactive.SetValue(puyo1_y, puyo1_x, puyo1);
				puyoactive.SetValue(puyo2_y - 1, puyo2_x + 1, puyo2);
				puyoactive.SetPuyoRotate(0);
			} else {
					puyoactive.SetValue(puyo1_y, puyo1_x, puyo1);
					puyoactive.SetValue(puyo2_y, puyo2_x, puyo2);
			}
			break;

		// 回転状態2の場合
		case 2:
			if ((puyo2_y < puyoactive.GetLine() - 1) && (puyostack.GetValue(puyo2_y + 1, puyo2_x) == NONE))
			{
				puyoactive.SetValue(puyo2_y, puyo2_x, puyo2);
				puyoactive.SetValue(puyo1_y + 1, puyo1_x + 1, puyo1);
				puyoactive.SetPuyoRotate(1);
			} else {
					puyoactive.SetValue(puyo1_y, puyo1_x, puyo1);
					puyoactive.SetValue(puyo2_y, puyo2_x, puyo2);
			}
			break;

		// 回転状態3の場合
		case 3:
			if ((0 < puyo2_x) && (puyostack.GetValue(puyo2_y, puyo2_x - 1) == NONE))
			{
				puyoactive.SetValue(puyo2_y, puyo2_x, puyo2);
				puyoactive.SetValue(puyo1_y + 1, puyo1_x - 1, puyo1);
				puyoactive.SetPuyoRotate(2);
			} else {
					puyoactive.SetValue(puyo1_y, puyo1_x, puyo1);
					puyoactive.SetValue(puyo2_y, puyo2_x, puyo2);
			}
			break;

		default:
			break;
		}
	}

private:
	// 次と次の次に来るぷよのデータを格納する構造体
	// standbypuyoX_Y = 待ち順Xで出現するぷよのうち左からY番目のぷよ情報
	typedef struct standbypuyo{
		puyocolor standbypuyo1_1;
		puyocolor standbypuyo1_2;
		puyocolor standbypuyo2_1;
		puyocolor standbypuyo2_2;
	} STANDBYPUYO;
	STANDBYPUYO standbypuyo;
};

// 個人の記録を格納する構造体
typedef struct record {
	char name[256];
	int score;
} RECORD;

// 上位3人の個人記録を格納する構造体
typedef struct records {
	RECORD No_1;
	RECORD No_2;
	RECORD No_3;
} RECORDS;

// 上位スコアのセーブに関する操作をまとめたクラス
class SaveData
{
public:
	SaveData()
	{
		// プログラム実行時に保存ファイルsave.datがなければ生成
		FILE *fp = fopen("save.dat", "rb");
		//点検のため毎起動ごとに起動したい場合は条件に" || true"を追加
		if (fp == NULL || true){
			RECORDS records = {
				{"Donald Trump", 30},
				{"Kim Jong Eun", 20},
				{"Shinzo Abe", 10}
			};
			fp = fopen("save.dat", "wb");
			fwrite(&records, sizeof(RECORDS), 1, fp);
			fclose(fp);
		}
	}

	// ゲーム終了時のスコアがトップ3に入っている場合にお知らせ・セーブする関数
	void Save(int end_score)
	{
		// save.datがなければセーブしない
		RECORDS records;
		FILE *fp = fopen("save.dat", "rb");
		if(fp == NULL){
			return;
		}
		fread(&records, sizeof(records), 1, fp);
		fclose(fp);

		// もしscoreがTop3に入っていれば名前とスコアを記録
		// 1位よりもスコアが高い場合
		if (records.No_1.score < end_score)
		{
			strcpy(records.No_3.name, records.No_2.name);
			records.No_3.score = records.No_2.score;
			strcpy(records.No_2.name, records.No_1.name);
			records.No_2.score = records.No_1.score;
			char name[256] = "Unknown";
			strcpy(records.No_1.name, name);
			records.No_1.score = end_score;
			PrintComment(1);
		}
		// 2位よりもスコアが高い場合
		else if (records.No_2.score < end_score)
		{
			records.No_3.score = records.No_2.score;
			char name[256] = "Unknown";
			strcpy(records.No_2.name, name);
			records.No_2.score = end_score;
			PrintComment(2);
		}
		// 3位よりもスコアが高い場合
		else if (records.No_3.score < end_score)
		{
			char name[256] = "playerName";
			strcpy(records.No_3.name, name);
			records.No_3.score = end_score;
			PrintComment(3);
		}
		// 記録の更新を行う
		fp = fopen("save.dat", "wb");
		if(fp == NULL){
			return;
		}
		fwrite(&records, sizeof(records), 1, fp);
		fclose(fp);
		PrintRecord(12, COLS - 35);
	}

// Top3ランキングに載った時にコメントを表示
	void PrintComment(int order)
	{
		switch (order)
		{
			case 1:
				mvprintw(18, COLS - 35, "# Wonderful!");
				mvprintw(19, COLS - 35, "# You got No.1!");
				break;
			case 2:
				mvprintw(18, COLS - 35, "# Excellent!");
				mvprintw(19, COLS - 35, "# You got No.2!");
			case 3:
				mvprintw(18, COLS - 35, "# Cool!");
				mvprintw(19, COLS - 35, "# You got No.3!");
		}
	}

// Top3の名前とスコアを指定した座標に表示
	void PrintRecord(int y, int x)
	{
		RECORDS records;
		FILE *fp = fopen("save.dat", "rb");
		fread(&records, sizeof(records), 1, fp);
		attrset(COLOR_PAIR(4));
		mvprintw(y, x + 7, "## RANKING ##");
		mvprintw(y + 1, x, "=============================");
		mvprintw(y + 2, x, "No.1: %s", records.No_1.name);
		mvprintw(y + 2, x + 21, "%05d pt", records.No_1.score);
		mvprintw(y + 3, x, "No.2: %s", records.No_2.name);
		mvprintw(y + 3, x + 21, "%05d pt", records.No_2.score);
		mvprintw(y + 4, x, "No.3: %s", records.No_3.name);
		mvprintw(y + 4, x + 21, "%05d pt", records.No_3.score);
		attrset(COLOR_PAIR(0));
		fclose(fp);
	}
};

// puyoactiveとpuyostackのぷよの情報を統合する関数
puyocolor Merge(PuyoArrayActive &puyoactive, PuyoArrayStack &puyostack, int y, int x)
{
	if (puyoactive.GetValue(y, x) != NONE){
		return puyoactive.GetValue(y, x);
	} else {
		return puyostack.GetValue(y, x);
	}
}

// 画面表示
void Display(PuyoArrayActive &puyoactive, PuyoArrayStack &puyostack, int score)
{
	//落下中ぷよ表示
	for (int y = 0; y < puyoactive.GetLine(); y++)
	{
		for (int x = 0; x < puyoactive.GetColumn(); x++)
		{
			switch (Merge(puyoactive, puyostack, y, x))
			{
			case NONE:
				mvaddch(y, x, '.');
				break;
			case RED:
				attrset(COLOR_PAIR(1));
				//出力する文字色を変更
				mvaddch(y, x, 'R');
				attrset(COLOR_PAIR(0));
				//出力する文字色を元に戻す
				break;
			case BLUE:
				attrset(COLOR_PAIR(2));
				mvaddch(y, x, 'B');
				attrset(COLOR_PAIR(0));
				break;
			case GREEN:
				attrset(COLOR_PAIR(3));
				mvaddch(y, x, 'G');
				attrset(COLOR_PAIR(0));
				break;
			case YELLOW:
				attrset(COLOR_PAIR(4));
				mvaddch(y, x, 'Y');
				attrset(COLOR_PAIR(0));
				break;
			default:
				mvaddch(y, x, '?');
				break;
			}
		}
	}

	//情報表示
	int count = 0;
	for (int y = 0; y < puyoactive.GetLine(); y++)
	{
		for (int x = 0; x < puyoactive.GetColumn(); x++)
		{
			if (Merge(puyoactive, puyostack, y, x) != NONE)
			{
				count++;
			}
		}
	}

	char state_msg[256];
	sprintf(state_msg, "Field: %d x %d, Puyo number: %03d", puyoactive.GetLine(), puyoactive.GetColumn(), count);
	mvaddstr(2, COLS - 35, state_msg);
	char score_msg[256];
	sprintf(score_msg, "Score: %05d pt", score);
	mvaddstr(3, COLS - 35, score_msg);

	mvprintw(5, COLS - 35, "Press 'q' to Quit");
	mvprintw(6, COLS - 35, "Press 'p' to Pause");

	refresh();
}

// ゲームの進行に関わる操作を集めたクラス
class GameControl
{
public:
	// ゲームの進行に関わる変数を格納
	GameControl()
	{
		nextaction = 0;
		height = 12;
		width = 12;
		speed = 20000;
		end_score = 0;
	}

	// ゲームを実行する際に必要な設定を行う
	void InitGame()
	{
			//画面の初期化
			initscr();
			//カラー属性を扱うための初期化
			start_color();
			//キーを押しても画面に表示しない
			noecho();
			//キー入力を即座に受け付ける
			cbreak();
			//カーソルを非表示にする
			curs_set(0);
			//キー入力受付方法指定
			keypad(stdscr, TRUE);

			//キー入力非ブロッキングモード
			timeout(0);

			//出力の際に使用する色を定義
			init_pair(0, COLOR_WHITE, COLOR_BLACK);
			init_pair(1, COLOR_RED, COLOR_BLACK);
			init_pair(2, COLOR_BLUE, COLOR_BLACK);
			init_pair(3, COLOR_GREEN, COLOR_BLACK);
			init_pair(4, COLOR_YELLOW, COLOR_BLACK);
			init_pair(5, COLOR_MAGENTA, COLOR_BLACK);
	}

	// ゲームの設定画面を表示して各パラメータの値（height, width, speed）を決定
	void GameSetting(SaveData &save)
	{
		save.PrintRecord(4, COLS - 35);
		while (1)
		{
			//盤面の高さを設定
			while(1){
				int ch = getch();
				switch (ch){
					case KEY_RIGHT:
					case KEY_UP:
						if (height < 17){
							height++;
						} else {
							flash();
						}
						break;
					case KEY_LEFT:
					case KEY_DOWN:
						if (7 < height){
							height--;
						} else {
							flash();
						}
						break;
					case '\n':
						break;
				}
				mvprintw(4, 10, "*** WELCOME TO PUYO-PUYO! ***");
				mvprintw(5, 10, "=============================");
				mvprintw(7, 10, "Field Height: %02d", height);
				attrset(COLOR_PAIR(5));
				mvprintw(13, 10, "Adjust Parameters with Arrow Keys");
				mvprintw(14, 10, "Press Enter to Confirm");
				attrset(COLOR_PAIR(0));
				if (ch == '\n'){ break; }
			}
			//盤面の幅を設定
			while(1){
				int ch = getch();
				switch (ch){
					case KEY_RIGHT:
					case KEY_UP:
						if (width < 17){
							width++;
						} else {
							flash();
						}
						break;
					case KEY_LEFT:
					case KEY_DOWN:
						if (7 < width){
							width--;
						} else {
							flash();
						}
						break;
					case '\n':
						break;
				}
				mvprintw(7, 10, "Field Height: %02d", height);
				mvprintw(8, 10, "Field Width: %02d", width);
				if (ch == '\n'){ break; }
			}
			//ぷよの落下スピードを設定
			int speed_degree = 3;
			while(1){
				int ch = getch();
				switch (ch){
					case KEY_RIGHT:
					case KEY_UP:
						if (speed_degree < 5){
							speed_degree++;
						} else {
							flash();
						}
						break;
					case KEY_LEFT:
					case KEY_DOWN:
						if (1 < speed_degree){
							speed_degree--;
						} else {
							flash();
						}
						break;
					case '\n':
						break;
				}
				switch (speed_degree){
					case 1: speed = 60000; break;
					case 2: speed = 40000; break;
					case 3: speed = 20000; break;
					case 4: speed = 10000; break;
					case 5: speed = 5000; break;
				}
				mvprintw(7, 10, "Field Height: %02d", height);
				mvprintw(8, 10, "Field Width: %02d", width);
				mvprintw(9, 10, "Falling Speed: %d", speed_degree);
				if (ch == '\n'){ break; }
			}
			erase();
			break;
		}
	}

	// ぷよぷよを実行
	void GameMain()
	{
		//盤面と操作のインスタンスを作成
		PuyoArrayActive puyoactive;
		PuyoArrayStack puyostack;
		PuyoControl control;

		//初期化処理
		puyoactive.ChangeSize(height, width);
		puyostack.ChangeSize(height, width);
		control.GeneratePuyo(puyoactive);	//最初のぷよ生成

		int delay = 0;
		int waitCount = speed;

		int score = 0;

		// 落ちコン用ぷよ
		puyostack.SetValue(5, 3, BLUE);
		puyostack.SetValue(6, 3, YELLOW);
		puyostack.SetValue(7, 3, YELLOW);
		puyostack.SetValue(8, 3, YELLOW);
		puyostack.SetValue(5, 4, YELLOW);
		puyostack.SetValue(6, 4, BLUE);
		puyostack.SetValue(7, 4, BLUE);
		puyostack.SetValue(8, 4, BLUE);
		puyostack.SetValue(6, 5, YELLOW);
		puyostack.SetValue(7, 5, YELLOW);
		puyostack.SetValue(8, 5, YELLOW);

		//メイン処理ループ
		while (1)
		{
			// 稼働時
			while (1)
			{
				//キー入力受付
				int ch;
				ch = getch();

				//Qの入力で終了
				if (ch == 'q')
				{
					end_score = score;
					return;
				}
				else if (ch == 'p')
				{
					break;
				}

				//入力キーごとの処理
				switch (ch)
				{
				case KEY_LEFT:
					control.MoveLeft(puyoactive, puyostack);
					break;
				case KEY_RIGHT:
					control.MoveRight(puyoactive, puyostack);
					break;
				case 'z':
					//ぷよ回転処理（時計回り）
					control.RotateClockwise(puyoactive, puyostack);
					break;
				case 'x':
					//ぷよ回転処理（反時計回り）
					control.RotateCounterClockwise(puyoactive, puyostack);
					break;
				case KEY_DOWN:
					//下ボタンを押すと下まで一瞬で落下する
					for (int i=0; i<puyoactive.GetLine()-1; i++){
						control.MoveDown(puyoactive, puyostack);
					}
					break;
				default:
					break;
				}

				// 処理速度調整のためのif文
				// 一定間隔で行う処理（ぷよ落下・着地判定・ゲームオーバー判定・表示など）
				if (delay%waitCount == 0){
					//ぷよ下に移動
					control.MoveDown(puyoactive, puyostack);

					//着地判定があると新しいぷよを生成
					//ぷよ着地判定
					if (control.LandingPuyo(puyoactive, puyostack))
					{
						//着地感のあるsleepを入れる
						Display(puyoactive, puyostack, score);
						usleep(200000);
						//連鎖が止まるまで落下・削除を実行
						while(1){
							for (int i=0; i<puyostack.GetLine()-1; i++)
							{
								//着地したらまず落ち得るぷよを落下させる
								control.StackMoveDown(puyostack);
							}
							Display(puyoactive, puyostack, score);
							usleep(200000);
							//着地したらぷよが4つ以上連結していないかチェックする
							//ぷよに変化がなければぷよ生成に進む
							int vanishednumber = control.VanishPuyo(puyostack);
							score += vanishednumber * 10;
							Display(puyoactive, puyostack, score);
							if (vanishednumber == 0){
								break;
							} else {
								usleep(200000);
							}
						}
						//そのターンの処理がすべて終わった時点で詰んでないか確認
						if (GameOverJudge(puyostack) == true)
						{
							flash();
							end_score = score;
							return;
						}
						//着地していたら新しいぷよ生成
						control.GeneratePuyo(puyoactive);
					}
				}
				delay++;
				//表示
				Display(puyoactive, puyostack, score);
			}
			mvprintw(6, COLS - 35, "Press 'p' to Continue");
			// ポーズ画面
			while (1)
			{
				int ch = getch();
				if (ch == 'p'){ clear(); break; }
				else if (ch == 'q'){ return; }
			}
		}
	}

	// ゲームオーバーしたかどうかを判定する
	bool GameOverJudge(PuyoArrayStack &puyostack)
	{
		if ((puyostack.GetValue(0, 5) != NONE) || (puyostack.GetValue(0, 6) != NONE))
		{
			return true;
		}
		else
		{
			return false;
		}
	}

	// ゲームオーバーまたは中断した場合にスコアを記録し次の処理を受け付ける
	void GameOver(SaveData &save)
	{
		// 文字列の初期化（盤面を表示しておくため局所的に上書き）
		mvprintw(6, COLS - 35, "                        ");
		// 画面に"GameOver"を表示する
		attrset(COLOR_PAIR(1));
		mvprintw(5, COLS - 35, "=================");
		mvprintw(6, COLS - 35, "#   GAME OVER   #");
		mvprintw(7, COLS - 35, "=================");
		attrset(COLOR_PAIR(0));
		//記録が残るスコアだったらここで登記
		save.Save(end_score);

		mvprintw(9, COLS - 35, "Press 'r' to Continue");
		mvprintw(10, COLS - 35, "Press 'q' to Quit");
		while (1)
		{
			int ch = getch();
			if (ch == 'q')
			{
				erase();
				nextaction = 0;
				return;
			}
			else if (ch == 'r')
			{
				erase();
				nextaction = 1;
				return;
			}
		}
	}

	// ゲームオーバー時の次の処理を示すnextactionの値を返す
	int GetNextAction()
	{
		return nextaction;
	}

private:
	int nextaction;
	int height, width, speed;
	int end_score;
};

//ここから実行される
int main(int argc, char **argv){
	GameControl game;
	SaveData save;
	game.InitGame();
	while(1){
		game.GameSetting(save);
		game.GameMain();
		//'q'を押すまたは詰みの時GameOver画面に移動
		game.GameOver(save);
		if ( game.GetNextAction() == 0 ){	break; }
		else { continue; }
	}
	//画面をリセット
	endwin();
	return 0;
}
