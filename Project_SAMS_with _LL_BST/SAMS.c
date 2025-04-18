
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>
#ifdef _WIN32
    #include <windows.h>
#else
    #include <unistd.h>
#endif

#define MAX_NAME_LENGTH 50
#define MAX_DEPT_LENGTH 50
#define MAX_USERNAME_LENGTH 20
#define MAX_PASSWORD_LENGTH 20
#define MAX_DATE_LENGTH 30

/* ----------------------------- Data Structures ----------------------------- */

// Node for a linked list to hold a student's semester payment details.
typedef struct PaymentNode {
    int semester;
    float amount_paid;
    struct PaymentNode* next;
} PaymentNode;

// Student structure which is stored in a Binary Search Tree (BST) keyed by student_id.
// It also contains a linked list of semester payments.
typedef struct Student {
    char student_id[20];
    char name[MAX_NAME_LENGTH];
    char department[MAX_DEPT_LENGTH];
    float admission_fee_paid;
    PaymentNode* payments; // Linked list for semester payments.
    char created_date[MAX_DATE_LENGTH];
    char last_updated[MAX_DATE_LENGTH];
    // BST pointers:
    struct Student* left;
    struct Student* right;
} Student;

// Administrative settings used throughout the program.
typedef struct {
    float tuition_fee;
    float admission_fee;
    float display_multiplier;
    char admin_username[MAX_USERNAME_LENGTH];
    char admin_password[MAX_PASSWORD_LENGTH];
} AdminSettings;

/* ----------------------------- Global Variables ----------------------------- */
Student* studentRoot = NULL; // Root of the BST containing student records.
AdminSettings admin_settings = {35067.0f, 55700.0f, 1.0f, "Tajwar", "tajwar123"};
char current_user[MAX_NAME_LENGTH] = "";
char user_type[10] = "";

/* ----------------------------- Sleep Helper Function ----------------------------- */
// sleep_sec: Sleeps for given seconds. Uses Sleep() on Windows (milliseconds) and sleep() on UNIX.
static void sleep_sec(int seconds) {
#ifdef _WIN32
    Sleep(seconds * 1000);
#else
    sleep(seconds);
#endif
}

/* ----------------------------- createPaymentNode Function ----------------------------- */
// Creates a new PaymentNode with the given semester and amount_paid.
PaymentNode* createPaymentNode(int semester, float amount_paid) {
    PaymentNode* new_payment = (PaymentNode*)malloc(sizeof(PaymentNode));
    if(new_payment == NULL) {
        perror("Failed to allocate memory for a new payment");
        exit(EXIT_FAILURE);
    }
    new_payment->semester = semester;
    new_payment->amount_paid = amount_paid;
    new_payment->next = NULL;
    return new_payment;
}

/* ----------------------------- Static Helper Functions ----------------------------- */

// Clears the terminal screen based on the operating system.
static void clearScreen() {
#ifdef _WIN32
    system("cls");
#else
    system("clear");
#endif
}

// Returns the current date and time string in the format "YYYY-MM-DD HH:MM:SS".
static void getCurrentDateTime(char* dateTime) {
    time_t now = time(NULL);
    struct tm* t = localtime(&now);
    strftime(dateTime, MAX_DATE_LENGTH, "%Y-%m-%d %H:%M:%S", t);
}

// Counts the total number of students in the BST.
static int bstCountNodes(Student* node) {
    if (node == NULL)
        return 0;
    return 1 + bstCountNodes(node->left) + bstCountNodes(node->right);
}

// Accumulates total admission fee, total tuition fee, and count of payment entries through in-order traversal.
static void inorderAccumTotals(Student* node, float* totalAdmission, float* totalTuition, int* totalEntries) {
    if (node == NULL)
        return;
    inorderAccumTotals(node->left, totalAdmission, totalTuition, totalEntries);
    *totalAdmission += node->admission_fee_paid;
    PaymentNode* p = node->payments;
    while (p != NULL) {
        *totalTuition += p->amount_paid;
        (*totalEntries)++;
        p = p->next;
    }
    inorderAccumTotals(node->right, totalAdmission, totalTuition, totalEntries);
}

