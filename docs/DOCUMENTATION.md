# Student Management System — Technical Documentation

---

## 1. Project Overview

The Student Management System (SMS) is a console application written in C that lets users
maintain a collection of student records—each identified by a unique integer ID, a name, and
a GPA—through an interactive menu.  The application supports inserting, searching, updating,
and deleting records, as well as computing statistical summaries of the stored data.

A **Binary Search Tree (BST)** was chosen as the underlying data structure because student
records need to be retrieved by their numeric ID.  A BST organises nodes so that every left
descendant holds a smaller ID and every right descendant holds a larger ID, enabling O(log n)
average-case lookup, insertion, and deletion on a balanced tree—far superior to the O(n) cost
of a linear scan over an array or linked list.  The tree is traversed in-order (left → root →
right) whenever sorted output is required, naturally producing records in ascending-ID order
without an extra sort step.

---

## 2. Data Structures Used

### `Student`

```c
typedef struct Student {
    int   id;        /* unique integer key                   */
    char  name[100]; /* student's full name, null-terminated */
    float gpa;       /* grade point average, in [0.0, 4.0]  */
} Student;
```

| Field  | Type        | Purpose |
|--------|-------------|---------|
| `id`   | `int`       | The BST key.  Every comparison that governs left/right navigation is performed on this field.  Must be unique across all records. |
| `name` | `char[100]` | Human-readable identifier stored inline (no heap allocation needed for the string itself), sized to hold typical full names with room to spare. |
| `gpa`  | `float`     | Single-precision floating-point GPA.  The application enforces the academic range `[0.0, 4.0]` at every input boundary. |

### `BSTNode`

```c
typedef struct BSTNode {
    Student        data;
    struct BSTNode *left;
    struct BSTNode *right;
} BSTNode;
```

Each node owns one `Student` record (copied by value, no extra indirection) and two child
pointers.  The **key** of a node is `data.id`.  The BST invariant guarantees:

- Every node in the *left* subtree of N has `data.id < N->data.id`.
- Every node in the *right* subtree of N has `data.id > N->data.id`.

Self-referential pointers (`struct BSTNode *`) are necessary because C requires a complete type
for value members but allows pointers to incomplete types.

### `BST`

```c
typedef struct BST {
    BSTNode *root;
    int      count;
} BST;
```

The wrapper struct decouples the *concept* of "the tree" from "a pointer to its root", giving
callers a stable handle even as the root pointer changes during rebalancing or deletion.
`count` is maintained as a running tally so that callers can query the number of students in
O(1) rather than traversing the whole tree.

### Sample BST (5 nodes, inserted in order 50 → 30 → 70 → 20 → 40)

```
              50          ← root
             /  \
            30   70
           /  \
          20   40
```

- An in-order traversal visits: **20 → 30 → 40 → 50 → 70** (ascending by ID).
- Height = 3 (longest path from root to a leaf: 50 → 30 → 20 or 50 → 30 → 40).
- `count` = 5.

---

## 3. Function Reference

---

### `BST *bst_create(void)`

**Purpose**

Bootstraps an empty Binary Search Tree.  It allocates the `BST` wrapper on the heap,
initialises `root` to `NULL` and `count` to `0`, and returns a pointer to the caller.
Separating allocation from use allows the caller to receive a fully-initialised object
with a single call, preventing unintialised-field bugs.

**Algorithm / Logic**

1. Call `malloc(sizeof(BST))`.
2. If `malloc` returns `NULL`, write a fatal error message to `stderr` and call `exit(EXIT_FAILURE)`—the program cannot operate without the tree struct.
3. Set `tree->root = NULL` and `tree->count = 0`.
4. Return `tree`.

**Edge Cases Handled**

- **Allocation failure**: The function never returns `NULL`.  Rather than placing the
  burden of null-checking on every call site, it terminates the process immediately with
  a diagnostic message, which is appropriate for a system-critical object.

**Time Complexity**

O(1) — a single fixed-size allocation and two field assignments.

**Space Complexity**

O(1) — `sizeof(BST)` bytes allocated on the heap.

**Example**

```c
BST *tree = bst_create();
/* tree->root  == NULL */
/* tree->count == 0    */
```

---

### `int bst_insert(BST *tree, int id, const char *name, float gpa)`

**Purpose**

Validates the new student's data and inserts it into the correct BST position determined
by `id`.  Duplicate IDs and out-of-range GPAs are rejected before any structural change
is made, keeping the tree in a consistent state at all times.

**Algorithm / Logic**

