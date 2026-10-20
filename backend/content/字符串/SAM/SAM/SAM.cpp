#include<bits/stdc++.h>
#define inf 0x3f3f3f3f
#define maxn 2000010
#define maxm 1000010
#define endl '\n'
#define ll long long
using namespace std;

// maxn 是状态数上限，必须开到 2 * |a|（SAM 的状态数 <= 2n - 1）；maxm 是串长上限，计数排序的桶按 len 开
// 空间：nxt[2e6][26] 一个就 208MB；n = 1e6 时实测峰值：全 a 约 126MB、随机 01 串约 240MB
// 内存限制 256MB 的题要按需把 maxn 调小，或者把 nxt 换成"只存实际转移"的邻接写法

int sz, last;
int len[maxn], lnk[maxn], nxt[maxn][26], cnt[maxn]; //lnk 就是后缀链接 link，nxt 是转移，避开 std::next 和 Linux 的 link()
int buc[maxm], ord[maxn];

void sam_init()
{
	sz = last = 1; //根固定是 1 号；0 号不用，lnk[1] = 0 当"没有后缀链接"
	len[1] = 0; lnk[1] = 0; cnt[1] = 0;
	memset(nxt[1], 0, sizeof(nxt[1]));
}

void extend(int c)
{
	int p = last, u = ++ sz;
	len[u] = len[p] + 1; //新主状态代表的最长串 = 当前前缀
	cnt[u] = 1; //主状态：一个前缀末尾贡献 1 次出现；clone 点必须清 0
	memset(nxt[u], 0, sizeof(nxt[u]));
	for(; p && !nxt[p][c]; p = lnk[p]) nxt[p][c] = u; //沿 lnk 把缺 c 转移的都补上
	if(!p) lnk[u] = 1; //一路补到根：u 的最长真后缀只剩空串
	else
	{
		int q = nxt[p][c];
		if(len[q] == len[p] + 1) lnk[u] = q;
		else //拆点：q 里混了两种 endpos，克隆出长度恰好 len[p]+1 的那一半
		{
			int clone = ++ sz;
			len[clone] = len[p] + 1;
			lnk[clone] = lnk[q];
			memcpy(nxt[clone], nxt[q], sizeof(nxt[q]));
			cnt[clone] = 0; //clone 不是某次插入的末尾，不能计 1
			lnk[u] = lnk[q] = clone;
			for(; p && nxt[p][c] == q; p = lnk[p]) nxt[p][c] = clone;
		}
	}
	last = u;
}

// 求 endpos 大小（= 出现次数）：按 len 计数排序得到"len 升序"的拓扑序，再倒着沿 lnk 累加
// 不能递归 DFS：lnk 树最坏是一条 n 深的链（全 a 的串），1e6 直接爆栈；
// 按 len 基数排序是因为 len[lnk[u]] < len[u]，len 降序天然就是 lnk 树自底向上的顺序
void build()
{
	int total = len[last]; //串长
	memset(buc, 0, sizeof(buc));
	for(int i = 1; i <= sz; i ++) buc[len[i]] ++;
	for(int i = 1; i <= total; i ++) buc[i] += buc[i - 1];
	for(int i = 1; i <= sz; i ++) ord[buc[len[i]] --] = i; //ord 按 len 升序
	for(int i = sz; i >= 2; i --)
	{
		int u = ord[i];
		cnt[lnk[u]] += cnt[u]; //子状态的 endpos 一定被父状态包含，直接加给 lnk
	}
}

// 两串最长公共子串：沿转移走，失配就跳 lnk 把当前匹配串缩成它的最长后缀
// 用法：ans = lcs(b);  （a 已经建好 SAM，build 不是必须的）
int lcs(const string &t)
{
	int v = 1, l = 0, best = 0;
	for(char ch : t)
	{
		int c = ch - 'a';
		while(v != 1 && !nxt[v][c])
		{
			v = lnk[v];
			l = len[v]; //缩到 lnk 代表的最长串
		}
		if(nxt[v][c]) v = nxt[v][c], l ++;
		else l = 0; //根上也没有 c 转移，只能从空串重新开始
		if(l > best) best = l;
	}
	return best;
}

int main()
{
	ios::sync_with_stdio(false);
	cin.tie(0); cout.tie(0);
	
	string a, b;
	getline(cin, a);
	getline(cin, b); //b 允许为空，所以用 getline 而不是 cin >> b
	
	sam_init();
	for(char ch : a) extend(ch - 'a');
	build();
	ll dif = 0; //本质不同子串数最多约 n^2/2 = 5e11，必须 ll
	for(int i = 2; i <= sz; i ++) dif += len[i] - len[lnk[i]];
	cout << dif << endl;
	ll best = 0; //len * cnt 最大
	for(int i = 2; i <= sz; i ++)
		if(cnt[i] >= 2) best = max(best, (ll)len[i] * cnt[i]);
	cout << best << endl;
	cout << lcs(b) << endl;
	return 0;
}

/*
Input:
aabab
aba

Output:
11
4
3
*/