// Writes student data in-order to the given file.
static void bstWriteStudentData(Student* node, FILE* file) {
    if (node == NULL)
        return;
    bstWriteStudentData(node->left, file);
    fwrite(node->student_id, sizeof(char), 20, file);
    fwrite(node->name, sizeof(char), MAX_NAME_LENGTH, file);
    fwrite(node->department, sizeof(char), MAX_DEPT_LENGTH, file);
    fwrite(&node->admission_fee_paid, sizeof(float), 1, file);
    fwrite(node->created_date, sizeof(char), MAX_DATE_LENGTH, file);
    fwrite(node->last_updated, sizeof(char), MAX_DATE_LENGTH, file);
    int payment_count = 0;
    PaymentNode* p = node->payments;
    while (p != NULL) {
        payment_count++;
        p = p->next;
    }
    fwrite(&payment_count, sizeof(int), 1, file);
    p = node->payments;
    while (p != NULL) {
        fwrite(&p->semester, sizeof(int), 1, file);
        fwrite(&p->amount_paid, sizeof(float), 1, file);
        p = p->next;
    }
    bstWriteStudentData(node->right, file);
}

// Helper for name search: in-order traversal to search student names (case-insensitive).
static void inorderSearchByNameHelper(Student* node, const char* search_lower) {
    if (node == NULL)
        return;
    inorderSearchByNameHelper(node->left, search_lower);
    // Convert student's name to lowercase for comparison.
    char name_lower[MAX_NAME_LENGTH];
    strcpy(name_lower, node->name);
    for (int i = 0; name_lower[i]; i++) {
        name_lower[i] = tolower(name_lower[i]);
    }
    if (strstr(name_lower, search_lower) != NULL) {
        printf("ID: %s, Name: %s, Dept: %s\n", node->student_id, node->name, node->department);
    }
    inorderSearchByNameHelper(node->right, search_lower);
}

/* ----------------------------- Function Prototypes (Non-Static) ----------------------------- */

void displayHeader();
int loginScreen();
void adminMenu();
void displayStudentMenu(Student* student);
void addStudent();
void viewAllStudents();
void searchStudent();
void updateStudentInfo();
void deleteStudent();
void makeSemesterPayment(Student* student);
void modifyFeeSettings();
void modifyDisplayMultiplier();
Student* bstInsert(Student* root, Student* new_student);
Student* bstSearch(Student* root, const char* student_id);
Student* bstDelete(Student* root, const char* student_id);
Student* minValueStudent(Student* node);
void inorderDisplay(Student* root);
float calculateTotalPaid(Student* student);
float calculateDue(Student* student);
void displayStudentInfo(Student* student);
void displayTotalAmountPaid();
void saveData();
void loadData();

/* ----------------------------- Function Implementations ----------------------------- */

/* Displays a header with program title and login information. */
void displayHeader() {
    clearScreen();
    printf("\n==================================================\n");
    printf("       STUDENT ACCOUNT MANAGEMENT SYSTEM\n");
    printf("==================================================\n");
    if (strlen(current_user) > 0) {
        printf("Logged in as: %s (%s)\n", current_user, user_type);
    }
    printf("--------------------------------------------------\n");
}

/* BST insertion function: inserts a new student node keyed by student_id. */
Student* bstInsert(Student* root, Student* new_student) {
    if (root == NULL)
        return new_student;
    if (strcmp(new_student->student_id, root->student_id) < 0)
        root->left = bstInsert(root->left, new_student);
    else if (strcmp(new_student->student_id, root->student_id) > 0)
        root->right = bstInsert(root->right, new_student);
    // Duplicate keys are not allowed.
    return root;
}

/* BST search for a student by their student_id. */
Student* bstSearch(Student* root, const char* student_id) {
    if (root == NULL || strcmp(root->student_id, student_id) == 0)
        return root;
    if (strcmp(student_id, root->student_id) < 0)
        return bstSearch(root->left, student_id);
    return bstSearch(root->right, student_id);
}

/* Returns the student node with the minimum student_id (leftmost node). */
Student* minValueStudent(Student* node) {
    Student* current = node;
    while (current && current->left != NULL)
        current = current->left;
    return current;
}

/* BST deletion for a student node keyed by student_id.
   Copies data from the in-order successor and deletes that node. */
Student* bstDelete(Student* root, const char* student_id) {
    if (root == NULL)
        return root;
    
    if (strcmp(student_id, root->student_id) < 0)
        root->left = bstDelete(root->left, student_id);
    else if (strcmp(student_id, root->student_id) > 0)
        root->right = bstDelete(root->right, student_id);
    else {
        // Node with one or no child.
        if (root->left == NULL) {
            Student* temp = root->right;
            free(root);
            return temp;
        } else if (root->right == NULL) {
            Student* temp = root->left;
            free(root);
            return temp;
        }
        // Node with two children: Get the inorder successor.
        Student* temp = minValueStudent(root->right);
        // Copy the inorder successor's data to this node.
        strcpy(root->student_id, temp->student_id);
        strcpy(root->name, temp->name);
        strcpy(root->department, temp->department);
        root->admission_fee_paid = temp->admission_fee_paid;
        strcpy(root->created_date, temp->created_date);
        strcpy(root->last_updated, temp->last_updated);
        // For simplicity, move the payment linked list pointer.
        root->payments = temp->payments;
        root->right = bstDelete(root->right, temp->student_id);
    }
    return root;
}

