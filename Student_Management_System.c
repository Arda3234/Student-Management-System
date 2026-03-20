/**
 * @file    student_management.c
 * @brief   Console-based Student Management System
 *
 * A full-featured student record management application written in C.
 * Supports persistent binary file storage, real-time input validation,
 * CRUD operations, and GPA-based sorting with human-readable report export.
 *
 * Features:
 *   - Add, search, edit, and delete student records
 *   - Weighted GPA calculation (Midterm 40% | Assignment 10% | Final 50%)
 *   - Bubble sort by GPA (descending)
 *   - Binary (.bin) and formatted text (.txt) file export
 *   - Robust input validation with graceful error handling
 *

 */

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

/* ─── Constants ──────────────────────────────────────────────────────────── */
#define SIZE               100                       /**< Max student capacity                  */
#define NAME_SIZE           52                       /**< Max characters for full name          */
#define FILE_NAME          "student.txt"             /**< Primary binary data store             */
#define SORTED_FILE        "sorted_by_gpa.txt"       /**< Binary export sorted by GPA           */
#define READABLE_FILE      "readable_student.txt"    /**< Human-readable full student report    */
#define SORTED_READABLE_FILE "readable_sorted_by_gpa.txt" /**< Human-readable GPA-sorted report */

/* ─── Data Model ─────────────────────────────────────────────────────────── */

/**
 * @struct student_info
 * @brief  Represents a single student record.
 *
 * Stores all academic information for one student.
 * The GPA field is derived — it is never entered by the user,
 * but computed automatically from the three grade components.
 */
typedef struct student_info {
    char  full_name[NAME_SIZE]; /**< Student's full name (letters and spaces only) */
    int   student_no;           /**< Unique student ID in range [1, 100]           */
    int   midterm;              /**< Midterm exam score  [0, 100]                  */
    int   assignment;           /**< Assignment score    [0, 100]                  */
    int   final;                /**< Final exam score    [0, 100]                  */
    float gpa;                  /**< Weighted average: (4*mid + assign + 5*final) / 10 */
} stdn;

/* ─── Globals ─────────────────────────────────────────────────────────────── */
stdn std_data[SIZE] = {"", 0, 0, 0, 0, 0.0}; /**< In-memory record table, indexed by (student_no - 1) */
int  student_count  = 0;                      /**< Number of currently registered students             */

/* ═══════════════════════════════════════════════════════════════════════════
 *  FILE I/O
 * ═══════════════════════════════════════════════════════════════════════════ */

/**
 * @brief Initializes the binary data file with empty student slots.
 *
 * Creates (or overwrites) the primary storage file and pre-fills it
 * with SIZE zeroed-out records, establishing a fixed-size random-access
 * structure that supports O(1) reads by student ID.
 */
void file_opener() {
    FILE *file = fopen(FILE_NAME, "wb");
    if (file == NULL) {
        puts("File cannot be opened...\n\n");
        return;
    }

    stdn null_record = {"", 0, 0, 0, 0, 0.0};
    for (int i = 0; i < SIZE; i++) {
        fwrite(&null_record, sizeof(stdn), 1, file);
    }
    fclose(file);
    puts("File created successfully...\n\n");
}

/**
 * @brief Persists the entire in-memory record table to disk.
 *
 * Writes all SIZE entries from the provided array to the binary file
 * in a single sequential pass, keeping the file and memory in sync
 * after every mutation (add / edit / delete).
 *
 * @param arr  Pointer to the global student array to be saved.
 */
void add_to_file(stdn *arr) {
    FILE *file = fopen(FILE_NAME, "wb");
    if (file == NULL) {
        printf("File cannot be opened...\n\n");
        return;
    }
    fwrite(arr, sizeof(stdn), SIZE, file);
    fclose(file);
}

/**
 * @brief Exports the student table as a formatted, human-readable text report.
 *
 * Iterates over the provided array and writes every non-empty record as
 * a fixed-width table row, suitable for printing or sharing without
 * specialised software.
 *
 * @param arr       Pointer to the student array to export.
 * @param fileName  Destination file path for the text report.
 */
