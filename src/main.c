/*
 * main.c
 * Student Management System — Menu-driven console interface.
 *
 * Compile: gcc -Wall -Wextra -std=c11 -o sms src/main.c src/bst.c
 * Run    : ./sms
 */

#include "bst.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ── print_menu ──────────────────────────────────────────────────────────── */

/* Prints the main menu to stdout. */
static void print_menu(void)
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

/* ── main ────────────────────────────────────────────────────────────────── */

int main(void)
{
    BST  *tree   = bst_create();
    char  buf[256];
    int   choice = -1;

    do {
        print_menu();

        /* Read the menu choice safely with fgets + sscanf. */
        if (!fgets(buf, sizeof(buf), stdin)) {
            /* EOF (e.g. Ctrl-D): treat as exit. */
            choice = 0;
            break;
        }

        if (sscanf(buf, "%d", &choice) != 1) {
            printf("Invalid choice. Please try again.\n");
            choice = -1;
            continue;
        }

        switch (choice) {

            /* ── [1] Insert Student ─────────────────────────────────── */
            case 1: {
                int   id  = 0;
                float gpa = 0.0f;
                char  name[100];

                printf("Enter Student ID  : ");
                if (!fgets(buf, sizeof(buf), stdin)) break;
                if (sscanf(buf, "%d", &id) != 1) {
                    printf("Invalid ID. Please enter an integer.\n");
                    break;
                }

                printf("Enter Student Name: ");
                if (!fgets(name, sizeof(name), stdin)) break;
                name[strcspn(name, "\n")] = '\0'; /* strip newline */
                if (strlen(name) == 0) {
                    printf("Error: Name cannot be empty.\n");
                    break;
                }

                printf("Enter GPA (0.0 - 4.0): ");
                if (!fgets(buf, sizeof(buf), stdin)) break;
                if (sscanf(buf, "%f", &gpa) != 1) {
                    printf("Invalid GPA. Please enter a decimal number.\n");
                    break;
                }

                if (bst_insert(tree, id, name, gpa) == 0) {
                    printf("Student inserted successfully.\n");
                }
                break;
            }

            /* ── [2] Search by ID ──────────────────────────────────── */
            case 2: {
                int id = 0;
                printf("Enter Student ID to search: ");
                if (!fgets(buf, sizeof(buf), stdin)) break;
                if (sscanf(buf, "%d", &id) != 1) {
                    printf("Invalid ID. Please enter an integer.\n");
                    break;
                }
                BSTNode *node = bst_search_by_id(tree, id);
                if (node) {
                    printf("Found: ID=%d, Name=%s, GPA=%.2f\n",
                           node->data.id, node->data.name, node->data.gpa);
                } else {
                    printf("Student with ID %d not found.\n", id);
                }
                break;
            }

            /* ── [3] Search by Name ────────────────────────────────── */
            case 3: {
                char name[100];
                printf("Enter Student Name to search: ");
                if (!fgets(name, sizeof(name), stdin)) break;
                name[strcspn(name, "\n")] = '\0';
                if (strlen(name) == 0) {
                    printf("Error: Name cannot be empty.\n");
                    break;
                }
                BSTNode *node = bst_search_by_name(tree, name);
                if (node) {
                    printf("Found: ID=%d, Name=%s, GPA=%.2f\n",
                           node->data.id, node->data.name, node->data.gpa);
                } else {
                    printf("Student named \"%s\" not found.\n", name);
                }
                break;
            }

            /* ── [4] Delete by ID ──────────────────────────────────── */
            case 4: {
                int id = 0;
                printf("Enter Student ID to delete: ");
                if (!fgets(buf, sizeof(buf), stdin)) break;
                if (sscanf(buf, "%d", &id) != 1) {
                    printf("Invalid ID. Please enter an integer.\n");
                    break;
                }
                if (bst_delete_by_id(tree, id) == 0) {
                    printf("Student with ID %d deleted successfully.\n", id);
                }
                break;
            }

            /* ── [5] Update Student Data ────────────────────────────── */
            case 5: {
                int id = 0;
                printf("Enter Student ID to update: ");
                if (!fgets(buf, sizeof(buf), stdin)) break;
                if (sscanf(buf, "%d", &id) != 1) {
                    printf("Invalid ID. Please enter an integer.\n");
                    break;
                }
                bst_update(tree, id);
                break;
            }

            /* ── [6] Display All Students ───────────────────────────── */
            case 6:
                bst_display_all(tree);
                break;

            /* ── [7] Display Tree Statistics ───────────────────────── */
            case 7:
                bst_tree_stats(tree);
                break;

            /* ── [8] Display GPA Statistics ────────────────────────── */
            case 8:
                bst_gpa_stats(tree);
                break;

            /* ── [9] Delete All Students ────────────────────────────── */
            case 9:
                bst_delete_all(tree);
                break;

            /* ── [10] Find Predecessor & Successor ─────────────────── */
            case 10: {
                int id = 0;
                printf("Enter Student ID: ");
                if (!fgets(buf, sizeof(buf), stdin)) break;
                if (sscanf(buf, "%d", &id) != 1) {
                    printf("Invalid ID. Please enter an integer.\n");
                    break;
                }
                bst_predecessor_successor(tree, id);
                break;
            }

            /* ── [0] Exit ───────────────────────────────────────────── */
            case 0:
                printf("Exiting... Goodbye!\n");
                break;

            /* ── Invalid ────────────────────────────────────────────── */
            default:
                printf("Invalid choice. Please try again.\n");
                break;
        }

    } while (choice != 0);

    bst_destroy(tree);
    return 0;
}
