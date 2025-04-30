#include <gtk/gtk.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "process.h"
#include "interpreter.h"
#include "memory.h"

int arrival1;
int arrival2;
int arrival3;
PCB* pcb1 = NULL;
PCB* pcb2 = NULL;
PCB* pcb3 = NULL;
int os_clock = 0;
int programs = 3;
Queue lvl1;
Queue lvl2;
Queue lvl3;
Queue lvl4;
Queue ready_queue;
Mutex file;
Mutex input;
Mutex output;
selected_schedule schedule;
MemoryManager mem[60];

// Load program file into lines
char** separatefunction(char* fileName, int* line_count) {
    FILE *file = fopen(fileName, "r");
    if (!file) {
        perror("Error opening file");
        return NULL;
    }

    char ch;
    char result[100] = "";
    char** lines = malloc(sizeof(char*) * 100); // Support up to 100 lines
    int i = 0;

    while ((ch = fgetc(file)) != EOF) {
        if (ch == '\n') {
            lines[i] = strdup(result);
            result[0] = '\0';
            i++;
        } else {
            char temp[2] = {ch, '\0'};
            strcat(result, temp);
        }
    }

    // Handle last line if file doesn’t end with newline
    if (strlen(result) > 0) {
        lines[i] = strdup(result);
        i++;
    }

    fclose(file);
    *line_count = i;

    // Free unused slots
    for (int j = i; j < 100; j++) {
        lines[j] = NULL;
    }

    return lines;
}

// Free dynamically allocated lines
void free_lines(char** lines, int line_count) {
    for (int i = 0; i < line_count; i++) {
        if (lines[i]) free(lines[i]);
    }
    free(lines);
}

// Structure to hold GUI widgets
typedef struct {
    GtkWidget *window;
    GtkWidget *overview_label;
    GtkListStore *process_store;
    GtkWidget *ready_queue_label;
    GtkWidget *blocking_queue_label;
    GtkWidget *running_process_label;
    GtkWidget *algo_combo;
    GtkWidget *quantum_entry;
    GtkWidget *start_button;
    GtkWidget *stop_button;
    GtkWidget *reset_button;
    GtkWidget *step_button;
    GtkWidget *arrival_entry;
    GtkWidget *mutex_status_label;
    GtkWidget *blocked_resource_label;
    GtkWidget *memory_view_label;
    GtkWidget *log_text_view;
    GtkWidget *add_program1_button; // Added to store button for enabling/disabling
    int clock_cycle;
    gboolean running;
} AppWidgets;

// Function prototypes
void on_add_program1_clicked(GtkButton *button, AppWidgets *app);
void on_add_program2_clicked(GtkButton *button, AppWidgets *app);
void on_add_program3_clicked(GtkButton *button, AppWidgets *app);
void on_start_clicked(GtkButton *button, AppWidgets *app);
void on_stop_clicked(GtkButton *button, AppWidgets *app);
void on_reset_clicked(GtkButton *button, AppWidgets *app);
void on_step_clicked(GtkButton *button, AppWidgets *app);
gboolean update_simulation(gpointer data);
void update_gui(AppWidgets *app);

