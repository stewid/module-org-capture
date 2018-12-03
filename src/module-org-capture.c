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
#include <glib/gi18n-lib.h>
#include <mail/e-mail-reader.h>
#include <mail/e-mail-paned-view.h>
#include <mail/message-list.h>

/* Standard GObject macros */
#define E_ORG_CAPTURE_TYPE \
	(e_org_capture_get_type ())
#define E_ORG_CAPTURE(obj) \
	(G_TYPE_CHECK_INSTANCE_CAST \
	((obj), E_ORG_CAPTURE_TYPE, EOrgCapture))
#define E_IS_ORG_CAPTURE(obj)		\
	(G_TYPE_CHECK_INSTANCE_TYPE \
	((obj), E_ORG_CAPTURE_TYPE))

typedef struct _EOrgCapture EOrgCapture;
typedef struct _EOrgCaptureClass EOrgCaptureClass;
typedef struct _EOrgCapturePrivate EOrgCapturePrivate;

struct _EOrgCapture
{
	EExtension parent;
	EOrgCapturePrivate *private;
};

struct _EOrgCaptureClass
{
	EExtensionClass parent;
};

GType	e_org_capture_get_type		(void);
void	e_org_capture_type_register	(GTypeModule *type_module);

struct _EOrgCapturePrivate {
	guint current_ui_id;
	GHashTable *ui_definitions;
};

G_DEFINE_DYNAMIC_TYPE (EOrgCapture, e_org_capture, E_TYPE_EXTENSION)
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
				/* system("emacsclient -a '' -c -F '(quote (name . \"capture\"))' -e '(activate-capture-frame)'"); */
				g_print ("   %s: %s\n", uid, title);

				g_clear_object (&info);
				g_clear_object (&folder);
			}

			g_ptr_array_unref (selected_uids);
		}
	}
}

static gboolean
org_capture_has_message(EMailView *mail_view)
{
	EMailReader	*reader	      = NULL;
	MessageList	*message_list = NULL;

	if (!E_IS_MAIL_PANED_VIEW (mail_view))
		return FALSE;

	reader	     = E_MAIL_READER (mail_view);
	message_list = MESSAGE_LIST (e_mail_reader_get_message_list (reader));

	return message_list_selected_count (message_list) == 1 ? TRUE : FALSE;
}

static void
org_capture_enable_actions (GtkActionGroup		*action_group,
			    const GtkActionEntry	*entries,
			    guint			 n_entries,
			    gboolean			 enable)
{
	gint i;

	g_return_if_fail (action_group != NULL);
	g_return_if_fail (entries != NULL);

	for (i = 0; i < n_entries; i++) {
		GtkAction *action;

		action = gtk_action_group_get_action (
			action_group,
			entries[i].name);
		if (action)
			gtk_action_set_sensitive (action, enable);
	}
}

static void
org_capture_update_actions_cb (EShellView     *shell_view,
			       GtkActionEntry *entries)
{
	EShellContent	*shell_content = NULL;
	EShellWindow	*shell_window  = NULL;
	EMailView	*mail_view     = NULL;
	GtkActionGroup	*action_group  = NULL;
	GtkUIManager	*ui_manager    = NULL;

	g_return_if_fail (E_IS_SHELL_VIEW (shell_view));

	shell_content = e_shell_view_get_shell_content (shell_view);
	g_object_get (shell_content, "mail-view", &mail_view, NULL);
	shell_window = e_shell_view_get_shell_window (shell_view);
	ui_manager   = e_shell_window_get_ui_manager (shell_window);
	action_group = e_lookup_action_group (ui_manager, "mail");

	org_capture_enable_actions (
		action_group,
		org_menu_entries,
		G_N_ELEMENTS (org_menu_entries),
		org_capture_has_message(mail_view));
}

void
org_capture_ui_init (GtkUIManager	 *ui_manager,
		     EShellView		 *shell_view,
		     gchar		**ui_definition)
{
	const gchar *ui_def =
		"<menubar name='main-menu'>\n"
		"  <placeholder name='custom-menus'>\n"
		"    <menu action='mail-message-menu'>\n"
		"      <placeholder name='mail-message-custom-menus'>\n"
		"        <menuitem action=\"org-capture-mail-message\"/>\n"
		"      </placeholder>\n"
		"    </menu>\n"
		"  </placeholder>\n"
                "</menubar>\n";

	EShellWindow *shell_window;
	GtkActionGroup *action_group;

	g_return_if_fail (ui_definition != NULL);

	*ui_definition = g_strdup (ui_def);

	shell_window = e_shell_view_get_shell_window (shell_view);
	action_group = e_shell_window_get_action_group (shell_window, "mail");

	e_action_group_add_actions_localized (
		action_group,
		GETTEXT_PACKAGE,
		org_menu_entries,
		G_N_ELEMENTS (org_menu_entries),
		shell_view);

	g_signal_connect (
		shell_view,
		"update-actions",
		G_CALLBACK (org_capture_update_actions_cb),
		shell_view);
}

