/*
 * bst.h
 * Student Management System — Binary Search Tree
 *
 * Header file: struct definitions and all public function prototypes.
 */

#ifndef BST_H
#define BST_H

/* ─────────────────────────────────────────────
 * Data Structures
 * ───────────────────────────────────────────── */

/* Holds a single student record. */
typedef struct Student {
    int   id;          /* Unique integer identifier; serves as the BST key  */
    char  name[100];   /* Student's full name, null-terminated               */
    float gpa;         /* Grade Point Average — must be in [0.0, 4.0]        */
} Student;

/* A single node in the Binary Search Tree. */
typedef struct BSTNode {
    Student        data;   /* The payload stored at this node                */
    struct BSTNode *left;  /* Pointer to the left child  (smaller id)        */
    struct BSTNode *right; /* Pointer to the right child (larger id)         */
} BSTNode;

/* Wrapper struct that owns the BST and tracks metadata. */
typedef struct BST {
    BSTNode *root;  /* Pointer to the root node; NULL when tree is empty     */
    int      count; /* Live node count — incremented on insert, decremented  *
                     * on delete so callers never have to traverse the tree  *
                     * just to know how many students exist.                 */
} BST;

/* ─────────────────────────────────────────────
 * Public API — Lifecycle
 * ───────────────────────────────────────────── */

/* Allocates and returns a new, empty BST. Exits on allocation failure. */
BST     *bst_create(void);

/* Frees all nodes (post-order) then frees the BST struct itself. */
void     bst_destroy(BST *tree);

/* ─────────────────────────────────────────────
 * Public API — Core Operations
 * ───────────────────────────────────────────── */

/*
 * Inserts a new student.
 * Returns  0 on success.
 * Returns -1 if id already exists (duplicate).
 * Returns -2 if gpa is outside [0.0, 4.0].
 */
int      bst_insert(BST *tree, int id, const char *name, float gpa);

/* Returns pointer to the node with the matching id, or NULL if not found. */
BSTNode *bst_search_by_id(BST *tree, int id);

/*
 * Full in-order traversal to find a student by name (case-insensitive).
 * Returns pointer to the FIRST match, or NULL if not found.
 */
BSTNode *bst_search_by_name(BST *tree, const char *name);

/*
 * Removes the node whose id matches; handles leaf, one-child, two-child cases.
 * Returns  0 on success.
 * Returns -1 if id not found.
 */
int      bst_delete_by_id(BST *tree, int id);

/*
 * Finds the node by id then presents a sub-menu to update Name, GPA, or ID.
 * Returns  0 on success.
 * Returns -1 if id not found.
 */
int      bst_update(BST *tree, int id);

/* ─────────────────────────────────────────────
 * Public API — Traversal and Display
 * ───────────────────────────────────────────── */

/* In-order traversal: prints all records sorted by id. */
void     bst_display_all(BST *tree);

/* ─────────────────────────────────────────────
 * Public API — Statistics
 * ───────────────────────────────────────────── */

/* Prints tree height and total student count. */
void     bst_tree_stats(BST *tree);

/* Prints min GPA, max GPA, average GPA, count above average, count below average. */
void     bst_gpa_stats(BST *tree);

/* ─────────────────────────────────────────────
 * Public API — Bulk Deletion
 * ───────────────────────────────────────────── */

/*
 * Frees every node without freeing the BST struct itself.
 * Sets root = NULL and count = 0.
 */
void     bst_delete_all(BST *tree);

/* ─────────────────────────────────────────────
 * Public API — Predecessor and Successor
 * ───────────────────────────────────────────── */

/*
 * Finds and prints the in-order predecessor and successor of the given id.
 * Prints "not found" messages when id is absent or has no predecessor/successor.
 */
void     bst_predecessor_successor(BST *tree, int id);

#endif /* BST_H */
