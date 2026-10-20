#include<bits/stdc++.h>
#define inf 0x3f3f3f3f
#define maxn 100050
#define endl '\n'
#define ll long long
using namespace std;

int n, q;
int pre[maxn];
int cnt[1 << 20]; //a_i < 2^20，异或不会把位数撑大，值域就在 2^20 以内，数组当桶比 map 快
int a[maxn]; //原数组：main 里读入时顺手存一份，或者用 restore 从 pre 还原

int main()
{
	ios::sync_with_stdio(false);
	cin.tie(0); cout.tie(0);
	
	cin >> n;
	for(int i = 1; i <= n; i ++)
	{
		int x; cin >> x;
		pre[i] = pre[i - 1] ^ x; //异或的逆运算还是异或，所以 pre[r] ^ pre[l-1] 正好把 1..l-1 抵消掉
	}
	cin >> q;
	while(q --)
	{
		int l, r; cin >> l >> r;
		cout << (pre[r] ^ pre[l - 1]) << endl;
	}
	ll ans = 0;
	cnt[0] = 1; //pre[0] = 0 是空前缀，也要算一个：少了它，所有从 a[1] 开始的零异或区间都会漏
	for(int i = 1; i <= n; i ++)
	{
		ans += cnt[pre[i]]; //先配对再计数：此刻桶里只有 i 之前的前缀，配出来的区间右端点才是 i，不会把自己算进去
		cnt[pre[i]] ++;
	}
	cout << ans << endl;
	return 0;
}

// 用法：count_xor_k(n, k) 统计 a[1..n] 里异或和恰好等于 k 的子数组个数（k 任意，不再是 0）；
// 异或和为 k ⇔ pre[r] ^ pre[l-1] = k，所以扫到 cur 时数前面有几个前缀等于 cur ^ k 就行
ll count_xor_k(int n, int k)
{
	map<int, int> mp; //k 任意时 pre[i] ^ k 可能超出 2^20，桶装不下；也不能污染 main 里的全局 cnt
	mp[0] = 1; //cnt[0] = 1 表示空前缀（这里换成 mp[0]）：少了它，所有从 a[1] 开始的区间都会漏
	int cur = 0;
	ll ans = 0;
	for(int i = 1; i <= n; i ++)
	{
		cur ^= a[i];
		ans += mp[cur ^ k]; //先查后插：配出来的区间右端点才是 i，不会把自己配进去
		mp[cur] ++;
	}
	return ans;
}

// 用法：restore(n) 由前缀异或数组 pre[1..n] 还原原数组，写回 a[1..n]（a[i] = pre[i] ^ pre[i-1]）；
// 原地会覆盖，先备份：想把 pre 直接当 a 用的话，先存一份或从右往左还原
void restore(int n)
{
	for(int i = 1; i <= n; i ++)
		a[i] = pre[i] ^ pre[i - 1]; //pre[i] ^ pre[i-1] 把 1..i-1 抵消掉，只剩 a[i]
}

// 用法：xor_1_to_n(x) 返回 1 ^ 2 ^ ... ^ x，O(1)，x < 0 返回 0；
// 连续整数异或每 4 个一循环：x % 4 == 0 → x，== 1 → 1，== 2 → x + 1，== 3 → 0
ll xor_1_to_n(ll x)
{
	if(x < 0) return 0;
	if(x % 4 == 0) return x;
	if(x % 4 == 1) return 1;
	if(x % 4 == 2) return x + 1;
	return 0;
}
/*
Input:
5
1 2 3 4 5
3
1 3
2 4
1 5

Output:
0
5
1
2
*/