1. Return `-1` immediately if `tree` is `NULL` (safety guard).
2. Check `gpa < 0.0f || gpa > 4.0f`.  If true, print an error to `stderr` and return `-2`.
3. Declare `int result = 0`.
4. Call the static recursive helper `insert_recursive(tree->root, id, name, gpa, &result)` and assign its return value back to `tree->root`.  This assignment is what attaches a newly-created root node to an empty tree.
5. Inside `insert_recursive`:
   - **Base case**: `node == NULL` — allocate a new node via `create_node`, set `*result = 0`, and return the new node.
   - **Go left**: `id < node->data.id` — recurse on `node->left` and re-assign `node->left` to the return value.
   - **Go right**: `id > node->data.id` — recurse on `node->right` and re-assign `node->right` to the return value.
   - **Duplicate**: `id == node->data.id` — set `*result = -1` and return `node` unchanged.
6. Back in `bst_insert`: if `result == -1`, print an error and return `-1`.
7. Otherwise, increment `tree->count` and return `0`.

**Recursive trace** (inserting ID=25 into a tree that has root=30, left=20):

```
insert_recursive(30, 25) → 25 < 30, recurse left
  insert_recursive(20, 25) → 25 > 20, recurse right
    insert_recursive(NULL, 25) → create node(25), *result=0, return node25
  20->right = node25; return node20
30->left = node20 (unchanged); return node30
tree->root = node30 (unchanged)
```

**Edge Cases Handled**

- `tree == NULL` → returns `-1` without dereferencing.
- `gpa` outside `[0.0, 4.0]` → returns `-2` with an error message; no node created.
- Duplicate `id` → returns `-1` with an error message; tree unchanged.
- Empty tree → the new node becomes the root via the assignment `tree->root = insert_recursive(NULL, ...)`.

**Time Complexity**

| Case | Complexity | Reason |
|------|-----------|--------|
| Average (balanced) | O(log n) | Tree height ≈ log₂ n; one comparison per level |
| Worst (skewed) | O(n) | IDs inserted in sorted order produce a linear chain |

**Space Complexity**

O(log n) average / O(n) worst — the recursion stack mirrors the tree height.

**Example**

```
Before: empty tree
Insert(50, "Alice", 3.8)
After:
    50 (root)

Insert(30, "Bob", 3.1)
After:
    50
   /
  30
```

---

### `BSTNode *bst_search_by_id(BST *tree, int id)`

**Purpose**

Locates a student record by its unique integer ID using the BST ordering property.
Because IDs are the BST key, each comparison eliminates half the remaining tree (on
average), delivering fast lookup without visiting every node.

**Algorithm / Logic**

1. Return `NULL` if `tree` is `NULL`.
2. Delegate to `search_by_id_recursive(tree->root, id)`.
3. Inside the recursive helper:
   - **Base case**: `node == NULL` — not found, return `NULL`.
   - `id == node->data.id` — match found, return `node`.
   - `id < node->data.id` — recurse on `node->left`.
   - `id > node->data.id` — recurse on `node->right`.

**Recursive trace** (searching ID=20 in tree {30, 20, 40}):

```
search(30, 20) → 20 < 30, recurse left
  search(20, 20) → 20 == 20, return node20  ✓
```

**Edge Cases Handled**

- `tree == NULL` → returns `NULL` safely.
- Empty tree (`root == NULL`) → base case returns `NULL` immediately.
- ID not present → eventually reaches a `NULL` child and returns `NULL`.

**Time Complexity**

O(log n) average, O(n) worst (skewed tree).

**Space Complexity**

O(log n) average / O(n) worst — recursion stack depth equals tree height.

**Example**

```c
BSTNode *node = bst_search_by_id(tree, 101);
if (node) printf("Found: %s\n", node->data.name);
```

---

### `BSTNode *bst_search_by_name(BST *tree, const char *name)`

**Purpose**

Finds a student by name rather than ID.  Because `name` is **not** the BST key, there
is no ordering property to exploit; the entire tree must be visited.  An in-order
traversal is used for logical consistency (results in ascending-ID visit order), and the
comparison is case-insensitive so that `"alice"` matches `"Alice"`.

**Algorithm / Logic**

1. Return `NULL` if `tree` or `name` is `NULL`.
2. Delegate to `search_by_name_inorder(tree->root, name)`.
3. Inside the recursive helper:
   - **Base case**: `node == NULL` → return `NULL`.
   - Recurse on `node->left`; if the result is non-`NULL`, return it immediately (first match wins).
   - Call `str_case_equal(node->data.name, name)`.  If equal, return `node`.
   - Recurse on `node->right` and return its result.
4. `str_case_equal` walks both strings character-by-character comparing `tolower()` of each byte, returning `1` only if every character matches and both strings terminate together.

**Edge Cases Handled**

