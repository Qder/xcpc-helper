#include<bits/stdc++.h>
#define inf 0x3f3f3f3f
#define maxn 25
#define maxm 200
#define endl '\n'
#define ll long long
using namespace std;

//-- windy 数：统计 [l, r] 里的 windy 数个数（正整数，且相邻两位数字之差的绝对值 >= 2）--

int dig_len;
int digit[maxn]; //digit[0] 是最高位
ll dp[maxn][11]; //dp[pos][pre]：前 pos 位已定、上一位是 pre、后面随便填的方案数（lead == false 时 pre 才是真实数字）

// dfs(pos, pre, limit, lead)：数位 dp 记忆化搜索骨架，四个参数的含义
// pos   ：当前正在填第几位，0 ~ dig_len-1，digit[0] 是最高位
// pre   ：上一位填的数字，10 表示还没有真实的上一位（配合 lead 使用）
// limit ：是否顶上限，为真时本位只能填 0 ~ digit[pos]，为假时本位 0 ~ 9 随便填
// lead  ：是否还在前导零，为真表示前面一位非零数都没填过
//
// 为什么要单独记前导零：前导零不是数位上的 0，它不参与“相邻两位之差 >= 2”的判断，
// 不区分的话 013 会被当成 0 和 1 相邻而误判；而且全程前导零就是数字 0 本身，windy 数不含它。
//
// 返回时能不能 memorized 的两个条件（必须同时满足）：
// ① 不顶上限（limit == false）：此时后面每位填什么都与上界无关，同一个 (pos, pre) 对任何数答案都一样；
//    顶上限时本位能填的范围由 digit[] 决定，每个数都不一样，缓存进去会串味。
// ② 不在前导零（lead == false）：前导零状态下的“上一位”是虚的，和真正的 pre 不是同一个状态，混着存会算错。
ll dfs(int pos, int pre, bool limit, bool lead)
{
	if(pos == dig_len) return !lead; //填完了：还在前导零说明整个数就是 0，不算 windy 数
	if(!limit && !lead && dp[pos][pre] != -1) return dp[pos][pre]; //两个记忆化条件都满足才能直接返回
	int up = limit ? digit[pos] : 9;
	ll res = 0;
	for(int d = 0; d <= up; d ++)
	{
		bool nxtLimit = limit && d == up; //这一位也顶到上界时，下一位才继续受限
		if(lead && d == 0) res += dfs(pos + 1, 10, nxtLimit, true); //本位仍是前导零，pre 保持 10
		else if(lead || abs(pre - d) >= 2) res += dfs(pos + 1, d, nxtLimit, false); //首个非零位可任填，之后必须与上一位差 >= 2
	}
	if(!limit && !lead) dp[pos][pre] = res; //同样两个条件都满足才存进记忆数组
	return res;
}

// 用法：cout << windy(l, r);  // 答案 = solve(r) - solve(l - 1)
ll solve(ll n)
{
	if(n <= 0) return 0; //0 和负数都不是 windy 数
	string s = to_string(n);
	dig_len = s.size();
	for(int i = 0; i < dig_len; i ++) digit[i] = s[i] - '0';
	memset(dp, -1, sizeof dp); //每次换了上界 digit[] 都要清空，否则上一组询问的记忆值会污染这一组
	return dfs(0, 10, true, true);
}

ll windy(ll l, ll r)
{
	if(l > r) return 0;
	return solve(r) - solve(l - 1);
}

//-- 另一个套路：数位和。前导零不影响数位和，所以不用 lead，只留 limit --
// 用法：cout << digit_sum(r, s) - digit_sum(l - 1, s);  // [l, r] 中数位和恰好为 s 的数的个数（s == 0 时含 0）

int dsum_len, dsum_target;
int dsum_digit[maxn];
ll dsum_dp[maxn][maxm]; //ll 最多 19 位，最大数位和 9 * 19 = 171

ll dsum_dfs(int pos, int sum, bool limit)
{
	if(pos == dsum_len) return sum == dsum_target;
	if(!limit && dsum_dp[pos][sum] != -1) return dsum_dp[pos][sum];
	int up = limit ? dsum_digit[pos] : 9;
	ll res = 0;
	for(int d = 0; d <= up && sum + d <= dsum_target; d ++) //sum + d > target 的位直接剪掉
		res += dsum_dfs(pos + 1, sum + d, limit && d == up);
	if(!limit) dsum_dp[pos][sum] = res;
	return res;
}

ll digit_sum(ll n, int s)
{
	if(n < 0) return 0;
	dsum_target = s;
	string t = to_string(n);
	dsum_len = t.size();
	for(int i = 0; i < dsum_len; i ++) dsum_digit[i] = t[i] - '0';
	memset(dsum_dp, -1, sizeof dsum_dp);
	return dsum_dfs(0, 0, true);
}

int main()
{
	ios::sync_with_stdio(false);
	cin.tie(0); cout.tie(0);
	
	ll l, r;
	cin >> l >> r;
	cout << windy(l, r);
	return 0;
}
/*
Input:
1 100

Output:
73
*/
