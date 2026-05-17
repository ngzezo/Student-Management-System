/*
 * bst.c
 * Student Management System — Binary Search Tree implementation.
 *
 * All static helpers are declared and defined before the public API functions
 * they support so that no forward declarations of helpers are needed except
 * for the mutual-dependency cases noted below.
 */

#include "bst.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/* ═══════════════════════════════════════════════════════════════════════════
 * Forward declarations of static helpers (used before their definitions).
 * ═══════════════════════════════════════════════════════════════════════════ */

static BSTNode *create_node(int id, const char *name, float gpa);
static BSTNode *insert_recursive(BSTNode *node, int id, const char *name,
                                 float gpa, int *result);
static void     destroy_recursive(BSTNode *node);
static BSTNode *search_by_id_recursive(BSTNode *node, int id);
static BSTNode *search_by_name_inorder(BSTNode *node, const char *name);
static int      str_case_equal(const char *a, const char *b);
static BSTNode *find_min_node(BSTNode *node);
static BSTNode *delete_recursive(BSTNode *node, int id, int *result);
static void     inorder_print(BSTNode *node);
static int      compute_height(BSTNode *node);
static void     collect_gpa_stats(BSTNode *node, float *min_gpa,
                                  float *max_gpa, float *sum, int *count);
static void     count_above_below(BSTNode *node, float avg,
                                  int *above, int *below);
static void     free_all_nodes(BSTNode *node);

/* ═══════════════════════════════════════════════════════════════════════════
 * Static Helper Implementations
 * ═══════════════════════════════════════════════════════════════════════════ */

/*
 * create_node — Allocates a new BSTNode, copies the student data into it,
 * and sets both child pointers to NULL. Exits on allocation failure.
 */
static BSTNode *create_node(int id, const char *name, float gpa)
{
    BSTNode *node = (BSTNode *)malloc(sizeof(BSTNode));
    if (!node) {
        fprintf(stderr, "Fatal: failed to allocate BSTNode.\n");
        exit(EXIT_FAILURE);
    }
    node->data.id  = id;
    node->data.gpa = gpa;
    strncpy(node->data.name, name, sizeof(node->data.name) - 1);
    node->data.name[sizeof(node->data.name) - 1] = '\0';
    node->left  = NULL;
    node->right = NULL;
    return node;
}

/*
 * insert_recursive — Traverses the BST by id and inserts a new node at the
 * correct position. Sets *result = 0 on success, -1 on duplicate id.
 */
static BSTNode *insert_recursive(BSTNode *node, int id, const char *name,
                                 float gpa, int *result)
{
    if (!node) {
        *result = 0;
        return create_node(id, name, gpa);
    }
    if (id < node->data.id) {
        node->left  = insert_recursive(node->left,  id, name, gpa, result);
    } else if (id > node->data.id) {
        node->right = insert_recursive(node->right, id, name, gpa, result);
    } else {
        /* id == node->data.id : duplicate */
        *result = -1;
    }
    return node;
}

/*
 * destroy_recursive — Performs a post-order traversal, freeing every node.
 * Called by bst_destroy; must not be called on an already-freed tree.
 */
static void destroy_recursive(BSTNode *node)
{
    if (!node) return;
    destroy_recursive(node->left);
    destroy_recursive(node->right);
    free(node);
}

/*
 * search_by_id_recursive — Standard BST search. Compares id at each node and
 * descends left or right, returning the matching node or NULL.
 */
static BSTNode *search_by_id_recursive(BSTNode *node, int id)
{
    if (!node)               return NULL;
    if (id == node->data.id) return node;
    if (id <  node->data.id) return search_by_id_recursive(node->left,  id);
    return search_by_id_recursive(node->right, id);
}

/*
 * str_case_equal — Returns 1 if strings a and b are identical ignoring ASCII
 * case, 0 otherwise. Used by the name search to enable case-insensitive match.
 */
static int str_case_equal(const char *a, const char *b)
{
    while (*a && *b) {
        if (tolower((unsigned char)*a) != tolower((unsigned char)*b))
            return 0;
        a++;
        b++;
    }
    return tolower((unsigned char)*a) == tolower((unsigned char)*b);
}