- `NULL` tree or `NULL` name → returns `NULL` safely.
- Empty tree → base case fires immediately.
- Name not found → returns `NULL` after visiting all nodes.
- Case mismatch → `str_case_equal` normalises both sides with `tolower`.
- Multiple matches — returns the **first** match encountered in in-order (smallest ID) order.

**Time Complexity**

O(n) in all cases — every node must be visited because name is not the sort key.

**Space Complexity**

O(log n) average / O(n) worst — recursion stack depth equals tree height.

**Example**

```c
/* Tree has: {101,"Alice Smith",3.8}, {99,"Carla Diaz",3.5} */
BSTNode *n = bst_search_by_name(tree, "alice smith");
/* Returns node with ID=101 despite the different capitalisation. */
```

---

### `int bst_delete_by_id(BST *tree, int id)`

**Purpose**

Removes the node whose `id` matches, maintaining the BST invariant after deletion.
Three structural cases must be handled correctly to keep the remaining nodes properly
connected; failure to handle any case would corrupt the tree.

**Algorithm / Logic**

1. Return `-1` if `tree` is `NULL`.
2. Declare `int result = 0`.
3. Call `delete_recursive(tree->root, id, &result)` and assign the return value back to `tree->root`.
4. Inside `delete_recursive`:
   - **Not found** (`node == NULL`): set `*result = -1`, return `NULL`.
   - `id < node->data.id`: recurse left, reassign `node->left`, return `node`.
   - `id > node->data.id`: recurse right, reassign `node->right`, return `node`.
   - **Found** (`id == node->data.id`): set `*result = 0` and apply one of the three cases (see §5).
5. Back in `bst_delete_by_id`: if `result == -1`, print "not found" and return `-1`; otherwise decrement `tree->count` and return `0`.

**Edge Cases Handled**

- `tree == NULL` → safe return.
- ID not in tree → `result == -1`, "not found" message, count unchanged.
- Leaf node (no children) → freed, parent's child pointer set to `NULL`.
- One child → parent links directly to the surviving child.
- Two children → in-order successor (leftmost of right subtree) replaces the node's data; successor deleted from right subtree.

**Time Complexity**

O(log n) average, O(n) worst.  Finding the successor (an extra descent) does not change the asymptotic class.

**Space Complexity**

O(log n) average / O(n) worst — recursion stack.

**Example**

```
Before: delete ID=30 (two children)
      50
     /  \
    30   70
   /  \
  20   40

Step 1: successor of 30 = leftmost of {40} = 40
Step 2: copy 40's data into node-30's slot
Step 3: delete 40 from right subtree (it is a leaf)

After:
      50
     /  \
    40   70
   /
  20
```

---

### `int bst_update(BST *tree, int id)`

**Purpose**

Provides an interactive sub-menu that allows the operator to modify any field of an
existing student record.  Name and GPA can be updated in place; updating the ID requires
a delete-then-reinsert cycle because the BST position depends on the key value.

**Algorithm / Logic**

1. Search for the node via `bst_search_by_id`.  If `NULL`, print "not found" and return `-1`.
2. Enter a `do-while` loop that continues until `choice == 4` (Done).
3. Each iteration: print the sub-menu showing the current ID and name, read `choice` with `fgets` + `sscanf`.
   - **[1] Update Name**: call `fgets`, strip the newline, validate non-empty, copy with `strncpy` into `node->data.name`.
   - **[2] Update GPA**: call `fgets` + `sscanf` for a float, validate `[0.0, 4.0]`, assign to `node->data.gpa`.
   - **[3] Update ID**:
     1. Read the new ID.
     2. Capture existing `name` and `gpa` from the node into local variables.
     3. Call `bst_delete_by_id(tree, id)` — the `node` pointer is now dangling; set `node = NULL`.
     4. Call `bst_insert(tree, new_id, saved_name, saved_gpa)`.
     5. On success: update the local `id` and `node` variables; the sub-menu header will reflect the new values on the next iteration.
     6. On failure (new_id already exists): re-insert under the original `id`; re-fetch `node`.
   - **[4] Done**: print confirmation; the `while` condition exits the loop.
4. Return `0`.

**Edge Cases Handled**

- Node not found → returns `-1` before entering the loop.
- Empty name string → rejected with an error message.
- GPA out of range → rejected; existing GPA unchanged.
- New ID equals current ID → no-op with informational message.
- New ID conflicts with existing record → original record restored, tree unchanged.
- EOF on `fgets` → treated as "Done" (`choice = 4`) to avoid infinite loop.

**Time Complexity**

O(log n) average per sub-menu operation (dominated by BST search/insert/delete).

**Space Complexity**

O(log n) average for the initial search; O(1) for the update loop itself.

**Example**