// Initialize the GUI
AppWidgets* init_gui() {
    AppWidgets *app = g_new(AppWidgets, 1);
    app->clock_cycle = 0;
    app->running = FALSE;

    // Create main window
    app->window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(app->window), "Scheduler Simulation");
    gtk_window_set_default_size(GTK_WINDOW(app->window), 1200, 800);
    g_signal_connect(app->window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    // Main container
    GtkWidget *main_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_container_add(GTK_CONTAINER(app->window), main_box);

    // Left panel: Dashboard and Process Creation
    GtkWidget *left_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_box_pack_start(GTK_BOX(main_box), left_box, TRUE, TRUE, 5);

    // Dashboard
    GtkWidget *dashboard_frame = gtk_frame_new("Main Dashboard");
    GtkWidget *dashboard_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_container_add(GTK_CONTAINER(dashboard_frame), dashboard_box);

    // Overview Section
    app->overview_label = gtk_label_new("Processes: 0 | Clock: 0 | Algorithm: None");
    gtk_box_pack_start(GTK_BOX(dashboard_box), app->overview_label, FALSE, FALSE, 5);

    // Process List
    app->process_store = gtk_list_store_new(5, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_INT, G_TYPE_STRING, G_TYPE_INT);
    GtkWidget *process_view = gtk_tree_view_new_with_model(GTK_TREE_MODEL(app->process_store));
    const char *columns[] = {"PID", "State", "Priority", "Memory", "PC"};
    for (int i = 0; i < 5; i++) {
        GtkCellRenderer *renderer = gtk_cell_renderer_text_new();
        GtkTreeViewColumn *column = gtk_tree_view_column_new_with_attributes(columns[i], renderer, "text", i, NULL);
        gtk_tree_view_append_column(GTK_TREE_VIEW(process_view), column);
    }
    GtkWidget *process_scroll = gtk_scrolled_window_new(NULL, NULL);
    gtk_container_add(GTK_CONTAINER(process_scroll), process_view);
    gtk_box_pack_start(GTK_BOX(dashboard_box), process_scroll, TRUE, TRUE, 5);

    // Queue Section
    GtkWidget *queue_frame = gtk_frame_new("Queues");
    GtkWidget *queue_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_container_add(GTK_CONTAINER(queue_frame), queue_box);
    app->ready_queue_label = gtk_label_new("Ready Queue: []");
    app->blocking_queue_label = gtk_label_new("Blocking Queue: []");
    app->running_process_label = gtk_label_new("Running: None");
    gtk_box_pack_start(GTK_BOX(queue_box), app->ready_queue_label, FALSE, FALSE, 5);
    gtk_box_pack_start(GTK_BOX(queue_box), app->blocking_queue_label, FALSE, FALSE, 5);
    gtk_box_pack_start(GTK_BOX(queue_box), app->running_process_label, FALSE, FALSE, 5);
    gtk_box_pack_start(GTK_BOX(dashboard_box), queue_frame, FALSE, FALSE, 5);
    gtk_box_pack_start(GTK_BOX(left_box), dashboard_frame, TRUE, TRUE, 5);

    // Process Creation
    GtkWidget *creation_frame = gtk_frame_new("Process Creation");
    GtkWidget *creation_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_container_add(GTK_CONTAINER(creation_frame), creation_box);

    // Three buttons for adding programs
    app->add_program1_button = gtk_button_new_with_label("Add Program 1");
    GtkWidget *add_program2_button = gtk_button_new_with_label("Add Program 2");
    GtkWidget *add_program3_button = gtk_button_new_with_label("Add Program 3");
    g_signal_connect(app->add_program1_button, "clicked", G_CALLBACK(on_add_program1_clicked), app);
    g_signal_connect(add_program2_button, "clicked", G_CALLBACK(on_add_program2_clicked), app);
    g_signal_connect(add_program3_button, "clicked", G_CALLBACK(on_add_program3_clicked), app);
    gtk_box_pack_start(GTK_BOX(creation_box), app->add_program1_button, FALSE, FALSE, 5);
    gtk_box_pack_start(GTK_BOX(creation_box), add_program2_button, FALSE, FALSE, 5);
    gtk_box_pack_start(GTK_BOX(creation_box), add_program3_button, FALSE, FALSE, 5);

    GtkWidget *arrival_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    GtkWidget *arrival_label = gtk_label_new("Arrival Time:");
    app->arrival_entry = gtk_entry_new();
    gtk_box_pack_start(GTK_BOX(arrival_box), arrival_label, FALSE, FALSE, 5);
    gtk_box_pack_start(GTK_BOX(arrival_box), app->arrival_entry, TRUE, TRUE, 5);
    gtk_box_pack_start(GTK_BOX(creation_box), arrival_box, FALSE, FALSE, 5);
    gtk_box_pack_start(GTK_BOX(left_box), creation_frame, FALSE, FALSE, 5);

    // Right panel: Controls, Resources, Memory, Log
    GtkWidget *right_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_box_pack_start(GTK_BOX(main_box), right_box, TRUE, TRUE, 5);

    // Scheduler Control Panel
    GtkWidget *control_frame = gtk_frame_new("Scheduler Control");
    GtkWidget *control_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_container_add(GTK_CONTAINER(control_frame), control_box);
    GtkWidget *algo_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    GtkWidget *algo_label = gtk_label_new("Algorithm:");
    app->algo_combo = gtk_combo_box_text_new();
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(app->algo_combo), "FCFS");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(app->algo_combo), "Round Robin");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(app->algo_combo), "MLFQ");
    gtk_combo_box_set_active(GTK_COMBO_BOX(app->algo_combo), 0);
    gtk_box_pack_start(GTK_BOX(algo_box), algo_label, FALSE, FALSE, 5);
    gtk_box_pack_start(GTK_BOX(algo_box), app->algo_combo, TRUE, TRUE, 5);
    gtk_box_pack_start(GTK_BOX(control_box), algo_box, FALSE, FALSE, 5);
    GtkWidget *quantum_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    GtkWidget *quantum_label = gtk_label_new("Quantum:");
    app->quantum_entry = gtk_entry_new();
    gtk_entry_set_text(GTK_ENTRY(app->quantum_entry), "2");
    gtk_box_pack_start(GTK_BOX(quantum_box), quantum_label, FALSE, FALSE, 5);
    gtk_box_pack_start(GTK_BOX(quantum_box), app->quantum_entry, TRUE, TRUE, 5);
    gtk_box_pack_start(GTK_BOX(control_box), quantum_box, FALSE, FALSE, 5);
    GtkWidget *button_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    app->start_button = gtk_button_new_with_label("Start");
    app->stop_button = gtk_button_new_with_label("Stop");
    app->reset_button = gtk_button_new_with_label("Reset");
    app->step_button = gtk_button_new_with_label("Step");
    g_signal_connect(app->start_button, "clicked", G_CALLBACK(on_start_clicked), app);
    g_signal_connect(app->stop_button, "clicked", G_CALLBACK(on_stop_clicked), app);
    g_signal_connect(app->reset_button, "clicked", G_CALLBACK(on_reset_clicked), app);
    g_signal_connect(app->step_button, "clicked", G_CALLBACK(on_step_clicked), app);
    gtk_box_pack_start(GTK_BOX(button_box), app->start_button, TRUE, TRUE, 5);
    gtk_box_pack_start(GTK_BOX(button_box), app->stop_button, TRUE, TRUE, 5);
    gtk_box_pack_start(GTK_BOX(button_box), app->reset_button, TRUE, TRUE, 5);
    gtk_box_pack_start(GTK_BOX(button_box), app->step_button, TRUE, TRUE, 5);
    gtk_box_pack_start(GTK_BOX(control_box), button_box, FALSE, FALSE, 5);
    gtk_box_pack_start(GTK_BOX(right_box), control_frame, FALSE, FALSE, 5);

    // Resource Management Panel
    GtkWidget *resource_frame = gtk_frame_new("Resource Management");
    GtkWidget *resource_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_container_add(GTK_CONTAINER(resource_frame), resource_box);
    app->mutex_status_label = gtk_label_new("Mutex: userInput: Free, userOutput: Free, file: Free");
    app->blocked_resource_label = gtk_label_new("Blocked for Resources: []");
    gtk_box_pack_start(GTK_BOX(resource_box), app->mutex_status_label, FALSE, FALSE, 5);
    gtk_box_pack_start(GTK_BOX(resource_box), app->blocked_resource_label, FALSE, FALSE, 5);
    gtk_box_pack_start(GTK_BOX(right_box), resource_frame, FALSE, FALSE, 5);

    // Memory Viewer
    GtkWidget *memory_frame = gtk_frame_new("Memory Viewer");
    app->memory_view_label = gtk_label_new("Memory: [Empty]");
    gtk_container_add(GTK_CONTAINER(memory_frame), app->memory_view_label);
    gtk_box_pack_start(GTK_BOX(right_box), memory_frame, TRUE, TRUE, 5);

    // Log & Console Panel
    GtkWidget *log_frame = gtk_frame_new("Log & Console");
    GtkWidget *log_scroll = gtk_scrolled_window_new(NULL, NULL);
    app->log_text_view = gtk_text_view_new();
    gtk_text_view_set_editable(GTK_TEXT_VIEW(app->log_text_view), FALSE);
    gtk_container_add(GTK_CONTAINER(log_scroll), app->log_text_view);
    gtk_container_add(GTK_CONTAINER(log_frame), log_scroll);
    gtk_box_pack_start(GTK_BOX(right_box), log_frame, TRUE, TRUE, 5);

    return app;
}