/* In-order traversal to display all students along with their payment summary. */
void inorderDisplay(Student* root) {
    if (root != NULL) {
        inorderDisplay(root->left);
        float total_paid = calculateTotalPaid(root) * admin_settings.display_multiplier;
        float due = calculateDue(root) * admin_settings.display_multiplier;
        printf("%-10s %-20s %-15s %-14.2f taka %-14.2f taka\n", 
            root->student_id, root->name, root->department, total_paid, due);
        inorderDisplay(root->right);
    }
}

/* Login screen that prompts for Admin or Student login.
   Returns 1 for Admin login and 0 when exiting the system. */
int loginScreen() {
    int choice;
    char student_id[20];
    char username[MAX_USERNAME_LENGTH];
    char password[MAX_PASSWORD_LENGTH];
    Student* student = NULL;
    
    while (1) {
        displayHeader();
        printf("\nPlease select login type:\n");
        printf("1. Admin Login\n");
        printf("2. User/Student Login\n");
        printf("3. Exit System\n");
        
        printf("\nEnter your choice (1-3): ");
        if (scanf("%d", &choice) != 1) {
            while(getchar() != '\n'); // Clear input
            continue;
        }
        getchar();
        
        switch (choice) {
            case 1:
                printf("\nEnter admin username: ");
                fgets(username, MAX_USERNAME_LENGTH, stdin);
                username[strcspn(username, "\n")] = 0;
                printf("Enter admin password: ");
                fgets(password, MAX_PASSWORD_LENGTH, stdin);
                password[strcspn(password, "\n")] = 0;
                if (strcmp(username, admin_settings.admin_username) == 0 && 
                    strcmp(password, admin_settings.admin_password) == 0) {
                    strcpy(current_user, username);
                    strcpy(user_type, "Admin");
                    printf("\nAdmin login successful!\n");
                    sleep_sec(1);
                    return 1;
                } else {
                    printf("\nInvalid admin credentials. Please try again.\n");
                    sleep_sec(1);
                }
                break;
            case 2:
                printf("\nEnter student ID: ");
                fgets(student_id, 20, stdin);
                student_id[strcspn(student_id, "\n")] = 0;
                student = bstSearch(studentRoot, student_id);
                if (student != NULL) {
                    strcpy(current_user, student->name);
                    strcpy(user_type, "Student");
                    printf("\nStudent login successful!\n");
                    sleep_sec(1);
                    displayStudentMenu(student);
                    strcpy(current_user, "");
                    strcpy(user_type, "");
                } else {
                    printf("\nStudent ID not found. Please try again.\n");
                    sleep_sec(1);
                }
                break;
            case 3:
                printf("\nExiting system. Goodbye!\n");
                saveData();
                return 0;
            default:
                printf("\nInvalid choice. Please try again.\n");
                sleep_sec(1);
                break;
        }
    }
}

/* Admin menu with options for managing students and settings. */
void adminMenu() {
    int choice;
    while (1) {
        displayHeader();
        printf("\nADMIN MENU\n");
        printf("--------------------------------------------------\n");
        printf("1. Add New Student\n");
        printf("2. View All Students\n");
        printf("3. Search Student by ID/Name\n");
        printf("4. Update Student Information\n");
        printf("5. Delete Student\n");
        printf("6. Modify Fee Settings\n");
        printf("7. Modify Display Multiplier\n");
        printf("8. Show Total Amount Paid by All Students\n");
        printf("9. Logout\n");
        
        printf("\nEnter your choice (1-9): ");
        if (scanf("%d", &choice) != 1) {
            while(getchar() != '\n');
            continue;
        }
        getchar();
        switch (choice) {
            case 1:
                addStudent();
                break;
            case 2:
                viewAllStudents();
                break;
            case 3:
                searchStudent();
                break;
            case 4:
                updateStudentInfo();
                break;
            case 5:
                deleteStudent();
                break;
            case 6:
                modifyFeeSettings();
                break;
            case 7:
                modifyDisplayMultiplier();
                break;
            case 8:
                displayTotalAmountPaid();
                break;
            case 9:
                printf("\nLogging out...\n");
                sleep_sec(1);
                saveData();
                return;
            default:
                printf("\nInvalid choice. Please try again.\n");
                sleep_sec(1);
                break;
        }
    }
}