```
Student: ID=101, Name="Alice", GPA=3.8
User selects [2] Update GPA → enters 3.95
→ GPA updated to 3.95.

User selects [3] Update ID → enters 200
→ Node 101 deleted; node 200 inserted with same name/GPA.
```

---

### `void bst_display_all(BST *tree)`

**Purpose**

Prints every student record in ascending ID order by leveraging the in-order BST
traversal property.  No sorting is needed; visiting left → root → right naturally yields
nodes in sorted key order.

**Algorithm / Logic**

1. If `tree` is `NULL` or `tree->root` is `NULL`, print "No students found." and return.
2. Print a column-header line.
3. Call `inorder_print(tree->root)`.
4. Inside `inorder_print`:
   - Base case: `node == NULL` → return.
   - Recurse left.
   - `printf` the current node's `id`, `name`, `gpa`.
   - Recurse right.

**Edge Cases Handled**

- `NULL` tree or empty tree → prints "No students found." instead of crashing.

**Time Complexity**

O(n) — every node is visited exactly once.

**Space Complexity**

O(log n) average / O(n) worst — recursion stack depth equals tree height.

**Example**

```
Tree contains IDs 101, 99, 102 (inserted in that order).
Output (in-order):
ID: 99      |  Name: Carla Diaz                      |  GPA: 3.50
ID: 101     |  Name: Alice Smith                     |  GPA: 3.85
ID: 102     |  Name: Bob Jones                       |  GPA: 2.90
```

---

### `void bst_tree_stats(BST *tree)`

**Purpose**

Reports two tree-level metrics useful for understanding the structure of the BST: its
height (a proxy for worst-case operation cost) and the live student count.

**Algorithm / Logic**

1. Guard against `NULL` `tree`.
2. Call `compute_height(tree->root)` and store the result.
3. Print `Tree Height` and `Total Students` (the pre-maintained `tree->count`).

**Height formula (recursive)**

```
height(NULL) = 0
height(node) = 1 + max(height(node->left), height(node->right))
```

The base case assigns height 0 to every absent child, so a leaf node returns
`1 + max(0, 0) = 1`.  The formula propagates upward, always taking the longer branch.

**Height trace** on tree {50, 30, 70, 20, 40}:

```
height(20) = 1 + max(0,0) = 1
height(40) = 1 + max(0,0) = 1
height(30) = 1 + max(1,1) = 2
height(70) = 1 + max(0,0) = 1
height(50) = 1 + max(2,1) = 3   ← tree height
```

**Edge Cases Handled**

- Empty tree (`root == NULL`) → `compute_height(NULL)` returns 0 immediately; height is reported as 0.

**Time Complexity**

O(n) — every node is visited to compute the height.

**Space Complexity**

O(log n) average / O(n) worst — recursion stack.

**Example**

```
Tree: {50, 30, 70, 20, 40}
Output:
Tree Height    : 3
Total Students : 5
```

---

### `void bst_gpa_stats(BST *tree)`

**Purpose**

Provides a five-number statistical summary of the GPA distribution: minimum, maximum,
arithmetic mean, count of students above average, and count below average.  Two
traversal passes are used: one to gather the raw values and one to categorise students
relative to the computed average.

**Algorithm / Logic**

1. Guard against `NULL` or empty tree.
2. Initialise `min_gpa = 4.0f`, `max_gpa = 0.0f`, `sum = 0.0f`, `count = 0`.
3. **Pass 1** — `collect_gpa_stats` (in-order):
   - Update `min_gpa` if current GPA is smaller.
   - Update `max_gpa` if current GPA is larger.
   - Add current GPA to `sum`; increment `count`.
4. Compute `avg = sum / count`.
5. **Pass 2** — `count_above_below` (in-order):
   - Increment `above` if `gpa > avg`, increment `below` if `gpa < avg`.
   - Students with `gpa == avg` contribute to neither bucket.
6. Print all five statistics.

**Edge Cases Handled**

- Empty tree → prints "No students found." and returns.
- All students share the same GPA → `above = 0`, `below = 0`; min, max, and avg are all equal.
- Single student → min = max = avg = that student's GPA; above = 0, below = 0.
- `min_gpa` initialised to `4.0f` safely handles the edge case where all GPAs equal 0.0
  (the first comparison `0.0 < 4.0` fires and sets `min_gpa = 0.0`).

**Time Complexity**

O(n) — two full traversals, each visiting every node once; O(2n) = O(n).

**Space Complexity**

O(log n) average / O(n) worst — recursion stack shared by both traversals.

**Example**

