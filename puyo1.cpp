#include <curses.h>
#include <stdlib.h> //rand, srand関数で使用
#include <time.h> //srand関数で使用
#include <unistd.h> //動作を止めるときに使用
#include <string.h> //.datを扱うために使用

//ぷよの色を表すの列挙型
//NONEが無し，RED,BLUE,..が色を表す
enum puyocolor { NONE, RED, BLUE, GREEN, YELLOW };

//ボーナス加点の情報
int comboBonus[20] = {0, 8, 16, 32, 64, 96, 128, 160, 1192, 224, 256, 288, 320, 352, 384, 416, 448, 480, 480, 512};
int vanishedNumberBonus[12] = {0, 0, 0, 0, 0, 2, 3, 4, 5, 6, 7, 10};
int colorBonus[6] = {0, 0, 3, 6, 2, 4};

//名前が入力されていないときでも記録するためのデフォルトの名前
char default_name[] = "Unknown";

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
	void ChangeSize(const unsigned int line, const unsigned int column)
	{
		Release();

		//新しいサイズでメモリ確保
		data = new puyocolor[line*column];

		data_line = line;
		data_column = column;

		//盤面をNONEで初期化
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
	puyocolor GetValue(const unsigned int y, const unsigned int x)
	{
		if (y >= GetLine() || x >= GetColumn())
		{
			//引数の値が正しくない
			return NONE;
		}

		return data[y*GetColumn() + x];
	}

	//盤面の指定された位置に値を書き込む
	void SetValue(const unsigned int y, const unsigned int x, const puyocolor value)
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
		//回転状態を初期化
		puyorotate = 1;
	}

	// ぷよの回転状態を返す
	int GetPuyoRotate(){
		return puyorotate;
	}

	// ぷよの回転状態を変更する
	void SetPuyoRotate(const int state){
		puyorotate = state;
	}

