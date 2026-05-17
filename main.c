#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

typedef struct Student {
    int   id;
    char  name[100];
    float gpa;
} Student;

typedef struct BSTNode {
    Student         data;
    int             height;
    struct BSTNode *left;
    struct BSTNode *right;
} BSTNode;

typedef struct BST {
    BSTNode *root;
    int      count;
} BST;

int      getHeight(BSTNode *node);
int      maxOfTwo(int firstValue, int secondValue);
int      getBalanceFactor(BSTNode *node);
void     updateHeight(BSTNode *node);
BSTNode *rotateRight(BSTNode *pivotNode);
BSTNode *rotateLeft(BSTNode *pivotNode);
BSTNode *balanceNode(BSTNode *node);

BSTNode *createNode(int id, const char *name, float gpa);
BSTNode *insertRecursive(BSTNode *node, int id, const char *name,
                                float gpa, int *outResult);
void     destroyRecursive(BSTNode *node);

BSTNode *searchByIdRecursive(BSTNode *node, int targetId);
int      strCaseEqual(const char *stringA, const char *stringB);
BSTNode *searchByNameInorder(BSTNode *node, const char *targetName);

BSTNode *findMinNode(BSTNode *subtreeRoot);
BSTNode *deleteRecursive(BSTNode *node, int id, int *outResult);

void     inorderPrint(BSTNode *node);

void     collectGpaStats(BSTNode *node, float *minGpa, float *maxGpa,
                                float *runningSum, int *visitedCount);
void     countAboveBelow(BSTNode *node, float averageGpa,
                                int *aboveCount, int *belowCount);
void     freeAllNodes(BSTNode *node);

void     printMenu(void);

int getHeight(BSTNode *node)
{
    if (!node) return 0;
    return node->height;
}

int maxOfTwo(int firstValue, int secondValue)
{
    return (firstValue > secondValue) ? firstValue : secondValue;
}

int getBalanceFactor(BSTNode *node)
{
    if (!node) return 0;
    return getHeight(node->left) - getHeight(node->right);
}

void updateHeight(BSTNode *node)
{
    node->height = 1 + maxOfTwo(getHeight(node->left), getHeight(node->right));
}

BSTNode *rotateRight(BSTNode *pivotNode)
{
    BSTNode *newRoot      = pivotNode->left;
    BSTNode *movedSubtree = newRoot->right;

    newRoot->right  = pivotNode;
    pivotNode->left = movedSubtree;

    updateHeight(pivotNode);
    updateHeight(newRoot);

    return newRoot;
}

BSTNode *rotateLeft(BSTNode *pivotNode)
{
    BSTNode *newRoot      = pivotNode->right;
    BSTNode *movedSubtree = newRoot->left;

    newRoot->left    = pivotNode;
    pivotNode->right = movedSubtree;

    updateHeight(pivotNode);
    updateHeight(newRoot);

    return newRoot;
}

BSTNode *balanceNode(BSTNode *node)
{
    updateHeight(node);

    int balanceFactor = getBalanceFactor(node);

    if (balanceFactor > 1) {
        if (getBalanceFactor(node->left) < 0) {
            node->left = rotateLeft(node->left);
        }

        return rotateRight(node);
    }

    if (balanceFactor < -1) {
        if (getBalanceFactor(node->right) > 0) {
            node->right = rotateRight(node->right);
        }

        return rotateLeft(node);
    }

    return node;
}

BSTNode *createNode(int id, const char *name, float gpa)
{
    BSTNode *newNode = (BSTNode *)malloc(sizeof(BSTNode));
    if (!newNode) {
        fprintf(stderr, "Fatal: failed to allocate BSTNode.\n");
        exit(EXIT_FAILURE);
    }

    newNode->data.id  = id;
    newNode->data.gpa = gpa;

    strncpy(newNode->data.name, name, sizeof(newNode->data.name) - 1);
    newNode->data.name[sizeof(newNode->data.name) - 1] = '\0';

    newNode->height = 1;
    newNode->left   = NULL;
    newNode->right  = NULL;

    return newNode;
}

BSTNode *insertRecursive(BSTNode *node, int id, const char *name,
                                float gpa, int *outResult)
{
    if (!node) {
        *outResult = 0;
        return createNode(id, name, gpa);
    }

    if (id < node->data.id) {
        node->left = insertRecursive(node->left, id, name, gpa, outResult);
    } else if (id > node->data.id) {
        node->right = insertRecursive(node->right, id, name, gpa, outResult);
    } else {
        *outResult = -1;
        return node;
    }

    return balanceNode(node);
}

