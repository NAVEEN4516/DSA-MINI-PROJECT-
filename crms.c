/*     BUGS IN THE CODE

1) If MAX_USERS_DYNAMIC is small, the program still inserts 5 default users,
   which causes memory overflow and can crash the system.

2) The dependency graph is inaccurate because it treats change IDs as if they
   were array indexes.

3) Manually added dependencies disappear after saving, since the graph gets
   rebuilt and overwrites them.

4) The code uses strcpy() without any size checks, so long input can overflow
   buffers and corrupt memory.

5) Mixing scanf() and fgets() leads to skipped or empty inputs because of
   leftover newline characters.

There may be more bugs in the system, but these are the main ones we found.

*/


#include "crms.h"

/***************************************************
 * GLOBAL VARIABLES
 ***************************************************/
User *users = NULL;                 // dynamic user array
int MAX_USERS_DYNAMIC = 200;        // default user capacity
int userCount = 0;

int currentIndex = -1;

Change changes[MAX_CHANGES];        // stays fixed-size
int changeCount = 0;
int nextChangeID = 1000;

Graph dependencyGraph;

TreeNode *categoryRoot = NULL;

/***************************************************
 * TREE HELPERS
 ***************************************************/
TreeNode* createNode(const char *label) {
    TreeNode *n = (TreeNode*)malloc(sizeof(TreeNode));
    if (!n) return NULL;

    strncpy(n->label, label, sizeof(n->label) - 1);
    n->label[sizeof(n->label) - 1] = '\0';

    n->left = NULL;
    n->right = NULL;
    return n;
}

void addChild(TreeNode *parent, TreeNode *child) {
    if (!parent || !child) return;

    if (!parent->left) {
        parent->left = child;
    } else {
        TreeNode *curr = parent->left;
        while (curr->right)
            curr = curr->right;
        curr->right = child;
    }
}

void freeTree(TreeNode *node) {
    if (!node) return;
    freeTree(node->left);
    freeTree(node->right);
    free(node);
}

/***************************************************
 * DEPENDENCY GRAPH
 ***************************************************/
void freeDependencyGraph() {
    for (int i = 0; i < MAX_CHANGES; ++i) {
        GraphNode *cur = dependencyGraph.head[i];
        while (cur) {
            GraphNode *tmp = cur;
            cur = cur->next;
            free(tmp);
        }
        dependencyGraph.head[i] = NULL;
    }
    dependencyGraph.nodeCount = 0;
}

void buildDependencyGraph() {
    freeDependencyGraph();
    for (int i = 0; i < MAX_CHANGES; ++i)
        dependencyGraph.head[i] = NULL;

    dependencyGraph.nodeCount = changeCount;

    for (int i = 0; i < changeCount; ++i) {
        for (int j = 0; j < changes[i].depCount; ++j) {
            int depID = changes[i].deps[j];
            GraphNode *newNode = malloc(sizeof(GraphNode));
            newNode->id = depID;
            newNode->next = dependencyGraph.head[i];
            dependencyGraph.head[i] = newNode;
        }
    }
}

/***************************************************
 * BASIC UTILS
 ***************************************************/
void clearInput(void) {
    int c;
    while ((c = getchar()) != '\n' && c != EOF) {}
}

void trim_newline(char *s) {
    char *p = strchr(s, '\n');
    if (p) *p = '\0';
}

void inputPassword(char *buf, int bufSize) {
    int i = 0;
    char ch;
    while (1) {
        ch = getch();
        if (ch == '\r' || ch == '\n') {
            buf[i] = '\0';
            printf("\n");
            return;
        } else if (ch == '\b' || ch == 127) {
            if (i > 0) { i--; printf("\b \b"); }
        } else if (i < bufSize - 1) {
            buf[i++] = ch;
            printf("*");
        }
    }
}

/***************************************************
 * FILE EXISTENCE CHECK
 ***************************************************/
void ensureFileExists(const char *filename) {
    if (access(filename, F_OK) == 0)
        return;

    FILE *fp = fopen(filename, "w");
    if (fp) {
        fclose(fp);
        printf("Created missing file: %s\n", filename);
    } else {
        printf("ERROR: Cannot create file: %s\n", filename);
    }
}

/***************************************************
 * USER FUNCTIONS
 ***************************************************/
