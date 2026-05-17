/*
 * main.c
 * Student Management System — AVL Tree, single-file implementation.
 *
 * Compile: gcc -Wall -Wextra -std=c11 -o sms main.c
 * Run    : ./sms
 *
 * This file contains every struct definition, every AVL helper, every public
 * API function, and main() in a single compilation unit.  The underlying data
 * structure is a self-balancing AVL tree keyed on student ID, which guarantees
 * O(log n) worst-case performance for insert, search, and delete regardless of
 * the order in which students are added.
 *
 * File layout
 * ───────────
 *  Section  1 — Data Structures
 *  Section  2 — Forward Declarations (all static helpers)
 *  Section  3 — AVL Utility Helpers  (getHeight, maxOfTwo, getBalanceFactor,
 *                                     updateHeight, rotateRight, rotateLeft,
 *                                     balanceNode)
 *  Section  4 — Node Creation        (createNode)
 *  Section  5 — AVL Insert Helper    (insertRecursive)
 *  Section  6 — Destroy Helper       (destroyRecursive)
 *  Section  7 — Search Helpers       (searchByIdRecursive, strCaseEqual,
 *                                     searchByNameInorder)
 *  Section  8 — Delete Helpers       (findMinNode, deleteRecursive)
 *  Section  9 — Display Helper       (inorderPrint)
 *  Section 10 — Statistics Helpers   (collectGpaStats, countAboveBelow,
 *                                     freeAllNodes)
 *  Section 11 — Public API: Lifecycle
 *  Section 12 — Public API: Core Operations
 *  Section 13 — Public API: Traversal and Display
 *  Section 14 — Public API: Statistics
 *  Section 15 — Public API: Bulk Deletion
 *  Section 16 — Public API: Predecessor and Successor
 *  Section 17 — UI Helper            (printMenu)
 *  Section 18 — main()
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/* ════════════════════════════════════════════════════════════════════════════
 * SECTION 1 — DATA STRUCTURES
 * ════════════════════════════════════════════════════════════════════════════ */

/*
 * Student — Holds one student's complete record.
 *   id   : unique integer; serves as the AVL tree key for all comparisons.
 *   name : full name stored inline in a fixed buffer (no extra heap allocation).
 *   gpa  : grade point average, enforced in [0.0, 4.0] at every input boundary.
 */
typedef struct Student {
    int   id;
    char  name[100];
    float gpa;
} Student;

/*
 * BSTNode — One node in the AVL tree.
 *   data   : the Student record stored at this node.
 *   height : AVL height of this node's subtree.
 *            By convention: a leaf node has height 1, a NULL pointer has height 0.
 *            This field is automatically maintained by insertRecursive,
 *            deleteRecursive, and the rotation functions so it is always current.
 *   left   : pointer to the left child  (its id is smaller than this node's id).
 *   right  : pointer to the right child (its id is larger  than this node's id).
 */
typedef struct BSTNode {
    Student         data;
    int             height;
    struct BSTNode *left;
    struct BSTNode *right;
} BSTNode;

/*
 * BST — Wrapper struct that owns the entire AVL tree.
 *   root  : pointer to the tree's root node; NULL when the tree is empty.
 *   count : live node count; incremented on every successful insert and
 *           decremented on every successful delete so callers never need a
 *           full traversal just to count students.
 */
typedef struct BST {
    BSTNode *root;
    int      count;
} BST;

/* ════════════════════════════════════════════════════════════════════════════
 * SECTION 2 — FORWARD DECLARATIONS (all static internal helpers)
 *
 * Declared here so that they can be defined in any order below without
 * requiring the compiler to have seen each function before it is called.
 * ════════════════════════════════════════════════════════════════════════════ */

/* AVL utility helpers */
static int      getHeight(BSTNode *node);
static int      maxOfTwo(int firstValue, int secondValue);
static int      getBalanceFactor(BSTNode *node);
static void     updateHeight(BSTNode *node);
static BSTNode *rotateRight(BSTNode *pivotNode);
static BSTNode *rotateLeft(BSTNode *pivotNode);
static BSTNode *balanceNode(BSTNode *node);

/* Node lifecycle */
static BSTNode *createNode(int id, const char *name, float gpa);
static BSTNode *insertRecursive(BSTNode *node, int id, const char *name,
                                float gpa, int *outResult);
static void     destroyRecursive(BSTNode *node);

/* Search helpers */
static BSTNode *searchByIdRecursive(BSTNode *node, int targetId);
static int      strCaseEqual(const char *stringA, const char *stringB);
static BSTNode *searchByNameInorder(BSTNode *node, const char *targetName);

/* Delete helpers */
static BSTNode *findMinNode(BSTNode *subtreeRoot);
static BSTNode *deleteRecursive(BSTNode *node, int id, int *outResult);

/* Display helper */
static void     inorderPrint(BSTNode *node);

/* Statistics helpers */
static void     collectGpaStats(BSTNode *node, float *minGpa, float *maxGpa,
                                float *runningSum, int *visitedCount);
static void     countAboveBelow(BSTNode *node, float averageGpa,
                                int *aboveCount, int *belowCount);
static void     freeAllNodes(BSTNode *node);

/* UI helper */
static void     printMenu(void);

/* ════════════════════════════════════════════════════════════════════════════
 * SECTION 3 — AVL UTILITY HELPERS
 * ════════════════════════════════════════════════════════════════════════════ */

/*
 * getHeight — Safely returns the AVL height stored inside a node.
 *
 * Because NULL pointers represent empty subtrees with height 0, every caller
 * that needs a child's height must go through this function rather than
 * accessing node->height directly — otherwise a NULL dereference would crash.
 *
 * Parameters:
 *   node — any BSTNode pointer, including NULL.
 *
 * Returns:
 *   The node's stored height, or 0 when node is NULL.
 */
static int getHeight(BSTNode *node)
{
    if (!node) return 0;     /* NULL subtrees contribute 0 to the height */
    return node->height;
}

/*
 * maxOfTwo — Returns the larger of two integers.
 *
 * Used exclusively inside updateHeight to pick the taller of the two child
 * subtrees.  Extracted into its own function to keep the height arithmetic
 * readable and to avoid repeating the ternary expression in multiple places.
 *
 * Parameters:
 *   firstValue  — first integer to compare.
 *   secondValue — second integer to compare.
 *
 * Returns:
 *   The larger of firstValue and secondValue.
 */