```
Students: {3.85, 2.90, 3.50}
Pass 1: min=2.90, max=3.85, sum=10.25, count=3 → avg=3.42
Pass 2: 3.85 > 3.42 → above++; 2.90 < 3.42 → below++; 3.50 > 3.42 → above++

Output:
Minimum GPA        : 2.90
Maximum GPA        : 3.85
Average GPA        : 3.42
Students above avg : 2
Students below avg : 1
```

---

### `void bst_delete_all(BST *tree)`

**Purpose**

Erases every student record and resets the tree to an empty state while keeping the
`BST` wrapper struct intact.  This differs from `bst_destroy` in that the caller retains
a valid, reusable `BST *` after the call.

**Algorithm / Logic**

1. Guard against `NULL` `tree`.
2. Call `free_all_nodes(tree->root)` — a post-order recursive traversal that frees left, then right, then the current node.
3. Assign `tree->root = NULL` and `tree->count = 0`.
4. Print the confirmation message.

**Why post-order?** A node must not be freed before its children; otherwise, the child
pointers become unreachable and the child memory leaks.  Post-order (children first, then
parent) guarantees every node is freed.

**Edge Cases Handled**

- `NULL` tree → no-op.
- Empty tree (`root == NULL`) → `free_all_nodes(NULL)` returns immediately; still prints the confirmation.

**Time Complexity**

O(n) — every node is visited and freed exactly once.

**Space Complexity**

O(log n) average / O(n) worst — recursion stack.

**Example**

```
Before: tree has 5 nodes, count=5
bst_delete_all(tree)
→ prints "All student records have been deleted."
After: tree->root == NULL, tree->count == 0
```

---

### `void bst_predecessor_successor(BST *tree, int id)`

**Purpose**

Finds and prints the **predecessor** (the student whose ID is the largest ID strictly
less than `id`) and the **successor** (the student whose ID is the smallest ID strictly
greater than `id`).  An iterative, traversal-based algorithm is used (see §6 for a
detailed walkthrough).

**Algorithm / Logic**

1. Guard against `NULL` tree.
2. Verify `id` exists with `bst_search_by_id`; print "not found" and return if absent.
3. **Predecessor pass** — initialise `pred = NULL`, traverse from root:
   - If `curr->data.id < id`: `pred = curr` (new best candidate), go right.
   - Otherwise: go left.
4. **Successor pass** — initialise `succ = NULL`, traverse from root:
   - If `curr->data.id > id`: `succ = curr` (new best candidate), go left.
   - Otherwise: go right.
5. Print predecessor (or "first record" message) and successor (or "last record" message).

**Edge Cases Handled**

- ID not in tree → prints "Student ID X not found." and returns.
- `id` is the minimum in the tree → `pred` remains `NULL`; prints "X is the first record — no predecessor."
- `id` is the maximum in the tree → `succ` remains `NULL`; prints "X is the last record — no successor."
- Single-node tree → both predecessor and successor are absent.

**Time Complexity**

O(log n) average, O(n) worst — each iterative pass descends from root to the target depth.

**Space Complexity**

O(1) — fully iterative; no recursion stack.

**Example**

```
Tree: {99, 101, 102}
bst_predecessor_successor(tree, 101)
→ Predecessor: ID=99, Name=Carla Diaz, GPA=3.50
→ Successor  : ID=102, Name=Bob Jones, GPA=2.90
```

---

### `void bst_destroy(BST *tree)`

**Purpose**

Performs a complete teardown: frees every node in the tree and then frees the `BST`
wrapper struct itself.  Must be called exactly once per `bst_create` call to prevent
memory leaks.

**Algorithm / Logic**

1. Guard against `NULL` `tree`.
2. Call `destroy_recursive(tree->root)` — identical post-order traversal to `free_all_nodes`, freeing every `BSTNode`.
3. Set `tree->root = NULL` and `tree->count = 0` (defensive zeroing before freeing the struct).
4. Call `free(tree)`.

**Edge Cases Handled**

- `NULL` tree → no-op; avoids a double-free or invalid-free crash.
- Empty tree (`root == NULL`) → `destroy_recursive(NULL)` returns immediately; `free(tree)` still frees the wrapper.

**Time Complexity**

O(n) — every node is visited once.

**Space Complexity**

O(log n) average / O(n) worst — recursion stack for the node traversal.

**Example**

```c
BST *tree = bst_create();
bst_insert(tree, 1, "Test", 3.0f);
/* ... use tree ... */
bst_destroy(tree);
/* tree is now a dangling pointer; should not be used again. */
```

---

## 4. Complexity Summary Table

