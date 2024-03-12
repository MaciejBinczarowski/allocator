#include <stdio.h>
#include <stdlib.h>
#include <allocator.h>

typedef struct Node 
{
    int key;
    struct Node* left;
    struct Node* right;
} Node;

Node* createNode(int key) 
{
    Node* newNode = (Node*)alloc(sizeof(Node));
    newNode->key = key;
    newNode->left = NULL;
    newNode->right = NULL;
    return newNode;
}

Node* insert(Node* root, int key) 
{
    if (root == NULL) 
    {
        return createNode(key);
    }

    if (key < root->key) 
    {
        root->left = insert(root->left, key);
    } else if (key > root->key) 
    {
        root->right = insert(root->right, key);
    }

    return root;
}

void printInorder(Node* root) 
{
    if (root != NULL) {
        printInorder(root->left);
        printf("%d ", root->key);
        printInorder(root->right);
    }
}

Node* minValueNode(Node* node) 
{
    Node* current = node;
    while (current && current->left != NULL) 
    {
        current = current->left;
    }
    return current;
}

Node* deleteNode(Node* root, int key) 
{
    if (root == NULL) 
    {
        return root;
    }

    if (key < root->key) 
    {
        root->left = deleteNode(root->left, key);
    } 
    else if (key > root->key) 
    {
        root->right = deleteNode(root->right, key);
    } 
    else 
    {
        if (root->left == NULL) 
        {
            Node* temp = root->right;
            dealloc(root);
            return temp;
        } 
        else if (root->right == NULL) 
        {
            Node* temp = root->left;
            dealloc(root);
            return temp;
        }

        Node* temp = minValueNode(root->right);
        root->key = temp->key;
        root->right = deleteNode(root->right, temp->key);
    }

    return root;
}

Node* search(Node* root, int key) 
{
    if (root == NULL || root->key == key) 
    {
        return root;
    }

    if (key < root->key) 
    {
        return search(root->left, key);
    }

    return search(root->right, key);
}

int main() 
{
    Node* root = NULL;

    root = insert(root, 50);
    root = insert(root, 30);
    root = insert(root, 20);
    root = insert(root, 40);
    root = insert(root, 70);
    root = insert(root, 60);
    root = insert(root, 80);

    printf("Inorder print: ");
    printInorder(root);
    printf("\n");

    root = deleteNode(root, 20);
    root = deleteNode(root, 50);
    root = deleteNode(root, 30);

    printf("Inorder traversal after deletion: ");
    printInorder(root);
    printf("\n");

    Node* searchResult = search(root, 60);
    if (searchResult != NULL) 
    {
        printf("Key 60 found in the tree\n");
    } 
    else 
    {
        printf("Key 60 not found in the tree\n");
    }

    return 0;
}
