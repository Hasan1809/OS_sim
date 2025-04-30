
#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "scheduler.h"  // Include scheduler functions
#include "memory.h"     // Include memory functions
#include "process.h" 
#include "interpreter.h"   // Include process functions
#include "queue.h"
#include "mutex.h"

char** separatefunction(char* fileName, int* line_count) {
    FILE *file = fopen(fileName, "r");
    if (!file) {
        perror("Error opening file");
        return NULL;
    }

    char ch;
    char result[100] = "";
    char** lines = malloc(sizeof(char*) * 100); // support up to 100 lines
    int i = 0;

    while ((ch = fgetc(file)) != EOF) {
        if (ch == '\n') {
            lines[i++] = strdup(result); // save the current line
            result[0] = '\0';            // clear buffer
        } else {
            char temp[2] = {ch, '\0'};
            strcat(result, temp);        // build line char by char
        }
    }

    // Handle last line if file doesn’t end with newline
    if (strlen(result) > 0) {
        lines[i++] = strdup(result);
    }

    fclose(file);
    *line_count = i;
    return lines;
}

static void activate(GtkApplication* app, gpointer user_data) {
    GtkWidget *window;
    GtkWidget *grid;
    GtkWidget *treeview;
    GtkWidget *scrolled_window;

    // Create window
    window = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(window), "Scheduler Simulator");
    gtk_window_set_default_size(GTK_WINDOW(window), 600, 400);

    // Create grid layout
    grid = gtk_grid_new();
    gtk_container_add(GTK_CONTAINER(window), grid);

    // Create scrolled window for TreeView
    scrolled_window = gtk_scrolled_window_new(NULL, NULL);
    gtk_widget_set_vexpand(scrolled_window, TRUE);
    gtk_widget_set_hexpand(scrolled_window, TRUE);
    gtk_grid_attach(GTK_GRID(grid), scrolled_window, 0, 0, 1, 1);

    // Create TreeView
    treeview = gtk_tree_view_new();
    gtk_container_add(GTK_CONTAINER(scrolled_window), treeview);

    // Define columns: PID, State, Priority
    GtkCellRenderer *renderer;
    GtkTreeViewColumn *column;
    GtkListStore *store;

    renderer = gtk_cell_renderer_text_new();

    // Create the columns dynamically from the pcb_list data
    column = gtk_tree_view_column_new_with_attributes("Process ID", renderer, "text", 0, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(treeview), column);

    column = gtk_tree_view_column_new_with_attributes("State", renderer, "text", 1, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(treeview), column);

    column = gtk_tree_view_column_new_with_attributes("Priority", renderer, "text", 2, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(treeview), column);

    // Create the ListStore (this can hold process data)
    store = gtk_list_store_new(3, G_TYPE_INT, G_TYPE_STRING, G_TYPE_INT);

    // Fill the ListStore dynamically from the global pcb_list array
    GtkTreeIter iter;
    for (int i = 0; i < 3; i++) {
        // Append a row to the store
        gtk_list_store_append(store, &iter);

        // Set the values from pcb_list[i] into the ListStore
        gtk_list_store_set(store, &iter, 
            0, pcb_list[i]->pid,        // Process ID
            1, state_to_string(pcb_list[i]),       // Process State
            2, pcb_list[i]->priority,    // Process Priority
            -1);
    }

    // Set the model for the TreeView
    gtk_tree_view_set_model(GTK_TREE_VIEW(treeview), GTK_TREE_MODEL(store));
    g_object_unref(store);

    gtk_widget_show_all(window);
}


int arrival1;
int arrival2;
int arrival3;
PCB* pcb1;
PCB* pcb2;
PCB* pcb3;
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
PCB* pcb_list[3];


int main(int argc, char **argv) {

    init_queue(&ready_queue);
    init_queue(&lvl1);
    init_queue(&lvl2);
    init_queue(&lvl3);
    init_queue(&lvl4);
    initMutex(&file);
    initMutex(&input);
    initMutex(&output);

    schedule = MLFQ;

    MemoryManager mem[60];
    init_memory(mem);

    arrival1 = 0;
    arrival2 = 0;
    arrival3 = 9;
    
    pcb1 = create_pcb(1, 10);
    pcb2 = create_pcb(2,12);
    pcb3 = create_pcb(3,14);

    pcb_list = {pcb1, pcb2, pcb3};


    int line_count;
    char** lines = separatefunction("Program_1.txt", &line_count);
    int line_count2;
    char** lines2 = separatefunction("Program_2.txt", &line_count2);
    int line_count3;
    char** lines3 = separatefunction("Program_3.txt", &line_count3);
    
    allocate_process(mem,pcb1,lines,line_count);   
    allocate_process(mem,pcb2,lines2,line_count2);
    allocate_process(mem,pcb3,lines3,line_count3);

    GtkApplication *app;
    int status;

    app = gtk_application_new("org.example.SchedulerSim", G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);
    status = g_application_run(G_APPLICATION(app), argc, argv);
    g_object_unref(app);

    return status;
}