void loadUsers() {
    FILE *fp = fopen(USERS_FILE, "r");
    userCount = 0;
    if (!fp) return;

    while (userCount < MAX_USERS_DYNAMIC) {
        char line[256];
        if (!fgets(line, sizeof(line), fp)) break;

        trim_newline(line);
        if (strlen(line) == 0) continue;

        char *p = strtok(line, ",");
        if (!p) continue;
        strcpy(users[userCount].username, p);

        p = strtok(NULL, ","); if (!p) continue;
        strcpy(users[userCount].password, p);

        p = strtok(NULL, ","); if (!p) continue;
        strcpy(users[userCount].role, p);

        userCount++;
    }
    fclose(fp);
}

void saveUsers() {
    FILE *fp = fopen(USERS_FILE, "w");
    if (!fp) { printf("ERROR: saving users.\n"); return; }

    for (int i = 0; i < userCount; ++i)
        fprintf(fp, "%s,%s,%s\n",
            users[i].username, users[i].password, users[i].role);

    fclose(fp);
}

int findUserIndex(const char *username) {
    for (int i = 0; i < userCount; ++i)
        if (strcmp(users[i].username, username) == 0)
            return i;
    return -1;
}

void seedIfNoUsers() {
    if (userCount > 0) return;

    strcpy(users[0].username, "admin");
    strcpy(users[0].password, "admin123");
    strcpy(users[0].role, "Admin");

    strcpy(users[1].username, "manager");
    strcpy(users[1].password, "manager123");
    strcpy(users[1].role, "Manager");

    strcpy(users[2].username, "dev");
    strcpy(users[2].password, "dev123");
    strcpy(users[2].role, "Developer");

    strcpy(users[3].username, "tester");
    strcpy(users[3].password, "tester123");
    strcpy(users[3].role, "Tester");

    strcpy(users[4].username, "user");
    strcpy(users[4].password, "user123");
    strcpy(users[4].role, "User");

    userCount = 5;
    saveUsers();
}

void registerUserInteractive() {
    if (userCount >= MAX_USERS_DYNAMIC) {
        printf("User limit reached.\n");
        return;
    }

    char un[32], pw[64], pw2[64];
    printf("Register username: ");
    if (scanf("%31s", un) != 1) { clearInput(); return; }
    clearInput();

    if (findUserIndex(un) != -1) {
        printf("Username exists.\n");
        return;
    }

    printf("Password: "); inputPassword(pw, sizeof(pw));
    printf("Confirm: "); inputPassword(pw2, sizeof(pw2));

    if (strcmp(pw, pw2) != 0) {
        printf("Mismatch.\n");
        return;
    }

    strcpy(users[userCount].username, un);
    strcpy(users[userCount].password, pw);
    strcpy(users[userCount].role, "User");

    userCount++;
    saveUsers();
    printf("User registered.\n");
}

int loginInteractive() {
    char un[32], pw[64];
    printf("Username: ");
    if (scanf("%31s", un) != 1) { clearInput(); return 0; }
    clearInput();

    printf("Password: "); inputPassword(pw, sizeof(pw));

    int idx = findUserIndex(un);

    if (idx == -1 || strcmp(users[idx].password, pw) != 0) {
        printf("Invalid.\n");
        return 0;
    }

    currentIndex = idx;
    printf("Logged in as %s (%s)\n", users[idx].username, users[idx].role);
    return 1;
}

void logoutInteractive() {
    if (currentIndex == -1) {
        printf("No user is logged in.\n");
        return;
    }
    printf("User '%s' logged out.\n", users[currentIndex].username);
    currentIndex = -1;
}

void changePasswordInteractive() {
    if (currentIndex == -1) {
        printf("Login required.\n");
        return;
    }

    char oldp[64], newp[64], conf[64];

    printf("Old password: "); inputPassword(oldp, sizeof(oldp));
    if (strcmp(oldp, users[currentIndex].password) != 0) {
        printf("Incorrect.\n");
        return;
    }

    printf("New password: "); inputPassword(newp, sizeof(newp));
    printf("Confirm: "); inputPassword(conf, sizeof(conf));

    if (strcmp(newp, conf) != 0) {
        printf("Mismatch.\n");
        return;
    }

    strcpy(users[currentIndex].password, newp);
    saveUsers();
    printf("Password changed.\n");
}