/*
 * search_by_name_inorder — In-order traversal that returns the first node
 * whose name matches the query (case-insensitive), or NULL if none found.
 */
static BSTNode *search_by_name_inorder(BSTNode *node, const char *name)
{
    if (!node) return NULL;

    /* Check left subtree first (in-order: left → root → right) */
    BSTNode *found = search_by_name_inorder(node->left, name);
    if (found) return found;

    if (str_case_equal(node->data.name, name)) return node;

    return search_by_name_inorder(node->right, name);
}

/*
 * find_min_node — Iteratively descends left to return the node with the
 * smallest id in a subtree; used to locate the in-order successor during
 * the two-children deletion case.
 */
static BSTNode *find_min_node(BSTNode *node)
{
    if (!node) return NULL;
    while (node->left) node = node->left;
    return node;
}

/*
 * delete_recursive — Recursively locates and removes the node with the given
 * id, handling all three BST deletion cases. Sets *result = 0 on success or
 * *result = -1 if the id is not found.
 */
static BSTNode *delete_recursive(BSTNode *node, int id, int *result)
{
    if (!node) {
        *result = -1;
        return NULL;
    }

    if (id < node->data.id) {
        node->left  = delete_recursive(node->left,  id, result);
    } else if (id > node->data.id) {
        node->right = delete_recursive(node->right, id, result);
    } else {
        /* Found the node to delete. */
        *result = 0;

        if (!node->left && !node->right) {
            /* Case 1: Leaf — simply remove. */
            free(node);
            return NULL;

        } else if (!node->left) {
            /* Case 2: Only right child — replace node with it. */
            BSTNode *child = node->right;
            free(node);
            return child;

        } else if (!node->right) {
            /* Case 2: Only left child — replace node with it. */
            BSTNode *child = node->left;
            free(node);
            return child;

        } else {
            /*
             * Case 3: Two children — copy in-order successor data into
             * this node, then delete the successor from the right subtree.
             * The successor is the leftmost node in the right subtree.
             */
            BSTNode *successor  = find_min_node(node->right);
            int      succ_id    = successor->data.id; /* capture before copy */
            node->data          = successor->data;    /* struct copy          */
            int dummy           = 0;
            node->right = delete_recursive(node->right, succ_id, &dummy);
        }
    }
    return node;
}

/*
 * inorder_print — In-order (left → root → right) traversal that prints one
 * student record per line in ascending id order.
 */
static void inorder_print(BSTNode *node)
{
    if (!node) return;
    inorder_print(node->left);
    printf("ID: %-6d  |  Name: %-30s  |  GPA: %.2f\n",
           node->data.id, node->data.name, node->data.gpa);
    inorder_print(node->right);
}

/*
 * compute_height — Recursively computes the height of the subtree rooted at
 * node. An empty subtree (NULL) has height 0; a single node has height 1.
 */
static int compute_height(BSTNode *node)
{
    if (!node) return 0;
    int lh = compute_height(node->left);
    int rh = compute_height(node->right);
    return 1 + (lh > rh ? lh : rh);
}

/*
 * collect_gpa_stats — Single in-order pass that updates the running minimum,
 * maximum, and sum of all GPA values, and counts the number of nodes visited.
 */
static void collect_gpa_stats(BSTNode *node, float *min_gpa,
                               float *max_gpa, float *sum, int *count)
{
    if (!node) return;
    collect_gpa_stats(node->left, min_gpa, max_gpa, sum, count);

    float g = node->data.gpa;
    if (g < *min_gpa) *min_gpa = g;
    if (g > *max_gpa) *max_gpa = g;
    *sum += g;
    (*count)++;

    collect_gpa_stats(node->right, min_gpa, max_gpa, sum, count);
}

/*
 * count_above_below — In-order pass that increments *above for every student
 * whose GPA strictly exceeds avg, and *below for every student strictly below.
 * Students whose GPA equals avg are counted in neither bucket.
 */
