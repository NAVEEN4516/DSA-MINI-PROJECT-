#ifndef CRMS_H
#define CRMS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <conio.h>
#include <ctype.h>
#include <unistd.h>

#define MAX_LEN 128
#define MAX_CHANGES 500
#define MAX_DEPS 10

#define USERS_FILE "users.csv"
#define CHANGES_FILE "changes.csv"
#define AUDIT_FILE "audit_log.csv"

/***************************************************
 * USER STRUCT
 ***************************************************/
typedef struct {
    char username[32];
    char password[64];
    char role[16];
} User;

/***************************************************
 * CHANGE STRUCT
 ***************************************************/
typedef struct {
    int id;
    char title[64];
    char description[256];
    char status[32];
    int priority;
    char requester[32];
    int deps[MAX_DEPS];
    int depCount;
} Change;

/***************************************************
 * GRAPH (DEPENDENCY GRAPH)
 ***************************************************/
typedef struct GraphNode {
    int id;
    struct GraphNode *next;
} GraphNode;

typedef struct {
    GraphNode *head[MAX_CHANGES];
    int nodeCount;
} Graph;

/***************************************************
 * TREE (REAL N-ARY TREE)
 ***************************************************/
typedef struct TreeNode {
    char label[128];
    struct TreeNode *left;
    struct TreeNode *right;
} TreeNode;

/***************************************************
 * GLOBAL VARIABLES (EXTERN)
 ***************************************************/
extern int MAX_USERS_DYNAMIC;
extern User *users;
extern int userCount;
extern int currentIndex;

extern Change changes[MAX_CHANGES];
extern int changeCount;
extern int nextChangeID;

extern Graph dependencyGraph;

extern TreeNode *categoryRoot;

/***************************************************
 * FUNCTION PROTOTYPES
 ***************************************************/
void clearInput(void);
void trim_newline(char *s);
void inputPassword(char *buf, int bufSize);

void ensureFileExists(const char *filename);

// user functions
void loadUsers();
void saveUsers();
void seedIfNoUsers();
void registerUserInteractive();
int loginInteractive();
void logoutInteractive();
void changePasswordInteractive();
int findUserIndex(const char *username);

// changes
void loadChanges();
void saveChanges();
int findChangeIndexByID(int id);
int findChangeIndexByTitleAndRequester(const char *title, const char *requester);
void submitChangeInteractive();

// dependency graph
void freeDependencyGraph();
void buildDependencyGraph();
int depsAllVerifiedOrCompleted(Change *c);

// menus
void approverMenuInteractive();
void finalApproverMenuInteractive();
void developerMenuInteractive();
void testerMenuInteractive();
void userMenuInteractive();

// reports
void printDependencies(Change *c);
void generateCategoryTreeReport();
void generateAuditTimeline();

// tree
TreeNode* createNode(const char *label);
void addChild(TreeNode *parent, TreeNode *child);
void freeTree(TreeNode *root);

// menu
void audit_log(int changeID, const char *action);

void showMainMenu();

#endif
