/* program to construct tree using inorder and preorder traversals */
#include<stdio.h>
#include<stdlib.h>
#include<cmath>
#include "binary_tree_pretty_print.hpp"
#include <iostream>
 
/* A binary tree node has data, pointer to left child
   and a pointer to right child */
   
 
/* Prototypes for utility functions */
int search(double arr[], int strt, int end, double value);
struct BinaryTree* newNode(double data);
 
/* Recursive function to construct binary of size len from
   Inorder traversal in[] and Preorder traversal pre[].  Initial values
   of inStrt and inEnd should be 0 and len -1.  The function doesn't
   do any error checking for cases where inorder and preorder
   do not form a tree */
struct BinaryTree* buildTree(double in[], double pre[], int inStrt, int inEnd)
{
  static int preIndex = 0;
 
  if(inStrt > inEnd)
     return NULL;
 
  /* Pick current node from Preorder traversal using preIndex
    and increment preIndex */
  struct BinaryTree *tNode = newNode(pre[preIndex++]);
 
  /* If this node has no children then return */
  if(inStrt == inEnd)
    return tNode;
 
  /* Else find the index of this node in Inorder traversal */
  int inIndex = search(in, inStrt, inEnd, tNode->data);
  //std::cout <<" Index " << inIndex << " Strt "<<inStrt << " end "<< inEnd<< std::endl;
  
  /* Using index in Inorder traversal, construct left and
     right subtress */
  tNode->left = buildTree(in, pre, inStrt, inIndex-1);
  tNode->right = buildTree(in, pre, inIndex+1, inEnd);
 
  return tNode;
}
 
/* UTILITY FUNCTIONS */
/* Function to find index of value in arr[start...end]
   The function assumes that value is present in in[] */
int search(double arr[], int strt, int end, double value)
{
  int i;
  for(i = strt; i <= end; i++)
  {
    if(arr[i] == value)
      return i;
  }
}
 
/* Helper function that allocates a new node with the
   given data and NULL left and right pointers. */
struct BinaryTree* newNode(double data)
{
  struct BinaryTree* node = new BinaryTree();
 // node->data = pow(2,data)*1000;
  
  node->data = data;
  node->left = NULL;
  node->right = NULL;
 
  return(node);
}
 
/* This funtcion is here just to test buildTree() */
void printInorder(struct BinaryTree* node)
{
  if (node == NULL)
     return;
 
  /* first recur on left child */
  printInorder(node->left);
 
  /* then print the data of node */
  std::cout << node->data;
 
  /* now recur on right child */
  printInorder(node->right);
}
 
/* Driver program to test above functions */
int printTreeStructure(double *in, double *pre, int len)
{
  std::cout << len << std::endl;
  struct BinaryTree *root = buildTree(in, pre, 0, len - 1);
 
  /* Let us test the built tree by printing Insorder traversal */
  //printf("\n Inorder traversal of the constructed tree is \n");
  printInorder(root);
  std::cout << "--------------------------------------" << std::endl;
  createTree(root);
  //getchar();
}