static void count_above_below(BSTNode *node, float avg,
                               int *above, int *below)
{
    if (!node) return;
    count_above_below(node->left, avg, above, below);
    if      (node->data.gpa > avg) (*above)++;
    else if (node->data.gpa < avg) (*below)++;
    count_above_below(node->right, avg, above, below);
}

/*
 * free_all_nodes — Post-order traversal that frees every node. Does NOT free
 * the BST wrapper struct; used by bst_delete_all to reset the tree in place.
 */
static void free_all_nodes(BSTNode *node)
{
    if (!node) return;
    free_all_nodes(node->left);
    free_all_nodes(node->right);
    free(node);
}

/* ═══════════════════════════════════════════════════════════════════════════
 * Public API — Lifecycle
 * ═══════════════════════════════════════════════════════════════════════════ */

/*
 * bst_create — Allocates and zero-initialises a new BST wrapper struct.
 * Exits the program with an error message on allocation failure.
 */
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

/*
 * bst_destroy — Frees every node via post-order traversal, resets the struct
 * fields, then frees the BST wrapper itself. Safe to call on an empty tree.
 */
void bst_destroy(BST *tree)
{
    if (!tree) return;
    destroy_recursive(tree->root);
    tree->root  = NULL;
    tree->count = 0;
    free(tree);
}

/* ═══════════════════════════════════════════════════════════════════════════
 * Public API — Core Operations
 * ═══════════════════════════════════════════════════════════════════════════ */

/*
 * bst_insert — Validates gpa range, then delegates to insert_recursive.
 * Prints an error and returns an error code on duplicate id or invalid gpa;
 * increments tree->count only on success.
 */
int bst_insert(BST *tree, int id, const char *name, float gpa)
{
    if (!tree) return -1;

    if (gpa < 0.0f || gpa > 4.0f) {
        fprintf(stderr,
                "Error: GPA %.2f is outside the valid range [0.0, 4.0].\n",
                gpa);
        return -2;
    }

    int result = 0;
    tree->root = insert_recursive(tree->root, id, name, gpa, &result);

    if (result == -1) {
        fprintf(stderr,
                "Error: Student with ID %d already exists.\n", id);
        return -1;
    }

    tree->count++;
    return 0;
}

/*
 * bst_search_by_id — Delegates to the recursive BST search helper.
 * Returns the matching BSTNode pointer, or NULL if not found.
 */
BSTNode *bst_search_by_id(BST *tree, int id)
{
    if (!tree) return NULL;
    return search_by_id_recursive(tree->root, id);
}

/*
 * bst_search_by_name — Delegates to the in-order name search helper.
 * The search is case-insensitive; returns the first match or NULL.
 */
BSTNode *bst_search_by_name(BST *tree, const char *name)
{
    if (!tree || !name) return NULL;
    return search_by_name_inorder(tree->root, name);
}

/*
 * bst_delete_by_id — Delegates to delete_recursive, prints "not found" if
 * the id is absent, and decrements tree->count on successful deletion.
 */
int bst_delete_by_id(BST *tree, int id)
{
    if (!tree) return -1;

    int result = 0;
    tree->root = delete_recursive(tree->root, id, &result);

    if (result == -1) {
        printf("Error: Student with ID %d not found.\n", id);
        return -1;
    }

    tree->count--;
    return 0;
}

/*
 * bst_update — Locates a student by id and presents an interactive sub-menu
 * allowing the user to update Name, GPA, or ID until they choose "Done".
 * Updating the ID performs a delete-then-reinsert; the old record is restored
 * automatically if the requested new id is already taken.
 */
