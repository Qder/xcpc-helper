#include<bits/stdc++.h>
#define inf 0x3f3f3f3f
#define maxn 1005
#define endl '\n'
#define ll long long
using namespace std;

// 用法：sg(x, moves, cnt) 求单堆 x 个石子、每次只能取 moves[0..cnt-1] 个（moves[i] >= 1）时的 SG 值（要求 x < maxn）；
// 多堆互相独立时把每堆 SG 异或起来，非 0 先手胜——这就是 SG 定理，尼姆只是 moves = {1..x} 的特例
int sg(int x, int moves[], int cnt)
{
	int sgv[maxn];
	for(int i = 0; i <= x; i ++)
	{
		bool vis[maxn] = {false}; //每轮重新清空：mex 只看当前局面的后继，SG 值不会超过后继个数 cnt
		for(int j = 0; j < cnt; j ++)
			if(i >= moves[j]) vis[sgv[i - moves[j]]] = true; //取走 moves[j] 个，剩下的 i - moves[j] 就是后继局面
		sgv[i] = 0;
		while(vis[sgv[i]]) sgv[i] ++; //mex：从 0 开始找第一个没在后继里出现过的非负整数
	}
	return sgv[x];
}

int main()
{
	ios::sync_with_stdio(false);
	cin.tie(0); cout.tie(0);
	
	int T; cin >> T;
	while(T --)
	{
		int op; cin >> op; //行首就是操作数，用 cin >> op 分支读，别和 getline 混着用（会吞掉换行）
		if(op == 1)
		{
			ll n, m; cin >> n >> m;
			//巴什：m + 1 是必败周期。剩下的是 m + 1 的倍数时，先手取 k 个，后手补 m + 1 - k 个，
			//每轮总共消掉 m + 1 个，最后一定轮到先手面对 0 个；反之先手先取 n % (m + 1) 个就能把局面丢给后手
			cout << (n % (m + 1) ? "WIN" : "LOSE") << endl;
		}
		else
		{
			int k; cin >> k;
			int s = 0;
			for(int i = 1; i <= k; i ++)
			{
				int x; cin >> x;
				s ^= x;
			}
			//尼姆：异或和为 0 必败——0 只能走到非 0，非 0 一定能取某一堆的最高位把它拉回 0
			cout << (s ? "WIN" : "LOSE") << endl;
		}
	}
	return 0;
}

// 用法：bash_misere(n, m) 反巴什（每次取 1..m 个，取走最后一个的人输），返回先手是否必胜
bool bash_misere(int n, int m)
{
	if(n <= 0) return false; //没有石子可取，当前玩家已经输
	//正常巴什必败是 n % (m+1) == 0；反规则只把余数 1 变成必败（谁面对 1 颗谁被迫取最后一颗），其余周期不变
	//m == 1 时每轮只能拿 1 颗，退化成奇偶：n 为奇数必败
	return n % (m + 1) != 1;
}

// 用法：nim_misere(a, n) 反尼姆（每次从一堆取任意正数，取走最后一个的人输），堆为 a[0..n-1]
bool nim_misere(int *a, int n)
{
	int s = 0, c = 0;
	bool all_one = true;
	for(int i = 0; i < n; i ++)
	{
		s ^= a[i];
		if(a[i] > 0) c ++;
		if(a[i] > 1) all_one = false;
	}
	if(c == 0) return false; //没有石子可取
	//全是 1 时谁都没有选择，只能一堆一堆拿：偶数堆先手逼对手取最后一颗，奇数堆则相反
	if(all_one) return c % 2 == 0;
	//只要有一堆 > 1，先手就能用正常尼姆的走法控制节奏，把"只剩奇数个 1"的必败局面丢给对手，于是回到异或判据
	return s != 0;
}

// 用法：staircase_nim(a, n) 阶梯尼姆（a[0] 是第 1 堆/最底层，从第 i 堆取走的石子落到第 i-1 堆，第 1 堆取走即消失），
// 返回先手是否必胜；结论只看奇数下标（1-based）堆的异或和，非 0 先手胜
bool staircase_nim(int *a, int n)
{
	//偶数层只会把石子送到奇数层，对手能把这批石子原样再送下一层，成对抵消；只有奇数层的石子真正决定胜负
	int s = 0;
	for(int i = 0; i < n; i += 2) s ^= a[i];
	return s != 0;
}
/*
Input:
7
1 10 3
1 8 3
2 3 1 2 3
2 2 5 5
2 3 3 4 5
1 5 1
2 1 7

Output:
WIN
LOSE
LOSE
LOSE
WIN
WIN
WIN
*/