static int maxOfTwo(int firstValue, int secondValue)
{
    return (firstValue > secondValue) ? firstValue : secondValue;
}

/*
 * getBalanceFactor — Computes the AVL balance factor of a node.
 *
 * Balance factor = height(left child) − height(right child).
 * The AVL invariant requires every node in the tree to have a balance factor
 * in the set {−1, 0, +1}.
 *   > +1 → left-heavy:  needs a right rotation (LL) or left-right double rotation (LR).
 *   < −1 → right-heavy: needs a left rotation (RR) or right-left double rotation (RL).
 *
 * Parameters:
 *   node — the node to evaluate.  Safe to call with NULL (returns 0).
 *
 * Returns:
 *   Integer balance factor; negative means right-heavy, positive means left-heavy.
 */
static int getBalanceFactor(BSTNode *node)
{
    if (!node) return 0;     /* an empty node is perfectly balanced by definition */
    return getHeight(node->left) - getHeight(node->right);
}

/*
 * updateHeight — Recalculates and stores the correct height of a node.
 *
 * Must be called after any structural change (rotation, child pointer
 * update) at or below this node.  Always called bottom-up: update the
 * children first, then the parent — never the other way around.
 *
 * Parameters:
 *   node — the node whose height needs refreshing (must not be NULL).
 *
 * Returns: nothing.
 */
static void updateHeight(BSTNode *node)
{
    /* Height = 1 (this node itself) + the taller of the two child subtrees. */
    node->height = 1 + maxOfTwo(getHeight(node->left), getHeight(node->right));
}

/*
 * rotateRight — Performs a single right (clockwise) rotation on pivotNode.
 *
 * Used to correct a Left-Left (LL) imbalance: pivotNode's left subtree is
 * taller by more than one level, and that left child is itself left-heavy or
 * balanced.
 *
 * ASCII diagram — before and after the rotation:
 *
 *   Before:                       After:
 *
 *         pivotNode                    newRoot
 *         /         \                  /       \
 *     newRoot     rightSub        leftSub    pivotNode
 *     /       \                              /       \
 *  leftSub  movedSub                    movedSub   rightSub
 *
 * Key insight: movedSub (newRoot's right child before rotation) has keys
 * satisfying  newRoot.id < movedSub.id < pivotNode.id, so it is legally
 * re-attached as pivotNode's left child after the rotation.
 *
 * Parameters:
 *   pivotNode — the over-balanced node to rotate (must have a non-NULL left child).
 *
 * Returns:
 *   Pointer to newRoot, the new root of this subtree; caller must store it.
 */
static BSTNode *rotateRight(BSTNode *pivotNode)
{
    BSTNode *newRoot      = pivotNode->left;  /* newRoot will rise to take pivotNode's place */
    BSTNode *movedSubtree = newRoot->right;   /* will be re-parented to pivotNode's left     */

    /* Perform the two pointer rewrites that constitute the rotation. */
    newRoot->right  = pivotNode;      /* pivotNode drops down to become newRoot's right child */
    pivotNode->left = movedSubtree;   /* movedSubtree re-attaches under pivotNode on the left */

    /*
     * Update heights bottom-up: pivotNode is now the lower node, so it must
     * be updated first.  newRoot is higher after the rotation, so it goes second.
     */
    updateHeight(pivotNode);
    updateHeight(newRoot);

    return newRoot;   /* the caller must link this as the new subtree root */
}

/*
 * rotateLeft — Performs a single left (counter-clockwise) rotation on pivotNode.
 *
 * Used to correct a Right-Right (RR) imbalance: pivotNode's right subtree is
 * taller by more than one level, and that right child is itself right-heavy or
 * balanced.
 *
 * ASCII diagram — before and after the rotation:
 *
 *   Before:                       After:
 *
 *   pivotNode                         newRoot
 *   /         \                       /       \
 * leftSub    newRoot            pivotNode    rightSub
 *            /       \          /       \
 *        movedSub  rightSub  leftSub  movedSub
 *
 * Key insight: movedSub (newRoot's left child before rotation) has keys
 * satisfying  pivotNode.id < movedSub.id < newRoot.id, so it is legally
 * re-attached as pivotNode's right child after the rotation.
 *
 * Parameters:
 *   pivotNode — the over-balanced node to rotate (must have a non-NULL right child).
 *
 * Returns:
 *   Pointer to newRoot, the new root of this subtree; caller must store it.
 */
static BSTNode *rotateLeft(BSTNode *pivotNode)
{
    BSTNode *newRoot      = pivotNode->right;  /* newRoot will rise to take pivotNode's place  */
    BSTNode *movedSubtree = newRoot->left;     /* will be re-parented to pivotNode's right     */

    /* Perform the two pointer rewrites that constitute the rotation. */
    newRoot->left    = pivotNode;     /* pivotNode drops down to become newRoot's left child  */
    pivotNode->right = movedSubtree;  /* movedSubtree re-attaches under pivotNode on the right */

    /*
     * Update heights bottom-up: pivotNode is now the lower node, so it must
     * be updated first.  newRoot is higher after the rotation, so it goes second.
     */
    updateHeight(pivotNode);
    updateHeight(newRoot);

    return newRoot;   /* the caller must link this as the new subtree root */
}

/*
 * balanceNode — Restores the AVL invariant at a single node after a change.
 *
 * Called on the way back up every recursive insert and delete.  It first
 * refreshes the node's stored height (which may have changed due to work done
 * deeper in the tree), then checks the balance factor and applies the
 * appropriate single or double rotation when the factor leaves {-1, 0, +1}.
 *
 * Four imbalance cases:
 *   LL (Left-Left)   — left child is left-heavy or balanced  → rotateRight.
 *   LR (Left-Right)  — left child is right-heavy             → rotateLeft(left child),
 *                                                               then rotateRight.
 *   RR (Right-Right) — right child is right-heavy or balanced → rotateLeft.
 *   RL (Right-Left)  — right child is left-heavy             → rotateRight(right child),
 *                                                               then rotateLeft.
 *
 * Parameters:
 *   node — the node to potentially re-balance (must not be NULL).
 *
 * Returns:
 *   Pointer to the (possibly new) subtree root after balancing; caller must store it.
 */