/***************************************************
 * LOAD & SAVE CHANGES
 ***************************************************/
void loadChanges() {
    FILE *fp = fopen(CHANGES_FILE, "r");
    changeCount = 0;
    nextChangeID = 1000;

    if (!fp) return;

    while (changeCount < MAX_CHANGES) {
        char line[1024];
        if (!fgets(line, sizeof(line), fp)) break;

        trim_newline(line);
        if (strlen(line) == 0) continue;

        char *p = strtok(line, "|");
        if (!p) continue;
        changes[changeCount].id = atoi(p);

        if (changes[changeCount].id >= nextChangeID)
            nextChangeID = changes[changeCount].id + 1;

        p = strtok(NULL, "|");
        strcpy(changes[changeCount].title, p ? p : "");

        p = strtok(NULL, "|");
        strcpy(changes[changeCount].description, p ? p : "");

        p = strtok(NULL, "|");
        strcpy(changes[changeCount].status, p ? p : "");

        p = strtok(NULL, "|");
        changes[changeCount].priority = p ? atoi(p) : 4;

        p = strtok(NULL, "|");
        strcpy(changes[changeCount].requester, p ? p : "");

        p = strtok(NULL, "|");
        changes[changeCount].depCount = 0;

        if (p) {
            char *q = strtok(p, ";");
            while (q && changes[changeCount].depCount < MAX_DEPS) {
                changes[changeCount].deps[changes[changeCount].depCount++] = atoi(q);
                q = strtok(NULL, ";");
            }
        }

        changeCount++;
    }

    fclose(fp);
    buildDependencyGraph();
}
void audit_log(int changeID, const char *action) {
    FILE *fp = fopen(AUDIT_FILE, "a");
    if (!fp) return;

    time_t now = time(NULL);
    char tbuf[64];
    struct tm *tmstruct = localtime(&now);

    strftime(tbuf, sizeof(tbuf), "%Y-%m-%d %H:%M:%S", tmstruct);

    fprintf(fp,
        "ChangeID:%d, Action:%s, By:%s, Time:%s\n",
        changeID,
        action,
        (currentIndex == -1 ? "SYSTEM" : users[currentIndex].username),
        tbuf
    );

    fclose(fp);
}


void saveChanges() {
    FILE *fp = fopen(CHANGES_FILE, "w");
    if (!fp) {
        printf("Failed to save changes.\n");
        return;
    }

    for (int i = 0; i < changeCount; ++i) {
        fprintf(fp, "%d|%s|%s|%s|%d|%s|",
            changes[i].id,
            changes[i].title,
            changes[i].description,
            changes[i].status,
            changes[i].priority,
            changes[i].requester
        );

        for (int j = 0; j < changes[i].depCount; ++j) {
            if (j) fprintf(fp, ";");
            fprintf(fp, "%d", changes[i].deps[j]);
        }

        fprintf(fp, "\n");
    }

    fclose(fp);
    buildDependencyGraph();
}

/***************************************************
 * CHANGE HELPERS
 ***************************************************/
int findChangeIndexByID(int id) {
    for (int i = 0; i < changeCount; ++i)
        if (changes[i].id == id)
            return i;
    return -1;
}

int findChangeIndexByTitleAndRequester(const char *title, const char *requester) {
    for (int i = 0; i < changeCount; ++i) {
        if (strcmp(changes[i].title, title) == 0 &&
            strcmp(changes[i].requester, requester) == 0 &&
            (strcmp(changes[i].status, "PendingApproval") == 0 ||
             strcmp(changes[i].status, "Approved") == 0))
            return i;
    }
    return -1;
}

int depsAllVerifiedOrCompleted(Change *c) {
    for (int i = 0; i < c->depCount; ++i) {
        int idx = findChangeIndexByID(c->deps[i]);
        if (idx == -1) {
            printf("Missing dependency %d.\n", c->deps[i]);
            return 0;
        }
        if (strcmp(changes[idx].status, "VerificationFailed") == 0 ||
            strcmp(changes[idx].status, "RolledBack") == 0)
            return 0;
        if (strcmp(changes[idx].status, "Verified") != 0 &&
            strcmp(changes[idx].status, "Completed") != 0)
            return 0;
    }
    return 1;
}

/***************************************************
 * CYCLE DETECTION
 ***************************************************/