private:
	// ぷよの回転状態を表す
	// 初期状態1で右回転毎に1増える
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
		//ランダムなぷよ生成のためにrootを初期化
		srand((unsigned int)time(NULL));
		//ゲーム開始時に待ちぷよのペアを二つ生成
		randColor(standbypuyo.standbypuyo1_1);
		randColor(standbypuyo.standbypuyo1_2);
		randColor(standbypuyo.standbypuyo2_1);
		randColor(standbypuyo.standbypuyo2_2);
		//スコアのボーナスの値を初期化
		bonus = 0;
		//ある盤面状態で消したぷよの色数をカウントする配列を初期化
		for (int i = 0; i < 4; i++){ colorCount[i] = 0; }
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
		//新しいぷよを生成
		puyoactive.SetValue(0, 5, standbypuyo.standbypuyo1_1);
		puyoactive.SetValue(1, 5, standbypuyo.standbypuyo1_2);
		//次の次に生成されるぷよを次に生成されるぷよに変更
		standbypuyo.standbypuyo1_1 = standbypuyo.standbypuyo2_1;
		standbypuyo.standbypuyo1_2 = standbypuyo.standbypuyo2_2;
		//ランダムなぷよ生成のためにrootを初期化
		srand((unsigned int)time(NULL));
		//次の次に生成されるぷよをランダムに生成
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
					puyoactive.SetPuyoRotate(1);
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
	void MoveDown(PuyoArrayActive &puyoactive, PuyoArrayStack &puyostack, int &score)
	{
		//一時的格納場所メモリ確保
		puyocolor *puyo_temp = new puyocolor[puyoactive.GetLine()*puyoactive.GetColumn()];

		for (int i = 0; i < puyoactive.GetLine()*puyoactive.GetColumn(); i++)
		{
			puyo_temp[i] = NONE;
		}

		bool moved = false;
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
					moved = true;
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

		if (moved){ score++; }
	}

	//各マスにおいてぷよの連結があるかを確認する関数
	//ある盤面状態において消えたぷよの総数を返す（色数ボーナスの計算+連結bonusの計算）
	int VanishPuyo(PuyoArrayStack &puyostack)
	{
		int totalvanishednumber = 0;
		//すべてのマスにおいて4以上の連結ぷよの削除を実行
		for (int y = 0; y < puyostack.GetLine(); y++)
		{
			for (int x = 0; x < puyostack.GetColumn(); x++)
			{
				int vanishednumber = VanishPuyo(puyostack, y, x);
				//【連結bonus】
				// 消えたぷよの連結数に従ってbonusに加点
				if (vanishednumber < 11){
					bonus += vanishedNumberBonus[vanishednumber];
				}	else {
					bonus += vanishedNumberBonus[11];
				}
				//ある盤面状態において消えたぷよの数の合計
				totalvanishednumber += vanishednumber;
			}
		}
		//【色数bonus】
		//ある盤面状態において消えたぷよの色の数に従ってbonusに加点
		int colorCountNum = 0;
		for (int i = 0; i < 4; i++){ if ( colorCount[i] == 1 ){ colorCountNum++; } }
		bonus += colorBonus[colorCountNum];
		//トータルの削除したぷよ数を返す
		return totalvanishednumber;
	}

	//あるマスにおいて4つ以上のぷよの連結があるかを確認してあれば削除する関数
	//あるマスに注目したときに連結しているぷよ数を返す
	int VanishPuyo(PuyoArrayStack &puyostack, const unsigned int target_y, const unsigned int target_x)
	{
		//空のマスは飛ばす
		if (puyostack.GetValue(target_y, target_x) == NONE)
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
		field_array_check[target_y*puyostack.GetColumn() + target_x] = CHECKING;

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
			//ぷよが削除された時のぷよの色を記録する
			switch (puyostack.GetValue(target_y, target_x)){
				case RED:
					colorCount[0] = 1;
					break;
				case BLUE:
					colorCount[1] = 1;
					break;
				case GREEN:
					colorCount[2] = 1;
					break;
				case YELLOW:
					colorCount[3] = 1;
					break;
				default:
					break;
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
				//回転時のサウンドを再生
				system("mpg123 sound/rotate.mp3 >/dev/null 2>&1 &");
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
				//回転時のサウンドを再生
				system("mpg123 sound/rotate.mp3 >/dev/null 2>&1 &");
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
				//回転時のサウンドを再生
				system("mpg123 sound/rotate.mp3 >/dev/null 2>&1 &");
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
				//回転時のサウンドを再生
				system("mpg123 sound/rotate.mp3 >/dev/null 2>&1 &");
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
				//回転時のサウンドを再生
				system("mpg123 sound/rotate.mp3 >/dev/null 2>&1 &");
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
				//回転時のサウンドを再生
				system("mpg123 sound/rotate.mp3 >/dev/null 2>&1 &");
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
				//回転時のサウンドを再生
				system("mpg123 sound/rotate.mp3 >/dev/null 2>&1 &");
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
				//回転時のサウンドを再生
				system("mpg123 sound/rotate.mp3 >/dev/null 2>&1 &");
			} else {
					puyoactive.SetValue(puyo1_y, puyo1_x, puyo1);
					puyoactive.SetValue(puyo2_y, puyo2_x, puyo2);
			}
			break;

		default:
			break;
		}
	}

	//次と次の次に生成されるぷよの情報（standbypuyo構造体）にアクセスするための関数
	puyocolor GetStandByPuyo(const int set, const int order)
	{
		//setはぷよが生成されるまでの待ち順
		//orderはあるぷよの組みの中でどちらのぷよであるかを示す
		switch (set){
			case 1:
				return order == 1 ? standbypuyo.standbypuyo1_1: standbypuyo.standbypuyo1_2;
			case 2:
				return order == 1 ? standbypuyo.standbypuyo2_1: standbypuyo.standbypuyo2_2;
		}
		return NONE;
	}

	//bonusの値を返す
	int GetBonus()
	{
		return bonus == 0 ? 1: bonus;
	}

	//ボーナス計算に関連したメンバ変数を初期化
	void ResetBonus()
	{
		bonus = 0;
		for (int i = 0; i < 4; i++){ colorCount[i] = 0; }
	}

	//コンボ数に従ってbonusに加点する
	void AddComboBonus(const int comboCount)
	{
		if (comboCount < 19){
			bonus += comboBonus[comboCount - 1];
		} else {
			bonus += comboBonus[19];
		}
	}