static BSTNode *balanceNode(BSTNode *node)
{
    /* Refresh this node's stored height before evaluating its balance. */
    updateHeight(node);

    int balanceFactor = getBalanceFactor(node);

    /* ── Left-heavy: balance factor exceeds +1 ── */
    if (balanceFactor > 1) {
        if (getBalanceFactor(node->left) < 0) {
            /*
             * LR case: left child is right-heavy.
             * Convert to LL by left-rotating the left child first,
             * then fall through to the standard LL right rotation below.
             */
            node->left = rotateLeft(node->left);
        }
        /* LL case (or LR converted to LL above): single right rotation fixes it. */
        return rotateRight(node);
    }

    /* ── Right-heavy: balance factor drops below -1 ── */
    if (balanceFactor < -1) {
        if (getBalanceFactor(node->right) > 0) {
            /*
             * RL case: right child is left-heavy.
             * Convert to RR by right-rotating the right child first,
             * then fall through to the standard RR left rotation below.
             */
            node->right = rotateRight(node->right);
        }
        /* RR case (or RL converted to RR above): single left rotation fixes it. */
        return rotateLeft(node);
    }

    /* Node is balanced — no rotation needed, return it unchanged. */
    return node;
}

/* ════════════════════════════════════════════════════════════════════════════
 * SECTION 4 — NODE CREATION
 * ════════════════════════════════════════════════════════════════════════════ */

/*
 * createNode — Allocates and fully initialises a new AVL tree leaf node.
 *
 * A newly inserted node is always a leaf (no children), so its height is
 * initialised to 1.  Both child pointers are set to NULL.  The student's
 * name is copied with strncpy and manually null-terminated to guarantee no
 * buffer overflow even if the caller's string is longer than 99 characters.
 *
 * Parameters:
 *   id   — unique integer key for BST/AVL ordering.
 *   name — student's full name (copied into the node's fixed-size buffer).
 *   gpa  — grade point average (caller must have validated it to [0.0, 4.0]).
 *
 * Returns:
 *   Pointer to the newly allocated node.
 *   Exits the program on allocation failure (unrecoverable for a tree node).
 */
static BSTNode *createNode(int id, const char *name, float gpa)
{
    BSTNode *newNode = (BSTNode *)malloc(sizeof(BSTNode));
    if (!newNode) {
        /* Allocation failure here is fatal: the caller cannot proceed without a node. */
        fprintf(stderr, "Fatal: failed to allocate BSTNode.\n");
        exit(EXIT_FAILURE);
    }

    newNode->data.id  = id;
    newNode->data.gpa = gpa;

    /* Copy name safely: strncpy fills up to the limit, manual '\0' ensures termination. */
    strncpy(newNode->data.name, name, sizeof(newNode->data.name) - 1);
    newNode->data.name[sizeof(newNode->data.name) - 1] = '\0';

    newNode->height = 1;    /* a new leaf always contributes exactly one level */
    newNode->left   = NULL;
    newNode->right  = NULL;

    return newNode;
}

/* ════════════════════════════════════════════════════════════════════════════
 * SECTION 5 — AVL INSERT HELPER
 * ════════════════════════════════════════════════════════════════════════════ */

/*
 * insertRecursive — Recursively inserts a student into an AVL subtree.
 *
 * Performs a standard BST insert: compare the new id against the current
 * node's id and recurse left or right until an empty slot is found.  On the
 * way back up (unwinding the recursion), balanceNode() is called at every
 * ancestor so the AVL height invariant is always restored before the call
 * returns to its parent.
 *
 * Parameters:
 *   node      — current subtree root (may be NULL for an empty subtree).
 *   id        — unique ID of the student to insert.
 *   name      — student's full name.
 *   gpa       — student's GPA (must already be validated to [0.0, 4.0]).
 *   outResult — output parameter: set to 0 on success, -1 on duplicate ID.
 *
 * Returns:
 *   Pointer to the (possibly rotated) subtree root after the insertion.
 */
static BSTNode *insertRecursive(BSTNode *node, int id, const char *name,
                                float gpa, int *outResult)
{
    /* Base case: we have reached an empty slot — create the new leaf here. */
    if (!node) {
        *outResult = 0;
        return createNode(id, name, gpa);
    }

    if (id < node->data.id) {
        /* New id belongs in the left subtree (smaller keys live on the left). */
        node->left = insertRecursive(node->left, id, name, gpa, outResult);
    } else if (id > node->data.id) {
        /* New id belongs in the right subtree (larger keys live on the right). */
        node->right = insertRecursive(node->right, id, name, gpa, outResult);
    } else {
        /* id == node->data.id: duplicate key — AVL/BST keys must be unique. */
        *outResult = -1;
        return node;   /* return node unchanged; no structural modification occurs */
    }

    /*
     * On the way back up the recursion stack: rebalance this node in case
     * the insertion disturbed the AVL invariant somewhere below it.
     * balanceNode also refreshes this node's stored height.
     */
    return balanceNode(node);
}

/* ════════════════════════════════════════════════════════════════════════════
 * SECTION 6 — DESTROY HELPER
 * ════════════════════════════════════════════════════════════════════════════ */

/*
 * destroyRecursive — Frees an entire subtree in post-order.
 *
 * Post-order traversal (free left, free right, then free current node)
 * guarantees that no child pointer becomes unreachable before it is freed.
 * Freeing a parent before its children would cause a memory leak because the
 * only way to reach the children is through the parent's pointers.
 *
 * Parameters:
 *   node — root of the subtree to free (safe to call with NULL).
 *
 * Returns: nothing.
 */
static void destroyRecursive(BSTNode *node)
{
    if (!node) return;              /* base case: nothing to free in an empty subtree */
    destroyRecursive(node->left);   /* free the entire left subtree first             */
    destroyRecursive(node->right);  /* then the entire right subtree                  */
    free(node);                     /* now safe to free this node (children are gone)  */
}

/* ════════════════════════════════════════════════════════════════════════════
 * SECTION 7 — SEARCH HELPERS
 * ════════════════════════════════════════════════════════════════════════════ */

