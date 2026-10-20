#include<bits/stdc++.h>
#define inf 0x3f3f3f3f
#define maxn 1000010
#define endl '\n'
#define ll long long
using namespace std;

// maxn 是节点数上限：PAM 每个非根节点对应一种本质不同回文串，节点数 = 种类数 + 2 <= n + 2
// 空间：nxt[maxn][26] 约 104MB，n = 1e6 时整份约 120MB

int sz, last; //sz 是最后建出的节点编号（0、1 是根），last 是当前前缀的最长回文后缀
int len[maxn], fail[maxn], nxt[maxn][26], cnt[maxn]; //cnt 建完后 = 该回文串的出现次数
string s;

void pam_init()
{
	sz = 1;
	len[0] = 0; fail[0] = 1; //0 号偶根：长度 0，fail 指向奇根
	len[1] = -1; fail[1] = 1; //1 号奇根：长度 -1，让单字符也能套同一个扩展公式
	memset(nxt[0], 0, sizeof(nxt[0]));
	memset(nxt[1], 0, sizeof(nxt[1]));
	last = 0;
}

// 从 u 沿 fail 往上找，返回第一个能在两边接上 s[pos] 的回文后缀
// 奇根 len = -1，条件 pos - len[u] - 1 >= 0 恒成立，所以循环一定停得下来
int getfail(int u, int pos)
{
	while(pos - len[u] - 1 < 0 || s[pos - len[u] - 1] != s[pos]) u = fail[u];
	return u;
}

void add(int pos)
{
	int c = s[pos] - 'a';
	int cur = getfail(last, pos);
	if(!nxt[cur][c]) //这种回文串第一次出现才建新点
	{
		int now = ++ sz;
		len[now] = len[cur] + 2;
		memset(nxt[now], 0, sizeof(nxt[now]));
		if(len[now] == 1) fail[now] = 0; //单字符的最长真回文后缀是空串，即偶根
		else fail[now] = nxt[getfail(fail[cur], pos)][c]; //接在 cur 的最长真回文后缀上
		nxt[cur][c] = now;
	}
	last = nxt[cur][c];
	cnt[last] ++; //这里只给"当前最长回文后缀"计 1，更短的出现在后面靠 fail 树累加
}

// 两种 cnt 语义：建树时的 cnt 是"以该回文串结尾的前缀个数"；沿 fail 树逆序累加后才是出现次数
// 不用建 fail 树：fail 一定指向更早创建的节点，所以编号从大到小就是 fail 树自底向上的顺序
void count_occurrences()
{
	for(int u = sz; u >= 2; u --) cnt[fail[u]] += cnt[u];
}

int main()
{
	ios::sync_with_stdio(false);
	cin.tie(0); cout.tie(0);
	
	cin >> s;
	pam_init();
	for(int i = 0; i < (int)s.size(); i ++) add(i);
	
	int mx = 0; //最长回文子串长度
	for(int i = 2; i <= sz; i ++) mx = max(mx, len[i]);
	count_occurrences();
	ll tot = 0; //回文子串总数（按区间计），全同字符时约 n^2/2 = 5e11，必须 ll
	for(int i = 2; i <= sz; i ++) tot += cnt[i];
	cout << sz - 1 << endl; //本质不同回文串个数 = 节点数减 2 = sz + 1 - 2
	cout << mx << endl;
	cout << tot << endl;
	return 0;
}

/*
Input:
ababa

Output:
5
5
9
*/