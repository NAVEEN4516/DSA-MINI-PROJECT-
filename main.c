#include "crms.h"

int main(int argc, char *argv[]) 
{

// Read max users from command line
if (argc > 1) {
    int n = atoi(argv[1]);
    if (n > 0) {
        MAX_USERS_DYNAMIC = n;
        printf("Max users set to %d from command line.\n", n);
    } else {
        printf("Invalid max user argument. Using default %d.\n", MAX_USERS_DYNAMIC);
    }
}
users = malloc(MAX_USERS_DYNAMIC * sizeof(User));
if (!users) {
    printf("Fatal: unable to allocate memory for %d users.\n", MAX_USERS_DYNAMIC);
    return 1;
}


srand((unsigned int)time(NULL));
ensureFileExists(USERS_FILE);
ensureFileExists(CHANGES_FILE);
ensureFileExists(AUDIT_FILE);

    loadUsers();
    seedIfNoUsers();
    loadChanges();
    printf("Loaded %d users and %d changes.\n", userCount, changeCount);

    while (1) {
        showMainMenu();
        int choice;
        if (scanf("%d", &choice) != 1) { clearInput(); continue; }
        clearInput();

        if (choice == 0) {
            printf("Exiting. Saving data...\n");
            saveUsers();
            saveChanges();
            freeDependencyGraph();
            if (categoryRoot) {
                freeTree(categoryRoot);
                categoryRoot = NULL;
            }
            return 0;
        } 
        else if (choice == 2) {
            registerUserInteractive();
        } 
        else if (choice == 1) {
            if (!loginInteractive()) continue;
            int running = 1;
            while (running && currentIndex != -1) {
                char *role = users[currentIndex].role;
                if (strcmp(role, "User") == 0) {
                    printf("\nUser Dashboard\n");
                    printf("1) User Menu\n2) Change Password\n0) Logout\n> ");
                    int c; if (scanf("%d", &c) != 1) { clearInput(); continue; } clearInput();
                    if (c == 0) { logoutInteractive(); break; }
                    else if (c == 1) userMenuInteractive();
                    else if (c == 2) changePasswordInteractive();
                    else printf("Invalid.\n");
                } 
                else if (strcmp(role, "Developer") == 0) {
                    printf("\nDeveloper Dashboard\n");
                    printf("1) Developer Menu\n2) Change Password\n0) Logout\n> ");
                    int c; if (scanf("%d", &c) != 1) { clearInput(); continue; } clearInput();
                    if (c == 0) { logoutInteractive(); break; }
                    else if (c == 1) developerMenuInteractive();
                    else if (c == 2) changePasswordInteractive();
                    else printf("Invalid.\n");
                } 
                else if (strcmp(role, "Tester") == 0) {
                    printf("\nTester Dashboard\n");
                    printf("1) Tester Menu\n2) Change Password\n0) Logout\n> ");
                    int c; if (scanf("%d", &c) != 1) { clearInput(); continue; } clearInput();
                    if (c == 0) { logoutInteractive(); break; }
                    else if (c == 1) testerMenuInteractive();
                    else if (c == 2) changePasswordInteractive();
                    else printf("Invalid.\n");
                } 
                else if (strcmp(role, "Admin") == 0 || strcmp(role, "Manager") == 0) {
                    printf("\nApprover Dashboard (%s)\n", role);
                    printf("1) Approver Menu (approve/check)\n2) Final Approver Menu (finalize/rollback)\n3) Category Tree Report\n4) Audit Timeline\n5) Change Password\n0) Logout\n> ");
                    int c; if (scanf("%d", &c) != 1) { clearInput(); continue; } clearInput();
                    if (c == 0) { logoutInteractive(); break; }
                    else if (c == 1) approverMenuInteractive();
                    else if (c == 2) finalApproverMenuInteractive();
                    else if (c == 3) generateCategoryTreeReport();
                    else if (c == 4) generateAuditTimeline();
                    else if (c == 5) changePasswordInteractive();
                    else printf("Invalid.\n");
                } 
                else {
                    printf("Unknown role. Logging out.\n");
                    logoutInteractive();
                    break;
                }
            }
        } 
        else {
            printf("Invalid option.\n");
        }
    }
    return 0;
}