/*
 * searchByIdRecursive — Standard BST/AVL search using the integer key.
 *
 * At each node, exactly one of three things is true: we have a match, the
 * target is smaller (recurse left), or the target is larger (recurse right).
 * This eliminates one entire subtree per step, giving O(log n) average depth.
 * The AVL balance guarantee limits worst-case tree height to ~1.44 log₂(n),
 * bounding the worst-case search to O(log n) as well.
 *
 * Parameters:
 *   node     — current subtree root (may be NULL).
 *   targetId — the student ID to locate.
 *
 * Returns:
 *   Pointer to the matching BSTNode, or NULL if the id is not in this subtree.
 */
static BSTNode *searchByIdRecursive(BSTNode *node, int targetId)
{
    if (!node)                           return NULL;   /* subtree exhausted: id not found */
    if (targetId == node->data.id)       return node;   /* exact match found               */
    if (targetId  < node->data.id)
        return searchByIdRecursive(node->left,  targetId);   /* search the smaller-id side */
    return     searchByIdRecursive(node->right, targetId);   /* search the larger-id side  */
}

/*
 * strCaseEqual — Case-insensitive string equality check.
 *
 * Walks both strings byte-by-byte, comparing the lowercase version of each
 * character using tolower().  This avoids the POSIX-specific strcasecmp so
 * the code compiles cleanly under strict C11 without any POSIX feature macros.
 *
 * Parameters:
 *   stringA — first string to compare.
 *   stringB — second string to compare.
 *
 * Returns:
 *   1 if both strings contain the same characters ignoring ASCII case, 0 otherwise.
 */
static int strCaseEqual(const char *stringA, const char *stringB)
{
    while (*stringA && *stringB) {
        /* Compare the lowercase version of each byte; bail on first mismatch. */
        if (tolower((unsigned char)*stringA) != tolower((unsigned char)*stringB))
            return 0;
        stringA++;
        stringB++;
    }
    /* Both pointers must reach '\0' simultaneously — otherwise one string is a prefix. */
    return tolower((unsigned char)*stringA) == tolower((unsigned char)*stringB);
}

/*
 * searchByNameInorder — Full in-order traversal searching for a student by name.
 *
 * Because name is NOT the AVL key, the tree ordering property cannot be used
 * to skip subtrees; every node must be visited.  In-order traversal (left →
 * root → right) visits nodes in ascending-ID order, so if multiple students
 * share the same name the one with the smallest ID is returned first.
 *
 * Parameters:
 *   node       — current subtree root (may be NULL).
 *   targetName — name to search for (comparison is case-insensitive).
 *
 * Returns:
 *   Pointer to the first matching BSTNode (smallest ID among matches), or NULL.
 */
static BSTNode *searchByNameInorder(BSTNode *node, const char *targetName)
{
    if (!node) return NULL;

    /* Visit the left subtree first (in-order: left before root). */
    BSTNode *leftResult = searchByNameInorder(node->left, targetName);
    if (leftResult) return leftResult;   /* propagate a match from the left immediately */

    /* Check this node. */
    if (strCaseEqual(node->data.name, targetName)) return node;

    /* Visit the right subtree last. */
    return searchByNameInorder(node->right, targetName);
}

/* ════════════════════════════════════════════════════════════════════════════
 * SECTION 8 — DELETE HELPERS
 * ════════════════════════════════════════════════════════════════════════════ */

/*
 * findMinNode — Returns the node with the smallest key in a subtree.
 *
 * The BST invariant guarantees the minimum key is always at the leftmost
 * position: follow left pointers until there are no more.  This function is
 * used by deleteRecursive to locate the in-order successor (minimum of the
 * right subtree) when deleting a node that has two children.
 *
 * Parameters:
 *   subtreeRoot — root of the subtree to search (must not be NULL).
 *
 * Returns:
 *   Pointer to the leftmost (minimum-key) node in the subtree.
 */
static BSTNode *findMinNode(BSTNode *subtreeRoot)
{
    /* Iterative descent: follow left pointers until the left child is NULL. */
    while (subtreeRoot->left) subtreeRoot = subtreeRoot->left;
    return subtreeRoot;
}

/*
 * deleteRecursive — Recursively removes the node with the given id from an AVL subtree.
 *
 * Handles all three structural deletion cases:
 *   Case 1 — Leaf node (no children)   : free the node, return NULL.
 *   Case 2 — One child                 : free the node, return its only child.
 *   Case 3 — Two children              : copy the in-order successor's data into
 *                                        this node's slot, then delete the successor
 *                                        from the right subtree (always Case 1 or 2).
 *
 * After any structural change, balanceNode() is called on the way back up the
 * call stack to restore the AVL invariant at every ancestor of the deleted node.
 *
 * Parameters:
 *   node      — current subtree root.
 *   id        — the student ID to remove.
 *   outResult — output parameter: set to 0 on success, -1 if id is not found.
 *
 * Returns:
 *   Pointer to the (possibly rotated) subtree root after the deletion.
 */
static BSTNode *deleteRecursive(BSTNode *node, int id, int *outResult)
{
    if (!node) {
        /* Reached a NULL child: the id does not exist anywhere in this subtree. */
        *outResult = -1;
        return NULL;
    }

    if (id < node->data.id) {
        /* Target id is smaller: it must be in the left subtree (if it exists). */
        node->left = deleteRecursive(node->left, id, outResult);
    } else if (id > node->data.id) {
        /* Target id is larger: it must be in the right subtree (if it exists). */
        node->right = deleteRecursive(node->right, id, outResult);
    } else {
        /* id == node->data.id: this is the node to delete. */
        *outResult = 0;

        if (!node->left && !node->right) {
            /* ── Case 1: Leaf node ─────────────────────────────────────────
             * No children to preserve.  Simply free and return NULL so the
             * parent's child pointer is set to NULL by the caller.           */
            free(node);
            return NULL;

        } else if (!node->left) {
            /* ── Case 2a: Only a right child ──────────────────────────────
             * Promote the right child to take this node's place.             */
            BSTNode *rightChild = node->right;
            free(node);
            return rightChild;   /* right child is the new subtree root here  */

        } else if (!node->right) {
            /* ── Case 2b: Only a left child ───────────────────────────────
             * Promote the left child to take this node's place.              */
            BSTNode *leftChild = node->left;
            free(node);
            return leftChild;    /* left child is the new subtree root here   */

        } else {
            /*
             * ── Case 3: Two children ─────────────────────────────────────
             * We cannot simply remove this node because both subtrees must
             * remain connected.  Solution: find the in-order successor (the
             * leftmost node of the right subtree), copy its data into this
             * slot, then delete the original successor node from the right
             * subtree.
             *
             * The in-order successor has at most one child (a right child),
             * so its deletion is always Case 1 or Case 2 — never Case 3.
             * This prevents infinite mutual recursion.
             */
            BSTNode *inorderSuccessor = findMinNode(node->right);

            /* Capture the successor's id BEFORE overwriting node->data,
             * because after the struct copy node->data.id == successorId,
             * and we still need the original successorId to delete it below. */
            int successorId = inorderSuccessor->data.id;

            node->data = inorderSuccessor->data;    /* struct copy of the payload */

            /* Delete the successor from the right subtree; its result is discarded
             * because we know the deletion will succeed (the node is definitely there). */
            int unusedResult = 0;
            node->right = deleteRecursive(node->right, successorId, &unusedResult);
        }
    }

    /*
     * On the way back up the call stack: restore AVL balance at this node.
     * balanceNode also refreshes this node's stored height, which may have
     * changed because a node was removed from one of its subtrees.
     */
    return balanceNode(node);
}