void destroyRecursive(BSTNode *node)
{
    if (!node) return;
    destroyRecursive(node->left);
    destroyRecursive(node->right);
    free(node);
}

BSTNode *searchByIdRecursive(BSTNode *node, int targetId)
{
    if (!node)                           return NULL;
    if (targetId == node->data.id)       return node;
    if (targetId  < node->data.id)
        return searchByIdRecursive(node->left,  targetId);
    return     searchByIdRecursive(node->right, targetId);
}

int strCaseEqual(const char *stringA, const char *stringB)
{
    while (*stringA && *stringB) {
        if (tolower((unsigned char)*stringA) != tolower((unsigned char)*stringB))
            return 0;
        stringA++;
        stringB++;
    }

    return tolower((unsigned char)*stringA) == tolower((unsigned char)*stringB);
}

BSTNode *searchByNameInorder(BSTNode *node, const char *targetName)
{
    if (!node) return NULL;

    BSTNode *leftResult = searchByNameInorder(node->left, targetName);
    if (leftResult) return leftResult;

    if (strCaseEqual(node->data.name, targetName)) return node;

    return searchByNameInorder(node->right, targetName);
}

BSTNode *findMinNode(BSTNode *subtreeRoot)
{
    while (subtreeRoot->left) subtreeRoot = subtreeRoot->left;
    return subtreeRoot;
}

BSTNode *deleteRecursive(BSTNode *node, int id, int *outResult)
{
    if (!node) {
        *outResult = -1;
        return NULL;
    }

    if (id < node->data.id) {
        node->left = deleteRecursive(node->left, id, outResult);
    } else if (id > node->data.id) {
        node->right = deleteRecursive(node->right, id, outResult);
    } else {
        *outResult = 0;

        if (!node->left && !node->right) {
            free(node);
            return NULL;

        } else if (!node->left) {
            BSTNode *rightChild = node->right;
            free(node);
            return rightChild;

        } else if (!node->right) {
            BSTNode *leftChild = node->left;
            free(node);
            return leftChild;

        } else {
            BSTNode *inorderSuccessor = findMinNode(node->right);

            int successorId = inorderSuccessor->data.id;

            node->data = inorderSuccessor->data;

            int unusedResult = 0;
            node->right = deleteRecursive(node->right, successorId, &unusedResult);
        }
    }

    return balanceNode(node);
}

void inorderPrint(BSTNode *node)
{
    if (!node) return;
    inorderPrint(node->left);
    printf("ID: %-6d  |  Name: %-30s  |  GPA: %.2f\n",
           node->data.id, node->data.name, node->data.gpa);
    inorderPrint(node->right);
}

void collectGpaStats(BSTNode *node, float *minGpa, float *maxGpa,
                            float *runningSum, int *visitedCount)
{
    if (!node) return;

    collectGpaStats(node->left, minGpa, maxGpa, runningSum, visitedCount);

    float currentGpa = node->data.gpa;
    if (currentGpa < *minGpa) *minGpa = currentGpa;
    if (currentGpa > *maxGpa) *maxGpa = currentGpa;
    *runningSum += currentGpa;
    (*visitedCount)++;

    collectGpaStats(node->right, minGpa, maxGpa, runningSum, visitedCount);
}

void countAboveBelow(BSTNode *node, float averageGpa,
                            int *aboveCount, int *belowCount)
{
    if (!node) return;
    countAboveBelow(node->left, averageGpa, aboveCount, belowCount);
    if      (node->data.gpa > averageGpa) (*aboveCount)++;
    else if (node->data.gpa < averageGpa) (*belowCount)++;
    countAboveBelow(node->right, averageGpa, aboveCount, belowCount);
}

void freeAllNodes(BSTNode *node)
{
    if (!node) return;
    freeAllNodes(node->left);
    freeAllNodes(node->right);
    free(node);
}

BST *bst_create(void)
{
    BST *tree = (BST *)malloc(sizeof(BST));
    if (!tree) {
        fprintf(stderr, "Fatal: failed to allocate BST struct.\n");
        exit(EXIT_FAILURE);
    }
    tree->root  = NULL;
    tree->count = 0;
    return tree;
}

void bst_destroy(BST *tree)
{
    if (!tree) return;
    destroyRecursive(tree->root);
    tree->root  = NULL;
    tree->count = 0;
    free(tree);
}