/* Student menu for logged in students. */
void displayStudentMenu(Student* student) {
    int choice;
    while (1) {
        displayHeader();
        printf("\nSTUDENT MENU\n");
        printf("--------------------------------------------------\n");
        printf("1. View My Information\n");
        printf("2. Make Semester Payment\n");
        printf("3. Logout\n");
        
        printf("\nEnter your choice (1-3): ");
        if (scanf("%d", &choice) != 1) {
            while(getchar() != '\n');
            continue;
        }
        getchar();
        switch (choice) {
            case 1:
                displayStudentInfo(student);
                break;
            case 2:
                makeSemesterPayment(student);
                break;
            case 3:
                printf("\nLogging out...\n");
                sleep_sec(1);
                return;
            default:
                printf("\nInvalid choice. Please try again.\n");
                sleep_sec(1);
                break;
        }
    }
}

/* Adds a new student to the system.
   Prompts for basic information and optionally for an immediate semester payment. */
void addStudent() {
    Student* new_student = (Student*)malloc(sizeof(Student));
    if (!new_student) {
        perror("Memory allocation error");
        exit(EXIT_FAILURE);
    }
    // Initialize BST pointers and payment list.
    new_student->payments = NULL;
    new_student->left = new_student->right = NULL;
    
    char admission_choice, payment_choice;
    displayHeader();
    printf("\nADD NEW STUDENT\n");
    printf("--------------------------------------------------\n");
    
    printf("Enter Student ID: ");
    fgets(new_student->student_id, 20, stdin);
    new_student->student_id[strcspn(new_student->student_id, "\n")] = 0;
    
    if (bstSearch(studentRoot, new_student->student_id) != NULL) {
        printf("\nError: A student with this ID already exists.\n");
        sleep_sec(1);
        free(new_student);
        return;
    }
    
    printf("Enter Student Name: ");
    fgets(new_student->name, MAX_NAME_LENGTH, stdin);
    new_student->name[strcspn(new_student->name, "\n")] = 0;
    
    printf("Enter Department: ");
    fgets(new_student->department, MAX_DEPT_LENGTH, stdin);
    new_student->department[strcspn(new_student->department, "\n")] = 0;
    
    printf("\nAdmission Fee (fixed): %.2f taka\n", admin_settings.admission_fee);
    printf("Has the student paid the admission fee? (y/n): ");
    scanf(" %c", &admission_choice);
    getchar();
    
    new_student->admission_fee_paid = (tolower(admission_choice) == 'y') ? admin_settings.admission_fee : 0;
    getCurrentDateTime(new_student->created_date);
    strcpy(new_student->last_updated, new_student->created_date);
    
    // Insert the new student into the BST.
    studentRoot = bstInsert(studentRoot, new_student);
    printf("\nStudent added successfully!\n");
    sleep_sec(1);
    
    printf("\nDo you want to add a semester payment now? (y/n): ");
    scanf(" %c", &payment_choice);
    getchar();
    if (tolower(payment_choice) == 'y') {
        makeSemesterPayment(new_student);
    } else {
        printf("\nNo semester payment added.\n");
        sleep_sec(1);
    }
    saveData();
}

/* Displays all student records using an in-order traversal of the BST. */
void viewAllStudents() {
    displayHeader();
    printf("\nALL STUDENTS\n");
    printf("--------------------------------------------------\n");
    
    if (studentRoot == NULL) {
        printf("No students found in the system.\n");
        printf("\nPress Enter to continue...");
        getchar();
        return;
    }
    
    printf("%-10s %-20s %-15s %-15s %-15s\n", "ID", "Name", "Department", "Total Paid", "Due Amount");
    printf("-----------------------------------------------------------------------\n");
    inorderDisplay(studentRoot);
    
    int count = bstCountNodes(studentRoot);
    printf("\nTotal Students: %d\n", count);
    printf("\nPress Enter to continue...");
    getchar();
}

/* Searches for a student either by ID or by name.
   In the case of a name search, an in-order traversal is used to list matching records. */
