#include<bits/stdc++.h>
#define maxn 200050
#define endl '\n'
using namespace std;

//-- 小根堆笛卡尔树：单调栈 O(n) 建树，中序遍历 = 原序列，且父节点值 < 子节点值（输入保证值互不相同）--

int n, top;
int a[maxn], lc[maxn], rc[maxn], stk[maxn], istk[maxn];

//单调栈里维护的是当前树的最右链；每个元素最多进栈一次、出栈一次，所以整体 O(n)
void build_min(int n)
{
	top = 0;
	for(int i = 1; i <= n; i ++)
	{
		int last = 0;
		while(top > 0 && a[stk[top]] > a[i]) //值互不相同，所以用 > 不含等号（含等号会变成右侧优先）
		{
			last = stk[top --];
		}
		lc[i] = last; //最后被弹出的节点（它原来的整棵子树都比 a[i] 大）挂成 i 的左孩子
		if(top > 0) rc[stk[top]] = i; //i 比栈顶大，成为栈顶的右孩子
		stk[++ top] = i;
	}
	//建完后栈底 stk[1] 才是根，不是栈顶 stk[top]
}

//-- 中序遍历验证 --
// 用法：int k = inorder(stk[1], ord); //把中序序列的下标按顺序填进 ord[1..k]，k 应当等于 n
//       笛卡尔树是「下标当 BST、值当堆」：中序遍历出的下标必须严格递增，也就是 ord[1..n] = 1,2,...,n
//       注意不是值递增——a[ord[1..n]] 恰好等于原序列 a[1..n]，本来就是乱序的
//       退化成长链时递归会爆栈，所以这里用显式栈迭代
int inorder(int root, int ord[])
{
	int tp = 0, u = root, k = 0;
	while(u || tp > 0)
	{
		while(u) istk[++ tp] = u, u = lc[u]; //一路向左压栈
		u = istk[tp --];
		ord[++ k] = u;
		u = rc[u];
	}
	return k;
}

// 用法：if(check_inorder(stk[1])) ...  //中序下标必须一路递增；出现 u < pre 说明某对左右孩子挂反了
bool check_inorder(int root)
{
	int tp = 0, u = root, pre = 0, cnt = 0;
	while(u || tp > 0)
	{
		while(u) istk[++ tp] = u, u = lc[u];
		u = istk[tp --];
		if(pre && u < pre) return false; //严格递增：出现逆序就说明建树错了
		pre = u;
		cnt ++;
		u = rc[u];
	}
	return cnt == n; //走完必须是 1..n 全部 n 个节点，少一个说明有节点没接上
}

int main()
{
	ios::sync_with_stdio(false);
	cin.tie(0); cout.tie(0);
	
	cin >> n;
	for(int i = 1; i <= n; i ++) cin >> a[i];
	build_min(n);
	for(int i = 1; i <= n; i ++) cout << lc[i] << ' ' << rc[i] << endl; //0 表示没有这个儿子
	return 0;
}
/*
Input:
5
9 3 7 1 8

Output:
0 0
1 3
0 0
2 5
0 0
*/