/* ════════════════════════════════════════════════════════════════════════════
 * SECTION 9 — DISPLAY HELPER
 * ════════════════════════════════════════════════════════════════════════════ */

/*
 * inorderPrint — Prints all student records to stdout in ascending-ID order.
 *
 * In-order traversal (left → root → right) of a BST/AVL tree naturally visits
 * nodes in ascending key order, so no separate sorting step is needed.
 * One formatted line is printed per node.
 *
 * Parameters:
 *   node — current subtree root (safe to call with NULL).
 *
 * Returns: nothing.
 */
static void inorderPrint(BSTNode *node)
{
    if (!node) return;
    inorderPrint(node->left);    /* visit all smaller-id students first   */
    printf("ID: %-6d  |  Name: %-30s  |  GPA: %.2f\n",
           node->data.id, node->data.name, node->data.gpa);
    inorderPrint(node->right);   /* then visit all larger-id students      */
}

/* ════════════════════════════════════════════════════════════════════════════
 * SECTION 10 — STATISTICS HELPERS
 * ════════════════════════════════════════════════════════════════════════════ */

/*
 * collectGpaStats — Single in-order pass gathering raw GPA statistics.
 *
 * Traverses the entire tree once, updating the running minimum, maximum, and
 * sum of GPA values, and counting how many nodes are visited.  These four
 * values allow the caller to compute the arithmetic average afterward.
 *
 * Parameters:
 *   node          — current subtree root (safe to call with NULL).
 *   minGpa        — pointer to the running minimum; updated when a smaller GPA is seen.
 *   maxGpa        — pointer to the running maximum; updated when a larger GPA is seen.
 *   runningSum    — pointer to the GPA accumulator for mean calculation.
 *   visitedCount  — pointer to the node counter; incremented for each node.
 *
 * Returns: nothing (results written through pointer parameters).
 */
static void collectGpaStats(BSTNode *node, float *minGpa, float *maxGpa,
                            float *runningSum, int *visitedCount)
{
    if (!node) return;

    collectGpaStats(node->left, minGpa, maxGpa, runningSum, visitedCount);

    float currentGpa = node->data.gpa;
    if (currentGpa < *minGpa) *minGpa = currentGpa;   /* new minimum found */
    if (currentGpa > *maxGpa) *maxGpa = currentGpa;   /* new maximum found */
    *runningSum += currentGpa;   /* add to total so we can compute the mean later */
    (*visitedCount)++;           /* count this node toward the average denominator */

    collectGpaStats(node->right, minGpa, maxGpa, runningSum, visitedCount);
}

/*
 * countAboveBelow — In-order pass classifying students relative to the mean GPA.
 *
 * Increments aboveCount when a student's GPA strictly exceeds the average, and
 * belowCount when it strictly falls below.  Students whose GPA exactly equals
 * the average are counted in neither bucket (they are "at average").
 *
 * Parameters:
 *   node        — current subtree root (safe to call with NULL).
 *   averageGpa  — the pre-computed mean GPA to compare against.
 *   aboveCount  — pointer to the count of students strictly above average.
 *   belowCount  — pointer to the count of students strictly below average.
 *
 * Returns: nothing.
 */
static void countAboveBelow(BSTNode *node, float averageGpa,
                            int *aboveCount, int *belowCount)
{
    if (!node) return;
    countAboveBelow(node->left, averageGpa, aboveCount, belowCount);
    if      (node->data.gpa > averageGpa) (*aboveCount)++;   /* strictly above the mean */
    else if (node->data.gpa < averageGpa) (*belowCount)++;   /* strictly below the mean */
    countAboveBelow(node->right, averageGpa, aboveCount, belowCount);
}

/*
 * freeAllNodes — Post-order traversal that frees every node in a subtree.
 *
 * Does NOT free the BST wrapper struct itself.  Used by bst_delete_all so the
 * caller retains a valid, reusable BST pointer after all student records are removed.
 *
 * Parameters:
 *   node — root of the subtree to free (safe to call with NULL).
 *
 * Returns: nothing.
 */
static void freeAllNodes(BSTNode *node)
{
    if (!node) return;
    freeAllNodes(node->left);    /* free children before parent (post-order) */
    freeAllNodes(node->right);
    free(node);
}

/* ════════════════════════════════════════════════════════════════════════════
 * SECTION 11 — PUBLIC API: LIFECYCLE
 * ════════════════════════════════════════════════════════════════════════════ */

/*
 * bst_create — Allocates and initialises a new, empty AVL tree.
 *
 * Sets root = NULL (no nodes yet) and count = 0 (no students).  The function
 * never returns NULL — it exits the program on allocation failure because the
 * BST wrapper is a critical object without which the program cannot operate.
 *
 * Parameters: none.
 *
 * Returns:
 *   Pointer to a freshly allocated, empty BST wrapper.
 */
BST *bst_create(void)
{
    BST *tree = (BST *)malloc(sizeof(BST));
    if (!tree) {
        fprintf(stderr, "Fatal: failed to allocate BST struct.\n");
        exit(EXIT_FAILURE);
    }
    tree->root  = NULL;   /* no nodes in the tree yet         */
    tree->count = 0;      /* no students enrolled yet          */
    return tree;
}