int bst_insert(BST *tree, int id, const char *name, float gpa)
{
    if (!tree) return -1;

    if (gpa < 0.0f || gpa > 4.0f) {
        fprintf(stderr,
                "Error: GPA %.2f is outside the valid range [0.0, 4.0].\n",
                gpa);
        return -2;
    }

    int insertionResult = 0;
    tree->root = insertRecursive(tree->root, id, name, gpa, &insertionResult);

    if (insertionResult == -1) {
        fprintf(stderr, "Error: Student with ID %d already exists.\n", id);
        return -1;
    }

    tree->count++;
    return 0;
}

BSTNode *bst_search_by_id(BST *tree, int id)
{
    if (!tree) return NULL;
    return searchByIdRecursive(tree->root, id);
}

BSTNode *bst_search_by_name(BST *tree, const char *name)
{
    if (!tree || !name) return NULL;
    return searchByNameInorder(tree->root, name);
}

int bst_delete_by_id(BST *tree, int id)
{
    if (!tree) return -1;

    int deletionResult = 0;
    tree->root = deleteRecursive(tree->root, id, &deletionResult);

    if (deletionResult == -1) {
        printf("Error: Student with ID %d not found.\n", id);
        return -1;
    }

    tree->count--;
    return 0;
}

int bst_update(BST *tree, int id)
{
    if (!tree) return -1;

    BSTNode *targetNode = bst_search_by_id(tree, id);
    if (!targetNode) {
        printf("Error: Student with ID %d not found.\n", id);
        return -1;
    }

    char inputBuffer[256];
    int  subMenuChoice = 0;

    do {
        printf("\n--- Update Student (ID: %d, Name: %s) ---\n",
               targetNode->data.id, targetNode->data.name);
        printf(" [1] Update Name\n");
        printf(" [2] Update GPA\n");
        printf(" [3] Update ID\n");
        printf(" [4] Done\n");
        printf("Enter choice: ");

        if (!fgets(inputBuffer, sizeof(inputBuffer), stdin)) {
            subMenuChoice = 4;
            break;
        }

        if (sscanf(inputBuffer, "%d", &subMenuChoice) != 1) {
            printf("Invalid input. Please enter a number between 1 and 4.\n");
            subMenuChoice = 0;
            continue;
        }

        switch (subMenuChoice) {
            case 1: {
                printf("Enter new name: ");
                if (!fgets(inputBuffer, sizeof(inputBuffer), stdin)) break;
                inputBuffer[strcspn(inputBuffer, "\n")] = '\0';

                if (strlen(inputBuffer) == 0) {
                    printf("Error: Name cannot be empty.\n");
                } else {
                    strncpy(targetNode->data.name, inputBuffer,
                            sizeof(targetNode->data.name) - 1);
                    targetNode->data.name[sizeof(targetNode->data.name) - 1] = '\0';
                    printf("Name updated successfully.\n");
                }
                break;
            }

            case 2: {
                printf("Enter new GPA (0.0 - 4.0): ");
                if (!fgets(inputBuffer, sizeof(inputBuffer), stdin)) break;
                float newGpa = 0.0f;
                if (sscanf(inputBuffer, "%f", &newGpa) != 1) {
                    printf("Invalid GPA. Please enter a decimal number.\n");
                } else if (newGpa < 0.0f || newGpa > 4.0f) {
                    printf("Error: GPA must be in the range [0.0, 4.0].\n");
                } else {
                    targetNode->data.gpa = newGpa;
                    printf("GPA updated to %.2f successfully.\n", newGpa);
                }
                break;
            }

            case 3: {
                printf("Enter new ID: ");
                if (!fgets(inputBuffer, sizeof(inputBuffer), stdin)) break;
                int newId = 0;
                if (sscanf(inputBuffer, "%d", &newId) != 1) {
                    printf("Invalid ID. Please enter an integer.\n");
                    break;
                }
                if (newId == id) {
                    printf("New ID is identical to the current ID. No change made.\n");
                    break;
                }

                char  savedName[100];
                float savedGpa = targetNode->data.gpa;
                strncpy(savedName, targetNode->data.name, sizeof(savedName) - 1);
                savedName[sizeof(savedName) - 1] = '\0';

                bst_delete_by_id(tree, id);
                targetNode = NULL;

                int insertResult = bst_insert(tree, newId, savedName, savedGpa);
                if (insertResult != 0) {
                    printf("Restoring original record with ID %d.\n", id);
                    bst_insert(tree, id, savedName, savedGpa);
                    targetNode = bst_search_by_id(tree, id);
                } else {
                    printf("ID updated from %d to %d successfully.\n", id, newId);
                    id         = newId;
                    targetNode = bst_search_by_id(tree, newId);
                }
                break;
            }

            case 4:
                printf("Update session complete.\n");
                break;

            default:
                printf("Invalid choice. Please enter 1, 2, 3, or 4.\n");
                break;
        }

    } while (subMenuChoice != 4);

    return 0;
}