private:
	// 次と次の次に来るぷよのデータを格納する構造体
	// standbypuyoX_Y = 待ち順Xで出現するぷよのうち上からY番目のぷよ情報
	typedef struct standbypuyo{
		puyocolor standbypuyo1_1;
		puyocolor standbypuyo1_2;
		puyocolor standbypuyo2_1;
		puyocolor standbypuyo2_2;
	} STANDBYPUYO;
	STANDBYPUYO standbypuyo;

	//スコア計算に使用するボーナスの値を保持する変数
	int bonus;
	//消したぷよの色の数をカウントするのに使用する配列
	int colorCount[4];
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
	SaveData(char *playername)
	{
		// プログラム実行時に保存ファイルsave.datがなければ生成
		FILE *fp = fopen("save.dat", "rb");
		//点検のため毎起動ごとに起動したい場合は条件に" || true"を追加
		if (fp == NULL){
			RECORDS records = {
				{"Donald Trump", 70000},
				{"Kim Jong Eun", 40000},
				{"Shinzo Abe", 20000}
			};
			fp = fopen("save.dat", "wb");
			fwrite(&records, sizeof(RECORDS), 1, fp);
			fclose(fp);
		}
		//プログラム実行時に渡された引数に従って名前を管理
		if ( playername == NULL ){ playerName = default_name; }
		else { playerName = playername; }
	}

	// ゲーム終了時のスコアがトップ3に入っている場合にお知らせ・セーブする関数
	void Save( const int end_score )
	{
		// save.datがなければセーブしない
		RECORDS records;
		FILE *fp = fopen("save.dat", "rb");
		if(fp == NULL){
			return;
		}
		//recordsに情報をコピー
		fread(&records, sizeof(records), 1, fp);
		fclose(fp);

		// もしscoreがTop3に入っていれば名前とスコアを記録
		// 1位よりもスコアが高い場合
		if (records.No_1.score < end_score)
		{
			//順位を入れ替えて記録
			strcpy(records.No_3.name, records.No_2.name);
			records.No_3.score = records.No_2.score;
			strcpy(records.No_2.name, records.No_1.name);
			records.No_2.score = records.No_1.score;
			strcpy(records.No_1.name, playerName);
			records.No_1.score = end_score;
			//順位に従ったコメントを表示
			PrintComment(1);
		}
		// 2位よりもスコアが高い場合
		else if (records.No_2.score < end_score)
		{
			//順位を入れ替えて記録
			records.No_3.score = records.No_2.score;
			strcpy(records.No_2.name, playerName);
			records.No_2.score = end_score;
			//順位に従ったコメントを表示
			PrintComment(2);
		}
		// 3位よりもスコアが高い場合
		else if (records.No_3.score < end_score)
		{
			//順位を入れ替えて記録
			strcpy(records.No_3.name, playerName);
			records.No_3.score = end_score;
			//順位に従ったコメントを表示
			PrintComment(3);
		}
		// 記録の更新を行う
		fp = fopen("save.dat", "wb");
		if(fp == NULL){
			return;
		}
		fwrite(&records, sizeof(records), 1, fp);
		fclose(fp);
		//更新した記録を表示する
		PrintRecord(12, COLS - 35);
	}

	// Top3ランキングに載った時にコメントを表示
	void PrintComment(const int order)
	{
		//orderは順位
		switch (order)
		{
			case 1:
				mvprintw(18, COLS - 35, "# Wonderful!");
				mvprintw(19, COLS - 35, "# You got No.1!");
				break;
			case 2:
				mvprintw(18, COLS - 35, "# Excellent!");
				mvprintw(19, COLS - 35, "# You got No.2!");
				break;
			case 3:
				mvprintw(18, COLS - 35, "# Cool!");
				mvprintw(19, COLS - 35, "# You got No.3!");
				break;
		}
	}

	// Top3の名前とスコアを指定した座標に表示
	void PrintRecord(const int y, const int x)
	{
		RECORDS records;
		FILE *fp = fopen("save.dat", "rb");
		fread(&records, sizeof(records), 1, fp);
		attrset(COLOR_PAIR(4));
		mvprintw(y, x + 9, "## RANKING ##");
		mvprintw(y + 1, x, "===============================");
		mvprintw(y + 2, x, "No.1: %s", records.No_1.name);
		mvprintw(y + 2, x + 20, "%08d pt", records.No_1.score);
		mvprintw(y + 3, x, "No.2: %s", records.No_2.name);
		mvprintw(y + 3, x + 20, "%08d pt", records.No_2.score);
		mvprintw(y + 4, x, "No.3: %s", records.No_3.name);
		mvprintw(y + 4, x + 20, "%08d pt", records.No_3.score);
		attrset(COLOR_PAIR(0));
		fclose(fp);
	}

private:
	//プレイヤーの名前を格納
	char *playerName;
};

// puyoactiveとpuyostackのぷよの情報を統合する関数
puyocolor Merge(PuyoArrayActive &puyoactive, PuyoArrayStack &puyostack, const int y, const int x)
{
	if (puyoactive.GetValue(y, x) != NONE){
		return puyoactive.GetValue(y, x);
	} else {
		return puyostack.GetValue(y, x);
	}
}