void readable_file_creation(stdn *arr, char *fileName) {
    FILE *file = fopen(fileName, "w");
    if (file == NULL) {
        printf("File cannot be opened...\n\n");
        return;
    }

    /* Table header */
    fprintf(file, "%-10s | %-50s | %-8s | %-10s | %-8s | %-6s\n",
            "ID", "Full Name", "GPA", "Midterm", "Assignment", "Final");
    fprintf(file, "-------------------------------------------------------------------------------------------------------------\n");

    /* Data rows — skip empty (deleted / never-used) slots */
    for (int i = 0; i < SIZE; i++) {
        if (arr[i].student_no != 0) {
            fprintf(file, "%-10d | %-50s | %-8.1f | %-10d | %-10d | %-6d\n",
                    arr[i].student_no, arr[i].full_name, arr[i].gpa,
                    arr[i].midterm, arr[i].assignment, arr[i].final);
            fprintf(file, "-------------------------------------------------------------------------------------------------------------\n");
        }
    }
    fclose(file);
    printf("Readable file '%s' created successfully!..\n", fileName);
}

/* ═══════════════════════════════════════════════════════════════════════════
 *  DISPLAY HELPERS
 * ═══════════════════════════════════════════════════════════════════════════ */

/**
 * @brief Prints one student record as a formatted table row to stdout.
 *
 * @param arr  Array containing the student data.
 * @param num  Zero-based index of the target record within @p arr.
 */
void print_single_student(stdn *arr, int num) {
    printf("%-10d | %-50s | %-8.1f | %-10d | %-10d | %-6d\n",
           arr[num].student_no, arr[num].full_name, arr[num].gpa,
           arr[num].midterm, arr[num].assignment, arr[num].final);
}

/**
 * @brief Lists all registered students in a formatted table to stdout.
 *
 * Skips empty slots (student_no == 0) so the output contains only
 * active records regardless of their position in the array.
 *
 * @param arr    Array to display.
 * @param limit  Upper bound of indices to check (typically SIZE or registered count).
 */
void list_registered(stdn *arr, int limit) {
    if (student_count == 0) {
        printf("There are No Data to Show!..\n\n");
        return;
    }

    printf("%-10s | %-50s | %-8s | %-10s | %-8s | %-6s\n",
           "ID", "Full Name", "GPA", "Midterm", "Assignment", "Final");
    printf("-------------------------------------------------------------------------------------------------------------\n");

    for (int i = 0; i < limit; i++) {
        if (arr[i].student_no != 0) {
            print_single_student(arr, i);
            printf("-------------------------------------------------------------------------------------------------------------\n");
        }
    }
    printf("\n\n");
}

/* ═══════════════════════════════════════════════════════════════════════════
 *  CRUD OPERATIONS
 * ═══════════════════════════════════════════════════════════════════════════ */

/**
 * @brief Interactively collects and validates a new student record.
 *
 * Guides the user through entering a full name, unique student ID,
 * and three grade components. Each field is validated independently:
 *
 *  - Full name : letters and spaces only, 3–50 characters; type "END" to cancel.
 *  - Student ID: digits only, range [1, 100], must be unique; type "0" to cancel.
 *  - Grades    : digits only, range [0, 100]; type "-1" to cancel.
 *
 * On success, the GPA is computed and the record is written to both
 * the in-memory table and the binary file.
 *
 * @return  0 on successful insertion, 1 if the user cancelled at any prompt.
 */