void bst_display_all(BST *tree)
{
    if (!tree || !tree->root) {
        printf("No students found.\n");
        return;
    }
    printf("\n%-8s  %-30s  %s\n", "ID", "Name", "GPA");
    printf("%-8s  %-30s  %s\n",
           "--------", "------------------------------", "----");
    inorderPrint(tree->root);
}

void bst_tree_stats(BST *tree)
{
    if (!tree) return;

    int treeHeight = getHeight(tree->root);
    printf("\nTree Height    : %d\n", treeHeight);
    printf("Total Students : %d\n",  tree->count);
}

void bst_gpa_stats(BST *tree)
{
    if (!tree || !tree->root) {
        printf("No students found.\n");
        return;
    }

    float minGpa       = 4.0f;
    float maxGpa       = 0.0f;
    float runningSum   = 0.0f;
    int   studentCount = 0;

    collectGpaStats(tree->root, &minGpa, &maxGpa, &runningSum, &studentCount);

    float averageGpa = (studentCount > 0)
                       ? (runningSum / (float)studentCount)
                       : 0.0f;

    int aboveCount = 0;
    int belowCount = 0;
    countAboveBelow(tree->root, averageGpa, &aboveCount, &belowCount);

    printf("\n--- GPA Statistics ---\n");
    printf("Minimum GPA        : %.2f\n", minGpa);
    printf("Maximum GPA        : %.2f\n", maxGpa);
    printf("Average GPA        : %.2f\n", averageGpa);
    printf("Students above avg : %d\n",   aboveCount);
    printf("Students below avg : %d\n",   belowCount);
}

void bst_delete_all(BST *tree)
{
    if (!tree) return;
    freeAllNodes(tree->root);
    tree->root  = NULL;
    tree->count = 0;
    printf("All student records have been deleted.\n");
}

void bst_predecessor_successor(BST *tree, int id)
{
    if (!tree) return;

    if (!bst_search_by_id(tree, id)) {
        printf("Student ID %d not found.\n", id);
        return;
    }

    BSTNode *predecessor = NULL;
    BSTNode *successor   = NULL;
    BSTNode *currentNode = NULL;

    currentNode = tree->root;
    while (currentNode) {
        if (currentNode->data.id < id) {
            predecessor = currentNode;
            currentNode = currentNode->right;
        } else {
            currentNode = currentNode->left;
        }
    }

    currentNode = tree->root;
    while (currentNode) {
        if (currentNode->data.id > id) {
            successor   = currentNode;
            currentNode = currentNode->left;
        } else {
            currentNode = currentNode->right;
        }
    }

    if (predecessor) {
        printf("Predecessor: ID=%d, Name=%s, GPA=%.2f\n",
               predecessor->data.id,
               predecessor->data.name,
               predecessor->data.gpa);
    } else {
        printf("%d is the first record — no predecessor.\n", id);
    }

    if (successor) {
        printf("Successor  : ID=%d, Name=%s, GPA=%.2f\n",
               successor->data.id,
               successor->data.name,
               successor->data.gpa);
    } else {
        printf("%d is the last record — no successor.\n", id);
    }
}

void printMenu(void)
{
    printf("\n");
    printf("══════════════════════════════════════\n");
    printf("     Student Management System\n");
    printf("══════════════════════════════════════\n");
    printf(" [1]  Insert Student\n");
    printf(" [2]  Search by ID\n");
    printf(" [3]  Search by Name\n");
    printf(" [4]  Delete by ID\n");
    printf(" [5]  Update Student Data\n");
    printf(" [6]  Display All Students\n");
    printf(" [7]  Display Tree Statistics\n");
    printf(" [8]  Display GPA Statistics\n");
    printf(" [9]  Delete All Students\n");
    printf(" [10] Find Predecessor & Successor\n");
    printf(" [0]  Exit\n");
    printf("══════════════════════════════════════\n");
    printf("Enter your choice: ");
}