// 指定された座標に属性に従ってぷよを表示する
void DisplayPuyo(const int y, const int x, const puyocolor puyo)
{
	switch (puyo){
		case NONE:
			mvaddch(y, x + 15, ' ');
			if ( y == 0 && x == 5 ){ mvaddch(y, x + 15, 'x'); }
			break;
		case RED:
			attrset(COLOR_PAIR(1));
			//出力する文字色を変更
			mvaddch(y, x + 15, '@');
			attrset(COLOR_PAIR(0));
			//出力する文字色を元に戻す
			break;
		case BLUE:
			attrset(COLOR_PAIR(2));
			mvaddch(y, x + 15, '@');
			attrset(COLOR_PAIR(0));
			break;
		case GREEN:
			attrset(COLOR_PAIR(3));
			mvaddch(y, x + 15, '@');
			attrset(COLOR_PAIR(0));
			break;
		case YELLOW:
			attrset(COLOR_PAIR(4));
			mvaddch(y, x + 15, '@');
			attrset(COLOR_PAIR(0));
			break;
		default:
			mvaddch(y, x + 10, '?');
			break;
	}
}

// 画面表示
void Display(PuyoArrayActive &puyoactive, PuyoArrayStack &puyostack, PuyoControl &control, const int score)
{
	//落下中ぷよ表示
	for (int y = 0; y < puyoactive.GetLine(); y++)
	{
		for (int x = 0; x < puyoactive.GetColumn(); x++)
		{
			DisplayPuyo(y, x, Merge(puyoactive, puyostack, y, x));
		}
	}

	//枠を表示
	for (int i = 0; i < puyoactive.GetLine() + 1; i++){ mvaddch(i, 14, '#'); }
	for (int i = 0; i < puyoactive.GetLine() + 1; i++){ mvaddch(i, puyoactive.GetColumn() + 15, '#'); }
	for (int i = 0; i < puyoactive.GetColumn() + 2; i++){ mvaddch(puyoactive.GetLine(), i + 14, '#'); }

	//次と次の次に生成される待ちぷよを表示する
	DisplayPuyo(1, puyoactive.GetColumn() + 3, control.GetStandByPuyo(1, 1));
	DisplayPuyo(2, puyoactive.GetColumn() + 3, control.GetStandByPuyo(1, 2));
	DisplayPuyo(4, puyoactive.GetColumn() + 3, control.GetStandByPuyo(2, 1));
	DisplayPuyo(5, puyoactive.GetColumn() + 3, control.GetStandByPuyo(2, 2));

	//盤面上にあるぷよの数をカウント
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

	//各種ステータス情報を表示
	char state_msg[256];
	sprintf(state_msg, "Field: %d x %d, Puyo number: %04d", puyoactive.GetLine(), puyoactive.GetColumn(), count);
	mvaddstr(2, COLS - 35, state_msg);
	char score_msg[256];
	sprintf(score_msg, "Score: %08d pt", score);
	mvaddstr(3, COLS - 35, score_msg);

	mvprintw(5, COLS - 35, "Press 'q' to Quit");
	mvprintw(6, COLS - 35, "Press 'p' to Pause");

	refresh();
}