| Function | Best Case | Average Case | Worst Case | Space |
|---|---|---|---|---|
| `bst_create` | O(1) | O(1) | O(1) | O(1) |
| `bst_insert` | O(1) | O(log n) | O(n) | O(log n) / O(n) |
| `bst_search_by_id` | O(1) | O(log n) | O(n) | O(log n) / O(n) |
| `bst_search_by_name` | O(1)* | O(n) | O(n) | O(log n) / O(n) |
| `bst_delete_by_id` | O(log n) | O(log n) | O(n) | O(log n) / O(n) |
| `bst_update` | O(log n) | O(log n) | O(n) | O(log n) / O(n) |
| `bst_display_all` | O(n) | O(n) | O(n) | O(log n) / O(n) |
| `bst_tree_stats` | O(n) | O(n) | O(n) | O(log n) / O(n) |
| `bst_gpa_stats` | O(n) | O(n) | O(n) | O(log n) / O(n) |
| `bst_delete_all` | O(n) | O(n) | O(n) | O(log n) / O(n) |
| `bst_predecessor_successor` | O(1) | O(log n) | O(n) | O(1) |
| `bst_destroy` | O(n) | O(n) | O(n) | O(log n) / O(n) |

> **Space column** format: *average-case stack / worst-case stack*.  All O(1) space
> entries are for functions that do not recurse.
>
> \* `bst_search_by_name` best case O(1) occurs only if the root matches the query and
> the search terminates immediately.

---

## 5. BST Deletion — Deep Dive

Deletion is the most structurally complex BST operation because it must handle three
fundamentally different situations.  All three cases are handled inside the
`delete_recursive` static helper.

### Sample tree used in all three examples

```
          40
         /  \
        20   60
       /    /  \
      10   50   70
```

---

### Case 1 — Leaf Node (no children)

**Target: delete ID = 70**

70 has no children.  It can be freed directly, and its parent's pointer to it is set to
`NULL` by returning `NULL` from the recursive call.

```
Before:                    After:
          40                         40
         /  \                       /  \
        20   60                    20   60
       /    /  \                  /    /
      10   50   70               10   50
```

Code path in `delete_recursive`:
```c
if (!node->left && !node->right) {
    free(node);
    return NULL;   /* parent assigns this to node->right (60->right) */
}
```

---

### Case 2 — One Child

**Target: delete ID = 20** (has only a left child, 10)

The node to delete is replaced by its single child.  The parent is re-linked to the
surviving child, and the deleted node is freed.

```
Before:                    After:
          40                         40
         /  \                       /  \
        20   60                    10   60
       /    /  \                       /  \
      10   50   70                    50   70
```

Code path in `delete_recursive`:
```c
} else if (!node->right) {
    BSTNode *child = node->left;   /* child = node(10) */
    free(node);                    /* free node(20)    */
    return child;                  /* parent assigns this to 40->left */
}
```

---

### Case 3 — Two Children

**Target: delete ID = 40 (root)** (has both children)

The node cannot simply be removed because two subtrees must remain connected.  The
algorithm replaces the node's **data** with its *in-order successor* (the leftmost node
in the right subtree), then deletes the successor from its original position (which is
always a Case 1 or Case 2 deletion, never Case 3 again).

```
In-order successor of 40 = leftmost node of right subtree {60, 50, 70} = 50.
```

```
Step 1 — Copy 50's data into node(40):
          50*             ← data overwritten, but old node(50) still exists below
         /  \
        20   60
       /    /  \
      10   50   70

Step 2 — Delete original node(50) from right subtree (leaf → Case 1):
          50
         /  \
        20   60
       /      \
      10       70
```

Code path in `delete_recursive`:
```c
BSTNode *successor = find_min_node(node->right);  /* node(50) */
int      succ_id   = successor->data.id;           /* 50       */
node->data         = successor->data;              /* overwrite */
int dummy          = 0;
node->right = delete_recursive(node->right, succ_id, &dummy);
```

**Why the successor is always safe to delete recursively:**
The in-order successor is the leftmost node of the right subtree, so it can have **at
most one child** (a right child).  This means its deletion is always Case 1 or Case 2,
never Case 3, preventing infinite recursion.

---

## 6. Predecessor and Successor — Deep Dive

### Sample tree (6 nodes, inserted: 40 → 20 → 60 → 10 → 30 → 50)

```
            40
           /  \
          20   60
         /  \ /
        10  30 50
```

In-order sequence: **10 → 20 → 30 → 40 → 50 → 60**

### Worked Example: `bst_predecessor_successor(tree, 40)`

---

**Predecessor pass** (target = 40, looking for largest id < 40):

```
curr = 40
  40 < 40? NO  → go left.  pred = (unchanged) NULL.
curr = 20
  20 < 40? YES → pred = node(20), go right.
curr = 30
  30 < 40? YES → pred = node(30), go right.
curr = NULL → stop.

pred = node(30)  ✓ (30 is the largest id strictly less than 40)
```