int add_student() {
    stdn temp;
    int  control;

    /* ── Full Name ─────────────────────────────────────────────────────── */
    while (1) {
        control = 1;
        char end_check[NAME_SIZE] = {"0"};

        printf("Please enter the full name of the student (Don't Use Special Characters)(Type END to exit)\n");
        fgets(temp.full_name, sizeof(temp.full_name), stdin);
        temp.full_name[strcspn(temp.full_name, "\n")] = '\0';

        if (strcspn(temp.full_name, "\0\n") > 50) {
            printf("Full Name Limit Exceeded, Please Stay Within the 50 Character Limit!..\n");
            while (getchar() != '\n') {}
            continue;
        }

        if (temp.full_name[0] == '\0') {
            printf("Invalid Name!..\n\n");
            continue;
        }

        /* Build uppercase copy to detect the "END" sentinel */
        int len = strlen(temp.full_name);
        for (int j = 0; j < len; j++) end_check[j] = toupper(temp.full_name[j]);
        end_check[len] = '\0';

        if (!strncmp(end_check, "END", 3) && strcspn(end_check, " \0") == 3) {
            printf("Terminating Process...\n\n");
            return 1;
        }

        /* Reject non-alphabetic characters and names shorter than 3 letters */
        for (int i = 0; i < strlen(temp.full_name); i++) {
            if ((!isalpha(temp.full_name[i]) && temp.full_name[i] != ' ') ||
                strcspn(temp.full_name, " \0") < 3) {
                printf("Invalid Name!..\n\n");
                control = 0;
                break;
            }
        }
        if (control == 0) continue;
        break;
    }

    /* ── Student ID ────────────────────────────────────────────────────── */
    char id[50], grade[10];

    while (1) {
        control = 1;
        printf("Enter the student's ID (Type '0' to exit)\n");
        fgets(id, sizeof(id), stdin);
        id[strcspn(id, "\n")] = '\0';

        if (id[0] == '\0') { printf("Invalid Student ID!..\n\n"); continue; }

        if (strlen(grade) > 5) {
            printf("Invalid Student ID!..\n\n");
            while (getchar() != '\n') {}
            continue;
        }

        for (int i = 0; i < strlen(id); i++) {
            if (!isdigit(id[i])) { printf("Invalid Student ID!..\n\n"); control = 0; break; }
        }
        if (control == 0) continue;

        temp.student_no = atoi(id);
        if (temp.student_no == 0) { printf("Terminating Process...\n\n"); return 1; }

        if (temp.student_no < 1 || temp.student_no > 100) {
            printf("Invalid Student ID!..\n\n");
            continue;
        }
        if (std_data[temp.student_no - 1].student_no != 0) {
            printf("A student with same number already exists!..\n\n");
            continue;
        }
        break;
    }

    /* ── Midterm Grade ─────────────────────────────────────────────────── */
    while (1) {
        control = 1;
        printf("Enter the student's midterm grade (Type '-1' to exit)\n");
        fgets(grade, sizeof(grade), stdin);
        grade[strcspn(grade, "\n")] = '\0';

        if (grade[0] == '\0') { printf("Invalid Midterm Grade!..\n\n"); continue; }
        if (strlen(grade) > 5) {
            printf("Invalid Midterm Grade!..\n\n");
            while (getchar() != '\n') {}
            continue;
        }
        if (!strcmp(grade, "-1")) { printf("Terminating Process...\n\n"); return 1; }

        for (int i = 0; i < strlen(grade); i++) {
            if (!isdigit(grade[i])) { printf("Invalid Midterm Grade!..\n\n"); control = 0; break; }
        }
        if (control == 0) continue;

        temp.midterm = atoi(grade);
        if (temp.midterm < 0 || temp.midterm > 100) { printf("Invalid Midterm Grade!..\n\n"); continue; }
        break;
    }

    /* ── Assignment Grade ──────────────────────────────────────────────── */
    while (1) {
        control = 1;
        printf("Enter the student's assignment grade (Type '-1' to exit)\n");
        fgets(grade, sizeof(grade), stdin);
        grade[strcspn(grade, "\n")] = '\0';

        if (grade[0] == '\0') { printf("Invalid Assignment Grade!..\n\n"); continue; }
        if (strlen(grade) > 5) {
            printf("Invalid Assignment Grade!..\n\n");
            while (getchar() != '\n') {}
            continue;
        }
        if (!strcmp(grade, "-1")) { printf("Terminating Process...\n\n"); return 1; }

        for (int i = 0; i < strlen(grade); i++) {
            if (!isdigit(grade[i])) { printf("Invalid Assignment Grade!..\n\n"); control = 0; break; }
        }
        if (control == 0) continue;

        temp.assignment = atoi(grade);
        if (temp.assignment < 0 || temp.assignment > 100) { printf("Invalid Assignment Grade!..\n\n"); continue; }
        break;
    }

    /* ── Final Grade ───────────────────────────────────────────────────── */
    while (1) {
        control = 1;
        printf("Enter the student's final grade (Type '-1' to exit)\n");
        fgets(grade, sizeof(grade), stdin);
        grade[strcspn(grade, "\n")] = '\0';

        if (grade[0] == '\0') { printf("Invalid Final Grade!..\n\n"); continue; }
        if (strlen(grade) > 5) {
            printf("Invalid Final Grade!..\n\n");
            while (getchar() != '\n') {}
            continue;
        }
        if (!strcmp(grade, "-1")) { printf("Terminating Process...\n\n"); return 1; }

        for (int i = 0; i < strlen(grade); i++) {
            if (!isdigit(grade[i])) { printf("Invalid Final Grade!..\n\n"); control = 0; break; }
        }
        if (control == 0) continue;

        temp.final = atoi(grade);
        if (temp.final < 0 || temp.final > 100) { printf("Invalid Final Grade!..\n\n"); continue; }
        break;
    }

    /* ── Compute GPA and commit record ─────────────────────────────────── */
    temp.gpa = (float)((4 * temp.midterm) + temp.assignment + (5 * temp.final)) / 10;

    std_data[temp.student_no - 1] = temp; /* Place record at ID-indexed slot */
    add_to_file(std_data);
    student_count++;

    printf("Student Data Successfully Added to the File...\n\n");
    return 0;
}