/*
 * bst_destroy — Completely tears down the AVL tree and frees the wrapper struct.
 *
 * Calls destroyRecursive (post-order) to free every BSTNode, then resets the
 * struct's fields as a defensive measure before freeing the wrapper itself.
 * Safe to call on a NULL pointer or an empty tree.
 *
 * Parameters:
 *   tree — the AVL tree to destroy.
 *
 * Returns: nothing.
 */
void bst_destroy(BST *tree)
{
    if (!tree) return;                   /* guard against a NULL argument              */
    destroyRecursive(tree->root);        /* free all nodes in post-order               */
    tree->root  = NULL;                  /* defensive zeroing before freeing the struct */
    tree->count = 0;
    free(tree);                          /* free the wrapper itself                    */
}

/* ════════════════════════════════════════════════════════════════════════════
 * SECTION 12 — PUBLIC API: CORE OPERATIONS
 * ════════════════════════════════════════════════════════════════════════════ */

/*
 * bst_insert — Validates and inserts a new student into the AVL tree.
 *
 * Rejects the insertion with an error code if the GPA is outside [0.0, 4.0]
 * or if a student with the same ID already exists.  On success, delegates to
 * insertRecursive (which handles AVL rebalancing) and increments tree->count.
 *
 * Parameters:
 *   tree — the AVL tree to insert into.
 *   id   — the student's unique integer ID.
 *   name — the student's full name.
 *   gpa  — the student's GPA (validated here at the system boundary).
 *
 * Returns:
 *    0  on success.
 *   -1  if a student with the same ID already exists (prints an error).
 *   -2  if GPA is outside [0.0, 4.0] (prints an error).
 */
int bst_insert(BST *tree, int id, const char *name, float gpa)
{
    if (!tree) return -1;   /* safety guard: caller passed a NULL tree pointer */

    if (gpa < 0.0f || gpa > 4.0f) {
        /* Validate at the boundary so invalid data never enters the tree. */
        fprintf(stderr,
                "Error: GPA %.2f is outside the valid range [0.0, 4.0].\n",
                gpa);
        return -2;
    }

    int insertionResult = 0;   /* insertRecursive will set this to -1 on a duplicate */
    tree->root = insertRecursive(tree->root, id, name, gpa, &insertionResult);

    if (insertionResult == -1) {
        fprintf(stderr, "Error: Student with ID %d already exists.\n", id);
        return -1;
    }

    tree->count++;   /* one more live student enrolled in the tree */
    return 0;
}

/*
 * bst_search_by_id — Finds a student by their unique integer ID.
 *
 * Delegates to searchByIdRecursive, which exploits the BST ordering property
 * to eliminate one subtree per comparison.  With AVL balancing, the tree height
 * is at most ~1.44 log₂(n), so the worst-case search cost is O(log n).
 *
 * Parameters:
 *   tree — the AVL tree to search.
 *   id   — the student ID to locate.
 *
 * Returns:
 *   Pointer to the matching BSTNode, or NULL if no student has that ID.
 */
BSTNode *bst_search_by_id(BST *tree, int id)
{
    if (!tree) return NULL;
    return searchByIdRecursive(tree->root, id);
}

/*
 * bst_search_by_name — Finds a student by name (case-insensitive, full scan).
 *
 * Name is not the AVL key, so the tree ordering cannot be exploited; the
 * entire tree is visited via in-order traversal.  The first match encountered
 * (smallest ID among all name matches) is returned.
 *
 * Parameters:
 *   tree — the AVL tree to search.
 *   name — the name to find (comparison is case-insensitive).
 *
 * Returns:
 *   Pointer to the first matching BSTNode, or NULL if no match is found.
 */
BSTNode *bst_search_by_name(BST *tree, const char *name)
{
    if (!tree || !name) return NULL;
    return searchByNameInorder(tree->root, name);
}

/*
 * bst_delete_by_id — Removes the student with the given ID from the AVL tree.
 *
 * Delegates to deleteRecursive, which handles all three BST deletion cases and
 * calls balanceNode on the way back up to maintain the AVL invariant.
 * Decrements tree->count only when deletion actually succeeds.
 *
 * Parameters:
 *   tree — the AVL tree to delete from.
 *   id   — the student ID to remove.
 *
 * Returns:
 *    0  on success.
 *   -1  if no student with that ID exists (prints a "not found" error).
 */
int bst_delete_by_id(BST *tree, int id)
{
    if (!tree) return -1;

    int deletionResult = 0;   /* deleteRecursive sets this to -1 when not found */
    tree->root = deleteRecursive(tree->root, id, &deletionResult);

    if (deletionResult == -1) {
        printf("Error: Student with ID %d not found.\n", id);
        return -1;
    }

    tree->count--;   /* one fewer live student in the tree */
    return 0;
}