**Successor pass** (target = 40, looking for smallest id > 40):

```
curr = 40
  40 > 40? NO  → go right.  succ = (unchanged) NULL.
curr = 60
  60 > 40? YES → succ = node(60), go left.
curr = 50
  50 > 40? YES → succ = node(50), go left.
curr = NULL → stop.

succ = node(50)  ✓ (50 is the smallest id strictly greater than 40)
```

**Output:**
```
Predecessor: ID=30, Name=..., GPA=...
Successor  : ID=50, Name=..., GPA=...
```

---

### Edge case: `bst_predecessor_successor(tree, 10)` (first record)

**Predecessor pass:**
```
curr=40: 10 < 40? NO (10 is not < 40, so go left).
  Wait — 10 < 40, so 40 is not < 10.
  Correct: curr->data.id (40) < id (10)? NO → go left.
curr=20: 20 < 10? NO → go left.
curr=10: 10 < 10? NO → go left.
curr=NULL → stop.   pred = NULL.
```
Output: `10 is the first record — no predecessor.`

**Successor pass:**
```
curr=40: 40 > 10? YES → succ=40, go left.
curr=20: 20 > 10? YES → succ=20, go left.
curr=10: 10 > 10? NO  → go right.
curr=NULL → stop.   succ = node(20).
```
Output: `Successor  : ID=20, ...`

---

### Why this algorithm is correct

At every node:
- **Predecessor**: we go right when we find a node smaller than the target (trying to get even closer to the target from below).  The last such node recorded is the answer.
- **Successor**: we go left when we find a node larger than the target (trying to get even closer from above).  The last such node recorded is the answer.

The invariant is that the recorded candidate is always valid, and the traversal only
updates it when a strictly better (closer) candidate is found.

---

## 7. How to Build and Run

### Prerequisites

- GCC (any version supporting C11) or Clang with `-std=c11`.
- A POSIX-compatible shell (Linux, macOS, WSL on Windows).

### Compilation

```bash
# From the project root directory:
gcc -Wall -Wextra -std=c11 -o sms src/main.c src/bst.c
```

`-Wall -Wextra` enables all common warnings.  The project compiles with zero warnings.

### Running

```bash
./sms
```

### Sample Session

```
══════════════════════════════════════
     Student Management System
══════════════════════════════════════
 [1]  Insert Student
 ...
══════════════════════════════════════
Enter your choice: 1
Enter Student ID  : 101
Enter Student Name: Alice Smith
Enter GPA (0.0 - 4.0): 3.85
Student inserted successfully.

Enter your choice: 1
Enter Student ID  : 99
Enter Student Name: Carla Diaz
Enter GPA (0.0 - 4.0): 3.50
Student inserted successfully.

Enter your choice: 6

ID        Name                            GPA
--------  ------------------------------  ----
ID: 99      |  Name: Carla Diaz                      |  GPA: 3.50
ID: 101     |  Name: Alice Smith                     |  GPA: 3.85

Enter your choice: 8

--- GPA Statistics ---
Minimum GPA        : 3.50
Maximum GPA        : 3.85
Average GPA        : 3.68
Students above avg : 1
Students below avg : 1

Enter your choice: 10
Enter Student ID: 99
99 is the first record — no predecessor.
Successor  : ID=101, Name=Alice Smith, GPA=3.85

Enter your choice: 0
Exiting... Goodbye!
```

### Cleaning up

```bash
rm sms
```

---

## 8. Why AVL Instead of Plain BST?

### 8.1 The Worst-Case Problem with a Plain BST

A Binary Search Tree preserves one invariant: every key in the left subtree is smaller than the root and every key in the right subtree is larger.  But it makes **no** promise about the tree's shape.

If you insert students in ascending order of ID — a perfectly natural workflow when IDs are assigned sequentially — every new node becomes the right child of the previous one:

```
Insert IDs in order: 10, 20, 30, 40, 50

10
  \
   20
     \
      30
        \
         40
           \
            50
```

This is a **degenerate tree** — effectively a linked list.  Its height is `n` (5 in the example), not log₂(n) ≈ 2.3.  Every search, insert, and delete now costs **O(n)** in the worst case.  For a university managing thousands of student records, a 10-fold slow-down from sorted imports is catastrophic.

### 8.2 How AVL Trees Fix It

An AVL tree (named after Adelson-Velsky and Landis, 1962) adds one extra rule on top of the BST invariant:

> **For every node, the heights of its left and right subtrees may differ by at most 1.**

This property — called the **AVL balance invariant** — is enforced automatically after every insert and delete using one or two pointer rewrites called **rotations**.  It can be mathematically proved that an AVL tree of `n` nodes has a height of at most 1.44 log₂(n), which guarantees **O(log n)** worst-case for all core operations.

