#include "gui.h"
#include <gtk/gtk.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "process.h"
#include "interpreter.h"
#include "memory.h"
#include "mutex.h"
#include "queue.h"
#include "scheduler.h"

int os_clock = 0;
int programs = 0;
int arr_index = 0;
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
AppWidgets *app; // Global AppWidgets
int quanta;
int current_quanta;
PCB* pcbs_list[50] = {NULL};
char* filepaths[50] = {NULL};

// Load program file into lines
char** separatefunction(char* fileName, int* line_count) {
    FILE *file = fopen(fileName, "r");
    if (!file) {
        perror("Error opening file");
        return NULL;
    }

    char ch;
    char result[100] = "";
    char** lines = malloc(sizeof(char*) * 100);
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

    if (strlen(result) > 0) {
        lines[i] = strdup(result);
        i++;
    }

    fclose(file);
    *line_count = i;

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

// Get user input via GUI dialog
char* get_gui_input(int pid) {
    if (!app) {
        fprintf(stderr, "Error: GUI not initialized\n");
        return strdup("");
    }

    GtkWidget *dialog = gtk_dialog_new_with_buttons(
        "Input Required",
        GTK_WINDOW(app->window),
        GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
        "_Submit", GTK_RESPONSE_ACCEPT,
        "_Cancel", GTK_RESPONSE_REJECT,
        NULL
    );

    char title[64];
    snprintf(title, sizeof(title), "Input for Process P%d", pid);
    gtk_window_set_title(GTK_WINDOW(dialog), title);

    GtkWidget *content_area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    GtkWidget *label = gtk_label_new("Enter value:");
    GtkWidget *entry = gtk_entry_new();
    gtk_box_pack_start(GTK_BOX(content_area), label, FALSE, FALSE, 5);
    gtk_box_pack_start(GTK_BOX(content_area), entry, FALSE, FALSE, 5);
    gtk_widget_show_all(content_area);

    char *input = NULL;
    gint response = gtk_dialog_run(GTK_DIALOG(dialog));
    if (response == GTK_RESPONSE_ACCEPT) {
        const char *text = gtk_entry_get_text(GTK_ENTRY(entry));
        input = strdup(text);
        char log[256];
        snprintf(log, sizeof(log), "P%d received input: %s\n", pid, input);
        gtk_text_buffer_insert_at_cursor(
            gtk_text_view_get_buffer(GTK_TEXT_VIEW(app->log_text_view)),
            log, -1);
    } else {
        input = strdup("");
        gtk_text_buffer_insert_at_cursor(
            gtk_text_view_get_buffer(GTK_TEXT_VIEW(app->log_text_view)),
            "Input canceled\n", -1);
    }

    gtk_widget_destroy(dialog);
    return input;
}

// Initialize the GUI
AppWidgets* init_gui() {
    AppWidgets *app = g_new(AppWidgets, 1);
    app->clock_cycle = 0;
    app->running = FALSE;

    app->window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(app->window), "Scheduler Simulation");
    gtk_window_set_default_size(GTK_WINDOW(app->window), 1200, 800);
    g_signal_connect(app->window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    GtkWidget *main_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_container_add(GTK_CONTAINER(app->window), main_box);

    GtkWidget *left_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_box_pack_start(GTK_BOX(main_box), left_box, TRUE, TRUE, 5);

    GtkWidget *dashboard_frame = gtk_frame_new("Main Dashboard");
    GtkWidget *dashboard_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_container_add(GTK_CONTAINER(dashboard_frame), dashboard_box);

    app->overview_label = gtk_label_new("Processes: 0 | Clock: 0 | Algorithm: None");
    gtk_box_pack_start(GTK_BOX(dashboard_box), app->overview_label, FALSE, FALSE, 5);

    app->process_store = gtk_list_store_new(6, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_INT, G_TYPE_STRING, G_TYPE_INT, G_TYPE_INT);
    GtkWidget *process_view = gtk_tree_view_new_with_model(GTK_TREE_MODEL(app->process_store));
    const char *columns[] = {"PID", "State", "Priority", "Memory", "PC", "Arrival Time"};
    for (int i = 0; i < 6; i++) {
        GtkCellRenderer *renderer = gtk_cell_renderer_text_new();
        GtkTreeViewColumn *column = gtk_tree_view_column_new_with_attributes(columns[i], renderer, "text", i, NULL);
        gtk_tree_view_append_column(GTK_TREE_VIEW(process_view), column);
    }
    GtkWidget *process_scroll = gtk_scrolled_window_new(NULL, NULL);
    gtk_container_add(GTK_CONTAINER(process_scroll), process_view);
    gtk_box_pack_start(GTK_BOX(dashboard_box), process_scroll, TRUE, TRUE, 5);

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

    GtkWidget *creation_frame = gtk_frame_new("Process Creation");
    GtkWidget *creation_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_container_add(GTK_CONTAINER(creation_frame), creation_box);

    app->add_process_button = gtk_button_new_with_label("Add Process");
    g_signal_connect(app->add_process_button, "clicked", G_CALLBACK(on_add_process_clicked), app);
    gtk_box_pack_start(GTK_BOX(creation_box), app->add_process_button, FALSE, FALSE, 5);

    GtkWidget *arrival_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    GtkWidget *arrival_label = gtk_label_new("Arrival Time:");
    app->arrival_entry = gtk_entry_new();
    gtk_box_pack_start(GTK_BOX(arrival_box), arrival_label, FALSE, FALSE, 5);
    gtk_box_pack_start(GTK_BOX(arrival_box), app->arrival_entry, TRUE, TRUE, 5);
    gtk_box_pack_start(GTK_BOX(creation_box), arrival_box, FALSE, FALSE, 5);
    gtk_box_pack_start(GTK_BOX(left_box), creation_frame, FALSE, FALSE, 5);

    GtkWidget *right_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_box_pack_start(GTK_BOX(main_box), right_box, TRUE, TRUE, 5);

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
    app->start_simulation_button = gtk_button_new_with_label("Start Simulation");
    app->start_button = gtk_button_new_with_label("▶");
    app->stop_button = gtk_button_new_with_label("Stop");
    app->reset_button = gtk_button_new_with_label("Reset");
    app->step_button = gtk_button_new_with_label("Step");
    g_signal_connect(app->start_simulation_button, "clicked", G_CALLBACK(on_start_simulation_clicked), app);
    g_signal_connect(app->start_button, "clicked", G_CALLBACK(on_start_clicked), app);
    g_signal_connect(app->stop_button, "clicked", G_CALLBACK(on_stop_clicked), app);
    g_signal_connect(app->reset_button, "clicked", G_CALLBACK(on_reset_clicked), app);
    g_signal_connect(app->step_button, "clicked", G_CALLBACK(on_step_clicked), app);
    gtk_box_pack_start(GTK_BOX(button_box), app->start_simulation_button, TRUE, TRUE, 5);
    gtk_box_pack_start(GTK_BOX(button_box), app->start_button, TRUE, TRUE, 5);
    gtk_box_pack_start(GTK_BOX(button_box), app->stop_button, TRUE, TRUE, 5);
    gtk_box_pack_start(GTK_BOX(button_box), app->reset_button, TRUE, TRUE, 5);
    gtk_box_pack_start(GTK_BOX(button_box), app->step_button, TRUE, TRUE, 5);
    gtk_box_pack_start(GTK_BOX(control_box), button_box, FALSE, FALSE, 5);
    gtk_box_pack_start(GTK_BOX(right_box), control_frame, FALSE, FALSE, 5);

    GtkWidget *resource_frame = gtk_frame_new("Resource Management");
    GtkWidget *resource_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_container_add(GTK_CONTAINER(resource_frame), resource_box);
    app->mutex_status_label = gtk_label_new("Mutex: userInput: Free, userOutput: Free, file: Free");
    app->blocked_resource_label = gtk_label_new("Blocked for Resources: []");
    gtk_box_pack_start(GTK_BOX(resource_box), app->mutex_status_label, FALSE, FALSE, 5);
    gtk_box_pack_start(GTK_BOX(resource_box), app->blocked_resource_label, FALSE, FALSE, 5);
    gtk_box_pack_start(GTK_BOX(right_box), resource_frame, FALSE, FALSE, 5);

    GtkWidget *memory_frame = gtk_frame_new("Memory Viewer");
    GtkWidget *memory_scroll = gtk_scrolled_window_new(NULL, NULL);
    app->memory_view_label = gtk_label_new("Memory: [Empty]");
    gtk_label_set_justify(GTK_LABEL(app->memory_view_label), GTK_JUSTIFY_LEFT);
    gtk_label_set_selectable(GTK_LABEL(app->memory_view_label), TRUE);
    gtk_container_add(GTK_CONTAINER(memory_scroll), app->memory_view_label);
    gtk_container_add(GTK_CONTAINER(memory_frame), memory_scroll);
    gtk_box_pack_start(GTK_BOX(right_box), memory_frame, TRUE, TRUE, 5);

    GtkWidget *log_frame = gtk_frame_new("Log & Console");
    GtkWidget *log_scroll = gtk_scrolled_window_new(NULL, NULL);
    app->log_text_view = gtk_text_view_new();
    gtk_text_view_set_editable(GTK_TEXT_VIEW(app->log_text_view), FALSE);
    gtk_container_add(GTK_CONTAINER(log_scroll), app->log_text_view);
    gtk_container_add(GTK_CONTAINER(log_frame), log_scroll);
    gtk_box_pack_start(GTK_BOX(right_box), log_frame, TRUE, TRUE, 5);

    return app;
}

// Update Memory Viewer label
void update_memory_view(AppWidgets *app) {
    char buffer[4096] = "Memory:\n";
    int pos = strlen(buffer);
    int any_used = 0;

    for (int i = 0; i < MEM_SIZE; i++) {
        if (mem[0].used[i]) {
            any_used = 1;
            pos += snprintf(buffer + pos, sizeof(buffer) - pos,
                           "Slot %d: [USED] Key: %s, Value: %s\n",
                           i,
                           mem[0].words[i].key ? mem[0].words[i].key : "NULL",
                           mem[0].words[i].value ? mem[0].words[i].value : "NULL");
        } else {
            pos += snprintf(buffer + pos, sizeof(buffer) - pos,
                           "Slot %d: [FREE] Key: NULL, Value: NULL\n", i);
        }
    }

    if (!any_used) {
        snprintf(buffer, sizeof(buffer), "Memory: [Empty]");
    }

    gtk_label_set_text(GTK_LABEL(app->memory_view_label), buffer);
}

// Update Ready Queue label
void update_ready_queue_label(AppWidgets *app) {
    char buffer[256] = "Ready Queue: [";
    int pos = strlen(buffer);

    if (is_empty(&ready_queue)) {
        pos += snprintf(buffer + pos, sizeof(buffer) - pos, "]");
    } else {
        int first = 1;
        for (int i = ready_queue.front; i < ready_queue.rear && i < MAX_SIZE; i++) {
            if (ready_queue.processes[i] == NULL || ready_queue.processes[i]->state != READY) continue;
            if (!first) {
                pos += snprintf(buffer + pos, sizeof(buffer) - pos, ", ");
            }
            pos += snprintf(buffer + pos, sizeof(buffer) - pos, "P%d", ready_queue.processes[i]->pid);
            first = 0;
        }
        pos += snprintf(buffer + pos, sizeof(buffer) - pos, "]");
    }

    gtk_label_set_text(GTK_LABEL(app->ready_queue_label), buffer);
    gtk_widget_queue_draw(app->ready_queue_label);
}

// Update Running and Blocked labels
void update_running_and_blocked_labels(AppWidgets *app) {
    char running_buffer[64] = "Running: None";
    for (int i = 0; i < programs; i++) {
        if (pcbs_list[i] != NULL && pcbs_list[i]->state == RUNNING) {
            snprintf(running_buffer, sizeof(running_buffer), "Running: P%d", pcbs_list[i]->pid);
            break;
        }
    }
    gtk_label_set_text(GTK_LABEL(app->running_process_label), running_buffer);
    gtk_widget_queue_draw(app->running_process_label);

    char blocked_buffer[256] = "Blocking Queue: [";
    int pos = strlen(blocked_buffer);
    int first = 1;

    for (int i = 0; i < programs; i++) {
        if (pcbs_list[i] != NULL && pcbs_list[i]->state == BLOCKED) {
            if (!first) {
                pos += snprintf(blocked_buffer + pos, sizeof(blocked_buffer) - pos, ", ");
            }
            pos += snprintf(blocked_buffer + pos, sizeof(blocked_buffer) - pos, "P%d", pcbs_list[i]->pid);
            first = 0;
        }
    }
    pos += snprintf(blocked_buffer + pos, sizeof(blocked_buffer) - pos, "]");
    gtk_label_set_text(GTK_LABEL(app->blocking_queue_label), blocked_buffer);
    gtk_widget_queue_draw(app->blocking_queue_label);
}

// Callback for Add Process
void on_add_process_clicked(GtkButton *button, AppWidgets *app) {
    if (programs >= 50) {
        gtk_text_buffer_insert_at_cursor(
            gtk_text_view_get_buffer(GTK_TEXT_VIEW(app->log_text_view)),
            "Error: Maximum processes reached (50)\n", -1);
        gtk_widget_set_sensitive(app->add_process_button, FALSE);
        return;
    }

    // Get arrival time
    const char *arrival_time_str = gtk_entry_get_text(GTK_ENTRY(app->arrival_entry));
    int arrival_time = atoi(arrival_time_str);
    if (arrival_time < 0 || strlen(arrival_time_str) == 0) {
        gtk_text_buffer_insert_at_cursor(
            gtk_text_view_get_buffer(GTK_TEXT_VIEW(app->log_text_view)),
            "Error: Invalid arrival time\n", -1);
        return;
    }

    // Create file chooser dialog
    GtkWidget *dialog = gtk_file_chooser_dialog_new(
        "Select Program File",
        GTK_WINDOW(app->window),
        GTK_FILE_CHOOSER_ACTION_OPEN,
        "_Open", GTK_RESPONSE_ACCEPT,
        "_Cancel", GTK_RESPONSE_REJECT,
        NULL
    );

    // Add filter for .txt files
    GtkFileFilter *filter = gtk_file_filter_new();
    gtk_file_filter_set_name(filter, "Text Files");
    gtk_file_filter_add_pattern(filter, "*.txt");
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter);

    char *filename = NULL;
    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
        filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
        if (!filename) {
            gtk_text_buffer_insert_at_cursor(
                gtk_text_view_get_buffer(GTK_TEXT_VIEW(app->log_text_view)),
                "Error: No valid file selected\n", -1);
            gtk_widget_destroy(dialog);
            return;
        }
    } else {
        gtk_text_buffer_insert_at_cursor(
            gtk_text_view_get_buffer(GTK_TEXT_VIEW(app->log_text_view)),
            "Process addition canceled\n", -1);
        gtk_widget_destroy(dialog);
        return;
    }
    gtk_widget_destroy(dialog);

    // Store filepath
    filepaths[programs] = strdup(filename);

    // Create PCB
    pcbs_list[programs] = create_pcb(programs + 1, arrival_time);

    free(filename);
    programs++;

    // Disable button if max processes reached
    if (programs >= 50) {
        gtk_widget_set_sensitive(app->add_process_button, FALSE);
    }
}