/*
 * bst_update — Interactively updates fields of an existing student record.
 *
 * Presents a sub-menu loop letting the operator change Name, GPA, or ID until
 * they select "Done".  Name and GPA are updated directly on the node (no tree
 * restructuring needed because they are not the key).  Updating the ID requires
 * a delete-then-reinsert cycle so the AVL tree remains correctly ordered; if the
 * requested new ID is already taken, the original record is automatically restored.
 *
 * Parameters:
 *   tree — the AVL tree containing the student to update.
 *   id   — the current ID of the student to update.
 *
 * Returns:
 *    0  on success.
 *   -1  if no student with that ID exists.
 */
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
        /* Show the sub-menu with the current (possibly already updated) values. */
        printf("\n--- Update Student (ID: %d, Name: %s) ---\n",
               targetNode->data.id, targetNode->data.name);
        printf(" [1] Update Name\n");
        printf(" [2] Update GPA\n");
        printf(" [3] Update ID\n");
        printf(" [4] Done\n");
        printf("Enter choice: ");

        if (!fgets(inputBuffer, sizeof(inputBuffer), stdin)) {
            subMenuChoice = 4;   /* treat EOF (Ctrl-D) as "Done" to exit cleanly */
            break;
        }

        if (sscanf(inputBuffer, "%d", &subMenuChoice) != 1) {
            printf("Invalid input. Please enter a number between 1 and 4.\n");
            subMenuChoice = 0;   /* reset so the loop continues */
            continue;
        }

        switch (subMenuChoice) {

            case 1: {
                /* ── Update Name ─────────────────────────────────────────── */
                printf("Enter new name: ");
                if (!fgets(inputBuffer, sizeof(inputBuffer), stdin)) break;
                inputBuffer[strcspn(inputBuffer, "\n")] = '\0';   /* remove trailing newline */

                if (strlen(inputBuffer) == 0) {
                    printf("Error: Name cannot be empty.\n");
                } else {
                    /* Copy new name into the fixed-size field; clamp to prevent overflow. */
                    strncpy(targetNode->data.name, inputBuffer,
                            sizeof(targetNode->data.name) - 1);
                    targetNode->data.name[sizeof(targetNode->data.name) - 1] = '\0';
                    printf("Name updated successfully.\n");
                }
                break;
            }

            case 2: {
                /* ── Update GPA ──────────────────────────────────────────── */
                printf("Enter new GPA (0.0 - 4.0): ");
                if (!fgets(inputBuffer, sizeof(inputBuffer), stdin)) break;
                float newGpa = 0.0f;
                if (sscanf(inputBuffer, "%f", &newGpa) != 1) {
                    printf("Invalid GPA. Please enter a decimal number.\n");
                } else if (newGpa < 0.0f || newGpa > 4.0f) {
                    printf("Error: GPA must be in the range [0.0, 4.0].\n");
                } else {
                    targetNode->data.gpa = newGpa;   /* in-place update; no tree restructuring */
                    printf("GPA updated to %.2f successfully.\n", newGpa);
                }
                break;
            }

            case 3: {
                /* ── Update ID ───────────────────────────────────────────── */
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

                /* Save the current name and GPA before any structural changes,
                 * because the node will be freed during deletion. */
                char  savedName[100];
                float savedGpa = targetNode->data.gpa;
                strncpy(savedName, targetNode->data.name, sizeof(savedName) - 1);
                savedName[sizeof(savedName) - 1] = '\0';

                /* Step 1: Delete the old node.  After this call, targetNode is a
                 * dangling pointer — the memory it pointed to has been freed. */
                bst_delete_by_id(tree, id);
                targetNode = NULL;   /* mark explicitly to catch accidental use */

                /* Step 2: Attempt to insert the student under the new ID. */
                int insertResult = bst_insert(tree, newId, savedName, savedGpa);
                if (insertResult != 0) {
                    /* New ID was already taken (bst_insert already printed the error).
                     * Restore the original record so data is not lost. */
                    printf("Restoring original record with ID %d.\n", id);
                    bst_insert(tree, id, savedName, savedGpa);
                    targetNode = bst_search_by_id(tree, id);   /* re-acquire valid pointer */
                } else {
                    printf("ID updated from %d to %d successfully.\n", id, newId);
                    id         = newId;                              /* track the new id locally */
                    targetNode = bst_search_by_id(tree, newId);     /* re-acquire valid pointer */
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

/* ════════════════════════════════════════════════════════════════════════════
 * SECTION 13 — PUBLIC API: TRAVERSAL AND DISPLAY
 * ════════════════════════════════════════════════════════════════════════════ */

/*
 * bst_display_all — Prints every student record sorted by ID.
 *
 * Delegates to inorderPrint which exploits the BST/AVL in-order traversal
 * property to produce ascending-ID output with no extra sorting step.
 * Prints a formatted header row and separator line before the records.
 * If the tree is empty, prints "No students found." instead.
 *
 * Parameters:
 *   tree — the AVL tree to display.
 *
 * Returns: nothing.
 */
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

/* ════════════════════════════════════════════════════════════════════════════
 * SECTION 14 — PUBLIC API: STATISTICS
 * ════════════════════════════════════════════════════════════════════════════ */

/*
 * bst_tree_stats — Prints the AVL tree height and total student count.
 *
 * The tree height is read from getHeight(tree->root), which simply returns
 * the height field of the root node — an O(1) operation because AVL trees
 * maintain this value continuously.  tree->count is also O(1).
 *
 * Parameters:
 *   tree — the AVL tree to report on.
 *
 * Returns: nothing.
 */
void bst_tree_stats(BST *tree)
{
    if (!tree) return;
    /* getHeight reads the stored height field in O(1), no traversal needed. */
    int treeHeight = getHeight(tree->root);
    printf("\nTree Height    : %d\n", treeHeight);
    printf("Total Students : %d\n",  tree->count);
}

/*
 * bst_gpa_stats — Computes and prints a five-number GPA summary.
 *
 * Uses two traversal passes:
 *   Pass 1 (collectGpaStats)  — collects min, max, running sum, and node count.
 *   Pass 2 (countAboveBelow)  — counts students strictly above and below the mean.
 * Prints all five figures.  If the tree is empty, prints "No students found."
 *
 * Parameters:
 *   tree — the AVL tree to analyse.
 *
 * Returns: nothing.
 */
void bst_gpa_stats(BST *tree)
{
    if (!tree || !tree->root) {
        printf("No students found.\n");
        return;
    }

    /* Initialise extremes conservatively so the first real GPA overrides them:
     * minGpa starts at the maximum possible so any real value is smaller;
     * maxGpa starts at the minimum possible so any real value is larger.       */
    float minGpa       = 4.0f;
    float maxGpa       = 0.0f;
    float runningSum   = 0.0f;
    int   studentCount = 0;

    collectGpaStats(tree->root, &minGpa, &maxGpa, &runningSum, &studentCount);

    /* Compute the arithmetic mean from the accumulated sum and node count. */
    float averageGpa = (studentCount > 0)
                       ? (runningSum / (float)studentCount)
                       : 0.0f;   /* defensive: tree is non-empty, so count > 0 here */

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

/* ════════════════════════════════════════════════════════════════════════════
 * SECTION 15 — PUBLIC API: BULK DELETION
 * ════════════════════════════════════════════════════════════════════════════ */

/*
 * bst_delete_all — Frees every node without destroying the BST wrapper struct.
 *
 * Unlike bst_destroy, this function leaves the BST pointer valid and reusable
 * for new insertions after the call.  It resets root = NULL and count = 0,
 * then prints a confirmation message.
 *
 * Parameters:
 *   tree — the AVL tree to empty.
 *
 * Returns: nothing.
 */
void bst_delete_all(BST *tree)
{
    if (!tree) return;
    freeAllNodes(tree->root);   /* post-order traversal frees every BSTNode  */
    tree->root  = NULL;         /* tree is now structurally empty             */
    tree->count = 0;
    printf("All student records have been deleted.\n");
}

/* ════════════════════════════════════════════════════════════════════════════
 * SECTION 16 — PUBLIC API: PREDECESSOR AND SUCCESSOR
 * ════════════════════════════════════════════════════════════════════════════ */

/*
 * bst_predecessor_successor — Finds and prints the predecessor and successor
 * of the student with the given ID.
 *
 * Predecessor: the student with the largest ID strictly less than the target.
 * Successor  : the student with the smallest ID strictly greater than the target.
 *
 * Both are found via independent iterative passes from the root, each keeping
 * track of the best candidate seen so far:
 *
 *   Predecessor pass: when the current node's id < target, record it as the
 *     new best candidate (closer than any previous) and go right to look for
 *     an even larger id that is still less than the target.  When current id
 *     >= target, go left (the answer, if any, is in the left subtree).
 *
 *   Successor pass: when the current node's id > target, record it as the new
 *     best candidate and go left to look for an even smaller id that is still
 *     greater than the target.  When current id <= target, go right.
 *
 * Parameters:
 *   tree — the AVL tree to search.
 *   id   — the reference student ID.
 *
 * Returns: nothing (prints results directly).
 */
void bst_predecessor_successor(BST *tree, int id)
{
    if (!tree) return;

    /* Verify the reference ID actually exists before searching for its neighbours. */
    if (!bst_search_by_id(tree, id)) {
        printf("Student ID %d not found.\n", id);
        return;
    }

    BSTNode *predecessor = NULL;   /* best predecessor candidate found so far */
    BSTNode *successor   = NULL;   /* best successor candidate found so far   */
    BSTNode *currentNode = NULL;

    /* ── Predecessor pass: find the largest id strictly less than target ── */
    currentNode = tree->root;
    while (currentNode) {
        if (currentNode->data.id < id) {
            predecessor = currentNode;        /* new best candidate */
            currentNode = currentNode->right; /* look for a larger id that is still < target */
        } else {
            currentNode = currentNode->left;  /* current id >= target; go left */
        }
    }

    /* ── Successor pass: find the smallest id strictly greater than target ── */
    currentNode = tree->root;
    while (currentNode) {
        if (currentNode->data.id > id) {
            successor   = currentNode;        /* new best candidate */
            currentNode = currentNode->left;  /* look for a smaller id that is still > target */
        } else {
            currentNode = currentNode->right; /* current id <= target; go right */
        }
    }

    /* Print the predecessor (or a "first record" message if none exists). */
    if (predecessor) {
        printf("Predecessor: ID=%d, Name=%s, GPA=%.2f\n",
               predecessor->data.id,
               predecessor->data.name,
               predecessor->data.gpa);
    } else {
        printf("%d is the first record — no predecessor.\n", id);
    }

    /* Print the successor (or a "last record" message if none exists). */
    if (successor) {
        printf("Successor  : ID=%d, Name=%s, GPA=%.2f\n",
               successor->data.id,
               successor->data.name,
               successor->data.gpa);
    } else {
        printf("%d is the last record — no successor.\n", id);
    }
}

/* ════════════════════════════════════════════════════════════════════════════
 * SECTION 17 — UI HELPER
 * ════════════════════════════════════════════════════════════════════════════ */

/*
 * printMenu — Displays the main interactive menu.
 *
 * Called at the start of every do-while iteration in main() so the user
 * always sees the complete option list before entering their choice.
 *
 * Parameters: none.
 * Returns: nothing.
 */
static void printMenu(void)
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

/* ════════════════════════════════════════════════════════════════════════════
 * SECTION 18 — MAIN
 * ════════════════════════════════════════════════════════════════════════════ */

/*
 * main — Program entry point.
 *
 * Initialises an empty AVL tree, then enters a do-while loop that displays the
 * menu, reads the user's choice with fgets + sscanf (to prevent buffer overflows),
 * and dispatches to the appropriate public API function.  Invalid choices print
 * an error and re-display the menu.  On exit (choice 0 or EOF), bst_destroy
 * releases every byte of allocated memory before the process terminates.
 */
int main(void)
{
    BST  *tree       = bst_create();  /* create the empty AVL tree              */
    char  inputBuffer[256];           /* reusable read buffer for all fgets calls */
    int   menuChoice = -1;            /* initialised to an invalid value          */

    do {
        printMenu();

        /* Read the menu choice with fgets to consume the entire line safely. */
        if (!fgets(inputBuffer, sizeof(inputBuffer), stdin)) {
            menuChoice = 0;   /* EOF (e.g. Ctrl-D in a terminal): treat as exit */
            break;
        }

        /* Parse the integer choice; reject non-numeric input gracefully. */
        if (sscanf(inputBuffer, "%d", &menuChoice) != 1) {
            printf("Invalid choice. Please try again.\n");
            menuChoice = -1;   /* reset to trigger the default case next iteration */
            continue;
        }

        switch (menuChoice) {

            /* ── [1] Insert Student ──────────────────────────────────────── */
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
                studentName[strcspn(studentName, "\n")] = '\0';   /* strip newline */
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

            /* ── [2] Search by ID ────────────────────────────────────────── */
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

            /* ── [3] Search by Name ──────────────────────────────────────── */
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

            /* ── [4] Delete by ID ────────────────────────────────────────── */
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

            /* ── [5] Update Student Data ─────────────────────────────────── */
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

            /* ── [6] Display All Students ────────────────────────────────── */
            case 6:
                bst_display_all(tree);
                break;

            /* ── [7] Display Tree Statistics ─────────────────────────────── */
            case 7:
                bst_tree_stats(tree);
                break;

            /* ── [8] Display GPA Statistics ──────────────────────────────── */
            case 8:
                bst_gpa_stats(tree);
                break;

            /* ── [9] Delete All Students ─────────────────────────────────── */
            case 9:
                bst_delete_all(tree);
                break;

            /* ── [10] Find Predecessor & Successor ───────────────────────── */
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

            /* ── [0] Exit ────────────────────────────────────────────────── */
            case 0:
                printf("Exiting... Goodbye!\n");
                break;

            /* ── Invalid input ───────────────────────────────────────────── */
            default:
                printf("Invalid choice. Please try again.\n");
                break;
        }

    } while (menuChoice != 0);

    bst_destroy(tree);   /* release all allocated memory before the process exits */
    return 0;
}
