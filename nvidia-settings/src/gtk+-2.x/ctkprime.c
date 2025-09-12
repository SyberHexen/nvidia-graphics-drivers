/*
 * nvidia-settings: A tool for configuring the NVIDIA X driver on Unix
 * and Linux systems.
 *
 * Copyright (C) 2013 Canonical Ltd.
 *
 * Author: Alberto Milone <alberto.milone@canonical.com>
 *
 * Based on ctkxvideo.c:
 * - Copyright (C) 2004 NVIDIA Corporation.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses>.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <gtk/gtk.h>

#include "ctkbanner.h"
#include "ctkutils.h"
#include "ctkprime.h"
#include "ctkscale.h"

#include "ctkhelp.h"


static void prime_changed(GtkWidget *widget, gpointer user_data);

static GtkWidget *prime_radio_button_add(CtkPrime *ctk_prime,
                                         GtkWidget *prev_radio,
                                         char *label,
                                         gchar *value,
                                         int index);

static void
prime_update_radio_buttons(CtkPrime *ctk_prime, gint value);

static void prime_changed(GtkWidget *widget, gpointer user_data);

#define FRAME_PADDING 5


/* Create an info message dialog */
static void ctk_display_info_msg(GtkWidget *parent, gchar * msg)
{
    GtkWidget *dlg;

    if (msg) {
        nv_warning_msg("%s", msg);

        if (parent) {
            dlg = gtk_message_dialog_new
            (GTK_WINDOW(parent),
             GTK_DIALOG_DESTROY_WITH_PARENT,
             GTK_MESSAGE_INFO,
             GTK_BUTTONS_OK,
             "%s", msg);
            gtk_dialog_run(GTK_DIALOG(dlg));
            gtk_widget_destroy(dlg);
        }
    }
}


/* See if the hardware is supported */
static gboolean is_supported(void) {
    gchar *command = "/usr/bin/prime-supported /dev/stderr";
    int supported = 1;
    gboolean status;
    int exit_status = 0;
    gchar *output = NULL;
    gchar *error_str = NULL;
    GError *error = NULL;
    status = g_spawn_command_line_sync(command, &output, &error_str, &exit_status, &error);

    if (!status) {
        g_warning("PRIME: %s", error->message);

        if (output)
            g_free(output);
        if (error_str)
            g_free(error_str);
        if (error)
            g_error_free(error);

        return status;
    }

    if (output)
        output[strlen(output) - 1] = '\0';

    if (strlen(error_str) > 0) {
        error_str[strlen(error_str) - 1] = '\0';
        g_message("PRIME: %s", error_str);

    }

    supported = (strcmp(output, "yes") == 0);

    if (output)
        g_free(output);

    if (error_str)
        g_free(error_str);

    if (error)
        g_error_free(error);

    return supported;
}


/* See if the on-demand mode is supported */
static gboolean is_on_demand_supported(void) {
    gchar *command = "/usr/bin/prime-select --help";
    int supported = 0;
    gboolean status;
    int exit_status = 0;
    gchar *output = NULL;
    gchar *error_str = NULL;
    GError *error = NULL;
    status = g_spawn_command_line_sync(command, &output, &error_str, &exit_status, &error);

    if (!status) {
        g_warning("PRIME: %s", error->message);

        if (output)
            g_free(output);
        if (error_str)
            g_free(error_str);
        if (error)
            g_error_free(error);

        return status;
    }

    if (output)
        output[strlen(output) - 1] = '\0';

    if (strlen(error_str) > 0) {
        error_str[strlen(error_str) - 1] = '\0';
        g_message("PRIME: %s", error_str);
    }

    supported = (g_strrstr(error_str, "on-demand") != NULL) ? 1 : 0;

    g_message("PRIME: on-demand mode: \"%d\"", supported);

    if (output)
        g_free(output);

    if (error_str)
        g_free(error_str);

    if (error)
        g_error_free(error);

    return supported;
}


/* Get the active GPU */
static gboolean get_active_gpu(gchar **gpu) {
    gchar *command = "/usr/bin/prime-select query";
    gboolean status;
    int exit_status = 0;
    gchar *output = NULL;
    gchar *error_str = NULL;
    GError *error = NULL;
    status = g_spawn_command_line_sync(command, &output, &error_str, &exit_status, &error);

    if (!status) {
        /* If we get here, something seriously broken */
        g_error("PRIME error: %s", error->message);
        if (output)
            g_free(output);

        if (error_str)
            g_free(error_str);

        if (error)
            g_error_free(error);

        return status;
    }

    if (output) {
        output[strlen(output) - 1] = '\0';
        *gpu = g_strdup(output);
    }

    if (output)
        g_free(output);

    if (error_str)
        g_free(error_str);

    if (error)
        g_error_free(error);

    return status;
}