int hasCycleUtilGraph(int idx, int visited[], int recStack[]) {
    if (!visited[idx]) {
        visited[idx] = 1;
        recStack[idx] = 1;

        GraphNode *curr = dependencyGraph.head[idx];
        while (curr) {
            int depIdx = findChangeIndexByID(curr->id);
            if (depIdx != -1) {
                if (!visited[depIdx] &&
                    hasCycleUtilGraph(depIdx, visited, recStack))
                    return 1;
                else if (recStack[depIdx])
                    return 1;
            }
            curr = curr->next;
        }
    }
    recStack[idx] = 0;
    return 0;
}

int hasCycleGraph() {
    int visited[MAX_CHANGES] = {0};
    int recStack[MAX_CHANGES] = {0};

    for (int i = 0; i < changeCount; i++)
        if (hasCycleUtilGraph(i, visited, recStack))
            return 1;

    return 0;
}

/***************************************************
 * SUBMIT CHANGE
 ***************************************************/
void submitChangeInteractive() {
    if (currentIndex == -1) {
        printf("Login required.\n");
        return;
    }

    char title[64];
    printf("Enter change title: ");
    fgets(title, sizeof(title), stdin);
    trim_newline(title);

    if (findChangeIndexByTitleAndRequester(title, users[currentIndex].username) != -1) {
        printf("Duplicate.\n");
        return;
    }

    Change c;
    c.id = nextChangeID++;
    strcpy(c.title, title);
    strcpy(c.description, "(to be added)");
    strcpy(c.status, "PendingApproval");
    strcpy(c.requester, users[currentIndex].username);
    c.priority = 0;
    c.depCount = 0;

    if (changeCount < MAX_CHANGES) {
        changes[changeCount++] = c;
        saveChanges();
        printf("Submitted ID %d\n", c.id);
    } else {
        printf("System full.\n");
    }
}

/***************************************************
 * PRINT DEPENDENCIES
 ***************************************************/
void printDependencyGraph(int id, int depth, int visited[]) {
    int idx = findChangeIndexByID(id);
    if (idx == -1) {
        for (int i = 0; i < depth; ++i) printf("    ");
        printf("-> %d (unknown)\n", id);
        return;
    }

    if (visited[idx]) {
        for (int i = 0; i < depth; ++i) printf("    ");
        printf("[Cycle detected: %d]\n", id);
        return;
    }

    visited[idx] = 1;

    for (int i = 0; i < depth; ++i) printf("    ");
    printf("Change %d: %s (Status: %s, Priority:%d)\n",
        changes[idx].id, changes[idx].title,
        changes[idx].status, changes[idx].priority);

    GraphNode *curr = dependencyGraph.head[idx];
    while (curr) {
        printDependencyGraph(curr->id, depth + 1, visited);
        curr = curr->next;
    }

    visited[idx] = 0;
}

void printDependencies(Change *c) {
    if (c->depCount == 0) {
        printf("  No dependencies.\n");
        return;
    }

    int visited[MAX_CHANGES] = {0};

    printf("\nDependencies for Change %d (%s):\n", c->id, c->title);
    for (int i = 0; i < c->depCount; ++i)
        printDependencyGraph(c->deps[i], 1, visited);
}

/***************************************************
 * MENUS (same logic, unchanged)
 ***************************************************/