// Callback for Add Program 1
void on_add_program1_clicked(GtkButton *button, AppWidgets *app) {
    const char *arrival_time_str = gtk_entry_get_text(GTK_ENTRY(app->arrival_entry));
    int arrival_time = atoi(arrival_time_str);
    if (pcb1 == NULL) {
        pcb1 = create_pcb(1, arrival_time);
        arrival1 = arrival_time;
        int line_count;
        char** lines = separatefunction("Program_1.txt", &line_count);
        if (lines) {
            allocate_process(mem, pcb1, lines, line_count);
            free_lines(lines, line_count);
        } else {
            gtk_text_buffer_insert_at_cursor(
                gtk_text_view_get_buffer(GTK_TEXT_VIEW(app->log_text_view)),
                "Error: Failed to load Program_1.txt\n", -1);
            free(pcb1); // Assuming free_pcb is not defined; using standard free
            pcb1 = NULL;
            return;
        }
        char pid_str[16];
        snprintf(pid_str, sizeof(pid_str), "P%d", pcb1->pid);
        char range_str[50];
        snprintf(range_str, sizeof(range_str), "%d-%d", pcb1->mem_start, pcb1->mem_end);
        GtkTreeIter iter;
        gtk_list_store_append(app->process_store, &iter);
        gtk_list_store_set(app->process_store, &iter,
                           0, pid_str,
                           1, state_to_string(pcb1),
                           2, pcb1->priority,
                           3, range_str,
                           4, pcb1->program_counter,
                           -1);
        char log[256];
        snprintf(log, sizeof(log), "Added Program1 as process P%d with arrival time %d\n", pcb1->pid, arrival_time);
        gtk_text_buffer_insert_at_cursor(
            gtk_text_view_get_buffer(GTK_TEXT_VIEW(app->log_text_view)),
            log, -1);
        // Disable the Add Program 1 button
        gtk_widget_set_sensitive(app->add_program1_button, FALSE);
    } else {
        gtk_text_buffer_insert_at_cursor(
            gtk_text_view_get_buffer(GTK_TEXT_VIEW(app->log_text_view)),
            "Program1 already added\n", -1);
    }
}

