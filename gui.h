#ifndef SCHEDULER_GUI_H
#define SCHEDULER_GUI_H

#include <gtk/gtk.h>

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
    GtkWidget *start_simulation_button;
    GtkWidget *stop_button;
    GtkWidget *reset_button;
    GtkWidget *step_button;
    GtkWidget *arrival_entry;
    GtkWidget *mutex_status_label;
    GtkWidget *blocked_resource_label;
    GtkWidget *memory_view_label;
    GtkWidget *log_text_view;
    GtkWidget *add_program1_button;
    GtkWidget *add_program2_button;
    GtkWidget *add_program3_button;
    int clock_cycle;
    gboolean running;
} AppWidgets;

extern AppWidgets *app;
char* get_gui_input(int pid);

#endif