int main(void)
{
    BST  *tree       = bst_create();
    char  inputBuffer[256];
    int   menuChoice = -1;

    do {
        printMenu();

        if (!fgets(inputBuffer, sizeof(inputBuffer), stdin)) {
            menuChoice = 0;
            break;
        }

        if (sscanf(inputBuffer, "%d", &menuChoice) != 1) {
            printf("Invalid choice. Please try again.\n");
            menuChoice = -1;
            continue;
        }

        switch (menuChoice) {
            case 1: {
                int   studentId  = 0;
                float studentGpa = 0.0f;
                char  studentName[100];

                printf("Enter Student ID  : ");
                if (!fgets(inputBuffer, sizeof(inputBuffer), stdin)) break;
                if (sscanf(inputBuffer, "%d", &studentId) != 1) {
                    printf("Invalid ID. Please enter an integer.\n");
                    break;
                }

                printf("Enter Student Name: ");
                if (!fgets(studentName, sizeof(studentName), stdin)) break;
                studentName[strcspn(studentName, "\n")] = '\0';
                if (strlen(studentName) == 0) {
                    printf("Error: Name cannot be empty.\n");
                    break;
                }

                printf("Enter GPA (0.0 - 4.0): ");
                if (!fgets(inputBuffer, sizeof(inputBuffer), stdin)) break;
                if (sscanf(inputBuffer, "%f", &studentGpa) != 1) {
                    printf("Invalid GPA. Please enter a decimal number.\n");
                    break;
                }

                if (bst_insert(tree, studentId, studentName, studentGpa) == 0) {
                    printf("Student inserted successfully.\n");
                }
                break;
            }

            case 2: {
                int studentId = 0;
                printf("Enter Student ID to search: ");
                if (!fgets(inputBuffer, sizeof(inputBuffer), stdin)) break;
                if (sscanf(inputBuffer, "%d", &studentId) != 1) {
                    printf("Invalid ID. Please enter an integer.\n");
                    break;
                }
                BSTNode *foundNode = bst_search_by_id(tree, studentId);
                if (foundNode) {
                    printf("Found: ID=%d, Name=%s, GPA=%.2f\n",
                           foundNode->data.id,
                           foundNode->data.name,
                           foundNode->data.gpa);
                } else {
                    printf("Student with ID %d not found.\n", studentId);
                }
                break;
            }

            case 3: {
                char studentName[100];
                printf("Enter Student Name to search: ");
                if (!fgets(studentName, sizeof(studentName), stdin)) break;
                studentName[strcspn(studentName, "\n")] = '\0';
                if (strlen(studentName) == 0) {
                    printf("Error: Name cannot be empty.\n");
                    break;
                }
                BSTNode *foundNode = bst_search_by_name(tree, studentName);
                if (foundNode) {
                    printf("Found: ID=%d, Name=%s, GPA=%.2f\n",
                           foundNode->data.id,
                           foundNode->data.name,
                           foundNode->data.gpa);
                } else {
                    printf("Student named \"%s\" not found.\n", studentName);
                }
                break;
            }

            case 4: {
                int studentId = 0;
                printf("Enter Student ID to delete: ");
                if (!fgets(inputBuffer, sizeof(inputBuffer), stdin)) break;
                if (sscanf(inputBuffer, "%d", &studentId) != 1) {
                    printf("Invalid ID. Please enter an integer.\n");
                    break;
                }
                if (bst_delete_by_id(tree, studentId) == 0) {
                    printf("Student with ID %d deleted successfully.\n", studentId);
                }
                break;
            }

            case 5: {
                int studentId = 0;
                printf("Enter Student ID to update: ");
                if (!fgets(inputBuffer, sizeof(inputBuffer), stdin)) break;
                if (sscanf(inputBuffer, "%d", &studentId) != 1) {
                    printf("Invalid ID. Please enter an integer.\n");
                    break;
                }
                bst_update(tree, studentId);
                break;
            }

            case 6:
                bst_display_all(tree);
                break;

            case 7:
                bst_tree_stats(tree);
                break;

            case 8:
                bst_gpa_stats(tree);
                break;

            case 9:
                bst_delete_all(tree);
                break;

            case 10: {
                int studentId = 0;
                printf("Enter Student ID: ");
                if (!fgets(inputBuffer, sizeof(inputBuffer), stdin)) break;
                if (sscanf(inputBuffer, "%d", &studentId) != 1) {
                    printf("Invalid ID. Please enter an integer.\n");
                    break;
                }
                bst_predecessor_successor(tree, studentId);
                break;
            }

            case 0:
                printf("Exiting... Goodbye!\n");
                break;

            default:
                printf("Invalid choice. Please try again.\n");
                break;
        }

    } while (menuChoice != 0);

    bst_destroy(tree);
    return 0;
}