GType ctk_prime_get_type(void)
{
    static GType ctk_prime_type = 0;

    if (!ctk_prime_type) {
        static const GTypeInfo ctk_prime_info = {
            sizeof (CtkPrimeClass),
            NULL, /* base_init */
            NULL, /* base_finalize */
            NULL, /* class_init */
            NULL, /* class_finalize */
            NULL, /* class_data */
            sizeof(CtkPrime),
            0, /* n_preallocs */
            NULL, /* instance_init */
            NULL  /* value_table */
        };

        ctk_prime_type = g_type_register_static
            (GTK_TYPE_VBOX, "CtkPrime", &ctk_prime_info, 0);
    }

    return ctk_prime_type;
}


/*
 * Create a radio button and plug it
 * into the prime radio group.
 */
static GtkWidget *prime_radio_button_add(CtkPrime *ctk_prime,
                                         GtkWidget *prev_radio,
                                         char *label,
                                         gchar *value,
                                         int index)
{
    GtkWidget *radio;

    if (prev_radio) {
        radio = gtk_radio_button_new_with_label_from_widget
            (GTK_RADIO_BUTTON(prev_radio), label);
    } else {
        radio = gtk_radio_button_new_with_label(NULL, label);
    }

    gtk_box_pack_start(GTK_BOX(ctk_prime->prime_button_box),
                       radio, FALSE, FALSE, 0);

    g_object_set_data(G_OBJECT(radio), "prime", value);

    g_signal_connect(G_OBJECT(radio), "toggled",
                     G_CALLBACK(prime_changed),
                     (gpointer) ctk_prime);

    ctk_prime->prime_buttons[index] = radio;

    return radio;
}


static void
prime_update_radio_buttons(CtkPrime *ctk_prime, gint value)
{
    GtkWidget *b, *button = NULL;
    int i;

    button = ctk_prime->prime_buttons[value];
    if (!button) return;

    /* turn off signal handling for all the sync buttons */
    for (i = 0; i < 24; i++) {
        b = ctk_prime->prime_buttons[i];
        if (!b) continue;

        g_signal_handlers_block_by_func
            (G_OBJECT(b), G_CALLBACK(prime_changed),
             (gpointer) ctk_prime);
    }

    /* set the appropriate button active */
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), TRUE);

    /* turn on signal handling for all the sync buttons */
    for (i = 0; i < 24; i++) {
        b = ctk_prime->prime_buttons[i];
        if (!b) continue;

        g_signal_handlers_unblock_by_func
            (G_OBJECT(b), G_CALLBACK(prime_changed),
             (gpointer) ctk_prime);
    }
}


/* get button value for gpu mode
 *   if ctk_prime->has_on_demand_mode == 1:
 *     nvidia: 0
 *     on-demand: 1
 *     intel: 2 # presents but disabled for if user issues "prime-select intel"
 *   else:
 *     nvidia: 0
 *     intel: 1
 */
static int mode_to_button_value(CtkPrime *ctk_prime, gchar *current_gpu) {
    int current_button;
    if (strcmp(current_gpu, "nvidia") == 0) {
        current_button = 0;
    } else if (strcmp(current_gpu, "on-demand") == 0) {
        current_button = 1;
    } else {
        if (ctk_prime->has_on_demand_mode) {
            current_button = 2;
        } else {
         current_button = 1;
        }
    }
    return current_button;
}


/* Child watch callback function */
static void
child_watch_cb (GPid     pid,
                gint     exit_status,
                gpointer user_data)
{
    gboolean success;
    gint status;

    g_autoptr(GError) error = NULL;
    GtkWidget *parent = NULL;
    CtkPrime *ctk_prime = CTK_PRIME(user_data);

    success = g_spawn_check_exit_status (exit_status, &error);

    /* Remove the source of the timeout */
    g_source_remove(ctk_prime->source_id);
    ctk_prime->source_id = 0;

    /* Get the parent widget */
    parent = ctk_get_parent_window(GTK_WIDGET(ctk_prime));

    /* Free the progress dialog */
    gtk_widget_destroy (GTK_WIDGET (ctk_prime->progress_dialog));
    ctk_prime->progress_dialog = NULL;

    /* if the child succeeded */
    if (success) {
        /* Did it really succeed though? */
        status = WEXITSTATUS(exit_status);

        if (status == 0) {
            /* Show a dialog to suggest restarting the computer */

            ctk_prime->current_button = mode_to_button_value(ctk_prime, ctk_prime->value);
            ctk_display_info_msg(parent,
                "Please restart the system to apply the changes");
        } else {
            if (status != 5) {
                /* Show an error only if the user did not
                 * cancel the operation
                 */
                ctk_display_error_msg(parent, error->message);
            }

            /* Set the radio button to the previous position */
            prime_update_radio_buttons(ctk_prime, ctk_prime->current_button);
        }
    } else {
        /* Something's wrong with the child */

        /* Show an error dialog */
        ctk_display_error_msg(parent, error->message);

        /* Set the radio button to the previous position */
        prime_update_radio_buttons(ctk_prime, ctk_prime->current_button);
    }

    /* Free the resources associated with the child */
    g_spawn_close_pid (pid);
    g_free(ctk_prime->value);
}