static void
org_capture_ui_definition (EShellView	 *shell_view,
			   const gchar	 *ui_manager_id,
			   gchar	**ui_definition)
{
	EShellWindow *shell_window;
	GtkUIManager *ui_manager;

	g_return_if_fail (shell_view != NULL);
	g_return_if_fail (ui_manager_id != NULL);
	g_return_if_fail (ui_definition != NULL);

	shell_window = e_shell_view_get_shell_window (shell_view);
	ui_manager = e_shell_window_get_ui_manager (shell_window);

	if (g_strcmp0 (ui_manager_id, "org.gnome.evolution.mail") == 0)
		org_capture_ui_init (ui_manager, shell_view, ui_definition);
}

static void
e_org_capture_shell_view_toggled_cb (EShellView		*shell_view,
				     EOrgCapture	*org_capture)
{
	EShellViewClass *shell_view_class;
	EShellWindow *shell_window;
	GtkUIManager *ui_manager;
	gpointer key = NULL, value = NULL;
	const gchar *ui_def;
	gboolean is_active, need_update;

	g_return_if_fail (E_IS_SHELL_VIEW (shell_view));
	g_return_if_fail (E_IS_ORG_CAPTURE (org_capture));

	shell_view_class = E_SHELL_VIEW_GET_CLASS (shell_view);
	g_return_if_fail (shell_view_class != NULL);

	shell_window = e_shell_view_get_shell_window (shell_view);
	ui_manager = e_shell_window_get_ui_manager (shell_window);

	need_update = org_capture->private->current_ui_id != 0;
	if (org_capture->private->current_ui_id) {
		gtk_ui_manager_remove_ui (ui_manager, org_capture->private->current_ui_id);
		org_capture->private->current_ui_id = 0;
	}

	is_active = e_shell_view_is_active (shell_view);
	if (!is_active) {
		if (need_update)
			gtk_ui_manager_ensure_update (ui_manager);

		return;
	}

	if (!g_hash_table_lookup_extended (org_capture->private->ui_definitions, shell_view_class->ui_manager_id, &key, &value)) {
		gchar *ui_definition = NULL;

		org_capture_ui_definition (
			shell_view,
			shell_view_class->ui_manager_id,
			&ui_definition);
		g_hash_table_insert (
			org_capture->private->ui_definitions,
			g_strdup (shell_view_class->ui_manager_id),
			ui_definition);
	}

	ui_def = g_hash_table_lookup (
		org_capture->private->ui_definitions,
		shell_view_class->ui_manager_id);
	if (ui_def) {
		GError *error = NULL;

		org_capture->private->current_ui_id = gtk_ui_manager_add_ui_from_string (
			ui_manager,
			ui_def,
			-1,
			&error);
		need_update = TRUE;

		if (error) {
			g_warning ("%s: Failed to add ui definition: %s", G_STRFUNC, error->message);
			g_error_free (error);
		}
	}

	if (need_update)
		gtk_ui_manager_ensure_update (ui_manager);
}

static void
e_org_capture_constructed (GObject *object)
{
	EExtension *extension;
	EExtensible *extensible;

	extension = E_EXTENSION (object);
	extensible = e_extension_get_extensible (extension);

	/* Chain up to parent's constructed() method. */
	G_OBJECT_CLASS (e_org_capture_parent_class)->constructed (object);

	g_signal_connect (
		E_SHELL_VIEW (extensible),
		"toggled",
		G_CALLBACK (e_org_capture_shell_view_toggled_cb),
		extension);
}

static void
e_org_capture_finalize (GObject *object)
{
	G_OBJECT_CLASS (e_org_capture_parent_class)->finalize (object);
	g_hash_table_destroy (E_ORG_CAPTURE (object)->private->ui_definitions);
}

static void
e_org_capture_class_init (EOrgCaptureClass *class)
{
	GObjectClass *object_class;
	EExtensionClass *extension_class;

	g_type_class_add_private (class, sizeof (EOrgCapturePrivate));

	object_class = G_OBJECT_CLASS (class);
	object_class->constructed = e_org_capture_constructed;
	object_class->finalize = e_org_capture_finalize;

	/* Set the type to extend, it's supposed to implement the EExtensible interface */
	extension_class = E_EXTENSION_CLASS (class);
	extension_class->extensible_type = E_TYPE_SHELL_VIEW;
}

static void
e_org_capture_class_finalize (EOrgCaptureClass *class)
{
        G_OBJECT_CLASS (e_org_capture_parent_class)->finalize (G_OBJECT (class));
}

static void
e_org_capture_init (EOrgCapture *extension)
{
	extension->private = G_TYPE_INSTANCE_GET_PRIVATE (
		extension,
		E_ORG_CAPTURE_TYPE,
		EOrgCapturePrivate);
	extension->private->current_ui_id = 0;
	extension->private->ui_definitions = g_hash_table_new_full (
		g_str_hash,
		g_str_equal,
		g_free,
		g_free);
}

void
e_org_capture_type_register (GTypeModule *type_module)
{
	e_org_capture_register_type (type_module);
}

G_MODULE_EXPORT void
e_module_load (GTypeModule *type_module)
{
	e_org_capture_type_register (type_module);
}

G_MODULE_EXPORT void
e_module_unload (GTypeModule *type_module)
{
}