void approverMenuInteractive() { while (1) {
        printf("\n--- Approver Menu (%s) ---\n", users[currentIndex].username);
        printf("1) View all PendingApproval requests\n");
        printf("2) Approve a request (add details)\n");
        printf("3) Check dependencies for a request\n");
        printf("4) View all requests\n");
        printf("0) Back\n> ");
        int choice;
        if (scanf("%d", &choice) != 1) { clearInput(); continue; }
        clearInput();
        if (choice == 0) break;
        if (choice == 1) {
            printf("\nPendingApproval Requests:\n");
            for (int i = 0; i < changeCount; ++i)
                if (strcmp(changes[i].status, "PendingApproval") == 0)
                    printf("ID:%d | Title:%s | Requester:%s | Priority:%d\n",
                           changes[i].id, changes[i].title, changes[i].requester, changes[i].priority);
        } 
        else if (choice == 2) {
            printf("Enter Change ID to approve: ");
            int id;
            if (scanf("%d", &id) != 1) { clearInput(); continue; }
            clearInput();
            int idx = findChangeIndexByID(id);
            if (idx == -1) { printf("Change not found.\n"); continue; }
            if (strcmp(changes[idx].status, "PendingApproval") != 0) {
                printf("Change is not pending approval (Current: %s)\n", changes[idx].status);
                continue;
            }
            printf("Enter detailed description: ");
            fgets(changes[idx].description, sizeof(changes[idx].description), stdin);
            trim_newline(changes[idx].description);
            printf("Does this change depend on other Change IDs? (y/n): ");
            char ans = getchar(); clearInput();
            changes[idx].depCount = 0;
            if (ans == 'y' || ans == 'Y') {
                printf("Enter dependency IDs (end with 0): ");
                while (1) {
                    int d;
                    if (scanf("%d", &d) != 1) { clearInput(); break; }
                    if (d == 0) break;
                    int depIndex = findChangeIndexByID(d);
                    if (depIndex == -1) {
                        printf("Change ID %d not found — cannot add as dependency.\n", d);
                        continue;
                    }
                    if (strcmp(changes[depIndex].status, "RolledBack") == 0 ||
                        strcmp(changes[depIndex].status, "VerificationFailed") == 0) {
                        printf("Change %d exists but is in invalid state (current: %s). Cannot depend on it.\n",
                               d, changes[depIndex].status);
                        continue;
                    }
                    if (changes[idx].depCount < MAX_DEPS) {
                        changes[idx].deps[changes[idx].depCount++] = d;
                        GraphNode *newNode = (GraphNode*)malloc(sizeof(GraphNode));
                        newNode->id = d;
                        newNode->next = dependencyGraph.head[idx];
                        dependencyGraph.head[idx] = newNode;
                        if (hasCycleGraph()) {
                            printf("Circular dependency detected involving Change %d and %d. Removing last dependency.\n",
                                   changes[idx].id, d);
                            changes[idx].depCount--;
                            GraphNode *toFree = dependencyGraph.head[idx];
                            if (toFree) {
                                dependencyGraph.head[idx] = toFree->next;
                                free(toFree);
                            }
                        } else {
                            printf("Added dependency on Change %d (%s)\n", d, changes[depIndex].status);
                        }
                    } else {
                        printf("Max dependencies reached for this change.\n");
                        break;
                    }
                }
                clearInput();
            }
            printf("Set priority (1=Critical, 2=Major, 3=Minor, 4=Aesthetic): ");
            if (scanf("%d", &changes[idx].priority) != 1) {
                clearInput();
                changes[idx].priority = 4;
            } else {
                clearInput();
            }
            strcpy(changes[idx].status, "Approved");
            saveChanges();
            audit_log(changes[idx].id, "Approved");
            printf("Change ID %d approved successfully.\n", id);
        } 
        else if (choice == 3) {
            printf("Enter change ID to check dependencies: ");
            int id;
            if (scanf("%d", &id) != 1) { clearInput(); continue; }
            clearInput();
            int idx = findChangeIndexByID(id);
            if (idx == -1) { printf("Change %d not found.\n", id); continue; }
            printDependencies(&changes[idx]);
        } 
        else if (choice == 4) {
            printf("\nAll Change Requests:\n");
            for (int i = 0; i < changeCount; ++i)
                printf("ID:%d | Title:%s | Status:%s | Priority:%d | Requester:%s\n",
                       changes[i].id, changes[i].title, changes[i].status,
                       changes[i].priority, changes[i].requester);
        } 
        else printf("Invalid option.\n");
    }}
