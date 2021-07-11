#pragma once
#define CATCH_CONFIG_MAIN
#include <iostream>
#include "catch.hpp"
#include "AVLTree.h"

using namespace fefu;

TEST_CASE("TEST") {
	SECTION("INSERT TEST") {
		AVLTree<int, int> tree({ {1,1}, {2,2}, {3,3}, {4,4}, {5,5}, {6,6} });

		tree.insert(7, 7);
		tree.insert(8, 8);
		tree.insert(9, 9);
		tree.insert({ 10, 10 });

		REQUIRE(tree.size() == 10);
		auto it = tree.begin();
		for (int i = 1; i <= tree.size(); i++) {
			REQUIRE(*tree.find(i) == *it);
			++it;
		}
		++it;
		REQUIRE(*it == *tree.end());
	}
	SECTION("DELETE TEST") {
		AVLTree<int, int> tree({ { 1,1 }, { 2,2 }, { 3,3 }, { 4,4 }, { 5,5 }, { 6,6 }, {7,7}, {8,8}, {9,9}, { 10, 10 } });

		auto it1 = tree.find(3);
		tree.erase(3);
		tree.erase(4);
		tree.erase(5);
		++it1;
		tree.insert(3, 3);

		for (int i = 0; i < 9; ++i) {
			++it1;
		}
		REQUIRE(*it1 == *tree.end());

		for (int i = 1; i < 11; ++i) {
			tree.erase(i);
		}
		REQUIRE(tree.empty());

	}

	SECTION("CONSISTENCY") {
		AVLTree<int, int> tree;
		AVLIterator<int, int> iter;
		tree.insert(std::pair<int, int>(2, 1));
		tree.insert(std::pair<int, int>(4, 3));
		tree.insert(std::pair<int, int>(6, 5));

		iter = tree.begin();
		iter++;

		REQUIRE(*iter == 4);

		tree.erase(3);

		iter++;

		REQUIRE(*iter == 6);
	}

	SECTION("RANDOM CONSISTENCY") {
		int n = 1000;
		srand(time(0));
		AVLTree<int, int> tree;

		for (int i = 0; i < n; ++i) {
			int value = rand() % n;
			tree.insert(value, i);
		}

		std::vector<AVLIterator<int, int>> its;

		for (int i = 0; i < n; ++i) {
			AVLIterator<int, int> iter = tree.begin();
			int m = std::rand() % (n / 2);

			for (int j = 1; j < m; ++j) {
				iter++;
				if (iter == tree.end()) break;
			}

			its.push_back(iter);
		}

		for (int i = 0; i < n; ++i) {
			int value = std::rand() % n;
			tree.erase(value);
		}

		for (auto it : its) {
			while (it != tree.end()) {
				it++;
			}
		}
	}
}
