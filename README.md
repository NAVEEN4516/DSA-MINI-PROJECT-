# Change Request Management System (CRMS)

A terminal-based Change Request Management System written in C. It supports multi-role workflows for submitting, approving, executing, verifying, and finalizing change requests, with dependency tracking, audit logging, and category reporting.

---

## Features

- Role-based access control (User, Developer, Tester, Manager, Admin)
- Full change request lifecycle: Submit → Approve → Execute → Verify → Complete
- Dependency graph with cycle detection
- Audit log with timestamps
- Category tree report (Critical / Major / Minor / Aesthetic)
- Persistent storage via CSV files
- Dynamic user capacity configurable at launch

---

## File Structure

```
crms.h          # Header: structs, constants, function prototypes
crms.c          # Core logic: users, changes, graph, menus, reports
main.c          # Entry point, argument parsing, main loop
users.csv       # Persisted user accounts (auto-created)
changes.csv     # Persisted change requests (auto-created)
audit_log.csv   # Append-only audit trail (auto-created)
```

---

## Building

Requires a C compiler (GCC recommended) and the `conio.h` header (Windows) or a compatible substitute on Linux/macOS.

```bash
gcc -o crms crms.c main.c
```

---

## Running

```bash
# Default (up to 200 users)
./crms

# Custom user capacity
./crms 50
```

The three data files (`users.csv`, `changes.csv`, `audit_log.csv`) are created automatically if they do not exist.

---

## Default Accounts

If no users are found on startup, the following accounts are seeded automatically:

| Username  | Password    | Role      |
|-----------|-------------|-----------|
| admin     | admin123    | Admin     |
| manager   | manager123  | Manager   |
| dev       | dev123      | Developer |
| tester    | tester123   | Tester    |
| user      | user123     | User      |

---

## Role Permissions

| Role            | Capabilities                                                      |
|-----------------|-------------------------------------------------------------------|
| User            | Submit change requests, view own requests                         |
| Developer       | View approved requests, mark as Executed                          |
| Tester          | View executed requests, mark as Verified or VerificationFailed    |
| Manager / Admin | Approve requests, set dependencies and priority, finalize, rollback, view reports |

---

## Change Request Lifecycle

```
PendingApproval → Approved → Executed → Verified → Completed
                                      ↘ VerificationFailed → RolledBack
```

A change can also be force-rolled back by an Admin/Manager from any state.

Dependencies must be in `Verified` or `Completed` status before a dependent change can be Executed.

---

## Data Files

### users.csv
```
username,password,role
admin,admin123,Admin
```

### changes.csv
Fields are pipe-delimited (`|`). Dependencies are semicolon-delimited within the last field.
```
1000|Fix login bug|Patch auth module|Completed|1|dev|
1001|Add dark mode|UI update|Approved|3|user|1000
```

### audit_log.csv
```
ChangeID:1000, Action:Approved, By:admin, Time:2025-05-22 10:34:01
```

---

## Known Bugs

The following bugs have been identified and are pending fixes:

1. **Memory overflow on seed** — If `MAX_USERS_DYNAMIC` is set to fewer than 5 (e.g. `./crms 2`), `seedIfNoUsers()` still writes 5 default users, overflowing the allocated buffer and potentially crashing.

2. **Incorrect dependency graph indexing** — `buildDependencyGraph()` uses the change ID as the array index into `dependencyGraph.head[]`. Since IDs start at 1000, this reads/writes far out of the valid range (0–`changeCount`), corrupting memory.

3. **Manually added dependencies lost on save** — When a dependency is added interactively during approval, it is inserted directly into the graph. However, `saveChanges()` calls `buildDependencyGraph()`, which rebuilds the graph from scratch using only `changes[i].deps[]`. Any graph-only additions that were not also stored in `deps[]` are silently dropped.

4. **Unsafe `strcpy()` calls** — User input from files and interactive prompts is copied into fixed-size struct fields using `strcpy()` with no length checks. Inputs longer than the field size (e.g. a 70-character password into a 64-byte buffer) will overflow and corrupt adjacent memory.

5. **Mixed `scanf()` / `fgets()` leaves stray newlines** — `scanf()` leaves a `\n` in the input buffer. A subsequent `fgets()` call (e.g. in `submitChangeInteractive`) immediately reads that newline as an empty string, causing the title to be blank without the user typing anything.

---

## Constants

Defined in `crms.h`:

| Constant       | Default | Meaning                              |
|----------------|---------|--------------------------------------|
| `MAX_CHANGES`  | 500     | Maximum number of change records     |
| `MAX_DEPS`     | 10      | Maximum dependencies per change      |
| `MAX_LEN`      | 128     | General-purpose string buffer size   |
