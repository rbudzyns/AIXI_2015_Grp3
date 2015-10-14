#ifndef __BUILD_TREE_HPP__
#define __BUILD_TREE_HPP__

struct BinaryTree {
  BinaryTree *left, *right;
  double data;
  //BinaryTree(double val) : left(NULL), right(NULL), data(val) { }
};


int createTree(struct BinaryTree* root);

#endif
