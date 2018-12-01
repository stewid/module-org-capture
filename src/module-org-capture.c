/* -*- mode: c; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 *  Capture notes in org-mode from the Evolution PIM application
 *
 *  Copyright (C) 2018 Stefan Widgren
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <shell/e-shell.h>
#include <shell/e-shell-view.h>
#include <libebackend/libebackend.h>
#include <glib/gi18n-lib.h>
#include <mail/e-mail-reader.h>
#include <mail/e-mail-paned-view.h>

typedef struct _EOrgCapture EOrgCapture;
typedef struct _EOrgCaptureClass EOrgCaptureClass;

struct _EOrgCapture {
        EExtension parent;
};

struct _EOrgCaptureClass {
        EExtensionClass parent_class;
};

/* Module Entry Points */
void e_module_load (GTypeModule *type_module);
void e_module_unload (GTypeModule *type_module);

/* Forward Declarations */
GType e_org_capture_get_type (void);

G_DEFINE_DYNAMIC_TYPE (EOrgCapture, e_org_capture, E_TYPE_EXTENSION)

static void
org_capture_mail_message_cb (GtkAction	*action,
			     EShellView	*shell_view);

static GtkActionEntry org_menu_entries[] = {
	{ "org-capture-mail-message",
	  NULL,
	  N_("Org capture message"),
	  "<Primary><Alt>c",
	  N_("Org capture message"),
	  G_CALLBACK (org_capture_mail_message_cb) }
};

static void
org_capture_mail_message_cb (GtkAction	*action,
			     EShellView	*shell_view)
{
	EShellContent	*shell_content;
	EMailView	*mail_view     = NULL;
	GPtrArray	*selected_uids = NULL;

	g_return_if_fail (E_IS_SHELL_VIEW (shell_view));

	shell_content = e_shell_view_get_shell_content (shell_view);
	g_object_get (shell_content, "mail-view", &mail_view, NULL);
	if (E_IS_MAIL_PANED_VIEW (mail_view)) {
		EMailReader *reader = E_MAIL_READER (mail_view);

		selected_uids = e_mail_reader_get_selected_uids (reader);
		if (selected_uids) {
			if (selected_uids->len == 1) {
				CamelMessageInfo	*info;
				CamelFolder		*folder;
				const gchar		*uid;
				const gchar		*title;

				folder = e_mail_reader_ref_folder (reader);
				uid    = g_ptr_array_index (selected_uids, 0);
				info   = camel_folder_get_message_info (folder, uid);

				title = camel_message_info_get_subject (info);
				if (title == NULL || *title == '\0')
					title = _("(No Subject)");

				/* FIXME: Start the emacsclient and org-capture */
				g_print ("   %s: %s\n", uid, title);

				g_clear_object (&info);
				g_clear_object (&folder);
			}

			g_ptr_array_unref (selected_uids);
		}
	}
}

static void
e_org_capture_constructed (GObject *object)
{
        EExtensible *extensible;

        /* This retrieves the EShell instance we're extending. */
        extensible = e_extension_get_extensible (E_EXTENSION (object));

        g_print ("Initialize 'module-org-capture' from %s.\n", G_OBJECT_TYPE_NAME (extensible));
}

static void
e_org_capture_finalize (GObject *object)
{
        g_print ("Finalize 'module-org-capture'.\n");

        /* Chain up to parent's finalize() method. */
        G_OBJECT_CLASS (e_org_capture_parent_class)->finalize (object);
}

static void
e_org_capture_class_init (EOrgCaptureClass *class)
{
        GObjectClass *object_class;
        EExtensionClass *extension_class;

        object_class = G_OBJECT_CLASS (class);
        object_class->constructed = e_org_capture_constructed;
        object_class->finalize = e_org_capture_finalize;

        /* Specify the GType of the class we're extending.
         * The class must implement the EExtensible interface. */
        extension_class = E_EXTENSION_CLASS (class);
        extension_class->extensible_type = E_TYPE_SHELL;
}

static void
e_org_capture_class_finalize (EOrgCaptureClass *class)
{
        /* This function is usually left empty. */
}

static void
e_org_capture_init (EOrgCapture *extension)
{
        /* The EShell object we're extending is not available yet,
         * but we could still do some early initialization here. */
}

G_MODULE_EXPORT void
e_module_load (GTypeModule *type_module)
{
        /* This registers our EShell extension class with the GObject
         * type system.  An instance of our extension class is paired
         * with each instance of the class we're extending. */
        e_org_capture_register_type (type_module);
}

G_MODULE_EXPORT void
e_module_unload (GTypeModule *type_module)
{
        /* This function is usually left empty. */
}