void searchStudent() {
    int choice;
    char search_term[MAX_NAME_LENGTH];
    Student* student = NULL;
    
    displayHeader();
    printf("\nSEARCH STUDENT\n");
    printf("--------------------------------------------------\n");
    printf("1. Search by ID\n");
    printf("2. Search by Name\n");
    printf("3. Return to Admin Menu\n");
    
    printf("\nEnter your choice (1-3): ");
    if (scanf("%d", &choice) != 1) {
        while(getchar()!='\n');
        return;
    }
    getchar();
    
    switch (choice) {
        case 1:
            printf("\nEnter Student ID to search: ");
            fgets(search_term, MAX_NAME_LENGTH, stdin);
            search_term[strcspn(search_term, "\n")] = 0;
            student = bstSearch(studentRoot, search_term);
            if (student != NULL) {
                displayStudentInfo(student);
            } else {
                printf("\nNo student found with that ID.\n");
                sleep_sec(1);
            }
            break;
        case 2:
            printf("\nEnter Student Name to search: ");
            fgets(search_term, MAX_NAME_LENGTH, stdin);
            search_term[strcspn(search_term, "\n")] = 0;
            // Convert search term to lowercase.
            for (int i = 0; search_term[i]; i++) {
                search_term[i] = tolower(search_term[i]);
            }
            printf("\nSearch Results:\n");
            printf("--------------------------------------------------\n");
            inorderSearchByNameHelper(studentRoot, search_term);
            printf("\nPress Enter to continue...");
            getchar();
            break;
        case 3:
            return;
        default:
            printf("\nInvalid choice.\n");
            sleep_sec(1);
            break;
    }
}

/* Updates the information of an existing student identified by student_id. */
void updateStudentInfo() {
    char student_id[20];
    Student* student = NULL;
    int choice;
    char new_name[MAX_NAME_LENGTH];
    char new_department[MAX_DEPT_LENGTH];
    char admission_choice;
    
    displayHeader();
    printf("\nUPDATE STUDENT INFORMATION\n");
    printf("--------------------------------------------------\n");
    printf("Enter Student ID to update: ");
    fgets(student_id, 20, stdin);
    student_id[strcspn(student_id, "\n")] = 0;
    
    student = bstSearch(studentRoot, student_id);
    if (student == NULL) {
        printf("\nNo student found with that ID.\n");
        sleep_sec(1);
        return;
    }
    displayStudentInfo(student);
    
    displayHeader();
    printf("\nUPDATE STUDENT INFORMATION\n");
    printf("--------------------------------------------------\n");
    printf("Student ID: %s\n", student->student_id);
    printf("\nWhat would you like to update?\n");
    printf("1. Name\n");
    printf("2. Department\n");
    printf("3. Admission Fee Payment Status\n");
    printf("4. Return to Admin Menu\n");
    
    printf("\nEnter your choice (1-4): ");
    if (scanf("%d", &choice) != 1) {
        while(getchar()!='\n');
        return;
    }
    getchar();
    
    switch (choice) {
        case 1:
            printf("\nCurrent Name: %s\n", student->name);
            printf("Enter New Name: ");
            fgets(new_name, MAX_NAME_LENGTH, stdin);
            new_name[strcspn(new_name, "\n")] = 0;
            strcpy(student->name, new_name);
            getCurrentDateTime(student->last_updated);
            printf("\nName updated successfully!\n");
            break;
        case 2:
            printf("\nCurrent Department: %s\n", student->department);
            printf("Enter New Department: ");
            fgets(new_department, MAX_DEPT_LENGTH, stdin);
            new_department[strcspn(new_department, "\n")] = 0;
            strcpy(student->department, new_department);
            getCurrentDateTime(student->last_updated);
            printf("\nDepartment updated successfully!\n");
            break;
        case 3:
            printf("\nCurrent Admission Fee Paid: %.2f taka of %.2f taka\n", 
                   student->admission_fee_paid, admin_settings.admission_fee);
            printf("Has the student paid the admission fee? (y/n): ");
            scanf(" %c", &admission_choice);
            getchar();
            student->admission_fee_paid = (tolower(admission_choice) == 'y') ? admin_settings.admission_fee : 0;
            getCurrentDateTime(student->last_updated);
            printf("\nAdmission fee status updated successfully!\n");
            break;
        case 4:
            return;
        default:
            printf("\nInvalid choice.\n");
            break;
    }
    sleep_sec(1);
    saveData();
}

/* Deletes a student from the BST and frees associated memory. */
void deleteStudent() {
    char student_id[20];
    char confirm;
    displayHeader();
    printf("\nDELETE STUDENT\n");
    printf("--------------------------------------------------\n");
    printf("Enter Student ID to delete: ");
    fgets(student_id, 20, stdin);
    student_id[strcspn(student_id, "\n")] = 0;
    
    Student* target = bstSearch(studentRoot, student_id);
    if (target != NULL) {
        displayStudentInfo(target);
        printf("\nAre you sure you want to delete this student? (y/n): ");
        scanf(" %c", &confirm);
        getchar();
        if (tolower(confirm) == 'y') {
            // Free all nodes in the payment linked list for this student.
            PaymentNode* p = target->payments;
            while (p != NULL) {
                PaymentNode* temp = p;
                p = p->next;
                free(temp);
            }
            studentRoot = bstDelete(studentRoot, student_id);
            printf("\nStudent deleted successfully!\n");
            saveData();
        } else {
            printf("\nDeletion cancelled.\n");
        }
    } else {
        printf("\nNo student found with that ID.\n");
    }
    sleep_sec(1);
}

