#include <bits/stdc++.h>

using namespace std;

int msbPos(int n) {
	int pos = 0;
	for (int i = 30; i >= 0; --i) {
		if ((n >> i) & 1) {
			pos = i;
			break;
		}
	}

	return pos;
}

int main() {
	ios_base::sync_with_stdio(false);
	cin.tie(NULL);

	int n, q, cur;
	cin >> n >> q;
	vector<vector<int>> counts(n + 1, vector<int>(30, 0));

	for (int i = 0; i < n; i++) {
		cin >> cur;

		for (int k = 0; k < 30; k++) {
			counts[i + 1][k] = counts[i][k];
		}

		counts[i + 1][msbPos(cur)]++;
	}

	int l, r;
	for (int i = 0; i < q; i++) {
		cin >> l >> r;
		int max = 0;

		for (int k = 0; k < 30; k++) {
			int count = counts[r][k] - counts[l - 1][k];
			if (count > max) {
				max = count;
			}
		}

		cout << max << "\n";
	}

	return 0;
}