void developerMenuInteractive() { while (1) {
        printf("\n--- Developer Menu (%s) ---\n", users[currentIndex].username);
        printf("1) View Approved requests\n");
        printf("2) Mark a request as Executed (enter ID)\n");
        printf("0) Back\n> ");
        int choice; if (scanf("%d", &choice) != 1) { clearInput(); continue; } clearInput();
        if (choice == 0) break;
        if (choice == 1) {
            printf("Approved requests:\n");
            for (int i = 0; i < changeCount; ++i) {
                if (strcmp(changes[i].status, "Approved") == 0) {
                    printf("ID:%d Title:%s Priority:%d\n", changes[i].id, changes[i].title, changes[i].priority);
                }
            }
        } else if (choice == 2) {
            printf("Enter change ID to mark Executed: ");
            int id; if (scanf("%d", &id) != 1) { clearInput(); continue; } clearInput();
            int idx = findChangeIndexByID(id);
            if (idx == -1) { printf("Change not found.\n"); continue; }
            if (strcmp(changes[idx].status, "Approved") != 0) {
                printf("Change must be Approved first. Current status: %s\n", changes[idx].status);
                continue;
            }
            if (!depsAllVerifiedOrCompleted(&changes[idx])) {
                printf(" Cannot execute Change %d until its dependencies are verified/completed.\n", id);
                continue;
            }
            strcpy(changes[idx].status, "Executed");
            saveChanges();
            audit_log(changes[idx].id, "Executed");
            printf("Change %d marked Executed.\n", id);
        } else printf("Invalid.\n");
    } }
void testerMenuInteractive() { while (1) {
        printf("\n--- Tester Menu (%s) ---\n", users[currentIndex].username);
        printf("1) View Executed requests\n");
        printf("2) Verify a request (enter ID) \n");
        printf("0) Back\n> ");
        int choice; if (scanf("%d", &choice) != 1) { clearInput(); continue; } clearInput();
        if (choice == 0) break;
        if (choice == 1) {
            printf("Executed requests:\n");
            for (int i = 0; i < changeCount; ++i)
                if (strcmp(changes[i].status, "Executed") == 0)
                    printf("ID:%d Title:%s\n", changes[i].id, changes[i].title);
        } else if (choice == 2) {
            printf("Enter change ID to verify: ");
            int id; if (scanf("%d", &id) != 1) { clearInput(); continue; } clearInput();
            int idx = findChangeIndexByID(id);
            if (idx == -1) { printf("Not found.\n"); continue; }
            if (strcmp(changes[idx].status, "Executed") != 0) { printf("Only Executed requests can be verified. Current: %s\n", changes[idx].status); continue; }
            printf("Enter result (1=Pass, 0=Fail): ");
            int res; if (scanf("%d", &res) != 1) { clearInput(); continue; } clearInput();
            if (res == 1) {
                strcpy(changes[idx].status, "Verified");
                audit_log(changes[idx].id, "Verified");
                printf("Change %d Verified.\n", id);
            } else {
                strcpy(changes[idx].status, "VerificationFailed");
                audit_log(changes[idx].id, "Verification Failed");
                printf("Verification Failed => Rollback triggered for change %d\n", id);
                strcpy(changes[idx].status, "RolledBack");
                audit_log(changes[idx].id, "Rolled Back");
                printf("Change %d Rolled Back.\n", id);
            }
            saveChanges();
        } else printf("Invalid.\n");
    } }
void finalApproverMenuInteractive() { while (1) {
        printf("\n--- Final Approver Menu (%s) ---\n", users[currentIndex].username);
        printf("1) View Verified requests\n");
        printf("2) Final approve (mark Completed) (enter ID)\n");
        printf("3) Force rollback (enter ID)\n");
        printf("0) Back\n> ");
        int choice; if (scanf("%d", &choice) != 1) { clearInput(); continue; } clearInput();
        if (choice == 0) break;
        if (choice == 1) {
            printf("Verified requests:\n");
            for (int i = 0; i < changeCount; ++i)
                if (strcmp(changes[i].status, "Verified") == 0)
                    printf("ID:%d Title:%s\n", changes[i].id, changes[i].title);
        } else if (choice == 2) {
            printf("Enter change ID to finalize: ");
            int id; if (scanf("%d", &id) != 1) { clearInput(); continue; } clearInput();
            int idx = findChangeIndexByID(id);
            if (idx == -1) { printf("Not found.\n"); continue; }
            if (strcmp(changes[idx].status, "Verified") != 0) { printf("Only Verified requests can be finalized.\n"); continue; }
            strcpy(changes[idx].status, "Completed");
            saveChanges();
            audit_log(changes[idx].id, "Completed");
            printf("Change %d marked Completed.\n", id);
        } else if (choice == 3) {
            printf("Enter change ID to force rollback: ");
            int id; if (scanf("%d", &id) != 1) { clearInput(); continue; } clearInput();
            int idx = findChangeIndexByID(id);
            if (idx == -1) { printf("Not found.\n"); continue; }
            strcpy(changes[idx].status, "RolledBack");
            saveChanges();
            audit_log(changes[idx].id, "Rolled Back (Admin)");
            printf("Change %d Rolled Back by admin.\n", id);
        } else printf("Invalid.\n");
    }}