// Callback for Add Program 2
void on_add_program2_clicked(GtkButton *button, AppWidgets *app) {
    const char *arrival_time_str = gtk_entry_get_text(GTK_ENTRY(app->arrival_entry));
    int arrival_time = atoi(arrival_time_str);
    if (pcb2 == NULL) {
        pcb2 = create_pcb(2, arrival_time);
        arrival2 = arrival_time;
        int line_count;
        char** lines = separatefunction("Program_2.txt", &line_count);
        if (lines) {
            allocate_process(mem, pcb2, lines, line_count);
            free_lines(lines, line_count);
        } else {
            gtk_text_buffer_insert_at_cursor(
                gtk_text_view_get_buffer(GTK_TEXT_VIEW(app->log_text_view)),
                "Error: Failed to load Program_2.txt\n", -1);
            free(pcb2); // Assuming free_pcb is not defined; using standard free
            pcb2 = NULL;
            return;
        }
        char pid_str[16];
        snprintf(pid_str, sizeof(pid_str), "P%d", pcb2->pid);
        char range_str[50];
        snprintf(range_str, sizeof(range_str), "%d-%d", pcb2->mem_start, pcb2->mem_end);
        GtkTreeIter iter;
        gtk_list_store_append(app->process_store, &iter);
        gtk_list_store_set(app->process_store, &iter,
                           0, pid_str,
                           1, state_to_string(pcb2),
                           2, pcb2->priority,
                           3, range_str,
                           4, pcb2->program_counter,
                           -1);
        char log[256];
        snprintf(log, sizeof(log), "Added Program2 as process P%d with arrival time %d\n", pcb2->pid, arrival_time);
        gtk_text_buffer_insert_at_cursor(
            gtk_text_view_get_buffer(GTK_TEXT_VIEW(app->log_text_view)),
            log, -1);
    } else {
        gtk_text_buffer_insert_at_cursor(
            gtk_text_view_get_buffer(GTK_TEXT_VIEW(app->log_text_view)),
            "Program2 already added\n", -1);
    }
}