/**
 * @brief Searches for a student by ID and offers edit or delete operations.
 *
 * Prompts the user for a student ID, retrieves the matching record,
 * displays it, then presents a sub-menu:
 *
 *  1. Delete — zeroes the slot and decrements the counter.
 *  2. Edit   — backs up the current record, clears the slot, and
 *              calls add_student(); restores the backup if the user cancels.
 *  3. Exit   — returns to the main menu without changes.
 */
void search_edit_delete() {
    int  temp_id, control;
    char id_check[20] = {"0"};

    if (student_count == 0) {
        printf("There are No Data to Search For!..\n\n");
        return;
    }

    /* ── ID Lookup ─────────────────────────────────────────────────────── */
    while (1) {
        control = 1;
        printf("Enter the student's ID that you want to search for (Enter '0' to exit)\n");
        fgets(id_check, sizeof(id_check), stdin);
        id_check[strcspn(id_check, "\n")] = '\0';

        if (!strcmp(id_check, "0")) { printf("Terminating Process...\n\n"); return; }
        if (id_check[0] == '\0')    { printf("Invalid Student ID!..\n\n");  continue; }

        for (int i = 0; i < strlen(id_check); i++) {
            if (!isdigit(id_check[i])) { printf("Invalid Student ID!..\n\n"); control = 0; break; }
        }
        if (control == 0) continue;

        temp_id = atoi(id_check);
        if (temp_id < 0 || temp_id > 100) { printf("Invalid Student ID!..\n\n"); continue; }
        break;
    }

    if (std_data[temp_id - 1].student_no == 0) {
        printf("Student cannot be found...\n\n");
        return;
    }

    /* Display the found record */
    printf("%-10s | %-50s | %-8s | %-10s | %-8s | %-6s\n",
           "ID", "Full Name", "GPA", "Midterm", "Assignment", "Final");
    printf("-------------------------------------------------------------------------------------------------------------\n");
    print_single_student(std_data, temp_id - 1);
    printf("-------------------------------------------------------------------------------------------------------------\n");

    /* ── Edit / Delete Sub-menu ────────────────────────────────────────── */
    char choice[10];
    while (1) {
        stdn blank = {"", 0, 0, 0, 0, 0.0};
        stdn save;

        printf("Choose the next operation\n1. Delete\n2. Edit\n3. Exit\n");
        fgets(choice, sizeof(choice), stdin);
        choice[strcspn(choice, "\n")] = '\0';

        if (strlen(choice) != 1) { printf("Invalid Input!..\n\n"); continue; }

        switch (choice[0]) {
            case '1': /* ── Delete ── */
                std_data[temp_id - 1] = blank;
                add_to_file(std_data);
                printf("Student Data Successfully Deleted...\n\n");
                student_count--;
                break;

            case '2': /* ── Edit ── */
                save = std_data[temp_id - 1]; /* Preserve original in case of cancellation */
                std_data[temp_id - 1] = blank;
                printf("Starting the Editing Process...\n\n");

                if (add_student()) {           /* User cancelled mid-edit → rollback        */
                    printf("Editing Failed...\n");
                    std_data[temp_id - 1] = save;
                    break;
                }
                student_count--;
                printf("Editing is Complete...\n\n");
                break;

            case '3': /* ── Exit ── */
                printf("Exiting...\n\n");
                break;

            default:
                printf("Invalid Input!..\n\n");
        }
        break;
    }
}

/* ═══════════════════════════════════════════════════════════════════════════
 *  SORTING
 * ═══════════════════════════════════════════════════════════════════════════ */

/**
 * @brief Sorts a student array by GPA in descending order (bubble sort).
 *
 * Operates in-place on the provided array using the classic O(n²)
 * bubble sort algorithm. Suitable for the fixed small dataset (≤ 100 records)
 * defined by this system's capacity.
 *
 * @param arr  Array of student records to sort in-place.
 */