void userMenuInteractive() { while (1) {
        printf("\n--- User Menu (%s) ---\n", users[currentIndex].username);
        printf("1) Submit new change request\n");
        printf("2) View my requests\n");
        printf("0) Back\n> ");
        int choice; if (scanf("%d", &choice) != 1) { clearInput(); continue; } clearInput();
        if (choice == 0) break;
        if (choice == 1) submitChangeInteractive();
        else if (choice == 2) {
            printf("Your requests:\n");
            for (int i = 0; i < changeCount; ++i) {
                if (strcmp(changes[i].requester, users[currentIndex].username) == 0) {
                    printf("ID:%d Title:%s Status:%s Priority:%d\n", changes[i].id, changes[i].title, changes[i].status, changes[i].priority);
                    printDependencies(&changes[i]);
                }
            }
        } else printf("Invalid.\n");
    } }

/***************************************************
 * REPORTS
 ***************************************************/
void generateCategoryTreeReport() { 
    typedef struct {
        char category[20];
        int count;
        int ids[100];
    } Cat;

    Cat cats[4];
    strcpy(cats[0].category, "Critical");   cats[0].count = 0;
    strcpy(cats[1].category, "Major");      cats[1].count = 0;
    strcpy(cats[2].category, "Minor");      cats[2].count = 0;
    strcpy(cats[3].category, "Aesthetic");  cats[3].count = 0;

    for (int i = 0; i < changeCount; ++i) {
        int p = changes[i].priority;
        int idx = (p >=1 && p <=4) ? (p-1) : 3;

        if (cats[idx].count < 100)
            cats[idx].ids[cats[idx].count++] = changes[i].id;
    }

    // Build REAL TREE internally
    if (categoryRoot) {
        freeTree(categoryRoot);
        categoryRoot = NULL;
    }

    categoryRoot = createNode("All Changes");

    TreeNode *catNodes[4];
    for (int i = 0; i < 4; i++) {
        char label[64];
        snprintf(label, sizeof(label), "%s Changes", cats[i].category);

        catNodes[i] = createNode(label);
        addChild(categoryRoot, catNodes[i]);

        for (int j = 0; j < cats[i].count; j++) {
            int id = cats[i].ids[j];
            int idxChange = findChangeIndexByID(id);
            if (idxChange == -1) continue;

            char leafLabel[128];
            snprintf(leafLabel, sizeof(leafLabel), "ID %d: %s",
                     id, changes[idxChange].title);

            TreeNode *leaf = createNode(leafLabel);
            addChild(catNodes[i], leaf);
        }
    }

    // Print (unchanged)
    printf("\n===== CHANGE CATEGORY TREE =====\nAll Changes\n");

    for (int i = 0; i < 4; i++) {
        printf(" |-- %s Changes\n", cats[i].category);

        if (cats[i].count == 0) {
            printf(" |     (none)\n");
        } else {
            for (int j = 0; j < cats[i].count; j++) {
                int id = cats[i].ids[j];
                int idx = findChangeIndexByID(id);

                printf(" |     |-- ID %d: %s (Status:%s)\n",
                       id, changes[idx].title, changes[idx].status);
            }
        }
    }

    printf("================================\n");
}

void generateAuditTimeline() { FILE *fp = fopen(AUDIT_FILE, "r");
    if (!fp) { printf("No audit log yet.\n"); return; }
    printf("\n--- ACTION TIMELINE REPORT ---\n");
    char line[512];
    while (fgets(line, sizeof(line), fp)) {
        printf("%s", line);
    }
    fclose(fp);
    printf("--- END OF TIMELINE ---\n"); }

/***************************************************
 * MAIN MENU
 ***************************************************/
void showMainMenu() {
    printf("\n=========================================\n");
    printf("   CHANGE REQUEST MANAGEMENT SYSTEM\n");
    printf("=========================================\n");
    printf("1) Login : (Usernames => admin(or)manager/user/tester/developer)\n");
    printf("2) Register\n");
    printf("0) Exit\n> ");
}