// ゲームの進行に関わる操作を集めたクラス
class GameControl
{
public:
	// ゲームの進行に関わる変数を初期化
	GameControl()
	{
		nextaction = 0;
		height = 12;
		width = 12;
		speed = 30000;
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

	// ゲームの設定画面を表示して盤面サイズを決定
	void GameSetting(SaveData &save)
	{
		//プログラムを実行してはじめに立ち上がる設定画面
		save.PrintRecord(4, COLS - 35);
		mvprintw(4, 10, "*** WELCOME TO PUYO-PUYO! ***");
		mvprintw(5, 10, "=============================");
		attrset(COLOR_PAIR(5));
		mvprintw(11, 10, "[<-] or [->] / [Enter]");
		mvprintw(12, 10, "Select Field Size");
		attrset(COLOR_PAIR(0));
		//フィールドサイズは0から2の3段階でデフォルトは1
		int fieldSize = 1;
		while (1)
		{
			//難易度を設定する
			while(1){
				int ch = getch();
				switch (ch){
					case KEY_RIGHT:
					case KEY_UP:
						if (fieldSize < 2){
							fieldSize++;
							//選択した際にサウンドを再生する
							system("mpg123 sound/select.mp3 >/dev/null 2>&1 &");
						} else {
							flash();
						}
						break;
					case KEY_LEFT:
					case KEY_DOWN:
						if (0 < fieldSize){
							 fieldSize--;
 							//選択した際にサウンドを再生する
							 system("mpg123 sound/select.mp3 >/dev/null 2>&1 &");
						} else {
							flash();
						}
						break;
				}
				//fieldSizeに従って盤面サイズを変更
				mvprintw(7, 10, "Field Size: ");
				attron(A_BLINK);
				switch (fieldSize){
					case 0:
						mvprintw(7, 22, "Small ");
						height = 8; width = 8;
						break;
					case 1:
						mvprintw(7, 22, "Normal");
						height = 12; width = 12;
						break;
					case 2:
						mvprintw(7, 22, "Large ");
						height = 14; width = 14;
						break;
				}
				attroff(A_BLINK);
				//決定したときのサウンドを再生
				if (ch == '\n'){ system("mpg123 sound/selected.mp3 >/dev/null 2>&1 &"); break; }
			}
			//盤面の大きさとぷよ落下速度は直接変更できないようにする
			/*
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
			//ぷよの落下スピードを設定／３段解（Slow/Normal/Fast）
			//設定初期値はNormal
			int speed_degree = 1;
			while(1){
				int ch = getch();
				switch (ch){
					case KEY_RIGHT:
					case KEY_UP:
						if (speed_degree < 2){
							speed_degree++;
						} else {
							flash();
						}
						break;
					case KEY_LEFT:
					case KEY_DOWN:
						if (0 < speed_degree){
							speed_degree--;
						} else {
							flash();
						}
						break;
					case '\n':
						break;
				}
				mvprintw(7, 10, "Field Height: %02d", height);
				mvprintw(8, 10, "Field Width: %02d", width);
				switch (speed_degree){
					case 0: speed = 50000; mvprintw(9, 10, "Falling Speed: Slow  "); break;
					case 1: speed = 30000; mvprintw(9, 10, "Falling Speed: Normal"); break;
					case 2: speed = 5000; mvprintw(9, 10, "Falling Speed: Fast  "); break;
				}
				if (ch == '\n'){ break; }
			}
			*/
			//盤面サイズを決定してから少し間を置いてゲーム開始
			usleep(800000);
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

		//スコアを記録
		int score = 0;

		// 落ちコンテスト用ぷよ
		/*
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
		*/

		//bgmを再生
		system("mpg123 -Z sound/bgm.mp3 >/dev/null 2>&1 &");

		//メイン処理ループ
		while (1)
		{
			// 稼働時
			while (1)
			{
				//キー入力受付
				int ch;
				ch = getch();

				//qの入力で終了
				if (ch == 'q')
				{
					end_score = score;
					//終了時にbgmを停止
					system("killall mpg123 >/dev/null 2>&1 &");
					return;
				}
				//pの入力でポーズ
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
					//落下速度を上昇させる
					waitCount = 100;
					//下ボタンを押すと下まで一瞬で落下する
					//テトリスで使用する
					/*
					for (int i=0; i<puyoactive.GetLine()-1; i++){
						control.MoveDown(puyoactive, puyostack);
					}
					*/
					break;
				default:
					break;
				}

				// 処理速度調整のためのif文
				// 一定間隔で行う処理（ぷよ落下・着地判定・ゲームオーバー判定・表示など）
				if (delay%waitCount == 0){
					//ぷよ下に移動
					control.MoveDown(puyoactive, puyostack, score);

					//着地判定があると新しいぷよを生成
					//ぷよ着地判定
					if (control.LandingPuyo(puyoactive, puyostack))
					{
						system("mpg123 sound/landed.mp3 >/dev/null 2>&1 &");
						//着地感のあるsleepを入れる
						//着地判定された時点で少し待機
						Display(puyoactive, puyostack, control, score);
						usleep(200000);

						//連鎖数=コンボ数をカウント
						int comboCount = 1;
						//連鎖が止まるまで落下・削除を実行
						while(1){
							for (int i=0; i<puyostack.GetLine()-1; i++)
							{
								//着地したらまず落下ぷよを落下させる
								control.StackMoveDown(puyostack);
							}

							//着地感のあるsleepを入れる
							//落下ぷよが全て落下した時点で少し待機
							Display(puyoactive, puyostack, control, score);
							usleep(200000);

							//着地したらぷよが4つ以上連結していないかチェックする
							//【連鎖bonus】
							int totalvanishednumber = control.VanishPuyo(puyostack);
							control.AddComboBonus(comboCount);

							//スコアを計算
							score += totalvanishednumber * control.GetBonus() * 10;
							//scoreに加算した後ボーナスの値を初期化
							control.ResetBonus();

							//ぷよが消滅した後の盤面を表示
							Display(puyoactive, puyostack, control, score);

							//ぷよに変化がなければぷよ生成に進む
							if (totalvanishednumber == 0)
							{
								//コンボ数表示の初期化
								mvprintw(puyoactive.GetLine(), puyoactive.GetColumn() + 18, "          ");

								//全消しボーナス判定
								bool exist = false;
								for (int y = 0; y < puyoactive.GetLine(); y++)
								{
									for (int x = 0; x < puyoactive.GetColumn(); x++)
									{
										if (puyostack.GetValue(y, x) != NONE){ exist = true; break; }
									}
								}
								if ( exist == false ){
									//scoreに全消しボーナス加算
									score += 3600;
									//全消しのテロップ
									mvprintw(puyoactive.GetLine(), puyoactive.GetColumn() + 18, "PERFECT!!");
									//全消しのサウンドを再生
									system("mpg123 sound/perfect.mp3 >/dev/null 2>&1 &");
								}
								break;
							}
							else
							{
								//ぷよが消滅するサウンドを再生
								system("mpg123 sound/vanished.mp3 >/dev/null 2>&1 &");
								//コンボ数加算
								comboCount++;
								//コンボ数の表示
								mvprintw(puyoactive.GetLine(), puyoactive.GetColumn() + 18, "%02d COMBO!!", comboCount - 1);
								//ぷよが消滅して空きマスがある感じをだすためにsleepを入れる
								//ぷよが消滅した時点で少し待機
								usleep(200000);
							}
						}

						//そのターンの処理がすべて終わった時点で詰んでないか確認
						if (GameOverJudge(puyostack) == true)
						{
							flash();
							end_score = score;
							//サウンドを全て停止
							system("killall mpg123 >/dev/null 2>&1 &");
							return;
						}
						//着地していたら新しいぷよ生成
						control.GeneratePuyo(puyoactive);
					}
					//時の流れをリセット
					waitCount = 30000;
				}
				delay++;
				//表示
				Display(puyoactive, puyostack, control, score);
			}
			//ボーズ画面
			mvprintw(6, COLS - 35, "Press 'p' to Continue");
			while (1)
			{
				int ch = getch();
				//pが押されるとゲームに戻る
				if (ch == 'p'){ clear(); break; }
				else if (ch == 'q'){ return; }
			}
		}
	}

	// ゲームオーバーしたかどうかを判定する
	bool GameOverJudge(PuyoArrayStack &puyostack)
	{
		if ((puyostack.GetValue(0, 5) != NONE))
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
		//記録が残るスコアだったらここで記録・表示
		save.Save(end_score);

		//ガイドを表示
		mvprintw(9, COLS - 35, "Press 'r' to Continue");
		mvprintw(10, COLS - 35, "Press 'q' to Quit");
		//ゲームオーバーのサウンドを再生
		system("mpg123 sound/gameover.mp3 >/dev/null 2>&1 &");
		//ゲームオーバー画面からどうするかを受け付ける
		while (1)
		{
			int ch = getch();
			if (ch == 'q')
			{
				//ゲーム終了
				erase();
				nextaction = 0;
				return;
			}
			else if (ch == 'r')
			{
				//設定画面に戻る
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
	//ゲームオーバー画面からどうするかの情報を格納
	int nextaction;
	//各種パラメータ
	int height, width, speed;
	//ゲームオーバー時のスコアを格納
	int end_score;
};

//ここから実行される
int main(int argc, char *argv[]){
	GameControl game;
	//コマンドライン引数からプレイヤー名を受け取る変数
	char *tmp;
	//入力されていなければデフォルトの名前を使用
	if (argc < 2){ tmp = NULL; } else { tmp = argv[1]; }
	SaveData save(tmp);
	//ゲームの初期化
	game.InitGame();
	while(1){
		game.GameSetting(save);
		game.GameMain();
		//'q'を押すまたは詰みの時GameOver画面に移動
		game.GameOver(save);
		//GameOver関数の戻り値に従って次の動作を実行
		if ( game.GetNextAction() == 0 ){	break; }
		else { continue; }
	}
	//画面をリセット
	endwin();
	return 0;
}