// Callback for Start (▶ button)
void on_start_clicked(GtkButton *button, AppWidgets *app) {
    if (!app->running) {
        app->running = TRUE;
        g_timeout_add(1000, update_simulation, app);
        gtk_text_buffer_insert_at_cursor(
            gtk_text_view_get_buffer(GTK_TEXT_VIEW(app->log_text_view)),
            "Simulation started via Start button\n", -1);
    }
}

// Callback for Start Simulation
void on_start_simulation_clicked(GtkButton *button, AppWidgets *app) {
    printf("clicked sim");
    if (programs == 0) {
        gtk_text_buffer_insert_at_cursor(
            gtk_text_view_get_buffer(GTK_TEXT_VIEW(app->log_text_view)),
            "Error: At least one process must be added\n", -1);
        return;
    }

    char *algo = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(app->algo_combo));
    if (strcmp(algo, "FCFS") == 0) {
        schedule = FCFS;
    } else if (strcmp(algo, "Round Robin") == 0) {
        schedule = RR;
        const char *quantum_str = gtk_entry_get_text(GTK_ENTRY(app->quantum_entry));
        quanta = atoi(quantum_str);
        init_quanta();
    } else if (strcmp(algo, "MLFQ") == 0) {
        printf("here");
        schedule = MLFQ;
    } else {
        gtk_text_buffer_insert_at_cursor(
            gtk_text_view_get_buffer(GTK_TEXT_VIEW(app->log_text_view)),
            "Error: Invalid scheduling algorithm selected\n", -1);
        g_free(algo);
        return;
    }

    gtk_widget_set_sensitive(app->start_simulation_button, FALSE);
    char log[256];
    snprintf(log, sizeof(log), "Scheduler setup complete with %s algorithm\n", algo);
    gtk_text_buffer_insert_at_cursor(
        gtk_text_view_get_buffer(GTK_TEXT_VIEW(app->log_text_view)),
        log, -1);
    g_free(algo);
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
    os_clock = 0;

    // Free all processes and filepaths
    for (int i = 0; i < programs; i++) {
        if (pcbs_list[i]) {
            free_process(mem, pcbs_list[i]);
            pcbs_list[i] = NULL;
        }
        if (filepaths[i]) {
            free(filepaths[i]);
            filepaths[i] = NULL;
        }
    }
    programs = 0;

    init_queue(&ready_queue);
    init_queue(&lvl1);
    init_queue(&lvl2);
    init_queue(&lvl3);
    init_queue(&lvl4);
    gtk_list_store_clear(app->process_store);
    gtk_label_set_text(GTK_LABEL(app->overview_label), "Processes: 0 | Clock: 0 | Algorithm: None");
    gtk_label_set_text(GTK_LABEL(app->ready_queue_label), "Ready Queue: []");
    gtk_label_set_text(GTK_LABEL(app->blocking_queue_label), "Blocking Queue: []");
    gtk_label_set_text(GTK_LABEL(app->running_process_label), "Running: None");
    gtk_label_set_text(GTK_LABEL(app->mutex_status_label), "Mutex: userInput: Free, userOutput: Free, file: Free");
    gtk_label_set_text(GTK_LABEL(app->blocked_resource_label), "Blocked for Resources: []");
    update_memory_view(app);
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(app->log_text_view));
    gtk_text_buffer_set_text(buffer, "Simulation reset\n", -1);
    gtk_widget_set_sensitive(app->add_process_button, TRUE);
    gtk_widget_set_sensitive(app->start_simulation_button, TRUE);
}

