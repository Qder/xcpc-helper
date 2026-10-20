#include<bits/stdc++.h>
#define maxn 1000005
#define endl '\n'
using namespace std;

int z[maxn];

// 用法：z_function(s, n) 之后 z[0] = n，z[i] = LCP(s, s[i..])，i in [0, n)。
// 复杂度 O(n)；n <= 1e6 用这个静态数组，再大就换成 vector<int>。
void z_function(const string &s, int n)
{
	z[0] = n;
	for(int i = 1, l = 0, r = 0; i < n; i ++)
	{
		if(i <= r) z[i] = min(r - i + 1, z[i - l]); //落在已匹配段里，先继承一部分
		while(i + z[i] < n && s[z[i]] == s[i + z[i]]) z[i] ++; //再暴力往外扩
		if(i + z[i] - 1 > r) { l = i; r = i + z[i] - 1; } //更新最右匹配段 [l, r]
	}
}

int main()
{
	ios::sync_with_stdio(false);
	cin.tie(0); cout.tie(0);
	
	string s;
	cin >> s;
	int n = (int)s.size();
	z_function(s, n);
	for(int i = 0; i < n; i ++) cout << z[i] << " \n"[i == n - 1];
	return 0;
}
/*
Input:
ababab

Output:
6 0 4 0 2 0
*/