/* Make the progress bar bounce */
static gboolean
bounce_bar (gpointer user_data)
{
    if (!user_data)
        return FALSE;

    GtkWidget *progress_bar = user_data;
    gtk_progress_bar_pulse(GTK_PROGRESS_BAR (progress_bar));
    return TRUE;
}


/*
 * Callback function for changes to the
 * prime radio button group;
 */
static void prime_changed(GtkWidget *widget, gpointer user_data)
{
    gboolean enabled;
    GtkWidget *content_area;
    GtkWidget *label;
    GtkWidget *progress_bar;
    gboolean status;
    gint child_stdout, child_stderr;
    GPid child_pid;

    CtkPrime *ctk_prime = CTK_PRIME(user_data);
    g_autoptr(GError) error = NULL;
    ctk_prime->source_id = 0;

    /* Get the toggle button state */
    enabled = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));

    if (enabled) {
        /* Get the parent window */
        GtkWidget *parent;
        parent = ctk_get_parent_window(GTK_WIDGET(ctk_prime));

        user_data = g_object_get_data(G_OBJECT(widget), "prime");

        /* We use "value" to know what to select */
        ctk_prime->value = g_strdup((gchar*)user_data);


        /* Create the progress dialog */
        ctk_prime->progress_dialog = gtk_dialog_new_with_buttons ("PRIME",
                                                                  GTK_WINDOW(parent),
                                                                  GTK_RESPONSE_NONE,
                                                                  NULL);

        /* Create a label */
        content_area = gtk_dialog_get_content_area (GTK_DIALOG (ctk_prime->progress_dialog));
        label = gtk_label_new ("Please wait for the operation to complete");
        gtk_container_add (GTK_CONTAINER (content_area), label);

        /* Create a progress bar */
        progress_bar = gtk_progress_bar_new();
        gtk_container_add (GTK_CONTAINER (content_area), progress_bar);

        /* keep the bar bouncing */
        ctk_prime->source_id = g_timeout_add (500, bounce_bar, GTK_PROGRESS_BAR (progress_bar));

        gchar * argv[] = {"/usr/bin/pkexec", "/usr/bin/prime-select", ctk_prime->value, NULL };

        status = g_spawn_async_with_pipes (NULL, argv, NULL, G_SPAWN_DO_NOT_REAP_CHILD, NULL,
                          NULL, &child_pid, NULL, &child_stdout,
                          &child_stderr, &error);

        if (!status || error != NULL) {
            /* Something's wrong with gspawn */
            g_error ("Spawning child failed: %s", error->message);
            gtk_widget_destroy(ctk_prime->progress_dialog);

            /* Show an error dialog */
            ctk_display_error_msg(parent, error->message);

            /* Set the radio button to the previous position */
            prime_update_radio_buttons(ctk_prime, ctk_prime->current_button);

            return;
        }

        /* Show the progress dialog */
        gtk_widget_show_all (ctk_prime->progress_dialog);

        /* Add a child watch function which will be called when the child
         * process exits
         */
        g_child_watch_add (child_pid, child_watch_cb, ctk_prime);
    }
}