// Callback for Step
void on_step_clicked(GtkButton *button, AppWidgets *app) {
    app->running = TRUE;
    update_simulation(app);
    app->running = FALSE;
}

// Update simulation state
gboolean update_simulation(gpointer data) {
    AppWidgets *app = (AppWidgets*)data;
    if (!app->running) return FALSE;

    app->clock_cycle++;

    // Log cycle start
    char cycle_log[256];
    snprintf(cycle_log, sizeof(cycle_log), "Starting Cycle %d\n", app->clock_cycle);
    gtk_text_buffer_insert_at_cursor(
        gtk_text_view_get_buffer(GTK_TEXT_VIEW(app->log_text_view)),
        cycle_log, -1);

    // Run scheduler
    switch (schedule) {
        case FCFS:
            fifo_scheduler(mem, &ready_queue);
            break;
        case RR:
            round_robin(mem, &ready_queue);
            break;
        case MLFQ:
            multilevel_feedback_queue(mem, &lvl1, &lvl2, &lvl3, &lvl4);
            break;
        default:
            break;
    }

    // Log the instruction that was executed
    for (int i = 0; i < programs; i++) {
        if (pcbs_list[i] != NULL && pcbs_list[i]->state == RUNNING) {
            int prev_pc = pcbs_list[i]->program_counter - 1;
            if (prev_pc >= pcbs_list[i]->mem_start && prev_pc < pcbs_list[i]->mem_end) {
                char *instruction = mem[0].words[prev_pc].value;
                if (instruction) {
                    char log[256];
                    snprintf(log, sizeof(log), "P%d executed instruction: %s\n",
                             pcbs_list[i]->pid, instruction);
                    gtk_text_buffer_insert_at_cursor(
                        gtk_text_view_get_buffer(GTK_TEXT_VIEW(app->log_text_view)),
                        log, -1);
                }
            }
            break;
        }
    }

    // Update process table
    gtk_list_store_clear(app->process_store);
    for (int i = 0; i < programs; i++) {
        if (pcbs_list[i]) {
            char pid_str[16];
            snprintf(pid_str, sizeof(pid_str), "P%d", pcbs_list[i]->pid);
            char range_str[50];
            snprintf(range_str, sizeof(range_str), "%d-%d", pcbs_list[i]->mem_start, pcbs_list[i]->mem_end);
            GtkTreeIter iter;
            gtk_list_store_append(app->process_store, &iter);
            gtk_list_store_set(app->process_store, &iter,
                               0, pid_str,
                               1, state_to_string(pcbs_list[i]),
                               2, pcbs_list[i]->priority,
                               3, range_str,
                               4, pcbs_list[i]->program_counter,
                               5, pcbs_list[i]->priority,
                               -1);
        }
    }

    update_memory_view(app);
    update_ready_queue_label(app);
    update_running_and_blocked_labels(app);
    update_gui(app);

    // Log cycle end
    snprintf(cycle_log, sizeof(cycle_log), "Cycle %d: Running: ", app->clock_cycle);
    int pos = strlen(cycle_log);
    int running_found = 0;
    for (int i = 0; i < programs; i++) {
        if (pcbs_list[i] != NULL && pcbs_list[i]->state == RUNNING) {
            pos += snprintf(cycle_log + pos, sizeof(cycle_log) - pos, "P%d", pcbs_list[i]->pid);
            running_found = 1;
            break;
        }
    }
    if (!running_found) {
        pos += snprintf(cycle_log + pos, sizeof(cycle_log) - pos, "None");
    }
    pos += snprintf(cycle_log + pos, sizeof(cycle_log) - pos, ", Ready: [");
    if (!is_empty(&ready_queue)) {
        int first = 1;
        for (int i = ready_queue.front; i < ready_queue.rear && i < MAX_SIZE; i++) {
            if (ready_queue.processes[i] == NULL || ready_queue.processes[i]->state != READY) continue;
            if (!first) pos += snprintf(cycle_log + pos, sizeof(cycle_log) - pos, ", ");
            pos += snprintf(cycle_log + pos, sizeof(cycle_log) - pos, "P%d", ready_queue.processes[i]->pid);
            first = 0;
        }
    }
    pos += snprintf(cycle_log + pos, sizeof(cycle_log) - pos, "], Blocked: [");
    int first = 1;
    for (int i = 0; i < programs; i++) {
        if (pcbs_list[i] != NULL && pcbs_list[i]->state == BLOCKED) {
            if (!first) pos += snprintf(cycle_log + pos, sizeof(cycle_log) - pos, ", ");
            pos += snprintf(cycle_log + pos, sizeof(cycle_log) - pos, "P%d", pcbs_list[i]->pid);
            first = 0;
        }
    }
    pos += snprintf(cycle_log + pos, sizeof(cycle_log) - pos, "]\n");
    gtk_text_buffer_insert_at_cursor(
        gtk_text_view_get_buffer(GTK_TEXT_VIEW(app->log_text_view)),
        cycle_log, -1);

    snprintf(cycle_log, sizeof(cycle_log), "Clock cycle %d executed\n", app->clock_cycle);
    gtk_text_buffer_insert_at_cursor(
        gtk_text_view_get_buffer(GTK_TEXT_VIEW(app->log_text_view)),
        cycle_log, -1);

    return TRUE;
}

// Update GUI elements
void update_gui(AppWidgets *app) {
    char overview[256];
    snprintf(overview, sizeof(overview), "Processes: %d | Clock: %d | Algorithm: %s",
             programs,
             app->clock_cycle,
             gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(app->algo_combo)));
    gtk_label_set_text(GTK_LABEL(app->overview_label), overview);
    gtk_widget_queue_draw(app->overview_label);
}

// Main function
int main(int argc, char *argv[]) {
    init_queue(&ready_queue);
    init_queue(&lvl1);
    init_queue(&lvl2);
    init_queue(&lvl3);
    init_queue(&lvl4);
    initMutex(&file);
    initMutex(&input);
    initMutex(&output);
    gtk_init(&argc, &argv);
    app = init_gui();
    gtk_widget_show_all(app->window);
    gtk_main();
    g_free(app);
    return 0;
}