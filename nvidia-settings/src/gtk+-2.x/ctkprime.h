/*
 * nvidia-settings: A tool for configuring the NVIDIA X driver on Unix
 * and Linux systems.
 *
 * Copyright (C) 2013 Canonical Ltd.
 *
 * Author: Alberto Milone <alberto.milone@canonical.com>
 *
 * Based on ctkxvideo.h:
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

#ifndef __CTK_PRIME_H__
#define __CTK_PRIME_H__

#include "ctkconfig.h"
#include "ctkevent.h"

G_BEGIN_DECLS

#define CTK_TYPE_PRIME (ctk_prime_get_type())

#define CTK_PRIME(obj) \
    (G_TYPE_CHECK_INSTANCE_CAST ((obj), CTK_TYPE_PRIME, CtkPrime))

#define CTK_PRIME_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_CAST ((klass), CTK_TYPE_PRIME, CtkPrimeClass))

#define CTK_IS_PRIME(obj) \
    (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CTK_TYPE_PRIME))

#define CTK_IS_PRIME_CLASS(class) \
    (G_TYPE_CHECK_CLASS_TYPE ((klass), CTK_TYPE_PRIME))

#define CTK_PRIME_GET_CLASS(obj) \
    (G_TYPE_INSTANCE_GET_CLASS ((obj), CTK_TYPE_PRIME, CtkPrimeClass))


typedef struct _CtkPrime       CtkPrime;
typedef struct _CtkPrimeClass  CtkPrimeClass;

struct _CtkPrime
{
    GtkVBox parent;

    CtkConfig *ctk_config;

    GtkWidget *prime_buttons[24];
    GtkWidget *prime_button_box;
    GtkWidget *progress_dialog;
    int current_button;
    int has_on_demand_mode;
    gchar *value;
    guint source_id;
};

struct _CtkPrimeClass
{
    GtkVBoxClass parent_class;
};

GType       ctk_prime_get_type  (void) G_GNUC_CONST;
GtkWidget*  ctk_prime_new       (CtkConfig *);

GtkTextBuffer *ctk_prime_create_help(GtkTextTagTable *, CtkPrime *);

G_END_DECLS

#endif /* __CTK_PRIME_H__ */

