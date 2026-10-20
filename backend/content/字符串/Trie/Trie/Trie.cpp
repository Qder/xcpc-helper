#include<bits/stdc++.h>
#define inf 0x3f3f3f3f
#define maxn 100005
#define maxm 3000005
#define endl '\n'
#define ll long long
using namespace std;

// 约束：n <= 1e5；所有插入串总长 <= 1e5（maxn）；op4 的二进制串长 <= 30（值 < 2^30）
// maxn：字母 Trie 节点数上限 = 所有插入串的总长度 + 1；maxm：01-Trie 节点数上限 = 插入次数 * 30
// 空间：ch[maxn][26] 约 10MB（maxn = 1e5），trie[maxm][2] 约 24MB；总插入长度到 1e6 时把 maxn 改成 1e6+5（约 104MB），
//       插入次数超过 1e5 时 maxm 也要同步调大（每多插一个数多 30 个节点）
int ch[maxn][26], cnt[maxn], sz[maxn], tot;
int trie[maxm][2], ttot, mx, ins; //mx = 当前两两异或最大值，ins = 已插入串数

// 边下标：小写字母用 c - 'a'，op4 的二进制串 '0'/'1' 用 c - '0'。
// 两组数据不会混在同一组里（op4 只对 '0'/'1' 串有意义），所以 26 个槽够用。
int cid(char c) { return c >= '0' && c <= '9' ? c - '0' : c - 'a'; }

// 只对 '0'/'1' 串保证语义：把串当 30 位以内的二进制数（前导 0 不影响值，长度超过 30 会截断）
int bin_val(const string &s)
{
	unsigned x = 0;
	for(char c : s) x = (x << 1 | (unsigned)(c - '0')) & 0x3fffffff;
	return (int)x;
}

void insert(const string &s)
{
	int u = 0;
	for(char c : s)
	{
		int &v = ch[u][cid(c)];
		if(!v) v = ++ tot;
		u = v;
		sz[u] ++; //经过 u 的串数 +1，也就是"以根到 u 的串为前缀"的串数
	}
	cnt[u] ++; //以 u 结尾的串数
}

int query_cnt(const string &s)
{
	int u = 0;
	for(char c : s)
	{
		u = ch[u][cid(c)];
		if(!u) return 0; //路径断了，说明这个串没插入过
	}
	return cnt[u];
}

int query_pre(const string &s)
{
	int u = 0;
	for(char c : s)
	{
		u = ch[u][cid(c)];
		if(!u) return 0;
	}
	return sz[u]; //sz 插入时已维护好，等价于子树里 cnt 之和，重复插入也照算
}

// 用法：插入 x 之前先 mx = max(mx, trie_max_xor(x))，再 trie_insert(x)；返回 x 与已有数的最大异或
int trie_max_xor(int x)
{
	int u = 0, res = 0;
	for(int i = 29; i >= 0; i --)
	{
		int b = (x >> i) & 1;
		if(trie[u][b ^ 1]) { res |= 1 << i; u = trie[u][b ^ 1]; } //能走相反位就走，这一位异或才是 1
		else u = trie[u][b];
	}
	return res;
}

void trie_insert(int x)
{
	int u = 0;
	for(int i = 29; i >= 0; i --)
	{
		int b = (x >> i) & 1;
		if(!trie[u][b]) trie[u][b] = ++ ttot;
		u = trie[u][b];
	}
}

int main()
{
	ios::sync_with_stdio(false);
	cin.tie(0); cout.tie(0);
	
	int n;
	cin >> n;
	while(n --)
	{
		int op; string w;
		cin >> op;
		if(op == 4)
		{
			cout << mx << endl; //空集 / 只有一个串时 mx 始终是 0
			continue;
		}
		cin >> w;
		if(op == 1)
		{
			insert(w);
			int x = bin_val(w);
			if(ins) mx = max(mx, trie_max_xor(x)); //先查后插，避免自己异或自己
			trie_insert(x);
			ins ++;
		}
		else if(op == 2) cout << query_cnt(w) << endl;
		else cout << query_pre(w) << endl;
	}
	return 0;
}
/*
Input:
10
1 1
1 10
1 101
2 10
3 10
4
1 10
2 10
3 1
4

Output:
1
2
7
2
4
7
*/