/* Records or updates a semester payment entry for the given student.
   If a payment for the entered semester already exists, asks for confirmation to overwrite it.
   Additionally, enforces that students can only make a payment for the next sequential semester. */
void makeSemesterPayment(Student* student) {
    int semester;
    float payment;
    PaymentNode* ptr = student->payments;
    PaymentNode* target = NULL;
    char overwrite;
    
    displayHeader();
    printf("\nMAKE SEMESTER PAYMENT\n");
    printf("--------------------------------------------------\n");
    printf("Student: %s (ID: %s)\n", student->name, student->student_id);
    printf("\nCurrent Semester Payments:\n");
    if (student->payments != NULL) {
        ptr = student->payments;
        while (ptr != NULL) {
            printf("Semester %d: %.2f taka\n", ptr->semester, ptr->amount_paid);
            ptr = ptr->next;
        }
    } else {
        printf("No semester payments recorded yet.\n");
    }
    
    // Compute the highest semester paid so far.
    int current_max = 0;
    PaymentNode* temp = student->payments;
    while (temp != NULL) {
        if (temp->semester > current_max)
            current_max = temp->semester;
        temp = temp->next;
    }
    
    while (1) {
        printf("\nEnter Semester Number: ");
        if (scanf("%d", &semester) != 1 || semester < 1) {
            printf("Please enter a valid positive number.\n");
            while (getchar() != '\n');
            continue;
        }
        getchar();
        break;
    }
    
    // Enforce sequential payment:
    // Allow update if semester <= current_max, or new payment if semester == current_max+1.
    // If student tries to pay for a semester greater than current_max+1, that's skipping.
    if (semester > current_max + 1) {
        printf("\nError: You cannot skip a semester payment. Next allowed semester is %d.\n", current_max + 1);
        sleep_sec(1);
        return;
    }
    
    // Check if a payment for this semester already exists.
    ptr = student->payments;
    while (ptr != NULL) {
        if (ptr->semester == semester) {
            target = ptr;
            break;
        }
        ptr = ptr->next;
    }
    
    if (target != NULL) {
        printf("\nWarning: Payment for Semester %d already exists (%.2f taka).\n", semester, target->amount_paid);
        printf("Do you want to overwrite? (y/n): ");
        scanf(" %c", &overwrite);
        getchar();
        if (tolower(overwrite) != 'y') {
            printf("\nPayment cancelled.\n");
            sleep_sec(1);
            return;
        }
    }
    
    printf("\nTuition Fee for Semester %d: %.2f taka\n", semester, admin_settings.tuition_fee);
    while (1) {
        printf("\nEnter Payment Amount: ");
        if (scanf("%f", &payment) != 1 || payment < 0) {
            printf("Payment cannot be negative.\n");
            while(getchar() != '\n');
            continue;
        }
        getchar();
        if (payment > admin_settings.tuition_fee) {
            printf("Warning: Payment exceeds semester fee (%.2f taka).\n", admin_settings.tuition_fee);
            printf("Confirm this payment? (y/n): ");
            scanf(" %c", &overwrite);
            getchar();
            if (tolower(overwrite) != 'y') {
                continue;
            }
        }
        break;
    }
    
    if (target != NULL) {
        target->amount_paid = payment;
    } else {
        PaymentNode* new_payment = createPaymentNode(semester, payment);
        if (student->payments == NULL) {
            student->payments = new_payment;
        } else {
            ptr = student->payments;
            while (ptr->next != NULL) {
                ptr = ptr->next;
            }
            ptr->next = new_payment;
        }
    }
    
    getCurrentDateTime(student->last_updated);
    printf("\nPayment recorded successfully!\n");
    sleep_sec(1);
    
    float total_paid = calculateTotalPaid(student);
    float due = calculateDue(student);
    float remaining = admin_settings.tuition_fee - payment;
    printf("\nRemaining for Semester %d: %.2f taka\n", semester, remaining);
    printf("Total Paid: %.2f taka\n", total_paid * admin_settings.display_multiplier);
    printf("Total Due: %.2f taka\n", due * admin_settings.display_multiplier);
    
    printf("\nPress Enter to continue...");
    getchar();
    saveData();
}