int bst_update(BST *tree, int id)
{
    if (!tree) return -1;

    BSTNode *node = bst_search_by_id(tree, id);
    if (!node) {
        printf("Error: Student with ID %d not found.\n", id);
        return -1;
    }

    char buf[256];
    int  choice = 0;

    do {
        /* node may be NULL only transiently during the ID-update case;
         * it is always restored before the next iteration header is printed. */
        printf("\n--- Update Student (ID: %d, Name: %s) ---\n",
               node->data.id, node->data.name);
        printf(" [1] Update Name\n");
        printf(" [2] Update GPA\n");
        printf(" [3] Update ID\n");
        printf(" [4] Done\n");
        printf("Enter choice: ");

        if (!fgets(buf, sizeof(buf), stdin)) {
            choice = 4; /* Treat EOF as "Done". */
            break;
        }

        if (sscanf(buf, "%d", &choice) != 1) {
            printf("Invalid input. Please enter a number between 1 and 4.\n");
            choice = 0;
            continue;
        }

        switch (choice) {

            case 1: {
                /* ── Update Name ── */
                printf("Enter new name: ");
                if (!fgets(buf, sizeof(buf), stdin)) break;
                buf[strcspn(buf, "\n")] = '\0'; /* strip trailing newline */

                if (strlen(buf) == 0) {
                    printf("Error: Name cannot be empty.\n");
                } else {
                    strncpy(node->data.name, buf,
                            sizeof(node->data.name) - 1);
                    node->data.name[sizeof(node->data.name) - 1] = '\0';
                    printf("Name updated successfully.\n");
                }
                break;
            }

            case 2: {
                /* ── Update GPA ── */
                printf("Enter new GPA (0.0 - 4.0): ");
                if (!fgets(buf, sizeof(buf), stdin)) break;
                float new_gpa = 0.0f;
                if (sscanf(buf, "%f", &new_gpa) != 1) {
                    printf("Invalid GPA. Please enter a decimal number.\n");
                } else if (new_gpa < 0.0f || new_gpa > 4.0f) {
                    printf("Error: GPA must be in the range [0.0, 4.0].\n");
                } else {
                    node->data.gpa = new_gpa;
                    printf("GPA updated to %.2f successfully.\n", new_gpa);
                }
                break;
            }

            case 3: {
                /* ── Update ID ── */
                printf("Enter new ID: ");
                if (!fgets(buf, sizeof(buf), stdin)) break;
                int new_id = 0;
                if (sscanf(buf, "%d", &new_id) != 1) {
                    printf("Invalid ID. Please enter an integer.\n");
                    break;
                }
                if (new_id == id) {
                    printf("New ID is identical to the current ID."
                           " No change made.\n");
                    break;
                }

                /* Capture existing data before any structural changes. */
                char  saved_name[100];
                float saved_gpa = node->data.gpa;
                strncpy(saved_name, node->data.name,
                        sizeof(saved_name) - 1);
                saved_name[sizeof(saved_name) - 1] = '\0';

                /* Step 1: remove the old node (node ptr becomes dangling). */
                bst_delete_by_id(tree, id);
                node = NULL; /* Explicitly mark as dangling. */

                /* Step 2: attempt to insert under the new id. */
                int ins = bst_insert(tree, new_id, saved_name, saved_gpa);
                if (ins != 0) {
                    /*
                     * Insert failed (new_id already exists).
                     * bst_insert already printed an error.
                     * Restore the original record.
                     */
                    printf("Restoring original record with ID %d.\n", id);
                    bst_insert(tree, id, saved_name, saved_gpa);
                    node = bst_search_by_id(tree, id);
                } else {
                    printf("ID updated from %d to %d successfully.\n",
                           id, new_id);
                    id   = new_id;
                    node = bst_search_by_id(tree, new_id);
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

    } while (choice != 4);

    return 0;
}

/* ═══════════════════════════════════════════════════════════════════════════
 * Public API — Traversal and Display
 * ═══════════════════════════════════════════════════════════════════════════ */

/*
 * bst_display_all — In-order traversal that prints every student record on a
 * single formatted line. Prints "No students found." when the tree is empty.
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
    inorder_print(tree->root);
}

/* ═══════════════════════════════════════════════════════════════════════════
 * Public API — Statistics
 * ═══════════════════════════════════════════════════════════════════════════ */

/*
 * bst_tree_stats — Computes and prints the height of the BST (using
 * compute_height) and the live student count stored in tree->count.
 */
void bst_tree_stats(BST *tree)
{
    if (!tree) return;
    int height = compute_height(tree->root);
    printf("\nTree Height    : %d\n", height);
    printf("Total Students : %d\n",  tree->count);
}

/*
 * bst_gpa_stats — Collects GPA min/max/sum in one traversal, computes the
 * average, then performs a second traversal to count students strictly above
 * and strictly below the average. Prints all five statistics.
 */
void bst_gpa_stats(BST *tree)
{
    if (!tree || !tree->root) {
        printf("No students found.\n");
        return;
    }

    float min_gpa = 4.0f;
    float max_gpa = 0.0f;
    float sum     = 0.0f;
    int   count   = 0;

    collect_gpa_stats(tree->root, &min_gpa, &max_gpa, &sum, &count);

    float avg = (count > 0) ? (sum / (float)count) : 0.0f;

    int above = 0;
    int below = 0;
    count_above_below(tree->root, avg, &above, &below);

    printf("\n--- GPA Statistics ---\n");
    printf("Minimum GPA        : %.2f\n", min_gpa);
    printf("Maximum GPA        : %.2f\n", max_gpa);
    printf("Average GPA        : %.2f\n", avg);
    printf("Students above avg : %d\n",   above);
    printf("Students below avg : %d\n",   below);
}

/* ═══════════════════════════════════════════════════════════════════════════
 * Public API — Bulk Deletion
 * ═══════════════════════════════════════════════════════════════════════════ */

/*
 * bst_delete_all — Frees every node via post-order traversal without freeing
 * the BST wrapper struct, then resets root and count and confirms to the user.
 */
void bst_delete_all(BST *tree)
{
    if (!tree) return;
    free_all_nodes(tree->root);
    tree->root  = NULL;
    tree->count = 0;
    printf("All student records have been deleted.\n");
}

/* ═══════════════════════════════════════════════════════════════════════════
 * Public API — Predecessor and Successor
 * ═══════════════════════════════════════════════════════════════════════════ */

/*
 * bst_predecessor_successor — Verifies the id exists, then makes two
 * independent iterative passes from the root to find the predecessor
 * (largest id < given id) and the successor (smallest id > given id).
 *
 * Predecessor pass: when the current node's id is LESS THAN the target,
 *   record it as a candidate and go RIGHT to look for a larger candidate.
 *   When >= target, go LEFT.
 *
 * Successor pass: when the current node's id is GREATER THAN the target,
 *   record it as a candidate and go LEFT to look for a smaller candidate.
 *   When <= target, go RIGHT.
 */
void bst_predecessor_successor(BST *tree, int id)
{
    if (!tree) return;

    if (!bst_search_by_id(tree, id)) {
        printf("Student ID %d not found.\n", id);
        return;
    }

    BSTNode *pred = NULL;
    BSTNode *succ = NULL;
    BSTNode *curr = NULL;

    /* ── Find predecessor ── */
    curr = tree->root;
    while (curr) {
        if (curr->data.id < id) {
            pred = curr;        /* Best candidate so far; look for a larger one. */
            curr = curr->right;
        } else {
            curr = curr->left;  /* Current id >= target; go left.               */
        }
    }

    /* ── Find successor ── */
    curr = tree->root;
    while (curr) {
        if (curr->data.id > id) {
            succ = curr;        /* Best candidate so far; look for a smaller one. */
            curr = curr->left;
        } else {
            curr = curr->right; /* Current id <= target; go right.               */
        }
    }

    /* ── Print results ── */
    if (pred) {
        printf("Predecessor: ID=%d, Name=%s, GPA=%.2f\n",
               pred->data.id, pred->data.name, pred->data.gpa);
    } else {
        printf("%d is the first record — no predecessor.\n", id);
    }

    if (succ) {
        printf("Successor  : ID=%d, Name=%s, GPA=%.2f\n",
               succ->data.id, succ->data.name, succ->data.gpa);
    } else {
        printf("%d is the last record — no successor.\n", id);
    }
}