// Callback for Add Program 3
void on_add_program3_clicked(GtkButton *button, AppWidgets *app) {
    gtk_text_buffer_insert_at_cursor(
        gtk_text_view_get_buffer(GTK_TEXT_VIEW(app->log_text_view)),
        "Program3 not implemented\n", -1);
}

// Callback for Start
void on_start_clicked(GtkButton *button, AppWidgets *app) {
    if (!app->running) {
        app->running = TRUE;
        g_timeout_add(1000, update_simulation, app);
        gtk_text_buffer_insert_at_cursor(
            gtk_text_view_get_buffer(GTK_TEXT_VIEW(app->log_text_view)),
            "Simulation started\n", -1);
    }
}

// Callback for Stop
void on_stop_clicked(GtkButton *button, AppWidgets *app) {
    app->running = FALSE;
    gtk_text_buffer_insert_at_cursor(
        gtk_text_view_get_buffer(GTK_TEXT_VIEW(app->log_text_view)),
        "Simulation stopped\n", -1);
}

// Callback for Reset
void on_reset_clicked(GtkButton *button, AppWidgets *app) {
    app->running = FALSE;
    app->clock_cycle = 0;
    if (pcb1) {
        free(pcb1); // Assuming free_pcb is not defined; using standard free
        pcb1 = NULL;
    }
    if (pcb2) {
        free(pcb2);
        pcb2 = NULL;
    }
    if (pcb3) {
        free(pcb3);
        pcb3 = NULL;
    }
    arrival1 = 0;
    arrival2 = 0;
    arrival3 = 0;
    gtk_list_store_clear(app->process_store);
    gtk_label_set_text(GTK_LABEL(app->overview_label), "Processes: 0 | Clock: 0 | Algorithm: None");
    gtk_label_set_text(GTK_LABEL(app->ready_queue_label), "Ready Queue: []");
    gtk_label_set_text(GTK_LABEL(app->blocking_queue_label), "Blocking Queue: []");
    gtk_label_set_text(GTK_LABEL(app->running_process_label), "Running: None");
    gtk_label_set_text(GTK_LABEL(app->mutex_status_label), "Mutex: userInput: Free, userOutput: Free, file: Free");
    gtk_label_set_text(GTK_LABEL(app->blocked_resource_label), "Blocked for Resources: []");
    gtk_label_set_text(GTK_LABEL(app->memory_view_label), "Memory: [Empty]");
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(app->log_text_view));
    gtk_text_buffer_set_text(buffer, "Simulation reset\n", -1);
    // Re-enable the Add Program 1 button
    gtk_widget_set_sensitive(app->add_program1_button, TRUE);
}

// Callback for Step
void on_step_clicked(GtkButton *button, AppWidgets *app) {
    update_simulation(app);
}

// Update simulation state
gboolean update_simulation(gpointer data) {
    AppWidgets *app = (AppWidgets*)data;
    if (!app->running) return FALSE;

    app->clock_cycle++;
    update_gui(app);

    char log[256];
    snprintf(log, sizeof(log), "Clock cycle %d executed\n", app->clock_cycle);
    gtk_text_buffer_insert_at_cursor(
        gtk_text_view_get_buffer(GTK_TEXT_VIEW(app->log_text_view)),
        log, -1);

    return TRUE;
}

// Update GUI elements
void update_gui(AppWidgets *app) {
    char overview[256];
    snprintf(overview, sizeof(overview), "Processes: %d | Clock: %d | Algorithm: %s",
             (pcb1 ? 1 : 0) + (pcb2 ? 1 : 0) + (pcb3 ? 1 : 0),
             app->clock_cycle,
             gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(app->algo_combo)));
    gtk_label_set_text(GTK_LABEL(app->overview_label), overview);
}

// Main function
int main(int argc, char *argv[]) {
    init_memory(mem);
    gtk_init(&argc, &argv);
    AppWidgets *app = init_gui();
    gtk_widget_show_all(app->window);
    gtk_main();
    g_free(app);
    return 0;
}