/* Constructor for the Prime widget */
GtkWidget* ctk_prime_new(CtkConfig *ctk_config)
{
    GObject *object;
    CtkPrime *ctk_prime;
    GtkWidget *banner;
    GtkWidget *frame;
    GtkWidget *alignment;
    GtkWidget *vbox;

    /*
     * before we do anything else, determine if PRIME is supported
     */
    int supported;
    supported = is_supported();
    g_message("PRIME: is it supported? %s", (supported ? "yes" : "no"));

    if (! supported)
        return NULL;

    /* create the Prime widget */
    object = g_object_new(CTK_TYPE_PRIME, NULL);
    ctk_prime = CTK_PRIME(object);

    ctk_prime->ctk_config = ctk_config;

    ctk_prime->has_on_demand_mode = is_on_demand_supported();
    g_message("PRIME: is \"on-demand\" mode supported? %s", (ctk_prime->has_on_demand_mode ? "yes" : "no"));

    gtk_box_set_spacing(GTK_BOX(ctk_prime), 10);

    /* Set the banner */
    banner = ctk_banner_image_new(BANNER_ARTWORK_GPU);
    gtk_box_pack_start(GTK_BOX(object), banner, FALSE, FALSE, 0);

    /* Create two radiobuttons for the two power profiles */
    GtkWidget *radio[24], *prev_radio;
    frame = gtk_frame_new("Select the GPU you would like to use");
    gtk_box_pack_start(GTK_BOX(object), frame, FALSE, FALSE, 0);

    vbox = gtk_vbox_new(FALSE, 5);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), FRAME_PADDING);
    gtk_container_add(GTK_CONTAINER(frame), vbox);
    ctk_prime->prime_button_box = vbox;

    /* Button for NVIDIA */
    radio[0] = prime_radio_button_add(ctk_prime,
                                      NULL,
                                      "NVIDIA (Performance Mode)",
                                      "nvidia",
                                      0);

    /* Set the tooltip for NVIDIA */
    ctk_config_set_tooltip(ctk_config, radio[0],
                           "Enabling this option ensures the best "
                           "graphics performance. This option is "
                           "applied after a system restart.");


    /* NOTE: if changing the buttons order, please also
     *       change the mode_to_button_value() function
     *       accordingly.
     */
    if (ctk_prime->has_on_demand_mode) {
        /* Button for NVIDIA On Demand */
        radio[1] = prime_radio_button_add(ctk_prime,
                                          radio[0],
                                          "NVIDIA On-Demand",
                                          "on-demand",
                                          1);

        /* Set the tooltip for ON-Demand */
        ctk_config_set_tooltip(ctk_config, radio[1],
                               "Enabling this option allows using the Nvidia GPU"
                               " in on-demand mode (only if nvidia version >= 450; "
                               "otherwise, the behavior will approach to "
                               "performance mode). This option is applied after "
                               "a system restart.");

        /* Button for Intel */
        radio[2] = prime_radio_button_add(ctk_prime,
                                          radio[1],
                                          "Intel (Power Saving Mode)",
                                          "intel",
                                          2);
        /* Set the tooltip for Intel */
        ctk_config_set_tooltip(ctk_config, radio[2],
                               "Enabling this option ensures the best "
                               "battery life. This option is "
                               "applied after a system restart.");

	gtk_widget_set_sensitive(radio[2], FALSE);
    } else {
        /* Button for Intel */
        radio[1] = prime_radio_button_add(ctk_prime,
                                          radio[0],
                                          "Intel (Power Saving Mode)",
                                          "intel",
                                          1);
        /* Set the tooltip for Intel */
        ctk_config_set_tooltip(ctk_config, radio[1],
                               "Enabling this option ensures the best "
                               "battery life. This option is "
                               "applied after a system restart.");
    }

    /* Get the current GPU in use from the switcher*/
    gchar *current_gpu = NULL;
    get_active_gpu(&current_gpu);

    int current_button = mode_to_button_value(ctk_prime, current_gpu);
    g_free(current_gpu);
    ctk_prime->current_button = current_button;

    /* Enable the button which matches the current
     * configuration
     */
    prime_update_radio_buttons(ctk_prime, current_button);

    alignment = gtk_alignment_new(1, 1, 0, 0);
    gtk_box_pack_start(GTK_BOX(object), alignment, TRUE, TRUE, 0);

    /* finally, show the widget */

    gtk_widget_show_all(GTK_WIDGET(ctk_prime));

    return GTK_WIDGET(ctk_prime);
}


GtkTextBuffer *ctk_prime_create_help(GtkTextTagTable *table,
                                      CtkPrime *ctk_prime)
{
    GtkTextIter i;
    GtkTextBuffer *b;

    b = gtk_text_buffer_new(table);

    gtk_text_buffer_get_iter_at_offset(b, &i, 0);

    ctk_help_title(b, &i, "PRIME Profiles Help");

    ctk_help_para(b, &i, "The Prime Profiles page gives "
                          "you control over which GPU you "
                          "desire to use.");

    ctk_help_para(b, &i, "It is recommended that you "
                         "select NVIDIA for the best performance, "
                         "and Intel for the best battery life.");

    ctk_help_finish(b);

    return b;
}