void bubble_sort(stdn *arr) {
    for (int i = 0; i < SIZE - 1; i++) {
        for (int j = 0; j < SIZE - 1 - i; j++) {
            if (arr[j].gpa < arr[j + 1].gpa) {
                stdn temp = arr[j];
                arr[j]     = arr[j + 1];
                arr[j + 1] = temp;
            }
        }
    }
}

/**
 * @brief Displays all students ranked by GPA and optionally exports the result.
 *
 * Compacts the sparse global table into a contiguous temporary array
 * (removing empty slots), sorts it by GPA descending, and prints the
 * ranking. The user is then prompted to optionally save both a binary
 * and a human-readable version of the sorted output.
 */
void sort_by_gpa() {
    if (student_count == 0) {
        printf("There are No Data to Show!..\n\n");
        return;
    }

    /* Build a compact, sorted copy — preserves the original table untouched */
    stdn sorted[SIZE] = {"", 0, 0, 0, 0, 0.0};
    int  registered   = 0;

    for (int i = 0; i < SIZE; i++) {
        if (std_data[i].student_no != 0) sorted[registered++] = std_data[i];
    }

    bubble_sort(sorted);
    list_registered(sorted, registered);

    /* ── Optional export ───────────────────────────────────────────────── */
    char check[20];
    while (1) {
        printf("\nWould You Like to Save the Sorted File ?\n(1. Yes 2. No)\n");
        fgets(check, sizeof(check), stdin);
        check[strcspn(check, "\n")] = '\0';

        if (strlen(check) > 1) { printf("Invalid Input!..\n\n"); continue; }

        if (check[0] == '1') {
            FILE *fSort = fopen(SORTED_FILE, "wb");
            for (int i = 0; i < registered; i++) fwrite(&sorted[i], sizeof(stdn), 1, fSort);
            readable_file_creation(sorted, SORTED_READABLE_FILE);
            fclose(fSort);
            break;
        } else if (check[0] == '2') {
            printf("Returning to the Main Menu...\n\n");
            break;
        } else {
            printf("Invalid Input, Try Again...\n\n");
        }
    }
}

/* ═══════════════════════════════════════════════════════════════════════════
 *  ENTRY POINT
 * ═══════════════════════════════════════════════════════════════════════════ */

/**
 * @brief Program entry point — drives the main interactive menu loop.
 *
 * Presents a numbered menu and dispatches to the appropriate subsystem.
 * Operations 2–5 require the file to be initialised first (option 1).
 * On exit (option 6) a human-readable report is written automatically
 * if any students are registered.
 *
 * @return 0 on normal program termination.
 */
int main() {
    int file_control = 0; /* Guards against operating on an uninitialised file */

    while (1) {
        char operation[50];

        printf("---STUDENT MANAGEMENT PROGRAM---\n");
        printf("Please Choose an Operation\n"
               "1. Open a File\n"
               "2. Add New Student\n"
               "3. List All Registered Students\n"
               "4. Search & Edit/Delete Student\n"
               "5. Sort Students by GPA\n"
               "6. Exit Program.\n");

        fgets(operation, sizeof(operation), stdin);

        /* Reject anything other than a single digit + newline */
        if (strlen(operation) != 2 || operation[1] != '\n') {
            printf("Invalid Operation Input!..\n\n");
            continue;
        }

        /* File must be opened before any data operation */
        if ((operation[0] > '1' && operation[0] < '6') && file_control == 0) {
            printf("You Need to Open a File to Operate!..\n\n");
            continue;
        }

        switch (operation[0]) {
            case '1': /* Open / initialise file */
                if (file_control != 0) { printf("You Have Already Opened a File!..\n\n"); break; }
                file_opener();
                file_control = 1;
                break;

            case '2': /* Register a new student */
                if (student_count == 100) {
                    printf("Maximum Capacity Reached, Can't Add Anymore Data...\n\n");
                    break;
                }
                add_student();
                break;

            case '3': /* Print all records */
                list_registered(std_data, SIZE);
                break;

            case '4': /* Search, edit, or delete */
                search_edit_delete();
                break;

            case '5': /* GPA ranking */
                sort_by_gpa();
                break;

            case '6': /* Graceful shutdown with auto-export */
                if (student_count != 0) readable_file_creation(std_data, READABLE_FILE);
                printf("Exiting Program...\n\n");
                return 0;

            default:
                printf("Invalid Operation Input!..\n\n");
        }
    }
}