/* Allows modification of tuition and admission fee settings. */
void modifyFeeSettings() {
    int choice;
    float new_fee;
    displayHeader();
    printf("\nMODIFY FEE SETTINGS\n");
    printf("--------------------------------------------------\n");
    printf("Current Tuition Fee: %.2f taka\n", admin_settings.tuition_fee);
    printf("Current Admission Fee: %.2f taka\n", admin_settings.admission_fee);
    
    printf("\n1. Change Tuition Fee\n");
    printf("2. Change Admission Fee\n");
    printf("3. Return to Admin Menu\n");
    
    printf("\nEnter your choice (1-3): ");
    if (scanf("%d", &choice) != 1) {
        while(getchar()!='\n');
        return;
    }
    getchar();
    
    switch (choice) {
        case 1:
            printf("\nEnter new Tuition Fee amount: ");
            if (scanf("%f", &new_fee) != 1 || new_fee < 0) {
                printf("\nInvalid amount. Fee not updated.\n");
            } else {
                admin_settings.tuition_fee = new_fee;
                printf("\nTuition Fee updated to %.2f taka\n", new_fee);
                saveData();
            }
            getchar();
            break;
        case 2:
            printf("\nEnter new Admission Fee amount: ");
            if (scanf("%f", &new_fee) != 1 || new_fee < 0) {
                printf("\nInvalid amount. Fee not updated.\n");
            } else {
                admin_settings.admission_fee = new_fee;
                printf("\nAdmission Fee updated to %.2f taka\n", new_fee);
                saveData();
            }
            getchar();
            break;
        case 3:
            return;
        default:
            printf("\nInvalid choice.\n");
            break;
    }
    sleep_sec(1);
}

/* Modifies the display multiplier used for scaling fee figures. */
void modifyDisplayMultiplier() {
    float new_multiplier;
    displayHeader();
    printf("\nMODIFY DISPLAY MULTIPLIER\n");
    printf("--------------------------------------------------\n");
    printf("Current Display Multiplier: %.2f\n", admin_settings.display_multiplier);
    printf("\nEnter new Display Multiplier: ");
    if (scanf("%f", &new_multiplier) != 1 || new_multiplier <= 0) {
        printf("\nInvalid multiplier. Must be greater than 0.\n");
    } else {
        admin_settings.display_multiplier = new_multiplier;
        printf("\nDisplay Multiplier updated to %.2f\n", new_multiplier);
        saveData();
    }
    getchar();
    sleep_sec(1);
}

/* Calculates the total amount paid by a student
   (combining admission fee and tuition payments). */
float calculateTotalPaid(Student* student) {
    float semester_total = 0.0f;
    PaymentNode* ptr = student->payments;
    while (ptr != NULL) {
        semester_total += ptr->amount_paid;
        ptr = ptr->next;
    }
    return student->admission_fee_paid + semester_total;
}

/* Calculates the total due amount for a student based on the highest semester paid. */
float calculateDue(Student* student) {
    int max_semester = 0;
    PaymentNode* ptr = student->payments;
    while (ptr != NULL) {
        if (ptr->semester > max_semester)
            max_semester = ptr->semester;
        ptr = ptr->next;
    }
    float total_expected = (max_semester * admin_settings.tuition_fee) + admin_settings.admission_fee;
    return total_expected - calculateTotalPaid(student);
}

/* Displays detailed information of a single student, including payment history. */
void displayStudentInfo(Student* student) {
    clearScreen();
    printf("\n==================================================\n");
    printf("STUDENT INFORMATION - ID: %s\n", student->student_id);
    printf("==================================================\n");
    printf("Name: %s\n", student->name);
    printf("Department: %s\n", student->department);
    printf("Admission Fee Paid: %.2f taka of %.2f taka\n", 
           student->admission_fee_paid * admin_settings.display_multiplier, 
           admin_settings.admission_fee * admin_settings.display_multiplier);
    if (student->payments != NULL) {
        printf("\nSemester Payment History:\n");
        PaymentNode* ptr = student->payments;
        while (ptr != NULL) {
            printf("  Semester %d: %.2f taka of %.2f taka\n", 
                   ptr->semester, 
                   ptr->amount_paid * admin_settings.display_multiplier,
                   admin_settings.tuition_fee * admin_settings.display_multiplier);
            ptr = ptr->next;
        }
    } else {
        printf("\nNo semester payments recorded yet.\n");
    }
    printf("\nPayment Summary:\n");
    printf("Total Paid: %.2f taka\n", calculateTotalPaid(student) * admin_settings.display_multiplier);
    printf("Total Due: %.2f taka\n", calculateDue(student) * admin_settings.display_multiplier);
    printf("\nCreated on: %s\n", student->created_date);
    printf("Last updated: %s\n", student->last_updated);
    printf("==================================================\n");
    printf("\nPress Enter to continue...");
    getchar();
}