Taking the same five sorted inserts from above, the AVL tree rebalances itself twice:

```
After inserting 10, 20:       After inserting 30 (RR imbalance at 10):
                               rotateLeft(10)

  10                              20
    \                            /  \
     20                        10   30
```

The final tree with all five nodes stays at **height 3** instead of 5 — verified by the `bst_tree_stats` output during the smoke test.

---

### 8.3 The Four Rotation Cases

Each rotation is a small, local pointer reshuffle.  Only two or three pointers change; the rest of the tree is untouched.  Heights are refreshed bottom-up immediately after each rotation.

---

#### LL Case — Single Right Rotation

**When it fires:** The imbalanced node's left subtree is too tall, **and** the left child is itself left-heavy (or balanced).

```
Before (BF = +2):          After rotateRight(Z):

       Z                         Y
      / \                       / \
     Y   T4                    X   Z
    / \                       /\  / \
   X   T3                   T1 T2 T3 T4
  / \
 T1  T2
```

**Plain English:** Y rises to become the new root.  Z drops to become Y's right child.  Y's old right subtree (T3) is re-attached as Z's left child — it was between Y and Z in key order before the rotation, and it still is afterward (BST ordering is preserved).

---

#### RR Case — Single Left Rotation

**When it fires:** The imbalanced node's right subtree is too tall, **and** the right child is itself right-heavy (or balanced).

```
Before (BF = -2):          After rotateLeft(Z):

   Z                               Y
  / \                             / \
 T1   Y                          Z   X
      / \                       /\  / \
     T2   X                   T1 T2 T3 T4
          /\
         T3 T4
```

**Plain English:** Y rises; Z drops to become Y's left child.  Y's old left subtree (T2) re-attaches as Z's right child — symmetric to the LL case.

---

#### LR Case — Left-Right Double Rotation

**When it fires:** The imbalanced node's left subtree is too tall, **but** the left child is right-heavy.  A single right rotation would not fix it; you need two rotations.

```
Before (BF = +2,            Step 1 — rotateLeft(Y):   Step 2 — rotateRight(Z):
left child BF = -1):

       Z                          Z                          X
      / \                        / \                        / \
     Y   T4                     X   T4                    Y   Z
    / \                        / \                       /\  / \
   T1   X                     Y  T3                    T1 T2 T3 T4
        /\                   /\
       T2 T3                T1 T2
```

**Plain English:** First left-rotate Y so that X rises in Y's place (converting the LR shape into an LL shape).  Then right-rotate Z so that X rises again to become the new subtree root.

---

#### RL Case — Right-Left Double Rotation

**When it fires:** The imbalanced node's right subtree is too tall, **but** the right child is left-heavy.  Symmetric to the LR case.

```
Before (BF = -2,            Step 1 — rotateRight(Y):  Step 2 — rotateLeft(Z):
right child BF = +1):

   Z                              Z                          X
  / \                            / \                        / \
 T1   Y                         T1   X                    Z   Y
      / \                           / \                  /\  / \
     X   T4                        T2   Y               T1 T2 T3 T4
    /\                                  /\
   T2 T3                               T3 T4
```

**Plain English:** First right-rotate Y so that X rises in Y's place (converting the RL shape into an RR shape).  Then left-rotate Z so that X becomes the new subtree root.

---

### 8.4 Updated Complexity Summary

| Operation          | Plain BST (average) | Plain BST (worst case) | **AVL Tree (guaranteed)** |
|--------------------|---------------------|------------------------|---------------------------|
| Insert             | O(log n)            | **O(n)**               | **O(log n)**              |
| Search by ID       | O(log n)            | **O(n)**               | **O(log n)**              |
| Delete             | O(log n)            | **O(n)**               | **O(log n)**              |
| Search by Name     | O(n)                | O(n)                   | O(n)                      |
| Display All        | O(n)                | O(n)                   | O(n)                      |
| Tree Height        | O(n)                | O(n)                   | **O(1)** ¹                |
| Student Count      | O(1) ²              | O(1) ²                 | O(1) ²                    |
| GPA Statistics     | O(n)                | O(n)                   | O(n)                      |
| Delete All         | O(n)                | O(n)                   | O(n)                      |
| Predecessor/Succ.  | O(log n)            | **O(n)**               | **O(log n)**              |

¹ Each node stores its height; the root's height is read in O(1) — no traversal needed.  
² `tree->count` is a live counter updated on every insert and delete.

The AVL rotation overhead (at most two rotations, each O(1)) is paid on the way back up the recursion stack after an insert or delete — this does **not** change the O(log n) asymptotic cost.  The constant factor is negligible in practice.

---

*End of documentation.*