/* Displays a summary of total fees collected from all students in the system. */
void displayTotalAmountPaid() {
    float total_admission = 0.0f, total_tuition = 0.0f;
    int total_semester_entries = 0;
    displayHeader();
    printf("\nTOTAL AMOUNT PAID SUMMARY\n");
    printf("--------------------------------------------------\n");
    if (studentRoot == NULL) {
        printf("No students found in the system.\n");
        printf("\nPress Enter to continue...");
        getchar();
        return;
    }
    inorderAccumTotals(studentRoot, &total_admission, &total_tuition, &total_semester_entries);
    float grand_total = total_admission + total_tuition;
    int count = bstCountNodes(studentRoot);
    printf("Total Students: %d\n", count);
    printf("Total Admission Fees Paid: %.2f taka\n", total_admission * admin_settings.display_multiplier);
    printf("Total Tuition Fees Paid: %.2f taka\n", total_tuition * admin_settings.display_multiplier);
    printf("Grand Total (Admission + Tuition): %.2f taka\n", grand_total * admin_settings.display_multiplier);
    printf("Total Semester Payment Entries: %d\n", total_semester_entries);
    printf("--------------------------------------------------\n");
    printf("\nPress Enter to continue...");
    getchar();
}

/* Saves student and settings data to binary files.
   The format used:
      int: number of student records,
      then for each student:
        student_id (20 chars), name, department, admission_fee_paid (float),
        created_date, last_updated,
        int: payment_count,
        followed by payment_count entries of (int: semester, float: amount_paid).
*/
void saveData() {
    FILE* student_file = fopen("students.dat", "wb");
    FILE* settings_file = fopen("settings.dat", "wb");
    if (student_file == NULL || settings_file == NULL) {
        printf("\nError: Could not open files for saving data.\n");
        sleep_sec(1);
        return;
    }
    int count = bstCountNodes(studentRoot);
    fwrite(&count, sizeof(int), 1, student_file);
    bstWriteStudentData(studentRoot, student_file);
    fwrite(&admin_settings, sizeof(AdminSettings), 1, settings_file);
    fclose(student_file);
    fclose(settings_file);
    printf("\nData saved successfully.\n");
    sleep_sec(1);
}

/* Loads student and settings data from binary files. */
void loadData() {
    FILE* student_file = fopen("students.dat", "rb");
    FILE* settings_file = fopen("settings.dat", "rb");
    if (student_file != NULL) {
        int count;
        fread(&count, sizeof(int), 1, student_file);
        for (int i = 0; i < count; i++) {
            Student* new_student = (Student*)malloc(sizeof(Student));
            if (!new_student) {
                perror("Memory allocation error");
                exit(EXIT_FAILURE);
            }
            fread(new_student->student_id, sizeof(char), 20, student_file);
            fread(new_student->name, sizeof(char), MAX_NAME_LENGTH, student_file);
            fread(new_student->department, sizeof(char), MAX_DEPT_LENGTH, student_file);
            fread(&new_student->admission_fee_paid, sizeof(float), 1, student_file);
            fread(new_student->created_date, sizeof(char), MAX_DATE_LENGTH, student_file);
            fread(new_student->last_updated, sizeof(char), MAX_DATE_LENGTH, student_file);
            new_student->payments = NULL;
            new_student->left = new_student->right = NULL;
            int payment_count;
            fread(&payment_count, sizeof(int), 1, student_file);
            PaymentNode* last_payment = NULL;
            for (int j = 0; j < payment_count; j++) {
                PaymentNode* new_payment = createPaymentNode(0, 0);
                fread(&new_payment->semester, sizeof(int), 1, student_file);
                fread(&new_payment->amount_paid, sizeof(float), 1, student_file);
                new_payment->next = NULL;
                if (new_student->payments == NULL) {
                    new_student->payments = new_payment;
                    last_payment = new_payment;
                } else {
                    last_payment->next = new_payment;
                    last_payment = new_payment;
                }
            }
            studentRoot = bstInsert(studentRoot, new_student);
        }
        fclose(student_file);
    }
    if (settings_file != NULL) {
        fread(&admin_settings, sizeof(AdminSettings), 1, settings_file);
        fclose(settings_file);
    }
}

/* Main function: loads data, then runs the login loop. */
int main() {
    int admin_login_status;
    loadData();
    while (1) {
        admin_login_status = loginScreen();
        if (admin_login_status == 0)
            break;
        else if (admin_login_status == 1)
            adminMenu();
    }
    return 0;
}
